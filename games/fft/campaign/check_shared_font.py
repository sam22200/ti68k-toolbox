#!/usr/bin/env python3
"""Portable font parser/drawing checks; native pixels, no emulator or live saves."""
import json,re
from check_flow import HERE,ROOT,run
from review_files import write_changed
from shared_font_assets import build

def main():
    out=HERE/'x/shared-font';data=out/'data';build(data)
    common=[HERE/'check_shared_font.c',ROOT/'lib/bitmap_font.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-fsanitize=address,undefined','-o',out/'font-test',*common,
         ROOT/'runtime/platform-sw/rt_sw.c'])
    pc=run([out/'font-test'],data);print(pc.strip())
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'fontchk',*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffserif.bin','ffserif'],data)
    native=run([ROOT/'tools/bin/ti-cycles','--frames',0,'--max',10000,'--file',data/'ffserif.89y',out/'fontchk.89z'])
    match=re.search(r'font ([0-9A-F]+) failures (\d+) checks (\d+)',pc);assert match and match[2]=='0',pc
    values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[3])],native
    write_changed(out/'font-pc.log',pc.encode());write_changed(out/'font-native.log',native.encode())
    write_changed(out/'checks.json',(json.dumps(dict(checks=int(match[3]),failures=0,
        screen_hash=match[1],rendered_screens=37,pc_sanitizers='address,undefined',
        native='Actual68000 binary; same accumulated visible-plane hash',
        limit='Font API only; full chapter packaging follows visual approval. No timing/FPS claim.'),indent=2)+'\n').encode())
    print('Shared font:37 PC/68000 screens agree; bounds/fallback/layout checks pass.')

if __name__=='__main__':main()
