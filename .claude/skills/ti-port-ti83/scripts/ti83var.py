#!/usr/bin/env python3
"""Read a TI-83 / TI-83+ / TI-84+ program file (.83p, .8xp) and find where it runs.

  ti83var.py PROG.83p OUTDIR [--base HEX]

Writes OUTDIR/body.bin (the program's bytes as loaded) and OUTDIR/info.json: model, name,
size, shell (header signature), load address (scored: the base that makes the most absolute
CALL/JP targets land on plausible code), entry point, and the ROM calls used, named from
ti83asm.inc / ti83plus.inc when they sit next to this script or in OUTDIR.
"""
import json, os, re, struct, sys
from collections import Counter

def parse_var(path):
    d = open(path, 'rb').read()
    sig = d[:8]
    model = {b'**TI83**': '83', b'**TI83F*': '83+'}.get(sig)
    if model is None:
        sys.exit('not a TI-83/83+ variable file: %r' % sig)
    p = 0x37
    hlen, dlen, typ = struct.unpack('<HHB', d[p:p + 5])
    name = d[p + 5:p + 13].rstrip(b'\0').decode('latin1')
    data = d[p + 4 + hlen:p + 4 + hlen + dlen]
    size = struct.unpack('<H', data[:2])[0]
    return model, name, typ, data[2:2 + size]

# Load addresses to try: TI-83 Send(9 at 9327, Venus 9329 (its BASIC stub E7 "9_[V?" 00),
# TI-83+ userMem 9D95 (the BB 6D token pair is at 9D95, so code assembled at 9D93 + 2).
BASES = [0x9327, 0x9329, 0x9D93, 0x9D95]
GOOD_FIRST = set([0x21, 0x11, 0x01, 0x3e, 0x06, 0x0e, 0x16, 0x1e, 0x26, 0x2e, 0xcd, 0xc3,
                  0xc5, 0xd5, 0xe5, 0xf5, 0xdd, 0xfd, 0xed, 0xcb, 0xaf, 0xb7, 0x3a, 0x2a,
                  0x7e, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x18, 0x10, 0xfe, 0xe6, 0xc9])

def score(body, base):
    n = 0
    for i in range(len(body) - 2):
        if body[i] in (0xcd, 0xc3):
            t = body[i + 1] | body[i + 2] << 8
            if base <= t < base + len(body):
                n += 1 if body[t - base] in GOOD_FIRST else -1
    return n

def entry(body, base):
    if body[:2] == b'\xbb\x6d':
        return base + 2
    if body[:1] == b'\xe7':                       # Venus: skip the stub and the description
        i = body.index(0, 1)                      # end of "9_[V?"
        i = body.index(0, i + 1)                  # end of the description
        return base + i + 1
    return base

def rom_names(dirs):
    names = {}
    for d in dirs:
        for f in ('ti83asm.inc', 'ti83plus.inc'):
            p = os.path.join(d, f)
            if os.path.exists(p):
                for line in open(p, encoding='latin1'):
                    m = re.match(r'\s*(_\w+)\s+(?:equ|\.equ|=)\s+\$?([0-9A-Fa-f]+)h?', line, re.I)
                    if m:
                        names.setdefault(int(m.group(2), 16), m.group(1))
    return names

def main():
    a = sys.argv[1:]
    src, out = a[0], a[1]
    os.makedirs(out, exist_ok=True)
    model, name, typ, body = parse_var(src)
    base = int(a[a.index('--base') + 1], 16) if '--base' in a else max(BASES, key=lambda b: score(body, b))
    open(os.path.join(out, 'body.bin'), 'wb').write(body)
    names = rom_names([os.path.dirname(os.path.abspath(__file__)), out])
    calls = Counter()
    for i in range(len(body) - 2):
        if body[i] in (0xcd, 0xc3) or (model == '83+' and body[i] == 0xef):   # call, jp, rst 28h
            t = body[i + 1] | body[i + 2] << 8
            if t < 0x8000 and (t in names or body[i] == 0xef):
                calls['%04x %s' % (t, names.get(t, '?'))] += 1
    shell = ('Venus' if body[:1] == b'\xe7' else 'asm (AsmPrgm/MirageOS/Ion)' if body[:2] == b'\xbb\x6d'
             else 'Send(9 plain asm')
    info = dict(model=model, name=name, type=typ, size=len(body), shell=shell,
                base='%04x' % base, end='%04x' % (base + len(body)), entry='%04x' % entry(body, base),
                base_scores={'%04x' % b: score(body, b) for b in BASES},
                rom_calls=dict(calls.most_common()))
    json.dump(info, open(os.path.join(out, 'info.json'), 'w'), indent=1)
    print(json.dumps(info, indent=1))

main()
