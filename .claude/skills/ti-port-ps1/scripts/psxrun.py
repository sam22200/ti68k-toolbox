#!/usr/bin/env python3
"""Run a PlayStation disc headless on the PC (the pcsx_rearmed libretro core, its HLE BIOS: no
Sony BIOS needed) and print a main-RAM trace per frame: the behavioural reference a port is
measured against. Also screenshots, GIFs, RAM dumps, save states, pokes.

usage: psxrun.py DISC.cue --frames N [--trace VARS] [--sym FILE] [--keys FILE] [--load STATE]
                 [--save STATE] [--poke ADDR=VAL,...] [--every K] [--from F] [--diff]
                 [--shot FRAME:FILE.png,...] [--gif FILE.gif] [--dump FRAME:FILE.bin,...]

--trace  comma list: a hex address (800a1234, or 0a1234: KSEG0 implied), ADDR:2 / ADDR:4 for a
         little-endian 16/32-bit word, ADDR:s2 / ADDR:s4 signed, LO-HI one byte each, a name
         from --sym (name:2 ...); each frame prints "<frame> name=hex ..." (or decimal if signed)
--sym    "ADDR name" lines (games/<name>/<name>.sym, written as the game is understood)
--keys   input script: lines "<frame> <keys...>" held until the next line; PS1 names UP DOWN
         LEFT RIGHT CROSS CIRCLE SQUARE TRIANGLE START SELECT L1 R1 L2 R2, and the runtime's
         A B C D through --map; frame 0 = the first frame run (after --load)
--map    runtime key -> pad button (default A=cross,B=square,C=start,D=select)
--load / --save   core save state (the injection door: a room reached once, reloaded in 0.1 s);
         --save writes after the last frame
--poke   values written before the first frame: ADDR=VAL (byte), ADDR:2=VAL, ADDR:4=VAL
--dump   the 2 MB of main RAM after the given frames (FRAME:FILE.bin): diff two dumps with
         ramdiff.py, or decompile a dump with code loaded at run time (overlays)
--vram   the 1 MB VRAM after the given frames (FRAME:FILE.bin, + FILE.png seen as 15-bit
         colour: 4/8-bit textures look striped, their layout and palettes show); needs the
         core built with scripts/pcsx_vram.patch
--memcard   memory card 1 (raw 128 KB .mcd/.mcr, as DuckStation, ePSXe, RetroArch write it)
         loaded before boot: a save made by hand in a windowed emulator, then CONTINUE;
         --memcard-out writes card 1 back after the run (saves made headless)
--shot   PNG of the frame displayed after the given frames; --gif the whole run (--every K)
Frame = one retro_run() = one displayed video frame (NTSC ~59.94 Hz). Games often run their
logic at 30 Hz or less: measure it (a counter that moves every 2nd frame).
"""
import argparse, ctypes as C, os, struct, sys

HERE = os.path.dirname(os.path.realpath(__file__))
CORE = os.path.realpath(os.path.join(HERE, '../../../../tools/pcsx_rearmed/pcsx_rearmed_libretro.so'))
PAD = {'B': 0, 'CROSS': 0, 'Y': 1, 'SQUARE': 1, 'SELECT': 2, 'START': 3, 'UP': 4, 'DOWN': 5,
       'LEFT': 6, 'RIGHT': 7, 'CIRCLE': 8, 'TRIANGLE': 9, 'L1': 10, 'R1': 11, 'L2': 12, 'R2': 13}
OPTIONS = {b'pcsx_rearmed_bios': b'HLE', b'pcsx_rearmed_drc': b'enabled',
           b'pcsx_rearmed_region': b'auto', b'pcsx_rearmed_show_bios_bootlogo': b'disabled',
           b'pcsx_rearmed_frameskip_type': b'disabled', b'pcsx_rearmed_spu_thread': b'disabled',
           b'pcsx_rearmed_gpu_thread_rendering': b'disabled', b'pcsx_rearmed_cd_turbo': b'disabled',
           b'pcsx_rearmed_memcard1': b'libretro'}
# PSX_OPTS="pcsx_rearmed_drc=disabled,..." overrides core options (a game that stalls under one)
OPTIONS.update((k.encode(), v.encode()) for k, v in
               (kv.split('=', 1) for kv in os.environ.get('PSX_OPTS', '').split(',') if kv))

ENV_CB = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO_CB = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUDIO_CB = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
AUDIOB_CB = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL_CB = C.CFUNCTYPE(None)
STATE_CB = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)


class GameInfo(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p), ('size', C.c_size_t), ('meta', C.c_char_p)]


class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]


class PSX:
    def __init__(self, disc, memcard=None):
        self.lib = C.CDLL(CORE)
        self.fmt = 0            # 0RGB1555 until the core asks for another
        self.frame = None       # (bytes, w, h, pitch)
        self.pressed = 0
        self.sysdir = os.path.join(os.path.dirname(CORE), 'system')
        os.makedirs(self.sysdir, exist_ok=True)
        self._keep = []
        self._opt = {}

        def env(cmd, data):
            cmd &= 0xffff
            if cmd == 10:                                        # SET_PIXEL_FORMAT
                self.fmt = C.cast(data, C.POINTER(C.c_int))[0]
                return True
            if cmd in (9, 31):                                   # GET_SYSTEM / SAVE_DIRECTORY
                s = C.c_char_p(self.sysdir.encode())
                self._keep.append(s)
                C.cast(data, C.POINTER(C.c_char_p))[0] = s.value
                return True
            if cmd == 15:                                        # GET_VARIABLE
                v = C.cast(data, C.POINTER(Variable))[0]
                val = OPTIONS.get(v.key)
                if val is None:
                    return False
                C.cast(data, C.POINTER(Variable))[0].value = val
                return True
            if cmd == 17:                                        # GET_VARIABLE_UPDATE
                C.cast(data, C.POINTER(C.c_bool))[0] = False
                return True
            if cmd == 3:                                         # GET_CAN_DUPE
                C.cast(data, C.POINTER(C.c_bool))[0] = True
                return True
            if cmd == 51:                                        # GET_INPUT_BITMASKS
                return True
            return False

        def video(data, w, h, pitch):
            if data:                                             # NULL = same frame again
                self.frame = (C.string_at(data, pitch * h), w, h, pitch)

        def state(port, device, index, id_):
            if port != 0 or device != 1:
                return 0
            if id_ == 256:                                       # RETRO_DEVICE_ID_JOYPAD_MASK
                return self.pressed
            return (self.pressed >> id_) & 1

        self._cbs = [ENV_CB(env), VIDEO_CB(video), AUDIO_CB(lambda l, r: None),
                     AUDIOB_CB(lambda d, n: n), POLL_CB(lambda: None), STATE_CB(state)]
        L = self.lib
        L.retro_set_environment(self._cbs[0])
        L.retro_set_video_refresh(self._cbs[1])
        L.retro_set_audio_sample(self._cbs[2])
        L.retro_set_audio_sample_batch(self._cbs[3])
        L.retro_set_input_poll(self._cbs[4])
        L.retro_set_input_state(self._cbs[5])
        L.retro_init()
        gi = GameInfo(os.path.realpath(disc).encode(), None, 0, None)
        if not L.retro_load_game(C.byref(gi)):
            sys.exit('retro_load_game failed: ' + disc)
        L.retro_set_controller_port_device(0, 1)
        L.retro_get_memory_data.restype = C.c_void_p
        L.retro_get_memory_size.restype = C.c_size_t
        L.retro_serialize_size.restype = C.c_size_t
        self.ram_size = L.retro_get_memory_size(2)              # RETRO_MEMORY_SYSTEM_RAM
        self.ram = (C.c_uint8 * self.ram_size).from_address(L.retro_get_memory_data(2))
        self.vram_size = L.retro_get_memory_size(3)               # RETRO_MEMORY_VIDEO_RAM (our patch)
        self.vram = ((C.c_uint8 * self.vram_size).from_address(L.retro_get_memory_data(3))
                     if self.vram_size else None)
        self.card_size = L.retro_get_memory_size(0)                # RETRO_MEMORY_SAVE_RAM: card 1
        self.card = (C.c_uint8 * self.card_size).from_address(L.retro_get_memory_data(0))
        if memcard:
            b = open(memcard, 'rb').read()
            if len(b) != self.card_size:                         # .gme/.vgs headers: the raw card is the end
                b = b[-self.card_size:]
            C.memmove(self.card, b, self.card_size)

    def run(self):
        self.lib.retro_run()

    def addr(self, a):
        return a & 0x1fffff

    def read(self, a, size=1, signed=False):
        o = self.addr(a)
        v = int.from_bytes(bytes(self.ram[o:o + size]), 'little')
        if signed and v >> (size * 8 - 1):
            v -= 1 << (size * 8)
        return v

    def write(self, a, v, size=1):
        o = self.addr(a)
        self.ram[o:o + size] = list((v & ((1 << size * 8) - 1)).to_bytes(size, 'little'))

    def save(self, path):
        n = self.lib.retro_serialize_size()
        buf = C.create_string_buffer(n)
        if not self.lib.retro_serialize(buf, n):
            sys.exit('retro_serialize failed')
        open(path, 'wb').write(buf.raw)

    def load(self, path):
        b = open(path, 'rb').read()
        # the core's state includes timing: run one frame first so it is fully started
        self.run()
        if not self.lib.retro_unserialize(C.create_string_buffer(b, len(b)), len(b)):
            sys.exit('retro_unserialize failed: ' + path)

    def image(self):
        from PIL import Image
        data, w, h, pitch = self.frame
        if self.fmt == 1:                                        # XRGB8888
            return Image.frombuffer('RGBX', (w, h), data, 'raw', 'BGRX', pitch, 1).convert('RGB')
        mode = 'BGR;16' if self.fmt == 2 else 'BGR;15'           # RGB565 / 0RGB1555
        return Image.frombuffer('RGB', (w, h), data, 'raw', mode, pitch, 1)


def vram_png(raw, path):
    """The 1024x512 VRAM seen as 15-bit colour (PS1 order: red in the low bits)."""
    import numpy as np
    from PIL import Image
    v = np.frombuffer(raw, '<u2').reshape(512, 1024)
    rgb = np.stack([(v & 31), (v >> 5) & 31, (v >> 10) & 31], -1).astype(np.uint8) << 3
    Image.fromarray(rgb, 'RGB').save(path)


def parse_num(s):
    return int(s, 16)


def load_sym(path):
    sym = {}
    for line in open(path):
        w = line.split(';')[0].split()
        if len(w) >= 2:
            sym[w[1]] = parse_num(w[0])
    return sym


def parse_vars(spec, sym):
    out = []
    for item in filter(None, (s.strip() for s in spec.split(','))):
        kind = '1'
        if ':' in item:
            item, kind = item.split(':')
        signed = kind.startswith('s')
        size = int(kind.lstrip('s'))
        if '-' in item and item not in sym:
            lo, hi = (parse_num(x) for x in item.split('-'))
            out += [('%06x' % (a & 0xffffff), a, 1, False) for a in range(lo, hi + 1)]
        else:
            a = sym[item] if item in sym else parse_num(item)
            out.append((item, a, size, signed))
    return out


def load_keys(path, keymap):
    script = []
    for line in open(path):
        w = line.split('#')[0].split()
        if not w:
            continue
        mask = 0
        for k in w[1:]:
            k = keymap.get(k.upper(), k.upper())
            mask |= 1 << PAD[k]
        script.append((int(w[0]), mask))
    return sorted(script)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('disc')
    ap.add_argument('--frames', type=int, required=True)
    ap.add_argument('--trace', default='')
    ap.add_argument('--sym')
    ap.add_argument('--keys')
    ap.add_argument('--map', default='A=cross,B=square,C=start,D=select')
    ap.add_argument('--load')
    ap.add_argument('--save')
    ap.add_argument('--poke', default='')
    ap.add_argument('--every', type=int, default=1)
    ap.add_argument('--from', dest='start', type=int, default=0)
    ap.add_argument('--diff', action='store_true')
    ap.add_argument('--shot', default='')
    ap.add_argument('--gif')
    ap.add_argument('--dump', default='')
    ap.add_argument('--vram', default='')
    ap.add_argument('--memcard')
    ap.add_argument('--memcard-out')
    a = ap.parse_args()

    sym = load_sym(a.sym) if a.sym else {}
    tvars = parse_vars(a.trace, sym)
    keymap = dict(kv.upper().split('=') for kv in a.map.split(',') if kv)
    keys = load_keys(a.keys, keymap) if a.keys else []
    shots = dict((int(f), p) for f, p in (s.split(':', 1) for s in a.shot.split(',') if s))
    dumps = dict((int(f), p) for f, p in (s.split(':', 1) for s in a.dump.split(',') if s))
    vrams = dict((int(f), p) for f, p in (s.split(':', 1) for s in a.vram.split(',') if s))

    # the core prints to stdout: keep it for the trace, send the core's fd 1 to /dev/null
    out = os.fdopen(os.dup(1), 'w', buffering=1)
    os.dup2(os.open(os.devnull, os.O_WRONLY), 1)
    psx = PSX(a.disc, a.memcard)
    if a.load:
        psx.load(a.load)
    for p in filter(None, a.poke.split(',')):
        lhs, v = p.split('=')
        addr, size = (lhs.split(':') + ['1'])[:2]
        psx.write(sym.get(addr, None) or parse_num(addr), int(v, 0), int(size))
    gif = []
    last = None
    ki = 0
    for f in range(a.frames):
        while ki < len(keys) and keys[ki][0] <= f:
            psx.pressed = keys[ki][1]
            ki += 1
        grab = f in shots or (a.gif and f % a.every == 0)
        psx.run()
        if grab and psx.frame:
            img = psx.image()
            if f in shots:
                img.save(shots[f])
            if a.gif and f % a.every == 0:
                gif.append(img)
        if f in dumps:
            open(dumps[f], 'wb').write(bytes(psx.ram))
        if f in vrams:
            if psx.vram is None:
                sys.exit('--vram: the core does not expose VRAM (apply scripts/pcsx_vram.patch)')
            raw = bytes(psx.vram)
            open(vrams[f], 'wb').write(raw)
            vram_png(raw, os.path.splitext(vrams[f])[0] + '.png')
        if tvars and f >= a.start and (f - a.start) % a.every == 0:
            vals = [(n, psx.read(ad, sz, sg), sz, sg) for n, ad, sz, sg in tvars]
            cur = ['%s=%s' % (n, v if sg else '%0*x' % (sz * 2, v)) for n, v, sz, sg in vals]
            if a.diff and last is not None:
                show = [c for c, l in zip(cur, last) if c != l]
                if show:
                    print(f, ' '.join(show), file=out)
            else:
                print(f, ' '.join(cur), file=out)
            last = cur
    if a.gif and gif:
        gif[0].save(a.gif, save_all=True, append_images=gif[1:], duration=int(1000 * a.every / 60), loop=0)
    if a.save:
        psx.save(a.save)
    if a.memcard_out:
        open(a.memcard_out, 'wb').write(bytes(psx.card))


if __name__ == '__main__':
    main()
