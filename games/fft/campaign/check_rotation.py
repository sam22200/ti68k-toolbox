#!/usr/bin/env python3
"""Current campaign turns: real key path, both chapters and independent geometry."""
import json,re,tempfile
from pathlib import Path
from check_flow import HERE,ROOT,run,data_links
from review_files import publish_images,write_changed

def main():
    out=HERE/'x/rotation-check';out.mkdir(parents=True,exist_ok=True)
    common=[HERE/'check_qa_turns.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-o',out/'turns',*common,ROOT/'runtime/platform-sw/rt_sw.c','-lm'])
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'turnsc',*common,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    banks={}
    for origin in (HERE.parent/'opening/data',HERE/'data'):
        for source in origin.glob('*.89y'):banks[source.stem]=source
    report=[]
    # Each chapter has a fresh process, matching the supervisor's file-cache lifetime.
    for chapter,scenario in (('orbonne',101),('gariland',201)):
        dest=out/chapter;dest.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='fft-rotation-')as name:
            data=Path(name);data_links(data);shots=data/'shots';shots.mkdir()
            pc=run([out/'turns',shots,scenario],data)
            publish_images(shots,dest)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',scenario,'--max',10000]
        for source in banks.values():cmd+=['--file',source]
        native=run([*cmd,out/'turnsc.89z'])
        match=re.search(r'turns ([0-9A-F]+) failures 0 checks (\d+)',pc);assert match,pc
        values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-4000:]
        assert len(re.findall(r'shot \d+ checksum',native))==41
        corners=int(re.search(r'direction (\d+)',pc)[1])
        write_changed(dest/'pc.log',pc.encode());write_changed(dest/'native.log',native.encode())
        report.append(dict(chapter=chapter,hash=match[1],checks=int(match[2]),frames=40,projected_corners=corners))
        print(chapter+': '+pc.strip(),flush=True)
    write_changed(out/'checks.json',(json.dumps(dict(
        scope='Current FFT_BATTLE/FFT_SOURCE_CAMERA, native160x100; same key/update/render path on PC and68000.',
        cases=report,limit='Checks rendered frames and logical order. No live LCD capture or new total-duration/FPS claim.'),indent=2)+'\n').encode())

if __name__=='__main__':main()
