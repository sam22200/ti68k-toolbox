#!/usr/bin/env python3
"""Compatible retail status spells and actual13-victim/physical hooks, PC and68000."""
import argparse,json,re,tempfile
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links
from check_core_boundaries import descriptor13


def native_trial(source,out,result):
    fixture=out/'fixture'
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'buffchk',*source,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],fixture)
    cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
    for bank in 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
        cmd+=['--file',(fixture if bank=='ffgd'else HERE/'data')/(bank+'.89y')]
    native=run([*cmd,out/'buffchk.89z']);(out/'native.log').write_text(native)
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(result['hash'],16),0,result['checks']]and '_main returned'in native,native[-3000:]
    result.update(native_equal=True,zones=[line.strip()for line in native.splitlines()
        if re.match(r'^(?:6[1-5]\s+|max 6[1-5]\s+)',line)])

def trial(label,source_name,out,pc_only):
    out.mkdir(parents=True,exist_ok=True)
    source=[HERE/source_name,ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=out/'fixture';fixture.mkdir(exist_ok=True);(fixture/'ffgd.bin').write_bytes(descriptor13())
    with tempfile.TemporaryDirectory(prefix='fft-buffs-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        pc=run([out/'check'],data)
        run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
             '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
        assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    (out/'pc.log').write_text(pc)
    m=re.search(label+r' (\w+) failures 0 checks (\d+)',pc);assert m,pc
    result=dict(checks=int(m[2]),hash=m[1],sanitizers_equal=True)
    if not pc_only:native_trial(source,out,result)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-buffs/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    from check_offensive_magic import catalogue_report
    coverage=catalogue_report();(out/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    # Keep the historical aggregate PC digest. Native test variables are bounded;
    # separate the unchanged menu control without changing a single assertion.
    result=trial('buffs','check_buffs.c',out,True)
    if not args.pc_only:
        rules=trial('buff_rules','check_buff_rules.c',out/'rules',False)
        windows=trial('buff_windows','check_buff_windows.c',out/'windows',False)
        assert rules['checks']+windows['checks']==result['checks']
        def rotate(v,n):
            n&=31
            return ((v<<n)|(v>>(32-n if n else 32)))&0xffffffff
        combined=rotate(int(rules['hash'],16),windows['checks'])^int(windows['hash'],16)^rotate(17,windows['checks'])
        assert combined==int(result['hash'],16)
        result.update(native_equal=True,native_parts=[rules,windows],native_recombined_hash='%08X'%combined)
    result.update(added_entries=22,catalogue_casts=len(coverage['batch']),production_bank=coverage['bank_sha256'],
                  capacity=trial('buff_capacity','check_buff_capacity.c',out/'capacity',args.pc_only))
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')


if __name__=='__main__':main()
