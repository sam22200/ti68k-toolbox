#!/usr/bin/env python3
"""Partition the existing cursor artwork into four outside edges, offline.

No ROM artwork. The optional contour renderer reuses the cursor's black/white
planes. Existing own render/UI constants also live in this archive tail, read
in place, to keep both native chapters inside TTPack's executable size limit.
"""
from pathlib import Path
import argparse
import subprocess
import re
import math
import struct
import art

HERE = Path(__file__).resolve().parents[1]


def attach(path):
    """Idempotent aligned tail after the original ZX0 stream; no new file slot."""
    path=Path(path);raw=path.read_bytes();masks=(HERE/'fftcont.bin').read_bytes()
    if len(raw)>=10 and raw[-10:-6] in (b'FC01',b'FC02'):
        original,offset,count=struct.unpack('>3H',raw[-6:])
        assert offset+count==len(raw)-10
        raw=raw[:original]
    offset=(len(raw)+1)&~1
    data=raw+bytes(offset-len(raw))+masks+b'FC02'+struct.pack('>3H',len(raw),offset,len(masks))
    assert len(data)<=65512, 'archive payload plus AMS trailer must fit one variable'
    if path.read_bytes()!=data:path.write_bytes(data)
    variable=path.with_suffix('.89y')
    if not variable.exists() or variable.stat().st_mtime<path.stat().st_mtime:
        subprocess.run([HERE/'../../tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',path.name,path.stem],cwd=path.parent,check=True,capture_output=True)
        packed=bytearray(variable.read_bytes());packed[0x49]=3;variable.write_bytes(packed)
    print('%s: %d contour bytes, offset %d, raw bank %d'%(path.name,len(masks),offset,len(data)))


def main():
    parser=argparse.ArgumentParser(__doc__)
    parser.add_argument('--attach',type=Path,nargs='*',default=[])
    args=parser.parse_args()
    keys, tops, rows, planes = art.slopes()
    shapes = [(art.cursor(), [(12, 0), (25, 6), (12, 12), (-1, 6)], 0)]
    offset = 0
    for key, top, count in zip(keys, tops, rows):
        lift = [(key >> (2*k)) & 3 for k in range(4)]
        vertices = [(12, 12-6*lift[0]), (25, 18-6*lift[1]),
                    (12, 24-6*lift[2]), (-1, 18-6*lift[3])]
        picture = art.slope_marker(lift, True)[top:top+count]
        assert art.plane_rows(picture, 24, count)[2] == planes['sc_m'][offset:offset+count]
        shapes.append((picture, vertices, top))
        offset += count
    bank = bytearray()
    offsets = []
    for picture, vertices, top in shapes:
        offsets.append(len(bank))
        for y, row in enumerate(picture):
            masks = [0]*4
            for x, pixel in enumerate(row):
                if pixel is None:
                    continue
                distances = []
                for k, (x0, y0) in enumerate(vertices):
                    x1, y1 = vertices[(k+1)&3]
                    dx, dy = x1-x0, y1-y0
                    distances.append(abs(dx*(y+top+.5-y0)-dy*(x+.5-x0))/math.hypot(dx, dy))
                edge = min(range(4), key=distances.__getitem__)
                masks[edge] |= 1 << (31-x)
            assert sum(masks) == art.plane_rows([row], 24, 1)[2][0]
            bank.extend(struct.pack('>4I', *masks))
    # Move existing immutable rendering/UI tables out of the nearly-full
    # native executable. Read directly from the same archive, no heap/cache.
    archived=[];blocks={}
    def table(name,typ,dims,values):
        assert math.prod(dims)==len(values),(name,dims,len(values))
        if len(bank)&1:bank.append(0)
        fmt={'u8':'B','u16':'H','u32':'I','s16':'h','s8':'b'}[typ]
        raw=struct.pack('>'+fmt*len(values),*values)
        if raw in blocks:at=blocks[raw]
        else:at=len(bank);blocks[raw]=at;bank.extend(raw)
        shape=''.join('[%d]'%n for n in dims[1:])
        cast='const %s (*)%s'%(typ,shape) if shape else 'const %s *'%typ
        archived.append('#define CONTOUR_REF_%s ((%s)(contour_gfx + %d))'%(name,cast,at))
    source=(HERE/'art.h').read_text()
    sizes={'FOE_H':7,'FOE_KO_H':5,'ACTIVE_H':13,'HAND_H':14,'NSLOPE':13,'SHADES':4}
    for typ,name,dim,body in re.findall(r'static const (u8|u16|u32) (\w+)((?:\[[^]]+\])+?) = \{(.*?)\};',source,re.S):
        if name=='cur_m':continue # Work has a field with this same token.
        dims=[int(n) if n.isdigit() else sizes[n] for n in re.findall(r'\[([^]]+)\]',dim)]
        values=[int(n.rstrip('UL'),0) for n in re.findall(r'0x[0-9A-F]+(?:UL)?|\b[0-9]+\b',body)]
        table(name,typ,dims,values)
    source=(HERE/'fft.c').read_text()
    for name,typ in [('lmask','u16'),('sin32','s16'),('idle_seq','u8'),('walk_seq','u8'),('digits','u8'),('job_jp','u16'),('walk_q','u8'),('walk_h','u8'),('dissolve','u16'),('tri','u8'),('sway','u8'),('bob','u8'),('reflected','u8'),('facing_sprite','u8'),('forecast_arrow','u8'),('forecast_mask','u8')]:
        dim,body=re.search(r'static const '+typ+' '+name+r'((?:\[[^]]+\])+?)\s*=\s*\{(.*?)\};',source,re.S).groups()
        table(name,typ,[sum(int(t.strip()) if t.strip().isdigit() else {'WALK_FRAMES':7,'CLIMB_FRAMES':10}[t.strip()] for t in n.split('+')) for n in re.findall(r'\[([^]]+)\]',dim)],[int(n,0) for n in re.findall(r'-?0x[0-9A-F]+|-?\b[0-9]+\b',body)])
    # Generated reciprocal and blade-offset tables (the opening executable is at TTPack's limit).
    body=re.search(r'ct_recip\[(\d+)\] = \{(.*?)\};',(HERE/'ct_recip.h').read_text(),re.S)
    table('ct_recip','u16',[int(body[1])],[int(n) for n in re.findall(r'\d+',body[2])])
    body=re.search(r'weapon_x\[(\d+)\] = \{(.*?)\};',(HERE/'weapons.h').read_text(),re.S)
    table('weapon_x','s8',[int(body[1])],[int(n) for n in re.findall(r'-?\d+',body[2])])
    for name,width in [('st_names',8),('signs',12),('equip_labels',12),('effects',32),('entries',16),('automatic',16),('team_names',8),('stat_labels',8),('chapter_ui',24)]:
        pattern=r'static const char \*const '+name+r'\[[^]]*\]\s*=\s*\{(.*?)\};'
        body=re.search(pattern,source,re.S)[1] if name!='chapter_ui' else re.search(r'static const char chapter_ui\[4\]\[24\]=\{(.*?)\};',(HERE/'config.inc').read_text(),re.S)[1]
        words=re.findall(r'"([^"]*)"',body);at=len(bank)
        for word in words:
            raw=word.encode('ascii');assert len(raw)<width
            bank.extend(raw.ljust(width,b'\0'))
        archived.append('#define CONTOUR_REF_%s ((const char (*)[%d])(contour_gfx + %d))'%(name,width,at))
    if len(bank)&1:bank.append(0)
    if not (HERE/'fftcont.bin').exists() or (HERE/'fftcont.bin').read_bytes()!=bank:
        (HERE/'fftcont.bin').write_bytes(bank)
    header=('// Generated by tools/contour_assets.py; own marker masks, no ROM art.\n'
        '#define CONTOUR_BYTES %d\nstatic const u16 contour_off[%d] = { %s };\n'
        % (len(bank), len(offsets), ', '.join(map(str, offsets))))
    if not (HERE/'contour.h').exists() or (HERE/'contour.h').read_text()!=header:
        (HERE/'contour.h').write_text(header)
    redirects='// Existing own tables, archived on native builds; generated with contour.h.\n'+ '\n'.join(archived)+'\n'
    include=HERE/'contour_art.inc'
    if not include.exists() or include.read_text()!=redirects:include.write_text(redirects)
    for path in args.attach:attach(path)
    print('Contour masks and archived tables: %d bytes, %d existing shapes' % (len(bank), len(shapes)))


if __name__ == '__main__':
    main()
