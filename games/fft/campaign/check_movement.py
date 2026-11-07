#!/usr/bin/env python3
"""Movement block: original rules, native consumers and bounded navigation."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links

def trial(name,source,out,native):
 out.mkdir(parents=True,exist_ok=True);src=[HERE/source]
 if name!="movement_rules":src += [ROOT/"runtime/core/rt_core.c"]
 libs=[ROOT/"runtime/platform-sw/rt_sw.c"] if name!="movement_rules" else []
 for label,flags in (("check",[]),("check-san",["-fsanitize=address,undefined","-fno-sanitize=shift"])):
  run(["cc","-std=gnu99","-O1","-g","-w",*flags,"-o",out/label,*src,*libs])
 with tempfile.TemporaryDirectory(prefix="fft-movement-") as folder:
  data=Path(folder);data_links(data);pc=run([out/"check"],data)
  assert run(["env","ASAN_OPTIONS=detect_leaks=0",out/"check-san"],data)==pc
 (out/"pc.log").write_text(pc);print(pc.strip(),flush=True)
 m=re.search(name+r" (\w+) failures 0 checks (\d+)",pc);assert m,pc
 report=dict(checks=int(m[2]),hash=m[1],sanitizers_equal=True)
 picture=re.search(r"pictures (\d+)",pc)
 if picture:report["pictures"]=int(picture[1])
 if native:
  run([ROOT/"tools/bin/ti-cc","-DRT_CYCLES","-o",out/"movechk",*src,ROOT/"lib/unpack68k.s",ROOT/"tools/extgraph/lib/tilemap.a"])
  cmd=[ROOT/"tools/bin/ti-cycles","--frames",0,"--arg",61,"--max",10000]
  if name!="movement_rules":
   for bank in "ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj".split():cmd += ["--file",HERE/"data"/(bank+".89y")]
  log=run([*cmd,out/"movechk.89z"]);(out/"native.log").write_text(log)
  values=[int(v,16) for v in re.findall(r"value: .*?\(0x([0-9a-f]+)\)",log)]
  assert values[-3:]==[int(m[1],16),0,int(m[2])] and "_main returned" in log,log[-2500:]
  report.update(native_equal=True,zones=[s.strip()for s in log.splitlines()if re.match(r"^(?:(?:61|62)\s+|max (?:61|62)\s+)",s)])
 (out/"results.json").write_text(json.dumps(report,indent=2)+"\n");return report

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument("--pc-only",action="store_true");args=ap.parse_args()
 out=HERE/"x/core-movement/check"
 report={name:trial(name,source,out/name,not args.pc_only)for name,source in (("movement_rules","check_movement_rules.c"),("equipped_movement","check_movement.c"),("movement_ui","check_movement_ui.c"))}
 (out/"results.json").write_text(json.dumps(report,indent=2)+"\n")
if __name__=="__main__":main()
