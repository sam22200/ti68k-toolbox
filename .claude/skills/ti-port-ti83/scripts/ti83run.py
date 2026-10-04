#!/usr/bin/env python3
"""Run a TI-83 / TI-83+ assembly program headless, without a TI ROM.

  ti83run.py OUTDIR [--ticks N] [--keys FILE] [--shot T:F.png ...] [--gif F --every N]
             [--dump T:F.bin ...] [--poke ADDR=HEX ...] [--watch LO-HI] [--rate HZ]
             [--load F.bin] [--quiet]

OUTDIR is the folder ti83var.py wrote (body.bin, info.json).  The program runs on a Z80 (the
`z80` package, C core) with the hardware a game touches: the T6A04 LCD (ports 10h/11h), the
keypad (port 1), the interrupt timer (IM 1 or IM 2, one interrupt per tick) and a few OS
routines written in Python (text, homescreen numbers, OP1/OP2, cphlde).  Every other ROM
address returns at once and is logged once: if a game needs one, add it to ROM83.

A tick is one timer interrupt (--rate per second of 6 MHz CPU time, default 140).  The screen
saved is the LCD averaged over the last 6 ticks: grayscale made of interleaved frames comes
out as its grey levels (0-3).  Keys file: `<tick> <key> [<hold ticks>]` per line, key names
in KEYS (up down left right enter clear 2nd alpha mode del xt 0-9 ...).
"""
import json, os, re, sys
import z80
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
CPU_HZ = 6000000

# keypad: name -> (group bit, key bit); port 1 out = group mask (active low), in = keys
KEYS = {'down': (0, 0), 'left': (0, 1), 'right': (0, 2), 'up': (0, 3),
        'enter': (1, 0), '+': (1, 1), '-': (1, 2), '*': (1, 3), '/': (1, 4), '^': (1, 5), 'clear': (1, 6),
        'neg': (2, 0), '3': (2, 1), '6': (2, 2), '9': (2, 3), ')': (2, 4), 'tan': (2, 5), 'vars': (2, 6),
        '.': (3, 0), '2': (3, 1), '5': (3, 2), '8': (3, 3), '(': (3, 4), 'cos': (3, 5), 'prgm': (3, 6), 'stat': (3, 7),
        '0': (4, 0), '1': (4, 1), '4': (4, 2), '7': (4, 3), ',': (4, 4), 'sin': (4, 5), 'matrx': (4, 6), 'xt': (4, 7),
        'sto': (5, 1), 'ln': (5, 2), 'log': (5, 3), 'x2': (5, 4), 'inv': (5, 5), 'math': (5, 6), 'alpha': (5, 7),
        'graph': (6, 0), 'trace': (6, 1), 'zoom': (6, 2), 'window': (6, 3), 'y=': (6, 4), '2nd': (6, 5),
        'mode': (6, 6), 'del': (6, 7)}
# _GetCSC scan codes, for the OS key routines
CSC = {'down': 1, 'left': 2, 'right': 3, 'up': 4, 'enter': 9, 'clear': 15, '2nd': 0x36, 'mode': 0x37,
       'del': 0x38, 'alpha': 0x30, 'xt': 0x28, 'stat': 0x20, '0': 0x21, '1': 0x22, '4': 0x23, '7': 0x24,
       '2': 0x1a, '5': 0x1b, '8': 0x1c, '3': 0x12, '6': 0x13, '9': 0x14, '.': 0x19, 'y=': 0x35,
       'window': 0x34, 'zoom': 0x33, 'trace': 0x32, 'graph': 0x31}

def ams_fonts():
    """The TI-89 AMS fonts (F_4x6 for the small font, F_6x8 for the homescreen): close enough
    to the TI-83's for screenshots.  From runtime/platform-sw/amsfont.h (made by `make`)."""
    p = os.path.join(HERE, '../../../../runtime/platform-sw/amsfont.h')
    f4, f6 = {}, {}
    if os.path.exists(p):
        txt = open(p).read()
        arr = [list(map(int, re.findall(r'\d+', b))) for b in re.findall(r'\{([^}]*)\}', txt)]
        a4, a6 = arr[0], arr[1]
        for c in range(256):
            f4[c] = (a4[c * 6], a4[c * 6 + 1:c * 6 + 6])
            f6[c] = a6[c * 8:c * 8 + 8]
    return f4, f6

class TI83:
    def __init__(self, out, rate=140, quiet=False):
        self.info = json.load(open(os.path.join(out, 'info.json')))
        body = open(os.path.join(out, 'body.bin'), 'rb').read()
        self.plus = self.info['model'] != '83' or self.info['base'] in ('9d93', '9d95')
        m = self.m = z80.Z80Machine()
        self.mem = m.memory
        for a in range(0x8000):
            self.mem[a] = 0xc9                       # unknown ROM routine: ret
        base = int(self.info['base'], 16)
        m.set_memory_block(base, body)
        m.mark_addrs(0, 0x8000, 1)                   # every ROM address is a breakpoint
        self.end = base + len(body)
        m.mark_addrs(self.end, 0x10000 - self.end, 1)  # and the shell's resident routines above
        self.seed = 0x1234
        m.pc = int(self.info['entry'], 16)
        m.sp = 0xfff0
        m.iy = 0x89f0                                # flags
        self.push(0x0001)                            # return address: program exit
        m.set_input_callback(self.port_in)
        m.set_output_callback(self.port_out)
        m.set_get_int_vector_callback(lambda: 0xff)
        self.tick_t = CPU_HZ // rate
        self.lx, self.ly, self.l8, self.ldir, self.lread = 0, 0, True, 5, 0
        self.lcd = [[0] * 120 for _ in range(64)]
        self.hist = []
        self.group = 0xff
        self.held = {}
        self.tick = 0
        self.exited = False
        self.unknown = set()
        self.quiet = quiet
        self.f4, self.f6 = ams_fonts()
        self.OP1, self.OP2 = (0x8478, 0x8483) if self.plus else (0x8039, 0x8044)
        self.PENCOL, self.PENROW = (0x86d7, 0x86d8) if self.plus else (0x8252, 0x8253)
        self.CURROW, self.CURCOL = (0x844b, 0x844c) if self.plus else (0x800c, 0x800d)
        self.PLOT = 0x9340 if self.plus else 0x8e29

    # ---- helpers
    def push(self, v):
        self.m.sp = (self.m.sp - 2) & 0xffff
        self.mem[self.m.sp], self.mem[self.m.sp + 1] = v & 0xff, v >> 8

    def pop(self):
        v = self.mem[self.m.sp] | self.mem[self.m.sp + 1] << 8
        self.m.sp = (self.m.sp + 2) & 0xffff
        return v

    def log(self, *a):
        if not self.quiet:
            print('[%d]' % self.tick, *a)

    # ---- ports
    def port_in(self, port):
        p = port & 0xff
        if p == 1:
            v = 0xff
            for k in self.held:
                g, b = KEYS[k]
                if not (self.group >> g) & 1:
                    v &= ~(1 << b)
            return v & 0xff
        if p == 0x10:
            return 0x00                              # LCD status: never busy
        if p == 0x11:
            v, self.lread = self.lread, self.lcd_get()
            self.lcd_step()
            return v
        if p == 0:
            return 0x03                              # link lines idle
        if p == 2:
            return 0x80 if self.plus else 0x00
        return 0xff

    def port_out(self, port, v):
        p = port & 0xff
        if p == 1:
            self.group = v                           # the groups selected by the last write
        elif p == 0x10:
            if v == 0: self.l8 = False
            elif v == 1: self.l8 = True
            elif 4 <= v <= 7: self.ldir = v
            elif 0x20 <= v <= 0x3f: self.ly = v - 0x20
            elif 0x80 <= v <= 0xbf: self.lx = v - 0x80
        elif p == 0x11:
            self.lcd_put(v)
            self.lcd_step()

    def lcd_get(self):
        w = 8 if self.l8 else 6
        r = self.lcd[self.lx & 63]
        v = 0
        for i in range(w):
            c = self.ly * w + i
            v = v << 1 | (r[c] if c < 120 else 0)
        return v

    def lcd_put(self, v):
        w = 8 if self.l8 else 6
        r = self.lcd[self.lx & 63]
        for i in range(w):
            c = self.ly * w + i
            if c < 120:
                r[c] = (v >> (w - 1 - i)) & 1

    def lcd_step(self):
        if self.ldir == 5: self.lx = (self.lx + 1) & 63
        elif self.ldir == 4: self.lx = (self.lx - 1) & 63
        elif self.ldir == 7: self.ly = (self.ly + 1) % (15 if self.l8 else 20)
        elif self.ldir == 6: self.ly = (self.ly - 1) % (15 if self.l8 else 20)

    # ---- OS routines (TI-83 addresses; TI-83+ bcalls go through rst 28h = 0x0028)
    def text(self, ch, buf):
        """Small font character at penCol/penRow; into plotSScreen when textWrite is set."""
        adv, rows = self.f4.get(ch, (4, [0] * 5))
        x, y = self.mem[self.PENCOL], self.mem[self.PENROW]
        for j in range(6):
            bits = rows[j] if j < 5 else 0
            for i in range(adv):
                on = (bits >> (7 - i)) & 1
                if x + i < 96 and y + j < 64:
                    if buf:
                        a = self.PLOT + (y + j) * 12 + (x + i) // 8
                        mask = 0x80 >> ((x + i) & 7)
                        self.mem[a] = (self.mem[a] | mask) if on else (self.mem[a] & ~mask)
                    else:
                        self.lcd[y + j][x + i] = on
        self.mem[self.PENCOL] = min(x + adv, 255)

    def big_char(self, ch):
        r, c = self.mem[self.CURROW], self.mem[self.CURCOL]
        rows = self.f6.get(ch, [0] * 8)
        for j in range(8):
            for i in range(6):
                if r * 8 + j < 64 and c * 6 + i < 96:
                    self.lcd[r * 8 + j][c * 6 + i] = (rows[j] >> (7 - i)) & 1
        self.mem[self.CURCOL] = (c + 1) & 0xff

    def get_float(self, a):
        e = self.mem[a + 1] - 0x80
        digits = ''.join('%02x' % self.mem[a + 2 + i] for i in range(7))
        v = int(digits[:e + 1]) if 0 <= e < 14 else 0
        return -v if self.mem[a] & 0x80 else v

    def set_float(self, a, v):
        s = str(abs(v))
        self.mem[a] = 0x80 if v < 0 else 0
        self.mem[a + 1] = 0x80 + len(s) - 1
        s = (s + '0' * 14)[:14]
        for i in range(7):
            self.mem[a + 2 + i] = int(s[2 * i:2 * i + 2], 16)
        self.mem[a + 9] = self.mem[a + 10] = 0

    def rom(self, pc):
        m, mem = self.m, self.mem
        textwrite = (mem[(m.iy + 0x14) & 0xffff] >> 7) & 1
        if pc == 0x0001:
            self.exited = True
            return False
        if pc == 0x0038:                              # IM 1 OS interrupt: ack and return
            m.iff1 = m.iff2 = 1
        elif pc == 0x0028 and self.plus:             # bcall: the word after rst 28h
            ret = self.pop()
            num = mem[ret] | mem[ret + 1] << 8
            self.push(ret + 2)
            self.bcall(num, textwrite)
            pc = None
        elif pc in self.ROM and pc < 0x8000:
            self.ROM[pc](self, textwrite)
        elif pc >= self.end and pc in self.SHELL.get(self.info['shell'], {}):
            self.SHELL[self.info['shell']][pc](self, textwrite)
        else:
            if pc not in self.unknown:
                self.unknown.add(pc)
                self.log('unknown %s call %04x (returns at once)' % ('ROM' if pc < 0x8000 else 'shell', pc))
        m.pc = self.pop()
        return True

    def bcall(self, num, tw):
        f = self.BCALL.get(num)
        if f:
            f(self, tw)
        elif num not in self.unknown:
            self.unknown.add(num)
            self.log('unknown bcall %04x (returns at once)' % num)

    def r_vputs(self, tw):
        a = self.m.hl
        while self.mem[a]:
            self.text(self.mem[a], tw)
            a += 1
        self.m.hl = a + 1

    def r_vputmap(self, tw):
        self.text(self.m.a, tw)

    def r_clrscr(self, tw):
        self.lcd = [[0] * 120 for _ in range(64)]

    def r_homeup(self, tw):
        self.mem[self.CURROW] = self.mem[self.CURCOL] = 0

    def r_cphlde(self, tw):
        hl, de = self.m.hl, self.m.de
        f = self.m.f & ~0xc1
        if hl == de: f |= 0x40
        if hl < de: f |= 0x01
        self.m.f = f

    def r_op2toop1(self, tw):
        self.mem[self.OP1:self.OP1 + 11] = self.mem[self.OP2:self.OP2 + 11]

    def r_setxxop1(self, tw):
        self.set_float(self.OP1, self.m.a)

    def r_setxxop2(self, tw):
        self.set_float(self.OP2, self.m.a)

    def r_setxxxxop2(self, tw):
        self.set_float(self.OP2, self.m.hl)

    def r_dispop1a(self, tw):
        s = str(self.get_float(self.OP1))[:max(self.m.a, 1)]
        for ch in s.encode():
            self.big_char(ch)

    def r_puts(self, tw):
        a = self.m.hl
        while self.mem[a]:
            self.big_char(self.mem[a])
            a += 1

    def r_getcsc(self, tw):
        self.m.a = next((CSC.get(k, 0) for k in self.held), 0)

    def r_random_hl(self, tw):
        # 16-bit Galois LFSR; deterministic, not the shell's own generator
        x = self.seed
        x = (x >> 1) ^ (0xb400 if x & 1 else 0)
        self.seed = x
        self.m.hl = x

    # Shell libraries resident in RAM above the program (add the routines a game calls)
    SHELL = {'Venus': {0xfe72: r_random_hl}}
    ROM = {0x4781: r_vputs, 0x477d: r_vputmap, 0x475d: r_clrscr, 0x4755: r_clrscr, 0x4775: r_homeup,
           0x4004: r_cphlde, 0x41c2: r_op2toop1, 0x4a74: r_setxxop1, 0x4a78: r_setxxop2,
           0x4a7c: r_setxxxxop2, 0x51d4: r_dispop1a, 0x470d: r_puts, 0x4014: r_getcsc}
    BCALL = {0x4561: r_vputs, 0x455e: r_vputmap, 0x4546: r_clrscr, 0x4540: r_clrscr, 0x4558: r_homeup,
             0x400c: r_cphlde, 0x412f: r_op2toop1, 0x478c: r_setxxop1, 0x4792: r_setxxxxop2,
             0x4bf7: r_dispop1a, 0x450a: r_puts, 0x4018: r_getcsc}

    # ---- run
    def run_tick(self):
        m = self.m
        left = self.tick_t
        while left > 0 and not self.exited:
            m.ticks_to_stop = left
            ev = m.run()
            left = m.ticks_to_stop if not (ev & 4) else 0
            if ev & 1:
                self.rom(m.pc)
        if self.exited:
            return
        if m.iff1:
            m.on_handle_active_int()
        self.tick += 1
        self.hist = (self.hist + [[r[:96] for r in self.lcd]])[-6:]

    def grey(self):
        """The LCD averaged over the last 6 ticks, as 4 grey levels (0 white .. 3 black)."""
        n = len(self.hist) or 1
        return [[min(3, (sum(h[y][x] for h in self.hist) * 3 + n // 2) // n) for x in range(96)]
                for y in range(64)]

    def image(self, scale=3):
        g = self.grey()
        im = Image.new('L', (96, 64))
        im.putdata([255 - 85 * v for row in g for v in row])
        return im.resize((96 * scale, 64 * scale), Image.NEAREST)

def main():
    a = sys.argv[1:]
    out = a[0]
    def opts(name):
        return [a[i + 1] for i, x in enumerate(a) if x == name]
    rate = int((opts('--rate') or ['140'])[0])
    t = TI83(out, rate, quiet='--quiet' in a)
    ticks = int((opts('--ticks') or ['600'])[0])
    for spec in opts('--load'):                         # a full 64 KB memory image from --dump
        t.m.set_memory_block(0x8000, open(spec, 'rb').read()[0x8000:])
    for p in opts('--poke'):
        ad, v = p.split('=')
        t.m.set_memory_block(int(ad, 16), bytes.fromhex(v))
    keys = []
    for kf in opts('--keys'):
        for line in open(kf):
            f = line.split('#')[0].split()
            if f:
                keys.append((int(f[0]), f[1], int(f[2]) if len(f) > 2 else 10))
    shots = {int(s.split(':')[0]): s.split(':', 1)[1] for s in opts('--shot') if not s.startswith('last:')}
    dumps = {int(s.split(':')[0]): s.split(':', 1)[1] for s in opts('--dump')}
    watch = [tuple(int(x, 16) for x in w.split('-')) for w in opts('--watch')]
    gif, every = (opts('--gif') or [None])[0], int((opts('--every') or ['4'])[0])
    frames, prev = [], None
    for _ in range(ticks):
        t.held = {k: 1 for (s, k, h) in keys if s <= t.tick < s + h}
        t.run_tick()
        if t.exited:
            print('# program returned at tick %d' % t.tick)
            break
        for lo, hi in watch:
            cur = bytes(t.mem[lo:hi + 1])
            if cur != prev:
                print('%d %04x %s' % (t.tick, lo, cur.hex()))
                prev = cur
        if t.tick in shots:
            t.image().save(shots[t.tick])
        if t.tick in dumps:
            open(dumps[t.tick], 'wb').write(bytes(t.mem))
        if gif and t.tick % every == 0:
            frames.append(t.image(2))
    for s in opts('--shot'):
        if s.startswith('last:'):
            t.image().save(s[5:])
    if gif and frames:
        frames[0].save(gif, save_all=True, append_images=frames[1:], duration=1000 * every // rate, loop=0)
    print('# %d ticks, pc %04x' % (t.tick, t.m.pc))

if __name__ == '__main__':
    main()
