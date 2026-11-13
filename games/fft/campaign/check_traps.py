#!/usr/bin/env python3
"""Focused PC/sanitizer trap replay using synthetic records in private data only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from check_flow import HERE, ROOT, data_links, run


def synthetic_common(source):
    data=bytearray(source)
    battle=struct.unpack_from('>H',data,4+13*2)[0]
    discovery=640*12+8+20+256+64+416+316+(384+480)+2048
    maps=battle+discovery+96
    # Four records at ground-floor fixture positions. Other maps stay original.
    rows=[bytes((0x22,33,0,0, 0x32,34,0,0, 0x42,36,0,0, 0x52,40,0,0)),
          bytes((0x22,2,1,1))+bytes((255,48,0,0))*3,
          bytes((0x22,63,0,0))+bytes((255,48,0,0))*3,
          bytes((0x22,48,0,0,0x22,33,0,0))+bytes((255,48,0,0))*2]
    data[maps:maps+64]=b''.join(rows)
    return bytes(data)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/tile-traps/pc')
    ap.add_argument('--review',action='store_true',help='also capture four working visual candidates')
    ap.add_argument('--native-numbers',action='store_true',help='opt in to one independent68000 arithmetic/bitmap check; no overlays/rendering')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    fixture=HERE/'x/tile-traps/oracle/fixture.h'
    if not fixture.exists():raise SystemExit('Run tools/pyenv/bin/python games/fft/tools/trap_oracle.py first')
    logs=[]
    for mode,flags in [('pc',[]),('san',['-fsanitize=address,undefined','-fno-sanitize=shift','-fno-pie','-no-pie'])]:
        binary=out/mode
        (out/f'{mode}-build.log').write_text(run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',binary,
              HERE/'check_traps.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']))
        with tempfile.TemporaryDirectory(prefix='fft-traps-')as name:
            data=Path(name);data_links(data)
            original=(data/'ffcj.bin').read_bytes();(data/'ffcj.bin').unlink()
            common=synthetic_common(original);(data/'ffcj.bin').write_bytes(common)
            descriptor=bytearray((data/'ffgd.bin').read_bytes());(data/'ffgd.bin').unlink()
            assert descriptor[:4]==b'FCD5'
            struct.pack_into('>H',descriptor,104,0);(data/'ffgd.bin').write_bytes(descriptor)
            command=['env','ASAN_OPTIONS=detect_leaks=0','UBSAN_OPTIONS=halt_on_error=1',binary]
            if args.review and mode=='pc':
                shots=out/'review';shots.mkdir(exist_ok=True);command+=[shots]
            proc=subprocess.run(list(map(str,command)),cwd=data,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            (out/f'{mode}.log').write_text(proc.stdout)
            if proc.returncode:print(proc.stdout);proc.check_returncode()
            # Review runs add prepare() assertions; compare the common test digest
            # before rendering, printed by the carrier itself.
            logs.append(proc.stdout.strip());print(mode+': '+proc.stdout.strip(),flush=True)
    assert logs[0]==logs[1],logs
    record=dict(result=logs[0],sanitizers_equal=True,original_common_sha256=hashlib.sha256(original).hexdigest(),
                synthetic_common_sha256=hashlib.sha256(common).hexdigest(),
                source_oracle_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),native=False,
                visual='working candidate; needs approval'if args.review else 'no captures')
    if args.native_numbers:
        numbers=out/'numbers';numbers.mkdir(exist_ok=True);source=HERE/'check_trap_numbers.c'
        (numbers/'pc-build.log').write_text(run(['cc','-std=gnu99','-O1','-w','-o',numbers/'pc',source]))
        pc=run([numbers/'pc']);(numbers/'pc.log').write_text(pc)
        (numbers/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',numbers/'trapnum',source]))
        native=run([ROOT/'tools/bin/ti-cycles','--frames',0,'--max',100,numbers/'trapnum.89z'])
        (numbers/'native.log').write_text(native)
        match=re.search(r'trap-numbers (\w+) failures 0 checks (\d+)',pc);assert match,pc
        values=[int(x,16)for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[1],16),0,int(match[2])],values
        record['native_numbers']=dict(hash=match[1],checks=int(match[2]),equal=True)
    (out/'results.json').write_text(json.dumps(record,indent=2)+'\n')


if __name__=='__main__':main()
