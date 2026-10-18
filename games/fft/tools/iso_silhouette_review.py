#!/usr/bin/env python3
"""Check white occluded interiors and unchanged exposed pixels on PC and native 68000."""
import json
import re

import turn_audit as audit

OUT = audit.HERE/'x/occlusion'
FLAGS = audit.FLAGS


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    data=audit.HERE/'x/config/data'
    source='tools/iso_silhouette_check.c'
    audit.run(['cc','-std=gnu99','-O1','-g',*FLAGS,'-o',OUT/'check-after',source,
               '../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'])
    pc=audit.run([OUT/'check-after',OUT/'after.png'],data);(OUT/'pc.log').write_text(pc)
    audit.run([audit.ROOT/'tools/bin/ti-cc','-o',OUT/'isohide','-DRT_CYCLES',*FLAGS,source,
               '../../runtime/core/rt_core.c','../../lib/unpack68k.s','../../tools/extgraph/lib/tilemap.a'])
    args=[audit.ROOT/'tools/bin/ti-cycles','--max',1000000]
    for path in sorted(data.glob('*.89y')):args+=['--file',path]
    log=audit.run([*args,OUT/'isohide.89z'],data);(OUT/'native.log').write_text(log)
    values=[int(n,16) for n in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
    assert values==[0x77000300,38166,0] and '_main returned' in log
    assert '768 cases, 38166 hidden pixels, 0 failures' in pc
    result=dict(cases=768,hidden_pixels=38166,pc_failures=0,native_failures=0,
                scope='4 cardinal views, 32 idle ticks, Ramza and adjacent enemy, three background tones. Dry-body silhouette interiors are white; all other opaque body pixels retain sprite art.',
                checks='Native actual pixel values read from light/dark planes; HUD excluded. Covered shadows remain transparent.',
                baseline='The same checker fails on the previous renderer: 38,166 hidden interior pixels show background instead of white.')
    (OUT/'checked.json').write_text(json.dumps(result,indent=2)+'\n')
    print(pc.strip());print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
