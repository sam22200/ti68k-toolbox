#!/usr/bin/env python3
"""Package ownership, switching and supervisor safety on PC/sanitizers/68000.

By default use synthetic data. --input checks a freshly built production source
group as well. All writes stay in --out or a temporary directory, never live saves.
"""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import struct

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE.parent/'tools'))
import chapter_packages as packages


def run(args, cwd, log=None):
    p = subprocess.run(list(map(str, args)), cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if log:
        Path(log).write_text(p.stdout)
    if p.returncode:
        raise RuntimeError(f"Command failed ({p.returncode}): {' '.join(map(str,args))}\n{p.stdout}")
    return p.stdout


def synthetic(core, chapter):
    source = {}
    for name in core['variables']+chapter['variables']:
        raw = name.encode()*4
        if name=='ffma':
            import monster_formation_bank as mfa
            body=bytes(208)+b'\xff'*104;face=bytes(232)+b'\xff'*116
            raw=mfa.pack([body+face]*16,[f'Monster{i}'.encode()for i in range(255)])
        if name in ('ffmb','ffmc','ffmd'):
            import monster_body_bank as mbb
            h,wb=8,2                                  # one family, one blank picture a pose
            picture=bytes((h,wb,h-1,8))+bytes(4*h*wb)
            job=94+3*('ffmb','ffmc','ffmd').index(name)
            raw=mbb.pack([{'jobs':[job],
                           'anims':{a:([picture],[8])for a in range(mbb.ANIMS)}}])[0]
        if name=='ffru':
            # The packager validates the bank's layout (bar_assets.validate) and
            # its gate triples against the chapter's declared rumor events
            # (rumor_rules.validate_bank), so the fixture builds the smallest
            # bank that satisfies both rather than filling bytes.
            import struct as _s, rumor_rules
            gates = rumor_rules.gates(chapter.get('rumor_events', []))
            start = 8 + 14*len(gates)
            title, text, index = start, start+2, start+7
            body = b'T\0' + b'Text\0' + b'\0\0'          # one page, at offset 0
            total = start + len(body)
            rows = b''.join(_s.pack('>4B5H', identity, first, last, 1, 1, title, text, 0, index)
                            for identity, first, last in gates)
            raw = b'FRU1' + _s.pack('>H', total) + bytes((len(gates), 0)) + rows + body
        if name=='ffbr':
            # bar_assets.validate(art=True) fixes this one exactly: 6360 bytes,
            # the picture at12 and360 bytes a row.
            import struct as _s
            raw=b'FBR1'+_s.pack('>4H',6360,12,360,0);raw+=bytes(6360-len(raw))
        if name in ('ffpr','ffpe','ffpt'):
            # core_package admits the Propositions banks only through
            # service_assets.validate(), so the fixture builds the smallest trio
            # that satisfies its cold bounds rather than filling bytes.
            import struct as _s, proposition_rules as pr
            if name=='ffpr':
                raw=bytearray(b'FPR1\0\0'+bytes((pr.COUNT,1))+_s.pack('>10H',*([100]*10)))
                raw.extend(bytes(pr.COUNT*25));title=len(raw);raw.extend(b'Job\0')
                for i in range(pr.COUNT):
                    at=28+25*i;row=bytearray(23)
                    row[0]=row[1]=row[6]=row[7]=row[9]=row[12]=row[19]=1;row[2]=i+1
                    raw[at:at+23]=row;_s.pack_into('>H',raw,at+23,title)
                _s.pack_into('>H',raw,4,len(raw));raw=bytes(raw)
            elif name=='ffpe':
                tables=[]
                for k,size in enumerate(pr.REWARD_SIZES):
                    if k<4:tables.append(bytes(size))                       # scores, <=100
                    elif k<6:tables.append(_s.pack('>%dH'%(size//2),*([1]*(size//2))))
                    elif k<8:tables.append(bytes([1]*size))                 # paired, both set
                    else:tables.append(_s.pack('>%dH'%(size//2),*([100]*(size//2))))
                at=28;offsets=[]
                for t in tables:offsets.append(at);at+=len(t)
                raw=b'FPE1'+_s.pack('>H',at)+b'\x0a\x01'+_s.pack('>10H',*offsets)+b''.join(tables)
            else:
                head=bytearray(738);body=b'Text\0'+b'\0\0'   # one text, one page at offset0
                head[0:4]=b'FPT1';head[6:8]=b'\x60\x4d'
                for i in range(96):_s.pack_into('>HHBB',head,8+6*i,738,743,1,0)
                for i in range(96,173):_s.pack_into('>H',head,584+2*(i-96),738)
                _s.pack_into('>H',head,4,738+len(body));raw=bytes(head)+body
        if name=='ffsv':
            # The packager validates this one against the spec it was given
            # (chapter_packages.py: "Service event bindings mismatch"), so the
            # fixture has to derive it the same way instead of filling bytes.
            import service_assets
            raw=service_assets.bindings(chapter)
        if re.fullmatch(r'ff[a-z0-9]v[0-3]',name):
            import extract
            raw=extract.view_bank(b'R4',(name.encode()*1600)[:8000],b'',bytes(400))
        if name in ('ffgd', 'ffod'):
            words = [0]*53
            words[9], words[10], words[13], words[50] = 10, 10, 100, 11
            if name=='ffod':words[42]=1
            words[44] = 1 if name=='ffod' else 2
            words[52]=56 if name=='ffod' else 22
            words[17]=106
            raw = b'FCD5'+struct.pack('>51H', *words[2:])
            units=bytearray(28*11);units[11]=2
            raw+=units
        code = name.startswith('fft') and not name.endswith('core') and name != 'fftitle'
        source[name] = packages.Variable(name, 0x21 if code else 0x1c,
                                        0 if name == 'fftstory' else 3,
                                        raw+(b'\xf3' if code else b'\0dat\0\xf8'))
    return source


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'x/chapter-packages/check')
    ap.add_argument('--input', type=Path)
    ap.add_argument('--no-native', action='store_true')
    args = ap.parse_args()
    out = args.out.resolve();out.mkdir(parents=True, exist_ok=True)
    core = json.loads((HERE/'packages/core.json').read_text())
    chapter = json.loads((HERE/'packages/chapter-01.json').read_text())
    source = packages.read_group(args.input) if args.input else synthetic(core, chapter)
    checks = 0

    def verify(ok):
        nonlocal checks
        checks += 1
        if not ok:
            raise AssertionError(f'Package integration check {checks}')

    def rejected(action):
        try:
            action()
        except (ValueError, KeyError):
            verify(True)
        else:
            verify(False)

    report, shared, packs = packages.build(source, core, [chapter], out/'delivery')
    variables = shared+packs[0][1]
    # The running supervisor's own image differs from its file: never a checked dependency.
    verify(b'fftstory' not in next(v for v in shared if v.name == 'ffcore').payload)
    generated={'ffgu'} if 'persistent_guests' in chapter else set()
    verify(len({v.name for v in variables}) == len(set(source)|generated)+2)
    verify(not set(v.name for v in shared)&set(v.name for v in packs[0][1]))
    verify(set(source)|generated == set(v.name for v in variables)-{'ffcore', 'ffpack'})
    roundtrip = out/'roundtrip.89g';roundtrip.write_bytes(packages.group_bytes(variables))
    verify(packages.read_group(roundtrip) == {v.name:v for v in variables})
    before = roundtrip.read_bytes()
    for position in (0, 58, 70, len(before)-3):
        corrupt = bytearray(before);corrupt[position] ^= 255;roundtrip.write_bytes(corrupt)
        rejected(lambda: packages.read_group(roundtrip))
    roundtrip.write_bytes(before)
    again, _, _ = packages.build(source, core, [chapter], out/'again')
    verify(report == again)
    # Archive accounting: only archived variables occupy Flash, and each costs the
    # measured header. Unarchived ones are RAM. The two must partition the total.
    row = report['chapters'][0]
    narch = sum(1 for v in row['variables']+report['core']['variables'] if v['attr'])
    verify(row['installed_archive_bytes']+row['unarchived_ram_bytes']
           == row['installed_with_core_bytes']+packages.ARCHIVE_PER_VARIABLE*narch)
    # Cross-check against the hardware measurement of the shipped ABI99 release
    # (experiments/archfree in TiEmu: EM_survey inUse = 990,766 for core + Chapter I).
    release = HERE/'x/core-abi99-release/packages/packages.json'
    if release.exists():
        man = json.loads(release.read_text())
        archived = [v for v in man['chapters'][0]['variables']+man['core']['variables'] if v['attr']]
        model = sum(v['bytes']+2+packages.ARCHIVE_PER_VARIABLE for v in archived)
        verify(len(archived) == 62 and 0 <= model-990766 <= 100)
    for name in ('fft-core.89g', 'chapter-01.89g', 'packages.json'):
        verify((out/'delivery'/name).read_bytes() == (out/'again'/name).read_bytes())
    chapter_source={v.name:v for v in packs[0][1] if v.name!='ffpack'}
    reused=packages.reuse_core(chapter_source,out/'delivery/fft-core.89g',core)
    reused_report,_,_=packages.build(reused,core,[chapter],out/'reused')
    verify(reused_report==report)
    for name in ('fft-core.89g','chapter-01.89g'):
        verify((out/'delivery'/name).read_bytes()==(out/'reused'/name).read_bytes())
    rejected(lambda:packages.reuse_core({**chapter_source,'fftstory':source['ffgd']},out/'delivery/fft-core.89g',core))
    for mutate in (
        lambda s: s['variables'].append('ffsave1'),
        lambda s: s['variables'].append('ffmg'),
        lambda s: s['routes'][1].update(band=1),
        lambda s: s['routes'][1].update(prefix='o'),
        lambda s: s['routes'][1].update(prefix='../g'),
        lambda s: s['routes'][1].update(prefix='c'),
        lambda s: s['steps'][0].update(argument=8),
        lambda s: s['steps'][1].update(argument=300),
        lambda s: s['variables'].remove('ffgd'),
        lambda s: s['variables'].remove('ffca'),
        lambda s: s['variables'].remove('ffost'),
        lambda s: s['variables'].remove('ffwm'),
    ):
        bad = deepcopy(chapter);mutate(bad)
        rejected(lambda: packages.chapter_package(source, bad, core, bytes.fromhex(report['core_key'])))
    oversized = dict(source);v = source['ffgd'];raw = bytearray(v.payload);raw[100:102] = b'\0\x0e'
    oversized['ffgd'] = packages.Variable(v.name,v.kind,v.attr,bytes(raw))
    rejected(lambda: packages.build(oversized,core,[chapter],out/'bad-envelope'))
    rejected(lambda: packages.build(source, core, [chapter], out/'bad-budget', budget=1000, reserve=1000))
    rejected(lambda: packages.build(source, core, [chapter, chapter], out/'duplicate'))
    # A second authored-data fixture contains ONLY Gariland; it uses another
    # encounter band and can replace the first without changing any core bytes.
    later = deepcopy(chapter)
    later.update(id=2, title='QA continuation fixture, not authored Chapter II', requires_flag=1,
                 previous=1, next=65535, routes=[{'band':3, 'prefix':'g'}], world_map=True)
    later['variables'] = [n for n in chapter['variables'] if not n.startswith('ffo') and n != 'ffs0']
    later['steps'] = [{**s,'argument':300 if s['program']!='scene' else s['argument']} for s in chapter['steps'][2:]]
    second, shared2, packs2 = packages.build(source, core, [later], out/'continuation', allow_unused=True)
    verify(packages.group_bytes(shared) == packages.group_bytes(shared2))
    verify(second['core_key'] == report['core_key'])
    verify(packages.read_group(out/'continuation/chapter-02.89g')['ffpack'].raw[48:52] == b'\x03g\0\0')
    # Simulate a calculator VAT: delete only the previous content-owned names.
    # Three slots and selection metadata must survive both removal and install.
    vat = {v.name:v.payload for v in variables}
    saves = {f'ffsave{i}': bytes((i,))*3202 for i in (1,2,3)}
    saves.update(ffsave=b'legacy save',ffslot=b'FSL1\x02\0',ffcfg=b'user configuration')
    vat.update(saves)
    for v in packs[0][1]:
        del vat[v.name]
    verify(all(vat[v.name] == v.payload for v in shared))
    verify(all(vat[n] == b for n,b in saves.items()))
    vat.update({v.name:v.payload for v in packs2[0][1]})
    verify('ffov0' not in vat and 'ffgv0' in vat)
    verify(all(vat[n] == b for n,b in saves.items()))
    # Check the actual C parser/policy against the same serialized variables,
    # including every corrupted metadata byte and every persisted resume point.
    data = out/'vat';data.mkdir(exist_ok=True)
    for v in variables:
        (data/(v.name+'.payload')).write_bytes(v.payload)
        (data/(v.name+('.89z' if v.kind==0x21 else '.89y'))).write_bytes(packages.group_bytes([v],single=True))
    src = HERE/'test_chapter_packages.c'
    for label, extra in [('pc', []), ('san', ['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])]:
        run(['cc','-std=gnu99','-O1','-Wall','-Wextra',*extra,src,'-o',out/label],ROOT,out/(label+'-build.log'))
        text = run([out/label],data,out/(label+'.log'))
        match = re.search(r'packages ([0-9A-F]+) failures 0 checks (\d+)',text)
        verify(bool(match))
        if label == 'pc':
            expected = [int(match[1],16),0,int(match[2])]
        else:
            verify([int(match[1],16),0,int(match[2])] == expected)
    if not args.no_native:
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'pkcheck',src],ROOT,out/'native-build.log')
        command = [ROOT/'tools/bin/ti-cycles','--max',100000]
        for v in variables:
            command += ['--file',data/(v.name+('.89z' if v.kind==0x21 else '.89y'))]
        command += [out/'pkcheck.89z']
        text = run(command,ROOT,out/'native.log')
        values = [int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',text)]
        verify(values[-3:] == expected)
    result = dict(host_checks=checks, c_checks=expected[2], digest=f'{expected[0]:08X}',
                  native=not args.no_native, production_input=bool(args.input),
                  core_key=report['core_key'], saves_preserved=list(saves),
                  continuation_fixture_not_shipping=True)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f"packages {expected[0]:08X} failures 0 checks {expected[2]}")
    print(f"Packages: {checks} host checks, {expected[2]} C checks/{expected[0]:08X}; PC/sanitizers"+('/68000 agree' if not args.no_native else ' agree'))


if __name__ == '__main__':
    main()
