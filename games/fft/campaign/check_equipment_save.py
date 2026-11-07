#!/usr/bin/env python3
"""Block B checks, identical PC/sanitizer/real 68000 fixtures."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links,fixture as save_fixture
import save_dump

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true');args=ap.parse_args()
 out=HERE/'x/core-discovery/regression-equipment-save';out.mkdir(parents=True,exist_ok=True);fixture=HERE/'x/core-equipment/oracle'
 src=[HERE/'check_equipment_save.c',ROOT/'runtime/core/rt_core.c']
 initial=bytearray(save_fixture());initial[36]=1;initial[294]=76;initial[294+25:294+30]=bytes((0,0,0,19,0));initial[38+19]=0;initial[38+20]=1;initial[38+112]=4;initial[38+38]=2
 run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*src,ROOT/'runtime/platform-sw/rt_sw.c'])
 with tempfile.TemporaryDirectory(prefix='fft-equipment-')as folder:
  data=Path(folder);data_links(data)
  (data/'ffsave.sav').write_bytes(initial)
  pc=run([out/'check'],data);saved=(data/'ffsave.sav').read_bytes();(data/'ffsave.sav').write_bytes(initial)
  run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift','-o',out/'check-san',*src,ROOT/'runtime/platform-sw/rt_sw.c'])
  san=run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data);assert pc==san and (data/'ffsave.sav').read_bytes()==saved
  (out/'settled.sav').write_bytes(saved)
 print(pc.strip(),flush=True);(out/'pc.log').write_text(pc)
 m=re.search(r'equipment_save (\w+) failures 0 checks (\d+)',pc);assert m,pc
 result=dict(checks=int(m[2]),hash=m[1],sanitizers_equal=True)
 if not args.pc_only:
  run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'equipchk',*src,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
  (out/'ffsave.sav').write_bytes(initial);run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','ffsave.sav','ffsave'],out)
  cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',20000]
  for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():cmd+=['--file',HERE/'data'/(bank+'.89y')]
  (out/'native-save').mkdir(exist_ok=True);cmd+=['--file',out/'ffsave.89y','--save-dir',out/'native-save']
  native=run([*cmd,out/'equipchk.89z']);(out/'native.log').write_text(native)
  values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
  assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-4000:]
  assert save_dump.payload(out/'native-save/ffsave.89y')==saved
  result.update(native_equal=True,serialized_equal=True)
 (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)
if __name__=='__main__':main()
