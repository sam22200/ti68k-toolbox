#!/usr/bin/env python3
"""One production formula08 batch: catalogue coverage and PC/sanitizer/68000 execution."""
import argparse, hashlib, json, re, struct, sys, tempfile
from pathlib import Path
from check import HERE, ROOT, run
from check_flow import data_links
from check_core_boundaries import descriptor13
sys.path.insert(0,str(HERE.parent/'tools'))
from common_tables import offensive_magic_record,healing_magic_record,percent_magic_record,decisive_magic_record,brave_magic_record,golem_magic_record,punch_record,steal_gil_record

SPELLS=(0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,
        0x1f,0x2c,0x3d,0x3e,0x3f,0x40,0x43,0x44,0x45,0x46,0x4a,0x4b)
PERCENT=(42,43,47,48,73)
CLEANSE=(14,56,167)                 # 167: Deathspell 2 (Holy Magic)
DECISIVE=(30,41,51,59)
CONTROL=(29,53,55)
BRAVE=(54,)
PROTECT=(7,)
GOLEM=(65,)
PUNCH=(105,106,107)
STRIKES=(101,102)
SPIN=(100,)
GUTS=(150,151,153)
GIL=(123,)
GEOMANCY=tuple(range(126,138))
SECRET_FIST=(104,)
LINE=(103,)
DRAW_OUT=tuple(range(76,86))
PERFORMANCE=tuple(range(86,100))
ALLEGIANCE=(109,116)
FLOAT=(38,)
CHOCOBO=(268,269)
TALK=(117,118,119,120,121,122,124,125)
HEALING=(1,2,3,4,5,6,60,72)
BUFFS=(8,9,10,11,12,13,28,32,33,34,35,36,37,39,46,49,50,52,57,58,66,71,212)   # 212: Mbarrier (adapted row)


def catalogue_report():
    b=(HERE/'data/ffcj.bin').read_bytes()
    section=lambda k:struct.unpack_from('>H',b,4+2*k)[0]
    raw=lambda a:b[section(5)+14*a:section(5)+14*(a+1)]
    norm=lambda a:b[section(13)+12*a:section(13)+12*(a+1)]
    def name(a):
        p=struct.unpack_from('>H',b,section(10)+2*a)[0]
        return b[p:b.index(0,p)].decode('ascii')
    for a in range(512):
        n=norm(a)
        if n[0]!=255 and(n[10]in range(8,17)or n[10]==18):assert n[4]&128,(a,n)
    actual=tuple(a for a in range(512)if norm(a)[4]&128)
    assert actual==tuple(sorted(SPELLS+HEALING+BUFFS+PERCENT+CLEANSE+DECISIVE+CONTROL+BRAVE+PROTECT+GOLEM+PUNCH+TALK+SPIN+GUTS+GIL+GEOMANCY+SECRET_FIST+LINE+DRAW_OUT+PERFORMANCE+ALLEGIANCE+FLOAT+CHOCOBO)),actual
    assert tuple(norm(265)[:5])==(0,0,1,4,10) and norm(265)[10]==1
    for a,kind,formula in ((268,13,81),(269,3,76)):
        r,n=raw(a),norm(a);assert n[0]==kind and n[4]==161 and n[10]==r[8]==formula
    # Reject unsupported behavior as a family; adding an id must not bypass it.
    base=raw(SPELLS[0])
    for offset,bit in ((3,32),(4,2),(4,4),(4,8),(5,16),(5,4),(6,32),(9,1),(11,1)):
        r=bytearray(base);r[offset]|=bit;assert offensive_magic_record(r) is None,(offset,bit)
    for a in SPELLS:
        r,n=raw(a),norm(a)
        assert n[0]==0 and n[1]==r[10] and n[2]==r[0] and n[3]==255
        assert n[4]==128|(32 if r[1]else 0)|1|(2 if r[6]&2 else 0)
        assert n[10:12]==bytes((8,r[7])) and offensive_magic_record(r)
    for a in HEALING:
        r,n=raw(a),norm(a);expected=healing_magic_record(r)
        assert expected and tuple(n[:5])==expected and n[10]==r[8] and n[11]==0
    for a in BUFFS:
        r,n=raw(a),norm(a);assert n[0]==10 and n[1]==(0x7F if a==212 else r[11]) and n[10]==r[8]   # Mbarrier: adapted row
        assert n[4]==128|32|1|(2 if r[6]&2 else 0)
    for a in PERCENT:
        r,n=raw(a),norm(a);assert tuple(n[:5])==percent_magic_record(r)
        assert n[10:12]==bytes((r[8],r[7]))
    for a in CLEANSE:
        r,n=raw(a),norm(a);assert n[0]==13 and n[1]==r[11] and n[10]==r[8]
        assert b[section(12)+6*r[11]]==16
    for a in DECISIVE:
        r,n=raw(a),norm(a)
        if a in(30,41):assert tuple(n[:5])==decisive_magic_record(r)
        else:assert n[0]==10 and n[1]==r[11]
        assert n[10]==r[8]
    for a in CONTROL+PROTECT:
        r,n=raw(a),norm(a);assert n[0]==10 and n[1]==(0x7F if a==212 else r[11]) and n[10]==r[8]   # Mbarrier: adapted row
    for a in GOLEM:
        r,n=raw(a),norm(a);assert tuple(n[:5])==golem_magic_record(r) and n[10]==0x14
    for a in PUNCH:
        r,n=raw(a),norm(a);assert n[0]in(4,13,14)and n[10]==r[8]in(0x33,0x34,0x35)and n[4]==(160 if a==107 else 161),(a,n)
    for a in TALK:
        r,n=raw(a),norm(a);assert n[0]in(10,17,18,20)and n[4]==161 and n[10]==r[8]==0x2a,(a,n)
    for a in(108,123):
        r,n=raw(a),norm(a);assert tuple(n[:5])==steal_gil_record(a,r)
        assert n[10:12]==r[8:10]
        bad=bytearray(r);bad[9]^=1;assert steal_gil_record(a,bad)is None
    for a in SPIN:
        r,n=raw(a),norm(a);assert n[0]==0 and n[4]==163 and n[10]==r[8]==0x31,(a,n)
    for a in GEOMANCY:
        r,n=raw(a),norm(a);assert tuple(n[:5])==(0,2,5,255,161)and n[10:12]==bytes((0x24,r[7]))
    for a in STRIKES:
        r,n=raw(a),norm(a);assert n[0]==0 and n[4]==2 and n[10]==r[8]in(0x31,0x32),(a,n)
    for a in BRAVE:
        r,n=raw(a),norm(a);assert tuple(n[:5])==brave_magic_record(r) and n[10]==0x61
    for a in(30,41):
        r=bytearray(raw(a));r[7]=1;assert decisive_magic_record(r)is None
    base=raw(47)
    for offset,value in ((1,1),(3,0),(4,0),(5,base[5]|128),(7,1)):
        r=bytearray(base);r[offset]=value;assert percent_magic_record(r)is None,(offset,value)
    ids=set()
    for job in range(0x4a,0x5e):
        skillset=b[section(2)+job];p=section(8)+25*skillset
        for k in range(16):
            a=b[p+3+k]+(256 if b[p+(k>>3)]&(128>>(k&7))else 0)
            if a:ids.add(a)
    groups={};entries=[]
    for a in sorted(ids):
        enabled=norm(a)[0]!=255
        family='formula_%02x'%raw(a)[8]if a<368 else 'special_dispatch'
        q=groups.setdefault(family,dict(total=0,enabled=0,remaining=0))
        q['total']+=1;q['enabled']+=enabled;q['remaining']+=not enabled
        entries.append(dict(id=a,name=name(a),family=family,enabled=enabled))
    enabled=sum(e['enabled']for e in entries)
    return dict(bank_sha256=hashlib.sha256(b).hexdigest(),bank_bytes=len(b),
        executable_records=sum(norm(a)[0]not in(255,26,27)for a in range(640)),
        source_records=sum(norm(a)[0]!=255 for a in range(512)),throw_items=sum(norm(a)[0]!=255 for a in range(512,640)),
        generic_actions=dict(total=len(ids),enabled=enabled,remaining=len(ids)-enabled),
        batch=[dict(id=a,name=name(a),power=raw(a)[10],mp=raw(a)[13],ct=raw(a)[12],
                    radius=raw(a)[1],vertical=raw(a)[2],element=raw(a)[7])for a in actual],
        groups=groups,entries=entries,
        limits=['Invocations reuse approved generic area effects, without PS1 summon cinematics.',
                'AI uses adapted immediate HP/status/team scoring, not full future-CT or Reflect simulation.'])


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc-only',action='store_true')
    ap.add_argument('--out',type=Path,default=HERE/'x/core-offensive-magic/check');args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    report=catalogue_report();(out/'coverage.json').write_text(json.dumps(report,indent=2)+'\n')
    source=[HERE/'check_offensive_magic.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'check',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-o',out/'check-san',*source,ROOT/'runtime/platform-sw/rt_sw.c'])
    fixture=out/'fixture';fixture.mkdir(exist_ok=True);(fixture/'ffgd.bin').write_bytes(descriptor13())
    with tempfile.TemporaryDirectory(prefix='fft-offensive-magic-')as folder:
        data=Path(folder);data_links(data)
        (data/'ffgd.bin').unlink();(data/'ffgd.bin').symlink_to(fixture/'ffgd.bin')
        pc=run([out/'check'],data)
        assert run(['env','ASAN_OPTIONS=detect_leaks=0',out/'check-san'],data)==pc
    print(pc.strip(),flush=True)
    m=re.search(r'offensive_magic (\w+) failures 0 checks (\d+)',pc);assert m,pc
    result=dict(checks=int(m[2]),hash=m[1],production_bank=report['bank_sha256'],batch_entries=14,
                catalogue_casts=len(report['batch']))
    if not args.pc_only:
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'spellchk',*source,
             ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],fixture)
        cmd=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',61,'--max',10000]
        for bank in 'ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split():
            cmd+=['--file',(fixture if bank=='ffgd'else HERE/'data')/(bank+'.89y')]
        native=run([*cmd,out/'spellchk.89z']);(out/'native.log').write_text(native)
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(m[1],16),0,int(m[2])]and '_main returned'in native,native[-3000:]
        result.update(native_equal=True,zones=[line.strip()for line in native.splitlines()
            if re.match(r'^(?:(?:71|72|73)\s+|max (?:71|72|73)\s+)',line)])
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result,flush=True)


if __name__=='__main__':main()
