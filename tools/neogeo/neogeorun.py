#!/usr/bin/env python3
"""Headless Neo Geo MVS/AES reference runner using our pinned FBNeo exports.

Work RAM is normalized to big-endian CPU byte order at 100000..10FFFF.
Frame 0 is the first retro_run after boot or state load, sampled after the run.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import zipfile

from expose_video import CORE_REV
from romset import ROOT

DEFAULT_CORE = ROOT / 'sources/neogeo_core/src/burner/libretro/fbneo_neogeo_libretro.so'
DEFAULT_ROM = ROOT / 'sources/windjammers_neogeo/wjammers.zip'
DEFAULT_BIOS = ROOT / 'roms/neogeo/neogeo.zip'
# Neo Geo names, default RetroPad layout: A/B/C/D = libretro B/A/Y/X.
PAD = {'UP': 4, 'DOWN': 5, 'LEFT': 6, 'RIGHT': 7,
       'A': 0, 'B': 8, 'C': 1, 'D': 9, 'COIN': 2, 'START': 3}
ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUDIO = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL = C.CFUNCTYPE(None)
INPUT = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)
# This pinned FBNeo library leaves driver globals across deinit. A second
# initialization can free stale pointers; use a fresh process for another boot.
_INITIALIZED_CORES = set()


class GameInfo(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p),
                ('size', C.c_size_t), ('meta', C.c_char_p)]


class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]


class SystemInfo(C.Structure):
    _fields_ = [('name', C.c_char_p), ('version', C.c_char_p),
                ('extensions', C.c_char_p), ('fullpath', C.c_bool),
                ('block_extract', C.c_bool)]


class Geometry(C.Structure):
    _fields_ = [('width', C.c_uint), ('height', C.c_uint),
                ('max_width', C.c_uint), ('max_height', C.c_uint), ('aspect', C.c_float)]


class Timing(C.Structure):
    _fields_ = [('fps', C.c_double), ('sample_rate', C.c_double)]


class AVInfo(C.Structure):
    _fields_ = [('geometry', Geometry), ('timing', Timing)]


class InputDescriptor(C.Structure):
    _fields_ = [('port', C.c_uint), ('device', C.c_uint),
                ('index', C.c_uint), ('id', C.c_uint), ('description', C.c_char_p)]


class Message(C.Structure):
    _fields_ = [('msg', C.c_char_p), ('frames', C.c_uint)]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def swap_words(data):
    if len(data) & 1:
        raise ValueError('word-swapped region must contain an even byte count')
    result = bytearray(len(data))
    result[0::2], result[1::2] = data[1::2], data[0::2]
    return bytes(result)


def archive_manifest(path):
    with zipfile.ZipFile(path) as archive:
        return [dict(name=i.filename, bytes=i.file_size, crc32=f'{i.CRC:08x}',
                     sha256=sha(archive.read(i)))
                for i in archive.infolist() if not i.is_dir()]


class NeoGeo:
    def __init__(self, rom=DEFAULT_ROM, bios=DEFAULT_BIOS, core=DEFAULT_CORE, options=None):
        self.rom, self.bios, self.core = (Path(p).resolve() for p in (rom, bios, core))
        if not self.bios.is_file():
            raise ValueError(f'missing local Neo Geo BIOS: {self.bios}; use --bios /path/neogeo.zip')
        if not self.rom.is_file():
            raise ValueError('missing game ZIP; run make -C tools/neogeo prepare')
        if not self.core.is_file():
            raise ValueError('missing local core; run make -C tools/neogeo core')
        if self.core in _INITIALIZED_CORES:
            raise ValueError('FBNeo already initialized in this process; start a fresh process for another cold boot')
        if sys.byteorder != 'little':
            raise ValueError('this pinned runner requires the little-endian Linux reference build')
        self.rom_manifest, self.bios_manifest = (archive_manifest(p) for p in (self.rom, self.bios))
        self.frame = None
        self.format = 0
        self.pressed = [0, 0]
        self.options, self.allowed_options, self.descriptors = {}, {}, []
        self.messages, self.callback_errors = [], []
        self.overrides = {
            b'fbneo-cpu-speed-adjust': b'100%',
            b'fbneo-fixed-frameskip': b'0',
            b'fbneo-hiscores': b'disabled',
            b'fbneo-allow-patched-romsets': b'disabled',
            b'fbneo-diagnostic-input': b'None',
            b'fbneo-memcard-mode': b'disabled',
        }
        self.overrides.update({k.encode(): v.encode() for k, v in (options or {}).items()})
        self.scratch = tempfile.TemporaryDirectory(prefix='ti-neogeo-')
        base = Path(self.scratch.name)
        system, save = base / 'system', base / 'save'
        (system / 'fbneo').mkdir(parents=True)
        (save / 'fbneo').mkdir(parents=True)
        (system / 'fbneo/neogeo.zip').symlink_to(self.bios)
        # FBNeo uses sizeof(g_*_dir) memcpy; retain padded backing buffers.
        self.directories = {9: C.create_string_buffer(str(system).encode(), 4096),
                            31: C.create_string_buffer(str(save).encode(), 4096)}
        self.lib = lib = C.CDLL(str(self.core))
        self.initialized = self.loaded = False

        def environment(cmd, data):
            try:
                return self.environment(cmd & 0xffff, data)
            except Exception as error:
                self.callback_errors.append(str(error))
                return False

        def video(data, width, height, pitch):
            if data and data != C.c_void_p(-1).value:
                self.frame = (C.string_at(data, pitch * height), width, height, pitch)

        self.callbacks = [ENV(environment), VIDEO(video), AUDIO(lambda l, r: None),
                          BATCH(lambda data, frames: frames), POLL(lambda: None),
                          INPUT(self.input_state)]
        try:
            for name, callback in zip(('environment', 'video_refresh', 'audio_sample',
                                      'audio_sample_batch', 'input_poll', 'input_state'), self.callbacks):
                getattr(lib, 'retro_set_' + name).argtypes = [type(callback)]
                getattr(lib, 'retro_set_' + name)(callback)
            for name, restype in (('retro_get_memory_data', C.c_void_p),
                                  ('retro_get_memory_size', C.c_size_t),
                                  ('retro_ti_neogeo_region', C.c_void_p),
                                  ('retro_ti_neogeo_size', C.c_uint32),
                                  ('retro_ti_neogeo_info', C.c_int32)):
                getattr(lib, name).argtypes = [C.c_uint]
                getattr(lib, name).restype = restype
            lib.retro_ti_neogeo_bios_name.restype = C.c_char_p
            lib.retro_ti_neogeo_bus_read.argtypes = [C.c_uint32, C.c_uint32]
            lib.retro_ti_neogeo_bus_read.restype = C.c_uint32
            if [lib.retro_ti_neogeo_info(i) for i in (100, 101, 102, 103, 104)] != [1, 1, 0, 0, 1]:
                raise ValueError('expected ABI1/RTC export, little-endian core, speedhacks/fastmath off; rebuild core')
            lib.retro_ti_neogeo_clock_init.argtypes = []
            lib.retro_ti_neogeo_clock_init.restype = C.c_int32
            lib.retro_load_game.argtypes = [C.POINTER(GameInfo)]
            lib.retro_load_game.restype = C.c_bool
            lib.retro_serialize_size.restype = C.c_size_t
            for name in ('retro_serialize', 'retro_unserialize'):
                getattr(lib, name).argtypes = [C.c_void_p, C.c_size_t]
                getattr(lib, name).restype = C.c_bool
            _INITIALIZED_CORES.add(self.core)
            lib.retro_init()
            self.initialized = True
            info = GameInfo(str(self.rom).encode(), None, 0, None)
            self.loaded = lib.retro_load_game(C.byref(info))
            if self.callback_errors:
                raise ValueError('; '.join(self.callback_errors))
            if not self.loaded:
                raise ValueError('retro_load_game failed')
            size, pointer = lib.retro_ti_neogeo_size(0), lib.retro_ti_neogeo_region(0)
            # FBNeo can return true while showing its missing-ROM error GUI.
            if size != 65536 or not pointer:
                raise ValueError('game driver did not initialize 64 KiB RAM; check BIOS/set: '
                                 + '; '.join(self.messages))
            if lib.retro_ti_neogeo_clock_init() != 1:
                raise ValueError('reference clock must be initialized before the first CPU frame')
            for key, value in self.overrides.items():
                # Some controls (e.g. hiscores) are absent when no data exists.
                # Explicit user overrides still require a registered option.
                if (key in self.options or key.decode() in (options or {})) and self.options.get(key) != value:
                    raise ValueError(f'core did not accept option {key.decode()}={value.decode()}')
            self.ram = (C.c_uint8 * size).from_address(pointer)
            for port in (0, 1):
                lib.retro_set_controller_port_device(port, 5)  # FBNeo Classic RetroPad
            av, system_info = AVInfo(), SystemInfo()
            lib.retro_get_system_av_info(C.byref(av))
            lib.retro_get_system_info(C.byref(system_info))
            self.fps = av.timing.fps
            self.identity = dict(core=system_info.name.decode(), version=system_info.version.decode(),
                                 core_revision=CORE_REV, core_sha256=sha(self.core.read_bytes()),
                                 fps=self.fps, geometry=[av.geometry.width, av.geometry.height],
                                 rom_sha256=sha(self.rom.read_bytes()), chips=self.rom_manifest,
                                 bios_archive_sha256=sha(self.bios.read_bytes()),
                                 bios_chips=self.bios_manifest,
                                 rtc_initial='2000-01-01 Saturday 00:00:00; advances on emulated ticks',
                                 persistence='fresh temporary save directory; no NVRAM/card imported',
                                 sampling='zero-based; after retro_run; state load clears both frontend pads',
                                 reference_build='SUBSET=neogeo USE_SPEEDHACKS=0 FASTMATH=0')
        except Exception:
            self.close()
            raise

    def environment(self, cmd, data):
        if cmd == 10:
            self.format = C.cast(data, C.POINTER(C.c_int))[0]
            return self.format in (0, 1, 2)
        if cmd in self.directories:
            C.cast(data, C.POINTER(C.c_void_p))[0] = C.addressof(self.directories[cmd])
            return True
        if cmd == 16:
            variables = C.cast(data, C.POINTER(Variable))
            i = 0
            while variables[i].key:
                key = variables[i].key
                choices = variables[i].value.split(b'; ', 1)[1].split(b'|')
                selected = self.overrides.get(key, self.options.get(key, choices[0]))
                if selected not in choices:
                    raise ValueError('unsupported option value for ' + key.decode())
                self.allowed_options[key] = choices
                self.options[key] = selected
                i += 1
            return True
        if cmd == 15:
            variable = C.cast(data, C.POINTER(Variable))
            value = self.options.get(variable[0].key)
            if value is None:
                return False
            variable[0].value = value
            return True
        if cmd == 70:  # SET_VARIABLE, used when core initializes cheats/DIPs.
            variable = C.cast(data, C.POINTER(Variable))[0]
            value = self.overrides.get(variable.key, variable.value)
            if value not in self.allowed_options.get(variable.key, []):
                return False
            self.options[variable.key] = value
            return True
        if cmd == 17:
            C.cast(data, C.POINTER(C.c_bool))[0] = False
            return True
        if cmd == 52:  # Request legacy SET_VARIABLES, including dynamic game DIPs.
            C.cast(data, C.POINTER(C.c_uint))[0] = 0
            return True
        if cmd == 3:
            C.cast(data, C.POINTER(C.c_bool))[0] = True
            return True
        if cmd == 47:
            C.cast(data, C.POINTER(C.c_int))[0] = 3  # render and emulate audio, discard samples
            return True
        if cmd == 11:  # SET_INPUT_DESCRIPTORS
            descriptors = C.cast(data, C.POINTER(InputDescriptor))
            self.descriptors = []
            i = 0
            while descriptors[i].description:
                d = descriptors[i]
                self.descriptors.append(dict(port=d.port, device=d.device, index=d.index,
                                             id=d.id, description=d.description.decode()))
                i += 1
            return True
        if cmd == 6:
            self.messages.append(C.cast(data, C.POINTER(Message))[0].msg.decode())
            return True
        if cmd in (35, 42, 51):  # controller descriptors, serialization quirks, bitmasks
            return True
        return False

    def input_state(self, port, device, index, button):
        if port > 1 or device & 255 != 1 or index:
            return 0
        mask = self.pressed[port]
        if button == 256:
            return mask if mask < 32768 else mask - 65536
        return (mask >> button) & 1

    def close(self):
        if self.loaded:
            self.lib.retro_unload_game()
            self.loaded = False
        if self.initialized:
            self.lib.retro_deinit()
            self.initialized = False
        self.scratch.cleanup()

    @staticmethod
    def address(address, size=1):
        if size not in (1, 2, 4) or not 0x100000 <= address <= 0x110000 - size:
            raise ValueError('work RAM access must be within 100000..10FFFF; size 1, 2 or 4')
        return address - 0x100000

    def read(self, address, size=1, signed=False):
        offset = self.address(address, size)
        data = bytes(self.ram[(offset + i) ^ 1] for i in range(size))
        return int.from_bytes(data, 'big', signed=signed)

    def write(self, address, value, size=1):
        offset = self.address(address, size)
        data = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'big')
        for i, byte in enumerate(data):
            self.ram[(offset + i) ^ 1] = byte

    def region(self, ident, expected):
        size, pointer = self.lib.retro_ti_neogeo_size(ident), self.lib.retro_ti_neogeo_region(ident)
        if not pointer or size != expected:
            raise ValueError('reference region unavailable; rebuild the patched Neo Geo core')
        return C.string_at(pointer, size)

    def dump(self):
        return swap_words(bytes(self.ram))

    def video(self):
        # RAM, graphics and palettes are word-swapped in the pinned Linux build.
        return {name: swap_words(self.region(ident, size))
                for name, ident, size in (('vram', 1, 131072),
                                         ('palette0', 2, 8192), ('palette1', 3, 8192))}

    def status(self):
        names = ('bios_index', 'system_type', 'system_setting', 'palette_bank',
                 'animation_frame', 'animation_speed', 'animation_timer', 'bios_fix',
                 'darken', 'graphics_enabled', 'sprites_enabled', 'fix_enabled',
                 'vram_pointer_bytes', 'vram_modulo_bytes')
        status = {name: self.lib.retro_ti_neogeo_info(i) for i, name in enumerate(names)}
        bios_name = self.lib.retro_ti_neogeo_bios_name()
        status['bios_name'] = bios_name.decode() if bios_name else None
        status['bios_crc32'] = f'{self.lib.retro_ti_neogeo_info(14) & 0xffffffff:08x}'
        return status

    def save(self, path):
        size = self.lib.retro_serialize_size()
        if not size:
            raise ValueError('state unavailable')
        data = C.create_string_buffer(size)
        if not self.lib.retro_serialize(data, size):
            raise ValueError('state serialization failed')
        write_file(path, data.raw)

    def load(self, path):
        data = Path(path).read_bytes()
        if len(data) != self.lib.retro_serialize_size():
            raise ValueError('state size differs from the current core/game/settings')
        if not self.lib.retro_unserialize(C.create_string_buffer(data), len(data)):
            raise ValueError('state load failed')
        self.pressed[:] = [0, 0]
        self.frame = None

    def step(self):
        self.lib.retro_run()
        if self.callback_errors:
            raise ValueError('; '.join(self.callback_errors))

    def image(self):
        from PIL import Image
        if self.frame is None:
            raise ValueError('no video frame; run one frame after boot/load')
        data, width, height, pitch = self.frame
        if self.format == 1:
            return Image.frombuffer('RGBX', (width, height), data, 'raw', 'BGRX', pitch, 1).convert('RGB')
        mode = 'BGR;16' if self.format == 2 else 'BGR;15'
        return Image.frombuffer('RGB', (width, height), data, 'raw', mode, pitch, 1)

    def snapshot(self, directory):
        directory = Path(directory)
        for name, data in {'ram': self.dump(), **self.video(),
                           'nvram.raw': self.region(4, 65536),
                           'card.raw': self.region(5, 131072),
                           'inputs': self.region(6, 32), 'z80ram': self.region(7, 2048)}.items():
            write_file(directory / (name + '.bin'), data)
        write_file(directory / 'status.json', json.dumps(self.status(), indent=2) + '\n')

    def metadata(self):
        return {**self.identity, 'status': self.status(), 'input_descriptors': self.descriptors,
                'options': {k.decode(): v.decode() for k, v in self.options.items()}}


def write_file(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(data, str):
        path.write_text(data)
    else:
        path.write_bytes(data)


def script(path):
    """Each line sets both controllers; unspecified players release all keys."""
    result = {}
    if path:
        for lineno, line in enumerate(Path(path).read_text().splitlines(), 1):
            fields = line.split('#', 1)[0].split()
            if not fields:
                continue
            frame, masks = int(fields[0]), [0, 0]
            if frame < 0 or frame in result:
                raise ValueError(f'negative/duplicate script frame on line {lineno}')
            for field in fields[1:]:
                parts = field.upper().split(':', 1)
                player, button = parts if len(parts) == 2 else ('P1', parts[0])
                if player not in ('P1', 'P2') or button not in PAD:
                    raise ValueError(f'unknown player/button {field} on line {lineno}')
                masks[int(player[1]) - 1] |= 1 << PAD[button]
            result[frame] = masks
    return result


def events(spec):
    result = {}
    for item in filter(None, spec.split(',')):
        frame, path = item.split(':', 1)
        frame = int(frame)
        if frame < 0 or frame in result:
            raise ValueError('negative/duplicate output event')
        result[frame] = Path(path)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path, nargs='?', default=DEFAULT_ROM)
    parser.add_argument('--bios', type=Path, default=DEFAULT_BIOS)
    parser.add_argument('--core', type=Path, default=DEFAULT_CORE)
    parser.add_argument('--frames', type=int, required=True)
    parser.add_argument('--keys', type=Path)
    parser.add_argument('--load', type=Path)
    parser.add_argument('--save', type=Path)
    parser.add_argument('--trace', default='', help='hex-address:size,...; s2 is signed word')
    parser.add_argument('--poke', default='', help='hex-address:size=value,...; before frame 0')
    parser.add_argument('--shot', default='', help='frame:path.png,...')
    parser.add_argument('--dump', default='', help='frame:path.bin,...; CPU byte order')
    parser.add_argument('--video', default='', help='frame:directory,...; RAM/VRAM/palettes/status')
    parser.add_argument('--gif', type=Path)
    parser.add_argument('--every', type=int, default=1)
    parser.add_argument('--metadata', type=Path)
    parser.add_argument('--option', action='append', default=[], help='core-key=value')
    args = parser.parse_args()
    if args.frames < 1 or args.every < 1:
        parser.error('--frames and --every must be positive')
    try:
        keys, shots, dumps, snapshots = script(args.keys), events(args.shot), events(args.dump), events(args.video)
        if any(i >= args.frames for table in (keys, shots, dumps, snapshots) for i in table):
            raise ValueError('script/output events must be within --frames')
        traces = []
        for item in filter(None, args.trace.split(',')):
            address, kind = (item.split(':') + ['1'])[:2]
            size = int(kind.lstrip('s'))
            NeoGeo.address(int(address, 16), size)
            traces.append((item, int(address, 16), size, kind.startswith('s')))
        options = dict(item.split('=', 1) for item in args.option)
        neo = NeoGeo(args.rom, args.bios, args.core, options)
        try:
            if args.load:
                neo.load(args.load)
            for item in filter(None, args.poke.split(',')):
                left, value = item.split('=')
                address, size = (left.split(':') + ['1'])[:2]
                neo.write(int(address, 16), int(value, 0), int(size))
            pictures = []
            for frame in range(args.frames):
                if frame in keys:
                    neo.pressed[:] = keys[frame]
                neo.step()
                if frame in shots:
                    shots[frame].parent.mkdir(parents=True, exist_ok=True)
                    neo.image().save(shots[frame])
                if frame in dumps:
                    write_file(dumps[frame], neo.dump())
                if frame in snapshots:
                    neo.snapshot(snapshots[frame])
                if frame % args.every == 0:
                    if traces:
                        print(frame, *(f'{name}={neo.read(address, size, signed)}'
                                       for name, address, size, signed in traces))
                    if args.gif:
                        pictures.append(neo.image())
            if pictures:
                args.gif.parent.mkdir(parents=True, exist_ok=True)
                pictures[0].save(args.gif, save_all=True, append_images=pictures[1:],
                                 duration=round(1000 * args.every / neo.fps), loop=0)
            if args.save:
                neo.save(args.save)
            if args.metadata:
                inputs = {name: {'path': str(path), 'sha256': sha(path.read_bytes())} if path else None
                          for name, path in (('keys', args.keys), ('load', args.load))}
                write_file(args.metadata, json.dumps({**neo.metadata(), 'frames': args.frames,
                           'inputs': inputs, 'pokes': args.poke}, indent=2) + '\n')
        finally:
            neo.close()
    except (OSError, ValueError, zipfile.BadZipFile, AttributeError) as error:
        parser.exit(1, f'Neo Geo reference failed: {error}\n')


if __name__ == '__main__':
    main()
