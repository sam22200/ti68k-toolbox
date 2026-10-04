"""Disassemble banked Game Boy code (MBC1/3/5 ROMs, where Ghidra + GhidraBoy only analyse bank 0).

usage: gbdis.py ROM BANK:ADDR [N] [--sym FILE] [--find HEX]
  BANK:ADDR  hex, e.g. 4f:5a0d (an address 0000-3FFF is bank 0 whatever BANK says)
  N          instructions to print (default 40); stops after N, not at RET (code often falls through)
  --sym      .sym file ("BANK:ADDR name"): names printed for labels and operands
  --find     hex byte pattern searched in the whole ROM, printed as BANK:ADDR (stores to a
             variable: ea LO HI = ld (HI LO),a; fa LO HI = ld a,(..); 21 LO HI = ld hl,..)
The opcode table is PyBoy's (pyboy.core.opcodes.CPU_COMMANDS). Relative jumps print their target.
"""
import argparse
import sys

import array
import os

import pyboy.core


def _pyboy_tables():
    # the compiled module hides them: read the two tables from opcodes.py's source
    src = open(os.path.join(os.path.dirname(pyboy.core.__file__), 'opcodes.py')).read()
    ns = {'array': array}
    for key in ('OPCODE_LENGTHS = ', 'CPU_COMMANDS = '):
        i = src.index(key)
        j = src.index(']', i) + 1
        exec(src[i:j] + (')' if 'array' in src[i:j] else ''), ns)
    return ns['CPU_COMMANDS'], ns['OPCODE_LENGTHS']


CPU_COMMANDS, OPCODE_LENGTHS = _pyboy_tables()


def phys(bank, addr):
    return addr if addr < 0x4000 else bank * 0x4000 + addr - 0x4000


def where(off):
    return '%02x:%04x' % (off // 0x4000, off if off < 0x4000 else 0x4000 + off % 0x4000)


def load_sym(path):
    names = {}
    if path:
        for line in open(path):
            p = line.split(';')[0].split()
            if len(p) >= 2 and ':' in p[0]:
                b, a = p[0].split(':')
                names[(int(b, 16), int(a, 16))] = p[1]
    return names


def name_of(names, bank, a):
    return names.get((0 if a < 0x4000 else bank, a)) or (names.get((0, a)) if a >= 0x8000 else None)


def dis(rom, bank, addr, n, names):
    for _ in range(n):
        off = phys(bank, addr)
        op = rom[off]
        if op == 0xcb:
            text, ln = CPU_COMMANDS[0x100 + rom[off + 1]], 2
        else:
            text, ln = CPU_COMMANDS[op], OPCODE_LENGTHS[op]
        raw = rom[off:off + ln]
        if 'r8' in text:
            d = raw[1] - 256 if raw[1] > 127 else raw[1]
            tgt = addr + ln + d
            text = text.replace('r8', '%04x' % tgt if text.startswith('JR') else '%d' % d)
            ref = tgt if text.startswith('JR') else None
        elif 'd16' in text or 'a16' in text:
            v = raw[1] | raw[2] << 8
            text = text.replace('d16', '%04x' % v).replace('a16', '%04x' % v)
            ref = v
        elif 'd8' in text or 'a8' in text:
            text = text.replace('d8', '%02x' % raw[1]).replace('a8', 'ff%02x' % raw[1])
            ref = 0xff00 | raw[1] if 'a8' in CPU_COMMANDS[op] else None
        else:
            ref = None
        label = name_of(names, bank, addr)
        if label:
            print('%s:' % label)
        note = name_of(names, bank, ref) if ref is not None else None
        print('  %02x:%04x  %-12s %s%s' % (0 if addr < 0x4000 else bank, addr, raw.hex(), text,
                                           ('  ; ' + note) if note else ''))
        addr += ln


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('rom')
    ap.add_argument('at', nargs='?')
    ap.add_argument('n', nargs='?', type=int, default=40)
    ap.add_argument('--sym')
    ap.add_argument('--find')
    a = ap.parse_args()
    rom = open(a.rom, 'rb').read()
    if a.find:
        pat = bytes.fromhex(a.find)
        i = rom.find(pat)
        while i >= 0:
            print(where(i))
            i = rom.find(pat, i + 1)
        return
    if not a.at:
        sys.exit('BANK:ADDR or --find')
    b, s = a.at.split(':')
    dis(rom, int(b, 16), int(s, 16), a.n, load_sym(a.sym))


if __name__ == '__main__':
    main()
