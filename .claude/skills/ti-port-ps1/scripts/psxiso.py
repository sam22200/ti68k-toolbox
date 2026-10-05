#!/usr/bin/env python3
"""Read a PlayStation disc image (.cue/.bin, MODE2/2352 or MODE1/2048 data track, or a plain
2048-byte .iso): list its ISO 9660 files, extract them, find the boot executable from
SYSTEM.CNF and decode its PS-X EXE header.

usage: psxiso.py DISC.cue OUTDIR [--all] [--file PATH ...]
-> OUTDIR/info.json (boot exe, header: load address, entry, sizes; the file list),
   OUTDIR/files.txt (path, LBA, size), OUTDIR/<BOOT>.EXE and OUTDIR/<BOOT>.bin (the text
   section alone, to load at t_addr in Ghidra), plus every file with --all or the given --file.
Only the first data track is read (audio tracks are ignored); XA/STR streams are copied as
user data (2048 bytes per sector, form-2 sectors lose their extra bytes: fine for reading).
"""
import argparse, json, os, re, struct, sys


class Disc:
    def __init__(self, path):
        if path.lower().endswith('.cue'):
            cue = open(path).read()
            fname = re.search(r'FILE\s+"([^"]+)"', cue).group(1)
            mode = re.search(r'TRACK\s+\d+\s+(\S+)', cue).group(1)
            path = os.path.join(os.path.dirname(path), fname)
            self.raw, self.off = {'MODE2/2352': (2352, 24), 'MODE1/2352': (2352, 16),
                                  'MODE1/2048': (2048, 0), 'MODE2/2336': (2336, 8)}[mode]
        else:
            self.raw, self.off = (2352, 24) if os.path.getsize(path) % 2352 == 0 else (2048, 0)
        self.f = open(path, 'rb')

    def sector(self, lba):
        self.f.seek(lba * self.raw + self.off)
        return self.f.read(2048)

    def read(self, lba, size):
        out = bytearray()
        while len(out) < size:
            out += self.sector(lba)
            lba += 1
        return bytes(out[:size])


def walk(disc, lba, size, prefix, out):
    data = disc.read(lba, size)
    i = 0
    while i < len(data):
        n = data[i]
        if n == 0:                       # records never cross a sector: skip to the next one
            i = (i // 2048 + 1) * 2048
            continue
        rec = data[i:i + n]
        elba, esize, flags, nlen = struct.unpack_from('<I', rec, 2)[0], \
            struct.unpack_from('<I', rec, 10)[0], rec[25], rec[32]
        name = rec[33:33 + nlen].decode('ascii', 'replace')
        i += n
        if name in ('\x00', '\x01'):
            continue
        name = name.split(';')[0]
        path = prefix + name
        if flags & 2:
            walk(disc, elba, esize, path + '/', out)
        else:
            out.append((path, elba, esize))


def exe_header(b):
    if b[:8] != b'PS-X EXE':
        return None
    pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size, s_addr, s_size = \
        struct.unpack_from('<10I', b, 0x10)
    return {'pc0': '%08x' % pc0, 'gp0': '%08x' % gp0, 't_addr': '%08x' % t_addr,
            't_size': t_size, 'b_addr': '%08x' % b_addr, 'b_size': b_size,
            's_addr': '%08x' % (s_addr + struct.unpack_from('<I', b, 0x34)[0]),
            'region': b[0x4c:0x80].split(b'\0')[0].decode('ascii', 'replace')}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('disc')
    ap.add_argument('out')
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--file', action='append', default=[])
    a = ap.parse_args()
    d = Disc(a.disc)
    pvd = d.sector(16)
    if pvd[1:6] != b'CD001':
        sys.exit('no ISO 9660 volume descriptor at sector 16')
    root = pvd[156:190]
    files = []
    walk(d, struct.unpack_from('<I', root, 2)[0], struct.unpack_from('<I', root, 10)[0], '', files)
    os.makedirs(a.out, exist_ok=True)
    with open(os.path.join(a.out, 'files.txt'), 'w') as f:
        for p, lba, size in files:
            f.write('%-40s %7d %10d\n' % (p, lba, size))
    byname = {p.upper(): (lba, size) for p, lba, size in files}
    info = {'volume': pvd[40:72].decode().strip(), 'files': len(files),
            'bytes': sum(s for _, _, s in files)}
    boot = 'PSX.EXE'
    if 'SYSTEM.CNF' in byname:
        cnf = d.read(*byname['SYSTEM.CNF']).decode('ascii', 'replace')
        info['system_cnf'] = cnf.strip()
        m = re.search(r'BOOT\s*=\s*cdrom:\\?([^;\s]+)', cnf, re.I)
        if m:
            boot = m.group(1).replace('\\', '/').upper()
    want = set(f.upper() for f in a.file) | {boot}
    for p, lba, size in files:
        if a.all or p.upper() in want:
            dst = os.path.join(a.out, p)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            open(dst, 'wb').write(d.read(lba, size))
    exe = d.read(*byname[boot])
    info['boot'] = boot
    info['exe'] = exe_header(exe)
    if info['exe']:
        text = os.path.join(a.out, os.path.basename(boot) + '.bin')
        open(text, 'wb').write(exe[0x800:0x800 + info['exe']['t_size']])
        info['exe']['text_file'] = text
    json.dump(info, open(os.path.join(a.out, 'info.json'), 'w'), indent=1)
    print(json.dumps({k: v for k, v in info.items() if k != 'system_cnf'}, indent=1))
    # the biggest files: where the rooms, graphics and overlays live
    for p, lba, size in sorted(files, key=lambda x: -x[2])[:15]:
        print('%-40s %10d' % (p, size))


if __name__ == '__main__':
    main()
