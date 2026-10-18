#!/usr/bin/env python3
"""Stage the existing camera fixture, build an interactive TI review and check its menu."""
import csv
import hashlib
import json
import re
import shutil
import struct
import sys

import turn_audit as audit

OUT = audit.HERE/'x/config'
FLAGS = audit.FLAGS + ['-DFFT_CONFIG', '-DFFT_CONFIG_INTERACTIVE', '-DFFT_TURN_LOWPOLY_NATIVE']
NATIVE_BANKS = ('fftv0','fftv1','fftv2','fftv3','fftu','fftw','ffti','fftmesh','fftlook')


def main():
    data = OUT/'data'
    data.mkdir(parents=True, exist_ok=True)
    source = audit.HERE/'x/turn-comparison/textured80x25/data'
    assert (source/'fftv0.bin').exists(), 'The selected contrast+edges map fixtures are required.'
    # Preserve historical research outside the active send directory. Only this explicit
    # manifest is staged; retired atlases, UVs, surfaces and attack fixtures cannot leak back.
    retired = OUT/'retired-banks'; retired.mkdir(exist_ok=True)
    for path in data.iterdir():
        if path.suffix in ('.bin','.89y') and path.stem not in NATIVE_BANKS + ('ffttex','fftiso'):
            shutil.move(str(path),str(retired/path.name))
    for name in ('fftv0','fftv1','fftv2','fftv3'):
        for suffix in ('.bin','.89y'):
            shutil.copy2(source/(name+suffix),data/(name+suffix))
    # Actor/UI offsets follow the current generated headers, including crystal phases
    # and banner strips. Keep the reviewed scenery/atlases but refresh mutable banks.
    for name in ('fftu', 'fftw', 'ffti', 'fftmesh'):
        for suffix in ('.bin', '.89y'):
            shutil.copy2(audit.HERE/(name+suffix), data/(name+suffix))
            if suffix == '.89y':
                variable = data/(name+suffix)
                raw = bytearray(variable.read_bytes()); raw[0x49] = 3
                variable.write_bytes(raw)  # keep large actor/UI banks archived, not in game RAM
    # PC-only atlas/style banks are never sent to the calculator.
    for name in ('ffttex.bin','fftiso.bin'):
        shutil.copy2(audit.HERE/name,data/name)
    # Retain the existing filtered scenery assets at the newly selected solid preset.
    prepare_flags = FLAGS + ['-DFFT_TEXTURE_REVIEW','-DFFT_TEXTURE_PREVIEW']
    audit.run(['cc','-std=gnu99','-O1','-g',*prepare_flags,'-o',OUT/'prepare',
               'tools/camera_demo_assets.c','../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    audit.run([OUT/'prepare',data/'fftlook.raw'])
    sys.path.insert(0,str(audit.ROOT/'lib'))
    import zx0pack
    raw=(data/'fftlook.raw').read_bytes()
    assert len(raw)==12008
    # Decode only the selected image into existing scene memory; no new output buffer.
    near=zx0pack.pack(raw[8:6008]); wide=zx0pack.pack(raw[6008:12008])
    header=bytearray(raw[:8]); header[3]=2
    (data/'fftlook.bin').write_bytes(header+struct.pack('>2H',12,12+len(near))+near+wide)
    audit.run([audit.ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fftlook.bin','fftlook'],data)
    variable=data/'fftlook.89y'; raw=bytearray(variable.read_bytes());raw[0x49]=3;variable.write_bytes(raw)
    common=['../../runtime/core/rt_core.c','../../lib/unpack68k.s','../../tools/extgraph/lib/tilemap.a']
    audit.run([audit.ROOT/'tools/bin/ti-cc','-o',OUT/'fftcfg',*FLAGS,'-DRT_GRAY_COPY_PRESENT',
               'tools/camera_tiemu_demo.c','../../runtime/platform-ti68k/rt_ti.c',*common])
    audit.run(['cc','-std=gnu99','-O1','-g',*FLAGS,'-o',OUT/'oracle','tools/config_check.c',
               '../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    pc=audit.run([OUT/'oracle'],data);(OUT/'pc.csv').write_text(pc)
    audit.run([audit.ROOT/'tools/bin/ti-cc','-o',OUT/'cfgcheck',*FLAGS,'-DRT_CYCLES',
               'tools/config_check.c',*common])
    args=[audit.ROOT/'tools/bin/ti-cycles','--max',1000000]
    for path in sorted(data.glob('*.89y')): args+=['--file',path]
    log=audit.run([*args,OUT/'cfgcheck.89z'],data);(OUT/'cycles.log').write_text(log)
    actual=re.findall(r'shot \d+ checksum (\w+)',log)
    expected=[row[1].lower() for row in csv.reader(pc.splitlines())]
    assert [s.lower() for s in actual]==expected and '_main returned' in log
    assert f'(0x7601{len(expected):04x})' in log
    result=dict(scope='Interactive native camera/config review with compact ISO UI; full tactical UI remains a PC build.',
                matched_pc_ti_frames=len(expected),idle_geometry_cache='20,284-byte optional heap; cooperative selected front view, immutable geometry, live actor masks; see PREFETCH.md.',defaults='low poly 160x100, wide, textured perspective',
                controls='CATALOG/F4, up/down, ENTER/2nd/left/right toggle, ESC/SHIFT/CATALOG close; APPS retains turn list.',
                rotations='Native160x100 low poly only; textured option and its atlases/UVs/surfaces removed.',
                perspective='Textured (unchecked) versus filled untextured mesh with visible edges (checked). Depth/actors retained in both. Filtered native fixture backgrounds only at matching solid camera descriptors.',
                presenter='Fixed-address plane copies, opt-in RT_GRAY_COPY_PRESENT; no old buffer-flip timings reused.',
                source_sha256={name:hashlib.sha256((audit.HERE/name).read_bytes()).hexdigest()
                    for name in ('fft.c','fft.h','config.inc','perspective.inc','fixed.inc','cinema.inc',
                                 'prefetch.inc','tools/camera_tiemu_demo.c','tools/config_check.c')},
                binary_bytes=(OUT/'fftcfg.89z').stat().st_size,
                binary_sha256=hashlib.sha256((OUT/'fftcfg.89z').read_bytes()).hexdigest(),
                native_banks={name:len((data/(name+'.bin')).read_bytes()) for name in NATIVE_BANKS})
    assert {p.stem for p in data.glob('*.89y')} == set(NATIVE_BANKS)
    result['data_bytes']=sum(result['native_banks'].values())
    (OUT/'checked.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
