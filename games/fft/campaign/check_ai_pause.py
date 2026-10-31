#!/usr/bin/env python3
"""Elapsed AI pause boundaries on PC/sanitizers and the actual 68000 code."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-ai-pause/check')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=[HERE/'check_ai_pause.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-ai-pause-')as folder:
        data=Path(folder);data_links(data)
        pc=run([out/'check'],data)
        assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    m=re.search(r'ai_pause (\w+) failures 0 checks (\d+)',pc);assert m
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'aipause',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
        cmd+=['--file',HERE/'data'/(bank+'.89y')]
    native=run([*cmd,out/'aipause.89z']);(out/'native.log').write_text(native)
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(m[1],16),0,int(m[2])]and '_main returned'in native,native[-2500:]
    report=dict(checks=int(m[2]),hash=m[1],native_equal=True,clock='injected256Hz',
        pause_ticks=384,new_heap_requests=0,state_work_save_layout_changed=False,
        scope='0..2s thought, elapsed deadline/wrap, same-clock fast updates, replan/new-turn/cancel, preserved frozen planes; no live IRQ test')
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)


if __name__=='__main__':main()
