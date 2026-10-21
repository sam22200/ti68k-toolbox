#!/usr/bin/env python3
"""Compare settled player-camera workloads on native 68000; not emulator FPS."""
from pathlib import Path
import json,re,statistics,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]
GAME=ROOT/'games/fft';OUT=GAME/'campaign/x/map-costs'
def run(args,cwd=ROOT):
    p=subprocess.run(list(map(str,args)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode:print(p.stdout);p.check_returncode()
    return p.stdout

def main():
    OUT.mkdir(parents=True,exist_ok=True);keys=OUT/'none.keys';keys.write_text('0\n')
    run([ROOT/'tools/pyenv/bin/python',GAME/'tools/contour_assets.py','--attach',GAME/'opening/data/ffov0.bin',GAME/'campaign/data/ffgv0.bin'])
    results=[]
    for scene,folder,scenario,banks in (
        ('orbonne','opening',1,['ffov0','ffov1','ffov2','ffov3','ffoa','ffoi','ffost','ffcu','ffcw','ffca','ffcb']),
        ('gariland','campaign',61,['ffgv0','ffgv1','ffgv2','ffgv3','ffga','ffgi','ffcu','ffcw','ffca','ffcb'])):
        source=GAME/folder;template=(source/'check_native.c').read_text()
        include='#include "opening.c"' if folder=='opening' else '#include "gariland.c"'
        adapter='#define game_scenario cost_scenario\n'+include+'\n#undef game_scenario\nvoid game_scenario(u16 n){cost_scenario('+str(scenario)+');st.rot=(u8)n;st.want=255;cam_target(&st.camx,&st.camy);scene_rot=vbuf_rot=255;W->bg_ok=0;}\n'
        template=template.replace(include,adapter)
        # Visible actors at the same player-centered camera are not identical workloads.
        host='#ifndef __m68k__\nunsigned visible=0;for(unsigned i=0;i<NUNIT;i++){Feet f=unit_feet(st.rot,i);if(f.x-st.camx>=-16&&f.x-st.camx<176&&f.y-st.camy>=0&&f.y-st.camy<126)visible++;}\nprintf("VISIBLE %u\\n",visible);\n#endif\n'
        needle='if(strcmp(argv[4],"-"))' if folder=='opening' else 'return sw_write_png(argv[4],1);'
        template=template.replace(needle,host+needle)
        f=tempfile.NamedTemporaryFile(mode='w',dir=source,prefix='.cost-',suffix='.c',delete=False)
        f.write(template);f.close();wrapper=Path(f.name)
        try:
            binary=OUT/scene;flags=[] if folder=='opening' else ['-DCHECK_GAR']
            common=[wrapper,ROOT/'runtime/core/rt_core.c']
            run(['cc','-std=gnu99','-O1',*flags,'-o',binary,*common,ROOT/'runtime/platform-sw/rt_sw.c'])
            run([ROOT/'tools/bin/ti-cc','-o',OUT/(scene+'c'),'-DRT_CYCLES','-DFFT_MEASURE','-DFFT_PROFILE_ONLY',*flags,*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
            for rot in range(4):
                stem=OUT/(scene+'-'+str(rot));pc=run([binary,rot,120,keys,str(stem)+'-pc.png'],source/'data')
                command=[ROOT/'tools/bin/ti-cycles','--max',1000,'--arg',rot,'--frames',120,'--keys',keys,'--png',str(stem)+'-ti.png']
                for bank in banks:command+=['--file',source/'data'/(bank+'.89y')]
                log=run([*command,OUT/(scene+'c.89z')]);stem.with_suffix('.log').write_text(log)
                checksum=re.search(r'checksum\s+(\w+)',log)[1]
                assert checksum.lower()==re.search(r'checksum\s+(\w+)',pc)[1].lower()
                vals=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
                count=len(vals)-1 if folder=='opening' else len(vals)-3
                costs={m&65535:c for m,c in zip(vals[:count:2],vals[1:count:2])}
                warm=[costs[i] for i in range(40,120)]
                zones={int(m[1]):dict(calls=int(m[2]),total_cycles=int(m[3]),average_cycles=int(m[4])) for m in re.finditer(r'^(\d+)\s+-\s+(\d+)\s+(\d+)\s+(\d+)',log,re.M)}
                row=dict(scene=scene,rot=rot,visible_actors=int(re.search(r'VISIBLE (\d+)',pc)[1]),warm_average_cycles=round(statistics.mean(warm)),warm_max_cycles=max(warm),zones=zones,checksum=checksum)
                results.append(row);print(json.dumps(row),flush=True)
        finally:wrapper.unlink(missing_ok=True)
    (OUT/'profile.json').write_text(json.dumps(dict(scope='Player camera, 120 updates per view; warm frames40-119. Different scenery/actors. MC68000 datasheet CPU only; no grayscale/waits/packed startup.',cases=results),indent=2)+'\n')
if __name__=='__main__':main()
