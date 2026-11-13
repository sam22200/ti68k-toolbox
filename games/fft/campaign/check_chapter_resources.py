#!/usr/bin/env python3
"""Verify shipping normalization, sharing, pixels and cold reference bounds.

Inputs/old proofs are read-only. Every save/shot/checker is task-local. The full
combat-family inventory is deliberately outside this asset-format check.
"""
import argparse
from copy import deepcopy
from dataclasses import replace
import json
from pathlib import Path
import re
import struct
import sys
import tempfile

from check_chapter_packages import run,HERE,ROOT
sys.path.insert(0,str(HERE.parent/'tools'))
import chapter_packages as packages
import chapter_resources as res
import chapter_desc


def installed(variables,data):
    data.mkdir(parents=True,exist_ok=True)
    for v in variables:
        if v.kind==0x1c and v.payload.endswith(res.TRAILER):(data/(v.name+'.bin')).write_bytes(v.raw)
        (data/(v.name+('.89z' if v.kind==0x21 else '.89y'))).write_bytes(packages.group_bytes([v],single=True))


def expanded_fixture(source,spec):
    """A different encounter on the same map, and different text on one backdrop."""
    source=dict(source);spec=deepcopy(spec)
    spec['routes'].append({'band':3,'prefix':'z'})
    for suffix in ('v0','v1','v2','v3','a','i','d'):
        name='ffz'+suffix;source[name]=replace(source['ffg'+suffix],name=name);spec['variables'].append(name)
    mapping,steps=res.scene_parts(source,'ffs1',True)
    steps=deepcopy(steps);kind,arg,values=steps[-1]
    # Same lengths, same placements, deliberately different final dialogue.
    text=bytearray(values[2]);text[-2]=ord('!') if text[-2]!=ord('!') else ord('?')
    values[2]=bytes(text)
    source['ffs3']=res.changed(replace(source['ffs1'],name='ffs3'),res.scene_bytes(mapping,steps))
    spec['variables'].append('ffs3');spec['steps'].append({'program':'scene','argument':3})
    return source,spec


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input',type=Path,default=HERE/'x/packages/source.89g')
    ap.add_argument('--out',type=Path,default=HERE/'x/chapter-resources')
    ap.add_argument('--no-native',action='store_true')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=packages.read_group(args.input)
    core=json.loads((HERE/'packages/core.json').read_text());spec=json.loads((HERE/'packages/chapter-01.json').read_text())
    report,shared,packs=packages.build(source,core,[spec],out/'delivery')
    result={v.name:v for v in shared+packs[0][1] if v.name not in ('ffcore','ffpack')}
    checks=0
    def check(ok):
        nonlocal checks
        checks+=1
        if not ok:raise AssertionError(f'Resource check {checks}')
    def rejects(call):
        try:call()
        except (ValueError,KeyError):check(True)
        else:check(False)
    # Exact source/normalized semantics. Only archived offsets are rebased.
    for route in spec['routes']:
        prefix=route['prefix'];dn='ff'+prefix+'d';inn='ff'+prefix+'i'
        before=source[dn].raw;after=res.resolve(result,dn);a=source[inn].raw;b=res.resolve(result,inn)
        field={n:4+2*i for i,n in enumerate(chapter_desc.FIELDS)}
        get=lambda raw,n:res.word(raw,field[n])
        mutable={'ABILITIES','SKILL_SETS','TILES','UNIT_RAW','UNIT_NAMES'}
        for f in field:
            if f not in mutable:check(get(before,f)==get(after,f))
        tiles=get(before,'MAP_W')*get(before,'MAP_H')*12
        check(a[get(before,'TILES'):get(before,'TILES')+tiles]==b[:tiles])
        head=4+2*len(chapter_desc.FIELDS)
        check(before[head:]==after[head:]) # units, water, collision columns: byte-identical
        check(get(after,'ABILITIES')==get(after,'SKILL_SETS')==0)
        count=get(before,'UNIT_COUNT')
        if get(before,'UNIT_RAW'):
            aa,bb=get(before,'UNIT_RAW'),get(after,'UNIT_RAW');check(a[aa:aa+20*count]==b[bb:bb+20*count])
        if get(before,'UNIT_NAMES'):
            for i in range(count):
                aa=res.word(a,get(before,'UNIT_NAMES')+2*i);bb=res.word(b,get(after,'UNIT_NAMES')+2*i)
                check(a[aa:a.index(0,aa)]==b[bb:b.index(0,bb)])
        for k in range(4):
            n='ff'+prefix+'v'+str(k);raw,tail=res.contour_tail(source[n].raw)
            check(raw==res.resolve(result,n));check(res.contour_tail(res.resolve(result,n))[1] is None)
            if tail is not None:check(tail==res.contour_tail(result['ffcu'].raw)[1])
        check(source['ff'+prefix+'a'].raw==res.resolve(result,'ff'+prefix+'a'))
    for n in ('ffca','ffcb','ffcp','ffcw','ffcj','ffwm','ffflow'):
        check(source[n]==result[n])
    for n in ('ffs0','ffs1','ffs2'):
        check(res.scene_parts(source,n,True)==res.scene_parts(result,n,True))
    again,_,_=packages.build(result,core,[spec],out/'again');check(report==again)
    check(sum(len(v.payload) for v in result.values())<sum(len(v.payload) for v in source.values()))
    # Byte-identical maps/poses/descriptors alias existing files. All scenes pool
    # pictures/titles/beat blocks across the chapter, not just within one file.
    expanded,extended=expanded_fixture(source,spec)
    ext_report,ext_core,ext_packs=packages.build(expanded,core,[extended],out/'expanded')
    extension={v.name:v for v in ext_core+ext_packs[0][1] if v.name not in ('ffcore','ffpack')}
    for suffix in ('v0','v1','v2','v3','a','i','d'):
        name='ffz'+suffix;check(extension[name].raw[:4]==b'FRF1')
        check(res.resolve(extension,name)==res.resolve(extension,'ffg'+suffix))
    check('ffsp' in extension)
    for n in ('ffs0','ffs1','ffs2','ffs3'):
        check(extension[n].raw[:4]==b'FSC2')
        check(res.scene_parts(expanded,n,True)==res.scene_parts(extension,n,True))
    ext_again,_,_=packages.build(extension,core,[extended],out/'expanded-again');check(ext_again==ext_report)
    alias=extension['ffzv0']
    malformed=dict(extension);malformed['ffgv0']=res.reference(extension['ffgv0'],'ffzv0',0,2)
    rejects(lambda:res.resolve(malformed,'ffzv0'))
    rejects(lambda:res.resolve(extension,'ffzv0',{'ffzv0'}))
    for off,size in ((1,2),(65534,2),(0,0),(0,65535)):
        raw=bytearray(alias.raw);struct.pack_into('>HH',raw,12,off,size)
        malformed={**extension,'ffzv0':res.changed(alias,raw)}
        rejects(lambda:res.resolve(malformed,'ffzv0'))
    # Pooling spans multiple archived variables once the chapter's pictures
    # exceed64K. These opaque stand-in streams exercise layout, not rendering.
    large={};template=source['ffs1'];beat=b'\0\1\0\4\7\0\0\0\0hello\0'
    for i in range(6):
        name='ffs'+str(i);picture=bytes((65+i%3,))*30000
        large[name]=res.changed(replace(template,name=name),res.scene_bytes(bytes(6),[(2,0,[picture,20,beat])]))
    original_large=dict(large);pools=res.pool_scenes(large,sorted(large))
    check(len(pools)==2);check(all(len(large[n].raw)<=65512 for n in pools))
    for n in original_large:check(res.scene_parts(original_large,n,False)==res.scene_parts(large,n,False))
    bad=replace(source['ffwm'],name='unpooled')
    rejects(lambda:res.audit([source['ffwm'],bad],source))
    # Pure C parser on host, sanitizers and actual big-endian 68000.
    src=HERE/'test_resource_file.c'
    c_result=None
    for label,flags in [('pc',[]),('san',['-fsanitize=address,undefined','-fno-pie','-no-pie'])]:
        run(['cc','-std=gnu99','-O1','-g','-Wall','-Wextra',*flags,'-o',out/label,src],ROOT,out/(label+'-build.log'))
        text=run([out/label],ROOT,out/(label+'.log'))
        m=re.search(r'resources (\w+) failures 0 checks (\d+)',text);check(bool(m))
        if c_result is None:c_result=m.groups()
        else:check(m.groups()==c_result)
    if not args.no_native:
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'rescheck',src],ROOT,out/'native-build.log')
        log=run([ROOT/'tools/bin/ti-cycles','--max',1000,out/'rescheck.89z'],ROOT,out/'native.log')
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
        check(values[-3:]==[int(c_result[0],16),0,int(c_result[1])])
    # Scene A/B: same renderer, full extraction banks vs actual installed banks.
    rt=ROOT/'runtime/core/rt_core.c';src=HERE/'check_native.c'
    for label,flags in [('scene',[]),('scene-san',['-fsanitize=address,undefined','-fno-pie','-no-pie'])]:
        run(['cc','-std=gnu99','-O1','-g',*flags,'-DCHECK_SCENE','-o',out/label,src,rt,ROOT/'runtime/platform-sw/rt_sw.c'],ROOT,out/(label+'-build.log'))
    if not args.no_native:
        run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-DCHECK_SCENE','-o',out/'resscene',src,rt,
             ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'],ROOT,out/'scene-native-build.log')
    cases=[('orbonne-title',0,30),('chapel',0,180),('chapel-dialogue',0,330),
           ('academy-narration',1,30),('academy-title',1,120),('academy-dialogue',1,260),
           ('academy-end',1,1000),('gariland-title',2,60),('pooled-dialogue',3,260)]
    scenes=[]
    with tempfile.TemporaryDirectory(prefix='fft-resource-pixels-') as tmp:
        tmp=Path(tmp);control=tmp/'control';installed(expanded.values(),control)
        optimized=tmp/'optimized';installed(extension.values(),optimized)
        keys=tmp/'advance.keys';keys.write_text(''.join(f'{f} ENTER\n{f+1}\n' for f in range(30,1001,15)))
        for label,scenario,frames in cases:
            before=run([out/'scene',scenario,frames,keys,tmp/'before.png'],control)
            after=run([out/'scene',scenario,frames,keys,tmp/'after.png'],optimized)
            check(before==after);check((tmp/'before.png').read_bytes()==(tmp/'after.png').read_bytes())
            san=run([out/'scene-san',scenario,frames,keys,tmp/'san.png'],optimized)
            check(after==san)
            m=re.search(r'executed (\d+) checksum (\w+) state (\w+)',after);check(bool(m))
            if not args.no_native:
                command=[ROOT/'tools/bin/ti-cycles','--frames',frames,'--arg',scenario,'--keys',keys,'--max',10000]
                for v in extension.values():command+=['--file',optimized/(v.name+('.89z' if v.kind==0x21 else '.89y'))]
                log=run([*command,out/'resscene.89z'],ROOT,out/(label+'-native.log'))
                values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
                check(values[-3:]==[int(m[1]),0xACAD0001,int(m[3],16)])
                check(re.search(r'checksum\s+(\w+)',log)[1].lower()==m[2].lower())
            scenes.append(dict(case=label,frames=int(m[1]),screen=m[2],state=m[3]))
            print(label+': '+after.strip(),flush=True)
    installed(result.values(),out/'installed')
    info=dict(host_checks=checks,c_checks=int(c_result[1]),c_digest=c_result[0],native=not args.no_native,
              scene_replays=scenes,shipping_bytes=report['core']['bytes']+report['chapters'][0]['bytes'],
              shared_arena_and_scene_fixture=True,expanded_core=ext_report['core']['sha256'])
    (out/'results.json').write_text(json.dumps(info,indent=2)+'\n')
    print(f'Resources: {checks} host checks, {c_result[1]} C checks, {len(scenes)} identical scene replays'+
          ('; PC/sanitizers/68000 agree' if not args.no_native else '; PC/sanitizers agree'))


if __name__=='__main__':main()
