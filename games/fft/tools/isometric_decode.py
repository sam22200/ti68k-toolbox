#!/usr/bin/env python3
"""Datasheet cycles for loading the three complete ISO styles; no TiEmu or new ASM."""
import json
import re
import subprocess

from texture_review import HERE

ROOT = HERE.parents[1]
OUT = HERE/'x/isometric-storage'


def run(args,cwd=HERE):
    return subprocess.check_output([str(a) for a in args],cwd=cwd,text=True)


def checksum(raw):
    a = b = 0
    for value in raw:
        a = (a+value)%255; b = (b+a)%255
    return (b<<8)|a


def main():
    storage = json.loads((OUT/'storage.json').read_text())
    assert storage['raw_total_bytes']//4 == 52320
    run([ROOT/'tools/bin/ti-cc','-o',OUT/'decbench','-DRT_CYCLES','tools/isometric_decode_bench.c',
         '../../runtime/core/rt_core.c','../../runtime/platform-ti68k/rt_ti.c','../../lib/unpack68k.s',
         '../../tools/extgraph/lib/tilemap.a'])
    bank = (HERE/'fftiso.bin').read_bytes(); plane = 8720
    result = {}
    for style in ['original','contrast','edges']:
        result[style] = []
        for rot in range(4):
            folder = OUT/style/f'decode-{rot}'; folder.mkdir(parents=True,exist_ok=True)
            packed = HERE/f'fftv{rot}.bin' if style == 'original' else OUT/style/f'fftv{rot}.bin'
            raw = (HERE/f'x/isometric-style/original-{rot}.bin').read_bytes()
            if style != 'original':
                offset = 8+((style == 'edges')*8+rot*2)*plane
                raw = bank[offset:offset+2*plane]+raw[2*plane:]
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',packed,'isocost'],folder)
            log = run([ROOT/'tools/bin/ti-cycles','--frames',1,'--max',1000,'--file',folder/'isocost.89y',OUT/'decbench.89z'])
            (folder/'cycles.log').write_text(log)
            assert '_main returned' in log
            values = [int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
            assert values[:2] == [len(raw),checksum(raw)]
            zone = re.search(r'^\s*4\s+ZX0 view decode\s+\d+\s+(\d+)',log,re.M)
            if not zone:
                raise RuntimeError(log)
            result[style].append(int(zone.group(1)))
    mean = {style:sum(values)/4 for style,values in result.items()}
    report = dict(scope='Headless 68000 datasheet timing, existing lib/unpack68k.s ZX0; one view load, not per-frame work.',
                  cycles=result,mean_cycles=mean,mean_ms_at_12mhz={s:v/12000 for s,v in mean.items()},
                  edges_vs_contrast_cycles=mean['edges']-mean['contrast'],
                  edges_vs_original_cycles=mean['edges']-mean['original'],
                  all_12_decode_lengths_and_checksums_match=True)
    (OUT/'decode.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
