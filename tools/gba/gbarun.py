#!/usr/bin/env python3
"""Headless GBA reference instrument using pinned mGBA/libretro.

Only canonical exported memory ranges are accepted, in little-endian CPU order.
RAM pokes are direct memory edits, not emulated CPU bus transactions.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
CORE_REV = '26b7884bc25a5933960f3cdcd98bac1ae14d42e2'
DEFAULT_CORE = ROOT / 'sources/gba_core/build/mgba_libretro.so'
PAD = {'UP': 4, 'DOWN': 5, 'LEFT': 6, 'RIGHT': 7,
       'A': 8, 'B': 0, 'L': 10, 'R': 11, 'SELECT': 2, 'START': 3}
REGIONS = {'ewram': (0x02000000, 0x40000), 'iwram': (0x03000000, 0x8000),
           'io': (0x04000000, 0x400), 'palette': (0x05000000, 0x400),
           'vram': (0x06000000, 0x18000), 'oam': (0x07000000, 0x400)}
ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUDIO = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL = C.CFUNCTYPE(None)
INPUT = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)


class GameInfo(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p),
                ('size', C.c_size_t), ('meta', C.c_char_p)]


class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]


class Descriptor(C.Structure):
    _fields_ = [('flags', C.c_uint64), ('ptr', C.c_void_p),
                ('offset', C.c_size_t), ('start', C.c_size_t),
                ('select', C.c_size_t), ('disconnect', C.c_size_t),
                ('len', C.c_size_t), ('addrspace', C.c_char_p)]


class MemoryMap(C.Structure):
    _fields_ = [('descriptors', C.POINTER(Descriptor)), ('count', C.c_uint)]


class Timing(C.Structure):
    _fields_ = [('fps', C.c_double), ('sample_rate', C.c_double)]


class Geometry(C.Structure):
    _fields_ = [('width', C.c_uint), ('height', C.c_uint),
                ('max_width', C.c_uint), ('max_height', C.c_uint),
                ('aspect', C.c_float)]


class AVInfo(C.Structure):
    _fields_ = [('geometry', Geometry), ('timing', Timing)]


class SystemInfo(C.Structure):
    _fields_ = [('name', C.c_char_p), ('version', C.c_char_p),
                ('extensions', C.c_char_p), ('fullpath', C.c_bool),
                ('block_extract', C.c_bool)]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class GBA:
    """One core/game per process; callers must close before opening another."""
    def __init__(self, rom, core=DEFAULT_CORE):
        if not Path(core).is_file():
            raise ValueError('missing local core; run make -C tools/gba core')
        self.rom, self.core = Path(rom), Path(core)
        self.lib = lib = C.CDLL(str(self.core.resolve()))
        self.frame = None
        self.format = 0
        self.pressed = 0
        self.maps = {}
        self.options = {}
        self.overrides = {b'mgba_use_bios': b'OFF', b'mgba_skip_bios': b'OFF',
                          b'mgba_frameskip': b'0',
                          b'mgba_idle_optimization': b"Don't Remove"}
        self.directory = str(self.core.resolve().parent).encode()

        def environment(cmd, data):
            cmd &= 0xffff
            if cmd == 10:
                self.format = C.cast(data, C.POINTER(C.c_int))[0]
                return self.format in (0, 1, 2)
            if cmd in (9, 31):
                C.cast(data, C.POINTER(C.c_char_p))[0] = self.directory
                return True
            if cmd == 16:  # Legacy options; newer option versions are declined.
                variables = C.cast(data, C.POINTER(Variable))
                i = 0
                while variables[i].key:
                    key = variables[i].key
                    default = variables[i].value.split(b'; ', 1)[1].split(b'|')[0]
                    self.options[key] = self.overrides.get(key, default)
                    i += 1
                return True
            if cmd == 15:
                variable = C.cast(data, C.POINTER(Variable))[0]
                value = self.options.get(variable.key)
                if value is None:
                    return False
                variable.value = value
                return True
            if cmd == 17:
                C.cast(data, C.POINTER(C.c_bool))[0] = False
                return True
            if cmd == 36:
                mapping = C.cast(data, C.POINTER(MemoryMap))[0]
                # mGBA supplies stack descriptors: copy values during the callback.
                self.maps = {d.start: (d.ptr + d.offset, d.len)
                             for d in mapping.descriptors[:mapping.count] if d.ptr}
                return True
            if cmd == 3:
                C.cast(data, C.POINTER(C.c_bool))[0] = True
                return True
            if cmd in (18, 51):
                return True
            return False

        def video(data, width, height, pitch):
            if data:
                self.frame = (C.string_at(data, pitch * height), width, height, pitch)

        def input_state(port, device, index, button):
            if port != 0 or (device & 255) != 1:
                return 0
            return self.pressed if button == 256 else (self.pressed >> button) & 1

        self.callbacks = [ENV(environment), VIDEO(video), AUDIO(lambda l, r: None),
                          BATCH(lambda data, frames: frames), POLL(lambda: None), INPUT(input_state)]
        for name, callback in zip(('environment', 'video_refresh', 'audio_sample',
                                   'audio_sample_batch', 'input_poll', 'input_state'), self.callbacks):
            setter = getattr(lib, 'retro_set_' + name)
            setter.argtypes = [type(callback)]
            setter(callback)
        lib.retro_init()
        # Supply bytes as well as a path: mGBA advertises need_fullpath=false.
        self.rom_buffer = C.create_string_buffer(self.rom.read_bytes())
        info = GameInfo(str(self.rom.resolve()).encode(), C.cast(self.rom_buffer, C.c_void_p),
                        len(self.rom_buffer) - 1, None)
        lib.retro_load_game.argtypes = [C.POINTER(GameInfo)]
        lib.retro_load_game.restype = C.c_bool
        if not lib.retro_load_game(C.byref(info)):
            lib.retro_deinit()
            raise ValueError('retro_load_game failed: ' + str(rom))
        lib.retro_set_controller_port_device(0, 1)
        lib.retro_get_memory_data.argtypes = [C.c_uint]
        lib.retro_get_memory_data.restype = C.c_void_p
        lib.retro_get_memory_size.argtypes = [C.c_uint]
        lib.retro_get_memory_size.restype = C.c_size_t
        lib.retro_serialize_size.restype = C.c_size_t
        for name in ('retro_serialize', 'retro_unserialize'):
            method = getattr(lib, name)
            method.argtypes = [C.c_void_p, C.c_size_t]
            method.restype = C.c_bool
        for name, (base, size) in REGIONS.items():
            if self.maps.get(base, (None, 0))[1] != size:
                self.close()
                raise ValueError('missing or unexpected GBA region: ' + name)
        av, system = AVInfo(), SystemInfo()
        lib.retro_get_system_av_info(C.byref(av))
        lib.retro_get_system_info(C.byref(system))
        self.fps = av.timing.fps
        self.identity = {'core': system.name.decode(), 'version': system.version.decode(),
                         'fps': self.fps, 'bios': 'mGBA HLE; external BIOS disabled',
                         'rom_sha256': sha(self.rom), 'core_sha256': sha(self.core),
                         'options': {k.decode(): v.decode() for k, v in self.options.items()}}

    def close(self):
        self.lib.retro_unload_game()
        self.lib.retro_deinit()

    def pointer(self, address, size):
        for base, length in REGIONS.values():
            if size > 0 and base <= address and address + size <= base + length:
                return self.maps[base][0] + address - base
        raise ValueError('access outside canonical exported GBA memory ranges')

    def read(self, address, size=1, signed=False):
        if size not in (1, 2, 4):
            raise ValueError('read sizes must be 1, 2 or 4')
        return int.from_bytes(C.string_at(self.pointer(address, size), size), 'little', signed=signed)

    def write(self, address, value, size=1):
        if size not in (1, 2, 4) or not any(base <= address and address + size <= base + length
            for base, length in (REGIONS['ewram'], REGIONS['iwram'])):
            raise ValueError('pokes only support canonical EWRAM/IWRAM, sizes 1, 2 or 4')
        raw = (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little')
        C.memmove(self.pointer(address, size), raw, size)

    def memory(self, name):
        base, size = REGIONS[name]
        return C.string_at(self.pointer(base, size), size)

    def savedata(self):
        size = self.lib.retro_get_memory_size(0)
        pointer = self.lib.retro_get_memory_data(0)
        if size and not pointer:
            raise ValueError('save memory has no exported pointer')
        return C.string_at(pointer, size) if size else b''

    def snapshot(self):
        return {**{name: self.memory(name) for name in REGIONS}, 'savedata': self.savedata()}

    def step(self, buttons=None):
        if buttons is not None:
            self.pressed = sum(1 << PAD[key] for key in set(buttons))
        self.lib.retro_run()

    def save(self, path):
        size = self.lib.retro_serialize_size()
        data = C.create_string_buffer(size)
        if not size or not self.lib.retro_serialize(data, size):
            raise ValueError('state serialization failed')
        # Libretro intentionally does not reload cartridge saves on unserialize.
        # Keep save bytes and provenance in our container, not a bare core state.
        metadata = json.dumps(self.identity, sort_keys=True).encode()
        savedata = self.savedata()
        Path(path).write_bytes(struct.pack('<8sIII', b'GBARUN1\0', len(metadata), size, len(savedata))
                               + metadata + data.raw + savedata)

    def load(self, path):
        raw = Path(path).read_bytes()
        if len(raw) < 20:
            raise ValueError('truncated gbarun state')
        magic, meta_size, core_size, save_size = struct.unpack('<8sIII', raw[:20])
        if magic != b'GBARUN1\0' or len(raw) != 20 + meta_size + core_size + save_size:
            raise ValueError('invalid gbarun state container')
        metadata = json.loads(raw[20:20 + meta_size])
        for key in ('rom_sha256', 'core_sha256', 'options', 'bios'):
            if metadata[key] != self.identity[key]:
                raise ValueError('state provenance mismatch: ' + key)
        start = 20 + meta_size
        if not self.lib.retro_unserialize(C.create_string_buffer(raw[start:start + core_size]), core_size):
            raise ValueError('core state load failed')
        if self.lib.retro_get_memory_size(0) != save_size:
            raise ValueError('state save-memory size mismatch')
        if save_size:
            C.memmove(self.lib.retro_get_memory_data(0), raw[start + core_size:], save_size)
        self.pressed = 0
        self.frame = None  # A new callback is required; do not reuse stale video.

    def image(self):
        from PIL import Image
        if self.frame is None:
            raise ValueError('run a frame before reading video')
        data, width, height, pitch = self.frame
        if self.format == 1:
            return Image.frombuffer('RGBX', (width, height), data, 'raw', 'BGRX', pitch, 1).convert('RGB')
        mode = 'BGR;16' if self.format == 2 else 'BGR;15'
        return Image.frombuffer('RGB', (width, height), data, 'raw', mode, pitch, 1)


def script(path):
    result = {}
    if path:
        for line in Path(path).read_text().splitlines():
            fields = line.split('#')[0].split()
            if fields:
                frame = int(fields[0])
                if frame < 0 or frame in result:
                    raise ValueError('key frames must be nonnegative and unique')
                result[frame] = sum(1 << PAD[key] for key in {key.upper() for key in fields[1:]})
    return result


def events(spec):
    return {int(frame): Path(path) for frame, path in
            (entry.split(':', 1) for entry in spec.split(',') if entry)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--core', type=Path, default=DEFAULT_CORE)
    parser.add_argument('--frames', type=int, required=True)
    parser.add_argument('--keys', type=Path)
    parser.add_argument('--load', type=Path)
    parser.add_argument('--save', type=Path)
    parser.add_argument('--trace', default='', help='hex-address:size,... (s2 = signed word)')
    parser.add_argument('--poke', default='', help='hex-address:size=value,...; RAM only')
    parser.add_argument('--shot', default='', help='frame:path.png,...')
    parser.add_argument('--dump', default='', help='frame:directory,...; all exported regions')
    parser.add_argument('--gif', type=Path)
    parser.add_argument('--every', type=int, default=1)
    parser.add_argument('--metadata', type=Path)
    args = parser.parse_args()
    if args.frames < 1 or args.every < 1:
        parser.error('--frames and --every must be positive')
    keys, shots, dumps = script(args.keys), events(args.shot), events(args.dump)
    if any(frame < 0 or frame >= args.frames for frame in (*keys, *shots, *dumps)):
        parser.error('key/shot/dump events must be within --frames')
    variables = []
    for entry in filter(None, args.trace.split(',')):
        address, kind = (entry.split(':') + ['1'])[:2]
        size = int(kind.lstrip('s'))
        if size not in (1, 2, 4):
            parser.error('trace sizes must be 1, 2 or 4')
        variables.append((entry, int(address, 16), size, kind.startswith('s')))
    gba = GBA(args.rom, args.core)
    try:
        load_hash = sha(args.load) if args.load else None
        if args.load:
            gba.load(args.load)
        for entry in filter(None, args.poke.split(',')):
            lhs, value = entry.split('=')
            address, size = (lhs.split(':') + ['1'])[:2]
            gba.write(int(address, 16), int(value, 0), int(size))
        pictures = []
        for frame in range(args.frames):
            if frame in keys:
                gba.pressed = keys[frame]
            gba.step()
            if frame in shots:
                shots[frame].parent.mkdir(parents=True, exist_ok=True)
                gba.image().save(shots[frame])
            if frame in dumps:
                dumps[frame].mkdir(parents=True, exist_ok=True)
                for name, data in gba.snapshot().items():
                    (dumps[frame] / (name + '.bin')).write_bytes(data)
            if frame % args.every == 0:
                if variables:
                    print(frame, *(f'{name}={gba.read(address, size, signed)}'
                                   for name, address, size, signed in variables))
                if args.gif:
                    pictures.append(gba.image())
        if args.gif and pictures:
            args.gif.parent.mkdir(parents=True, exist_ok=True)
            pictures[0].save(args.gif, save_all=True, append_images=pictures[1:],
                             duration=round(1000 * args.every / gba.fps), loop=0)
        if args.save:
            args.save.parent.mkdir(parents=True, exist_ok=True)
            gba.save(args.save)
        if args.metadata:
            args.metadata.parent.mkdir(parents=True, exist_ok=True)
            args.metadata.write_text(json.dumps({**gba.identity,
                'pinned_revision': CORE_REV if args.core.resolve() == DEFAULT_CORE else None,
                'frames': args.frames, 'keys': str(args.keys) if args.keys else None,
                'keys_sha256': sha(args.keys) if args.keys else None,
                'load': str(args.load) if args.load else None,
                'load_sha256': load_hash,
                'pokes': args.poke, 'frame_index': 'zero-based, sampled after retro_run'}, indent=2) + '\n')
    finally:
        gba.close()


if __name__ == '__main__':
    main()
