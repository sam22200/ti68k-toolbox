#!/usr/bin/env python3
"""First-render range cleanup, including cold colour reload, on both CPUs/maps."""
import json,re,tempfile
from pathlib import Path
from check_flow import HERE,ROOT,run,data_links
from review_files import write_changed

def main():
    out=HERE/'x/qa-followup';out.mkdir(parents=True,exist_ok=True)
    common=[HERE/'check_range_cancel.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-w','-o',out/'range',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'rangec',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    files={}
    for origin in (HERE.parent/'opening/data',HERE/'data'):
        for path in origin.glob('*.89y'):files[path.stem]=path
    report=[]
    for chapter,scenario in (('orbonne',101),('gariland',201)):
        with tempfile.TemporaryDirectory(prefix='fft-cancel-')as folder:
            data=Path(folder);data_links(data);pc=run([out/'range',scenario],data)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',scenario,'--max',10000]
        for path in files.values():cmd+=['--file',path]
        native=run([*cmd,out/'rangec.89z'])
        match=re.search(r'cancel ([0-9A-F]+) failures 0 checks (\d+)',pc);assert match,pc
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[1],16),0,int(match[2])],(pc,values[-5:])
        write_changed(out/(chapter+'-cancel-native.log'),native.encode())
        write_changed(out/(chapter+'-cancel-pc.log'),pc.encode())
        report.append(dict(chapter=chapter,checks=int(match[2]),hash=match[1],failures=0))
        print(chapter+': '+pc.strip(),flush=True)
    write_changed(out/'cancel-checks.json',(json.dumps(report,indent=2)+'\n').encode())

if __name__=='__main__':main()
