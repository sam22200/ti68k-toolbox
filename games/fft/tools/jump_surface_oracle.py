#!/usr/bin/env python3
"""Original horizontal-jump terrain check with controlled two-layer columns.

No actor records: this proves intermediate terrain clearance only. The source
instructions are executed unchanged; generated fixture banks stay local.
"""
import argparse,json,struct
from pathlib import Path
import surface_data as surface

surface.FUNCTIONS['horizontal_jump']=(0x8017622c,684,'551a8a31fcf7897c')


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=surface.HERE/'campaign/x/core-jump/oracle')
    args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    disc,files=surface.extract.disc_files(str(surface.core_audit.DEFAULT_DISC))
    columns=set();sources=[]
    for number in (22,56,1,29,53):
        raw,meta,provenance=surface.terrain(disc,files,number)
        columns.update(surface.column(raw,i)for i in range(meta['width']*meta['height']))
        sources.append(provenance)
    # Reversed slopes, gaps, underpasses, water, blocked upper and byte wraps.
    for half in (0,1,3,7):
        for slope in (0,1,4,16,64,0x1b,0xe4):
            for depth in (0,1,5):
                for blocked in (0,128):
                    columns.add(bytes((2,half,slope,depth,8,0,0,1|blocked)))
    original=surface.Original(surface.battle.Disc(str(surface.core_audit.DEFAULT_DISC)))
    original.write(0x8018f4f0,0x801d6000)
    cases=[]
    for col in sorted(columns):
        original.bind(col)
        for layer in range(2):
            original.uc.mem_write(0x1d6000+layer*256,bytes((col[4*layer+3]&128,)))
        for side in (0,2,4,6):
            heights={0,1,5,6,12,63,126,127,250,255}
            for layer in range(2):
                h,dh,slope,flags=col[4*layer:4*layer+4]
                edges=[(2*h+(dh&31)*((slope>>shift)&3))&255 for shift in (side,side^2)]
                low=(min(edges)-2*(flags&31))&255;high=(max(edges)+2*(dh>>5))&255
                for boundary in (low,high):
                    heights.update(v for v in (boundary-6,boundary-5,boundary-1,boundary,boundary+1)if 0<=v<=255)
            for body in (4,6):
                original.uc.mem_write(0x1d001b,bytes((body,)))
                for height in sorted(heights):
                    expected=1-original.call('horizontal_jump',0,side,side^2,height)
                    assert expected in (0,1)
                    cases.append(col+struct.pack('>BBhBB',body,side,height,expected,0))
    # <=16 rt_file banks. Keep rows14B and each archived variable below64KiB.
    assert len(cases)<=16*4000
    for i in range(0,len(cases),4000):
        chunk=cases[i:i+4000]
        (args.out/f'fjq{i//4000:02}.bin').write_bytes(b'FJO1'+struct.pack('>H',len(chunk))+b''.join(chunk))
    report=dict(cases=len(cases),unique_columns=len(columns),banks=(len(cases)+3999)//4000,
                function_hashes=original.function_hashes,unicorn=original.version,sources=sources,
                scope='Original MIPS terrain-only intermediate jump check; no actor spans, full spread, animation or PS1 timing.')
    (args.out/'source.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:report[k]for k in ('cases','unique_columns','banks')}),flush=True)


if __name__=='__main__':main()
