#!/usr/bin/env python3
"""Actual range/inspection/confirmation/order replay; run check_area_ui.py first."""
import json,re,sys,tempfile,struct
from pathlib import Path
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'x/core-area-ui'
sys.path.insert(0,str(h));from check import run
from check_flow import data_links
s=(h/'check_native.c').read_text()
s=s.replace('    return sw_write_png(argv[4],1);','    printf("area_state %u %u %u %u\\n",st.mode,st.abil,W->area_ui.confirm,st.acted);\n    return sw_write_png(argv[4],1);')
# A compact native trace of the same selector state, apart from existing game hashes.
pos=s.rfind('}');s=s[:pos]+'BENCH_VALUE((u32)st.mode|((u32)st.abil<<8)|((u32)W->area_ui.confirm<<24)|((u32)st.acted<<25));'+s[pos:]
source=o/'full.c';source.write_text(s)
common=[source,r/'runtime/core/rt_core.c']
run(['cc','-std=gnu99','-O1','-w','-DCHECK_GAR','-I'+str(h),'-o',o/'fullpc',*common,r/'runtime/platform-sw/rt_sw.c'])
if '--pc-only'not in sys.argv:run([r/'tools/bin/ti-cc','-DRT_CYCLES','-DCHECK_GAR','-I'+str(h),'-o',o/'fullti',*common,r/'lib/unpack68k.s',r/'tools/extgraph/lib/tilemap.a'])
keys=o/'target.keys';keys.write_text('20 DOWN\n21\n40 ENTER\n41\n60 DOWN\n61\n80 ENTER\n81\n100 ENTER\n101\n120 ENTER\n121\n140 ENTER\n141\n160 DOWN\n161\n180 ENTER\n181\n')
report=[]
with tempfile.TemporaryDirectory(prefix='fft-area-full-')as name:
 data=Path(name);data_links(data);fixture=data/'native';fixture.mkdir()
 raw=bytearray((h/'x/core-area-ui/fixture/ffcj.bin').read_bytes());primary,rg,norm=struct.unpack_from('>H',raw,12)[0],struct.unpack_from('>H',raw,14)[0],struct.unpack_from('>H',raw,30)[0]
 for at,step in ((primary,8),(rg,14),(norm,12)):raw[at+step*152:at+step*153]=raw[at+step*16:at+step*17]
 (data/'ffcj.bin').unlink();(data/'ffcj.bin').write_bytes(raw);(fixture/'ffcj.bin').write_bytes(raw)
 if '--pc-only'not in sys.argv:run([r/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffcj.bin','ffcj'],fixture)
 for frames in (111,131,171,191):
  pc=run([o/'fullpc',61,frames,keys,data/'pc.png'],data);print(pc.strip(),flush=True)
  mode,ab,confirm,acted=map(int,re.search(r'area_state (\d+) (\d+) (\d+) (\d+)',pc).groups())
  assert ab==152 and mode==(15 if frames==111 else 3 if frames==191 else 16)
  assert acted==(frames==191)
  if frames in (131,171):assert confirm==(frames==171)
  row=dict(frames=frames,mode=mode,ability=ab,confirm=confirm,acted=acted)
  if '--pc-only'not in sys.argv:
   cmd=[r/'tools/bin/ti-cycles','--frames',frames,'--arg',61,'--max',10000,'--keys',keys]
   for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():cmd+=['--file',(fixture if bank=='ffcj'else h/'data')/(bank+'.89y')]
   ti=run([*cmd,o/'fullti.89z']);(o/('full-'+str(frames)+'.log')).write_text(ti)
   v=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
   state=int(re.search(r'state (\w+)',pc)[1],16)
   assert v[-5:-2]==[frames,0xacad0001,state]and v[-1]==(mode|ab<<8|confirm<<24|acted<<25),v[-5:]
   assert re.search(r'checksum\s+(\w+)',ti)[1].lower()==re.search(r'checksum (\w+)',pc)[1].lower()
   row['native_equal']=True
  report.append(row)
(o/'full-replay.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
