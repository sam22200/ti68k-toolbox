#!/usr/bin/env python3
"""Block B checks, identical PC/sanitizer/real 68000 fixtures."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true');args=ap.parse_args()
 out=HERE/'x/core-equipment/ui';out.mkdir(parents=True,exist_ok=True);fixture=HERE/'x/core-equipment/oracle'
 src=[HERE/'check_equipment_ui.c',ROOT/'runtime/core/rt_core.c']
 run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*src,ROOT/'runtime/platform-sw/rt_sw.c'])
 with tempfile.TemporaryDirectory(prefix='fft-equipment-')as folder:
  data=Path(folder);data_links(data)
  for name in ():(data/(name+'.bin')).symlink_to(fixture/(name+'.bin'))
  pc=run([out/'check'],data)
  run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift','-o',out/'check-san',*src,ROOT/'runtime/platform-sw/rt_sw.c'])
  san=run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data);assert pc==san
 print(pc.strip(),flush=True);(out/'pc.log').write_text(pc)
 m=re.search(r'equipment_ui (\w+) failures 0 checks (\d+)',pc);assert m,pc
 result=dict(checks=int(m[2]),hash=m[1],sanitizers_equal=True)
 if not args.pc_only:
  run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'equipchk',*src,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
  for name in ():run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',name+'.bin',name],fixture)
  cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',20000]
  for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():cmd+=['--file',HERE/'data'/(bank+'.89y')]
  for bank in ():cmd+=['--file',fixture/(bank+'.89y')]
  native=run([*cmd,out/'equipchk.89z']);(out/'native.log').write_text(native)
  values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
  assert values[-3:]==[int(m[1],16),0,int(m[2])] and '_main returned'in native,native[-4000:]
  peaks={k:int(v)for k,v in re.findall(r'max (61|62|63)\s+-\s+(\d+)',native)}
  assert len(peaks)==3 and max(peaks.values())<=360000,peaks
  result.update(native_equal=True,pictures=10,peaks=peaks,scope='Equipment draw/input zones; broader tactical whole-frame gate remains separate')
 (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)
if __name__=='__main__':main()
