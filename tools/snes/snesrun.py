#!/usr/bin/env python3
"""Headless SNES reference runner (pinned Snes9x/libretro).

Frame 0 is the first frame after boot or state load. WRAM addresses use 7E0000..7FFFFF and little-endian CPU byte order.
Super FX cartridge RAM is available separately through sram().
The core is a local build; no ROM, state or extracted data is distributed.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CORE = ROOT / 'sources/snes_core/libretro/snes9x_libretro.so'
PAD = {'UP': 4, 'DOWN': 5, 'LEFT': 6, 'RIGHT': 7,
       'A': 8, 'B': 0, 'X': 9, 'Y': 1, 'L': 10, 'R': 11,
       'SELECT': 2, 'START': 3}
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


class SystemInfo(C.Structure):
    _fields_ = [('name', C.c_char_p), ('version', C.c_char_p),
                ('extensions', C.c_char_p), ('fullpath', C.c_bool),
                ('block_extract', C.c_bool)]


class Timing(C.Structure):
    _fields_ = [('fps', C.c_double), ('sample_rate', C.c_double)]


class Geometry(C.Structure):
    _fields_ = [('width', C.c_uint), ('height', C.c_uint),
                ('max_width', C.c_uint), ('max_height', C.c_uint),
                ('aspect', C.c_float)]


class AVInfo(C.Structure):
    _fields_ = [('geometry', Geometry), ('timing', Timing)]


class SNES:
    def __init__(self, rom, core=DEFAULT_CORE):
        if not Path(core).is_file():
            raise ValueError('missing local core; run make -C tools/snes core')
        self.lib = lib = C.CDLL(str(Path(core).resolve()))
        self.frame = None
        self.format = 0
        self.pressed = 0
        self.options = {}
        self.directory = str(Path(core).resolve().parent).encode()

        def environment(cmd, data):
            cmd &= 0xffff
            if cmd == 10:
                self.format = C.cast(data, C.POINTER(C.c_int))[0]
                return self.format in (0, 1, 2)
            if cmd in (9, 31):
                C.cast(data, C.POINTER(C.c_char_p))[0] = self.directory
                return True
            if cmd == 16:  # SET_VARIABLES: retain defaults from the core.
                variables = C.cast(data, C.POINTER(Variable))
                i = 0
                while variables[i].key:
                    self.options[variables[i].key] = variables[i].value.split(b'; ', 1)[1].split(b'|')[0]
                    i += 1
                return True
            if cmd == 15:
                variable = C.cast(data, C.POINTER(Variable))
                value = self.options.get(variable[0].key)
                if value is None:
                    return False
                variable[0].value = value
                return True
            if cmd == 17:
                C.cast(data, C.POINTER(C.c_bool))[0] = False
                return True
            if cmd == 3:
                C.cast(data, C.POINTER(C.c_bool))[0] = True
                return True
            if cmd == 51:
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
            getattr(lib, 'retro_set_' + name)(callback)
        lib.retro_init()
        info = GameInfo(str(Path(rom).resolve()).encode(), None, 0, None)
        lib.retro_load_game.argtypes = [C.POINTER(GameInfo)]
        lib.retro_load_game.restype = C.c_bool
        if not lib.retro_load_game(C.byref(info)):
            lib.retro_deinit()
            raise ValueError('retro_load_game failed: ' + str(rom))
        lib.retro_set_controller_port_device(0, 1)
        lib.retro_get_memory_data.restype = C.c_void_p
        lib.retro_get_memory_size.restype = C.c_size_t
        size = lib.retro_get_memory_size(2)
        pointer = lib.retro_get_memory_data(2)
        if size != 131072 or not pointer:
            self.close()
            raise ValueError('expected 128 KiB SNES WRAM')
        self.ram = (C.c_uint8 * size).from_address(pointer)
        lib.retro_serialize_size.restype = C.c_size_t
        lib.retro_serialize.argtypes = [C.c_void_p, C.c_size_t]
        lib.retro_serialize.restype = C.c_bool
        lib.retro_unserialize.argtypes = [C.c_void_p, C.c_size_t]
        lib.retro_unserialize.restype = C.c_bool
        av = AVInfo()
        lib.retro_get_system_av_info(C.byref(av))
        self.fps = av.timing.fps
        system = SystemInfo()
        lib.retro_get_system_info(C.byref(system))
        self.identity = {'core': system.name.decode(), 'version': system.version.decode(),
                         'fps': self.fps, 'ram_bytes': size}

    def close(self):
        self.lib.retro_unload_game()
        self.lib.retro_deinit()

    @staticmethod
    def address(address, size=1):
        if size not in (1, 2, 4) or not 0x7e0000 <= address <= 0x800000 - size:
            raise ValueError('WRAM access must be within 7E0000..7FFFFF; size 1, 2 or 4')
        return address - 0x7e0000

    def read(self, address, size=1, signed=False):
        if 0x700000 <= address < 0x720000:
            offset = address - 0x700000
            raw = self.sram()
            if size not in (1, 2, 4) or offset + size > len(raw):
                raise ValueError('cartridge RAM access outside exported region')
            return int.from_bytes(raw[offset:offset + size], 'little', signed=signed)
        offset = self.address(address, size)
        return int.from_bytes(bytes(self.ram[offset:offset + size]), 'little', signed=signed)

    def write(self, address, value, size=1):
        if 0x700000 <= address < 0x720000:
            offset = address - 0x700000
            length = self.lib.retro_get_memory_size(0)
            if size not in (1, 2, 4) or offset + size > length:
                raise ValueError('cartridge RAM access outside exported region')
            ram = (C.c_uint8 * length).from_address(self.lib.retro_get_memory_data(0))
        else:
            offset = self.address(address, size)
            ram = self.ram
        raw = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
        ram[offset:offset + size] = raw

    def dump(self):
        return bytes(self.ram)

    def memory(self, ident, expected=None):
        size = self.lib.retro_get_memory_size(ident)
        pointer = self.lib.retro_get_memory_data(ident)
        if not pointer or not size or (expected is not None and size != expected):
            raise ValueError('snapshot unavailable; rebuild tools/snes core')
        return C.string_at(pointer, size)

    def sram(self):
        """Cartridge RAM, including the Super FX workspace for Yoshi's Island."""
        return self.memory(0)

    def ppu(self):
        """VRAM bytes, CGRAM LE words, OAM bytes, and raw PPU register mirrors.

        Register mirrors are not a complete latched PPU state (scroll/latches
        and mid-frame HDMA need separate study). These exports require our patch.
        """
        return (self.memory(3, 65536), self.memory(0x10000, 512),
                self.memory(0x10001, 544), self.memory(0x10002, 64))

    def save(self, path):
        size = self.lib.retro_serialize_size()
        data = C.create_string_buffer(size)
        if not self.lib.retro_serialize(data, size):
            raise ValueError('state serialization failed')
        Path(path).write_bytes(data.raw)

    def load(self, path):
        data = Path(path).read_bytes()
        self.lib.retro_run()  # Initialize video/audio before loading the state.
        if not self.lib.retro_unserialize(C.create_string_buffer(data), len(data)):
            raise ValueError('state load failed: ' + str(path))

    def image(self):
        from PIL import Image
        if self.frame is None:
            raise ValueError('no video frame available')
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
                result[int(fields[0])] = sum(1 << PAD[key] for key in set(fields[1:]))
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
    parser.add_argument('--poke', default='', help='hex-address:size=value,...; value accepts 0x')
    parser.add_argument('--shot', default='', help='frame:path.png,...')
    parser.add_argument('--dump', default='', help='frame:path.bin,...; CPU byte order')
    parser.add_argument('--ppu', default='', help='frame:directory,...; VRAM/CGRAM/OAM/register mirrors/SRAM')
    parser.add_argument('--gif', type=Path)
    parser.add_argument('--every', type=int, default=1)
    parser.add_argument('--metadata', type=Path)
    args = parser.parse_args()
    if args.frames < 1 or args.every < 1:
        parser.error('--frames and --every must be positive')
    keys, shots, dumps = script(args.keys), events(args.shot), events(args.dump)
    snapshots = events(args.ppu)
    variables = []
    for entry in filter(None, args.trace.split(',')):
        address, kind = (entry.split(':') + ['1'])[:2]
        size = int(kind.lstrip('s'))
        if size not in (1, 2, 4):
            parser.error('trace sizes must be 1, 2 or 4')
        variables.append((entry, int(address, 16), size, kind.startswith('s')))
    snes = SNES(args.rom, args.core)
    try:
        if args.load:
            snes.load(args.load)
        for entry in filter(None, args.poke.split(',')):
            lhs, value = entry.split('=')
            address, size = (lhs.split(':') + ['1'])[:2]
            snes.write(int(address, 16), int(value, 0), int(size))
        pictures = []
        for frame in range(args.frames):
            if frame in keys:
                snes.pressed = keys[frame]
            snes.lib.retro_run()
            if frame in shots:
                snes.image().save(shots[frame])
            if frame in dumps:
                dumps[frame].write_bytes(snes.dump())
            if frame in snapshots:
                directory = snapshots[frame]
                directory.mkdir(parents=True, exist_ok=True)
                for name, data in zip(('vram', 'cgram', 'oam', 'regs', 'sram'), (*snes.ppu(), snes.sram())):
                    (directory / (name + '.bin')).write_bytes(data)
            if frame % args.every == 0:
                if variables:
                    print(frame, *(f'{name}={snes.read(address, size, signed)}'
                                   for name, address, size, signed in variables))
                if args.gif:
                    pictures.append(snes.image())
        if args.gif and pictures:
            pictures[0].save(args.gif, save_all=True, append_images=pictures[1:],
                             duration=round(1000 * args.every / snes.fps), loop=0)
        if args.save:
            snes.save(args.save)
        if args.metadata:
            args.metadata.write_text(json.dumps({**snes.identity,
                'rom_sha256': hashlib.sha256(args.rom.read_bytes()).hexdigest(),
                'core_sha256': hashlib.sha256(args.core.read_bytes()).hexdigest(),
                'frames': args.frames, 'keys': str(args.keys) if args.keys else None,
                'load': str(args.load) if args.load else None,
                'options': {k.decode(): v.decode() for k, v in snes.options.items()},
                'pokes': args.poke, 'frame_index': 'zero-based, sampled after retro_run' }, indent=2) + '\n')
    finally:
        snes.close()


if __name__ == '__main__':
    main()
