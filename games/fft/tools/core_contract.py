#!/usr/bin/env python3
"""Freeze/check the exact installed core interface and native layout offline.

No disc extraction or emulator. A chapter reuses this core byte for byte;
unreviewed source/layout drift requires a new explicitly frozen core release.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib

import chapter_packages as packages

FFT=Path(__file__).resolve().parents[1];ROOT=FFT.parents[1]
PUBLIC=['fft.h','module.h','core_limits.h','chapters.h','view_format.h','common.h','chapter_validate.h',
        'scene_buffer.h',
        'campaign/save_types.h','campaign/world_types.h','campaign/proposition_types.h',
        'actors.h','campaign/pack_format.h','campaign/pack_story.h','campaign/packages/core.json']


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()


def capture(folder,layout):
    manifest=json.loads((folder/'packages.json').read_text());core=packages.read_group(folder/manifest['core']['file'])
    group=folder/manifest['core']['file'];assert sha(group)==manifest['core']['sha256']
    spec=json.loads((FFT/'campaign/packages/core.json').read_text())
    source={n:v for n,v in core.items() if n!='ffcore'}
    _,key=packages.core_package(source,spec);assert key.hex()==manifest['core_key']
    records=[];tokens=[];signatures=[]
    for name in ('ffbm','ffbn','ffbo','ffbp'):
        b=core[name].raw;assert b[:4]==b'FMB3';first,count,total,token=struct.unpack_from('>3HI',b,4)
        assert first==len(records) and total==29;tokens.append(token)
        offsets=list(struct.unpack_from('>'+str(count)+'H',b,14))+[len(b)]
        for begin,end in zip(offsets,offsets[1:]):
            m=b[begin:end];assert m[:4]==b'FMD2';entry,size,context=struct.unpack_from('>3H',m,4)
            assert entry<size and context+layout['ModCtx']<=size
            signatures.append(struct.unpack_from('>H',m,10+context)[0]);records.append(m)
    assert len(set(tokens))==1 and tokens[0]==zlib.crc32(b''.join(records))
    assert set(signatures)=={layout['MOD_SIG']}
    return dict(schema='FFT_CORE_CONTRACT_1',core_key=key.hex(),core_sha256=sha(group),native_layout=layout,
        envelope={**spec['envelope'],'width_pixels':352,'depth_bytes':layout['DEPTH_MAX'],'actor_frames':layout['AC_N'],'actor_banks':layout['ACTOR_BANKS'],
                  'module_zone_bytes':max(struct.unpack_from('>H',m,6)[0] for i,m in enumerate(records) if i!=27),
                  'companion_bytes':struct.unpack_from('>H',records[27],6)[0]},
        formats={n:v for n,v in spec.items() if n.endswith('_format')},
        shared_variables={n:dict(kind=v.kind,bytes=len(v.payload),sha256=hashlib.sha256(v.payload).hexdigest()) for n,v in core.items()},
        public_header_sha256={n:sha(FFT/n) for n in PUBLIC},
        chapter_policy=dict(data_only=True,matching_core_key=True,no_save_variables=True,
            service_events_explicit=True,resources_pooled=True,palettes_one_per_physical_model=True),
        caveat='Core interface freeze is not certification of unripped maps, all Chapter I content, installed16KiB RAM or real hardware.')


def native_layout(out):
    sdk=ROOT/'tools/gcc4ti-bin';source=out/'contract_layout.c';asm=out/'contract_layout.s'
    source.write_text('#include "'+str(FFT/'tools/state_layout.c')+'"\n'+
        'S(Work,sizeof(Work)) S(ModCtx,sizeof(ModCtx)) S(MOD_SIG,MOD_SIG) S(MOD_ABI,MOD_ABI) S(AC_N,AC_N) S(ACTOR_BANKS,ACTOR_BANKS) S(DEPTH_MAX,FFT_CORE_DEPTH_MAX)\n'+
        ''.join('S(Save__'+f+',offsetof(Save,'+f+'))\n' for f in ('flags','unit','money','flow','deploy','world','world_rng','profiles','missions','poach','generic_identity')))
    p=subprocess.run([str(sdk/'bin/tigcc'),'-S','-Os','-mno-bss','-mregparm=5','-I'+str(ROOT/'tools/extgraph/lib'),'-o',str(asm),str(source)],
        env={**os.environ,'TIGCC':str(sdk)},text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (out/'layout-build.log').write_text(p.stdout);p.check_returncode();s=asm.read_text()
    result={k:int(v)&65535 for k,v in re.findall(r'^layout_(\w+):\s*\n\s*\.word\s+(-?\d+)',s,re.M)}
    result.update({k:0 for k in re.findall(r'^layout_(\w+):\s*\n\s*\.skip\s+2',s,re.M)})
    assert result['Save']==4684 and result['Save__missions']==4024 and result['MOD_ABI']==99
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--packages',type=Path,required=True)
    ap.add_argument('--contract',type=Path,default=FFT/'campaign/packages/core-contract.json')
    ap.add_argument('--out',type=Path,default=FFT/'campaign/x/core-contract-check')
    ap.add_argument('--freeze',action='store_true',help='explicitly replace the reviewed core contract')
    args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    actual=capture(args.packages.resolve(),native_layout(args.out.resolve()))
    if args.freeze:args.contract.write_text(json.dumps(actual,indent=2,sort_keys=True)+'\n')
    else:
        expected=json.loads(args.contract.read_text())
        if expected!=actual:raise SystemExit('Core contract drift: rebuild/review a matching core before freezing a new contract')
    (args.out/'verification.json').write_text(json.dumps(dict(core_key=actual['core_key'],abi=actual['native_layout']['MOD_ABI'],
        save_bytes=actual['native_layout']['Save'],matching_images=29,verified=True),indent=2)+'\n')
    print('Core contract verified:',actual['core_key'],'ABI',actual['native_layout']['MOD_ABI'],'Save',actual['native_layout']['Save'])


if __name__=='__main__':main()
