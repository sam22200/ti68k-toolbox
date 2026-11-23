#!/usr/bin/env python3
"""Playable Propositions and Fur services, actual campaign saves, PC first."""
import hashlib
import json
import os
from pathlib import Path
import struct
import tempfile

import review_town_services as review
from check_flow import data_links
import service_assets

HERE,ROOT=review.HERE,review.ROOT
OUT=HERE/'x/town-complete'


def bank_fixture(data):
    (data/'ffsv.bin').unlink()
    (data/'ffsv.bin').write_bytes(service_assets.bindings(dict(service_events=[dict(event=117,flag=33),dict(event=203,flag=34)])))
    source=bytearray((HERE/'data/ffsh.bin').read_bytes());types=source[5]+1
    source[6+512+256+256+6*types+43+5]=33
    (data/'ffsh.bin').unlink();(data/'ffsh.bin').write_bytes(source)


def seed(data,fur=False):
    raw=bytearray((HERE/'x/check/ffsave_won.sav').read_bytes())
    if raw[:4]==b'FSV7' and len(raw)==3992:raw[:4]=b'FSV8';raw.extend(bytes(32))
    assert len(raw)==4024 and raw[:4]==b'FSV8'
    raw[8]|=3
    if fur:raw[37]=10
    (data/'ffsave.sav').write_bytes(raw);(data/'ffsave1.sav').write_bytes(raw)
    return bytes(raw)


def main():
    OUT.mkdir(parents=True,exist_ok=True);common=[ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
    results={};screens=None
    for sanitize in (False,True):
        label='sanitizers' if sanitize else 'pc';out=OUT/label;out.mkdir(exist_ok=True)
        flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'] if sanitize else []
        programs={}
        for name,source in [('props','test_town_propositions.c'),('world','test_town_world.c'),('deploy','test_service_deploy.c'),('poach','test_service_poach.c')]:
            exe=out/name
            (out/(name+'-build.log')).write_text(review.run(['cc','-std=gnu99','-O1','-g','-w',*flags,'-o',exe,HERE/source,*common]))
            programs[name]=exe
        logs=[]
        with tempfile.TemporaryDirectory(prefix='fft-live-services-') as temp:
            data=Path(temp);data_links(data);bank_fixture(data);original=seed(data)
            for name,args in [('props',['dispatch',out]),('deploy',[]),('world',['travel',out]),('props',['collect',out]),('props',['resume',out])]:
                logs.append(review.run([programs[name],*args],data,os.environ.copy()))
            saved=(data/'ffsave.sav').read_bytes();assert len(saved)==4684 and saved[:4]==b'FSV9'
            assert (data/'ffsave1.sav').read_bytes()==original
            # Recall uses fresh state: ordinary failed returns may have a retry cooldown.
            original=seed(data)
            for name,args in [('props',['dispatch',out]),('props',['recall',out]),('world',['recall-day',out]),('props',['collect',out])]:
                logs.append(review.run([programs[name],*args],data,os.environ.copy()))
            assert (data/'ffsave1.sav').read_bytes()==original
            original=seed(data,fur=True);logs.append(review.run([programs['world'],'fur',out],data,os.environ.copy()))
            assert (data/'ffsave1.sav').read_bytes()==original
            original=seed(data);logs.append(review.run([programs['poach']],data,os.environ.copy()))
            assert (data/'ffsave1.sav').read_bytes()==original
            assert not (data/'ffsave.tmp').exists()
        hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.png')}
        if screens is None:screens=hashes
        else:assert screens==hashes
        (out/'flows.log').write_text(''.join(logs));results[label]=len(logs)
    (OUT/'playable-results.json').write_text(json.dumps(dict(flows=results,pc_sanitizers_equal=True,
        screens=screens,save_bytes=4684,legacy_migrated='FSV8 actual session',slots_unchanged=True,
        scope='Actual management/world/deployment input, dispatch/day/recall/report/Fur transaction save flows'),indent=2)+'\n')
    print('Playable town services:',results,'screens',len(screens),'Save4684; slots unchanged')


if __name__=='__main__':main()
