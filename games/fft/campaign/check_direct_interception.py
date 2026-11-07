#!/usr/bin/env python3
"""Direct projectile recipients and frozen impacts, PC/sanitizers/actual 68000."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true');ap.add_argument('--native-only',action='store_true');ap.add_argument('--only',choices=('direct_interception','direct_interception_render'));args=ap.parse_args()
 out=HERE/'x/core-direct-interception/check';out.mkdir(parents=True,exist_ok=True);results={}
 for label,file,program in [('direct_interception','check_direct_interception.c','dircheck'),('direct_interception_render','check_direct_interception_render.c','dirview')]:
  if args.only and args.only!=label:continue
  src=[HERE/file,ROOT/'runtime/core/rt_core.c'];platform=[ROOT/'runtime/platform-sw/rt_sw.c'];log=out/(label+'-pc.log')
  if args.native_only:pc=log.read_text()
  else:
   run(['cc','-std=gnu99','-O1','-g','-w','-o',out/program,*src,*platform])
   with tempfile.TemporaryDirectory(prefix='fft-direct-interception-')as folder:
    data=Path(folder);data_links(data);(data/"fdir.bin").symlink_to(HERE/"x/core-direct-interception/oracle/fdir.bin");pc=run([out/program],data)
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift','-o',out/(program+'-san'),*src,*platform])
    san=run(['env','ASAN_OPTIONS=detect_leaks=0',out/(program+'-san')],data);assert pc==san
   log.write_text(pc)
  print(pc.strip(),flush=True);m=re.search(label+r' (\w+) failures 0 checks (\d+)',pc);assert m,pc
  result=dict(checks=int(m[2]),hash=m[1],sanitizers_equal=True)
  if not args.pc_only:
   run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-DFFT_CHECK_NATIVE_ORDINAL','-o',out/program,*src,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
   cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',20000]
   for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():cmd+=['--file',HERE/'data'/(bank+'.89y')]
   fixture=HERE/'x/core-direct-interception/oracle'
   run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','fdir.bin','fdir'],fixture)
   cmd+=['--file',fixture/'fdir.89y']
   native=run([*cmd,out/(program+'.89z')]);(out/(label+'-native.log')).write_text(native)
   values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
   assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-4000:]
   result['native_equal']=True
   if label=='direct_interception':result['native_layouts']=dict(zip(('Unit','State','Work','Save','ModCtx'),values[-8:-3]))
  results[label]=result
 previous=json.loads((out/'results.json').read_text())if(out/'results.json').exists()else{};previous.update(results)
 (out/'results.json').write_text(json.dumps(previous,indent=2)+'\n');print(previous,flush=True)
if __name__=='__main__':main()
