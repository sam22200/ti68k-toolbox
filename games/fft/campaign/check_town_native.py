#!/usr/bin/env python3
"""Bounded native checks for the changed save, supervisor and pinned compositor.

This does not run the emulator or write installed banks/groups. Playable flows
remain PC-first in check_town_complete.py.
"""
import argparse
import json
import os
from pathlib import Path
import re
import tempfile

import review_town_services as review
import chapter_packages as packages
from check_town_complete import bank_fixture, data_links, seed

HERE, ROOT = review.HERE, review.ROOT
OUT = HERE / 'x/town-complete/native-check'


def values(log):
    return [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]


def main():
    global OUT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--only',nargs='+',choices=['migration','progress','blit','atomic','render'])
    parser.add_argument('--out',type=Path,default=OUT)
    options=parser.parse_args()
    global_output=options.out.resolve()
    # Keep older proof trees intact when validating a later milestone.
    OUT=global_output
    OUT.mkdir(parents=True, exist_ok=True)
    result_path=OUT/'results.json'
    # A selected run reports only what it executed. Never turn old log files
    # into current native evidence without checking their runtime fingerprints.
    results={}
    common = [ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c']
    with tempfile.TemporaryDirectory(prefix='fft-services-native-') as temp:
        data = Path(temp); data_links(data); bank_fixture(data); seed(data)
        for name in ('ffmg','ffcp','ffcj','ffeg','ffma','ffid','ffwb','ffbr','ffpr','ffpe','ffpt','ffsv','ffsh'):
            var = packages.Variable(name, 0x1c, 3, (data/(name+'.bin')).read_bytes()+b'\0dat\0\xf8')
            (OUT/(name+'.89y')).write_bytes(packages.group_bytes([var], single=True))
        var = packages.Variable('ffsave',0x1c,0,(data/'ffsave.sav').read_bytes()+b'\0sav\0\xf8')
        (OUT/'ffsave.89y').write_bytes(packages.group_bytes([var],single=True))
        for name,source in [('migration','test_world_save.c'),('progress','test_save_progress.c'),
                            ('blit','check_window_blit.c'),('atomic','check_atomic_save.c'),('render','check_services_render.c')]:
            if options.only and name not in options.only:continue
            # Compile a private wrapper: no compiler intermediates beside user files.
            wrapper = OUT/(name+'.c'); wrapper.write_text('#include "'+str(HERE/source)+'"\n')
            exe = OUT/name
            if name != 'atomic':
                extras = common if name == 'render' else []
                review.run(['cc','-std=gnu99','-O1','-w','-o',str(exe)+'-pc',wrapper,*extras])
                if name != 'render':
                    log = review.run([str(exe)+'-pc'],data,os.environ.copy())
                    (OUT/(name+'-pc.log')).write_text(log)
            extras = [ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'] if name in ('render','atomic') else []
            log = review.run([ROOT/'tools/bin/ti-cc','-o',name,wrapper,*extras],OUT)
            (OUT/(name+'-build.log')).write_text(log)
            if name == 'render':
                files = [str(OUT/(n+'.89y')) for n in ('ffmg','ffcp','ffcj','ffeg','ffma','ffid','ffwb','ffbr','ffpr','ffpe','ffpt','ffsv','ffsh','ffsave')]
                args = [a for f in files for a in ('--file',f)]
                for scenario in (0,1):
                    pc = review.run([str(exe)+'-pc',str(scenario),OUT/('render-pc-'+str(scenario)+'.png')],data)
                    native = review.run([ROOT/'tools/bin/ti-cycles','--max','200','--frames','2','--arg',str(scenario),
                                        '--png',OUT/('render-ti-'+str(scenario)+'.png'),*args,exe.with_suffix('.89z')])
                    (OUT/('render-'+str(scenario)+'.log')).write_text(native)
                    match = re.search(r'services ([0-9A-F]+) valid(\d+) state([0-9A-F]+) Save(\d+)',pc)
                    assert values(native)[-3:]==[1,int(match[3],16),4684], (pc,native)
                    assert int(re.search(r'checksum ([0-9A-F]+)',native)[1],16)==int(match[1],16),(pc,native)
                    results['render'+str(scenario)] = dict(checksum=match[1],state=match[3],save_bytes=4684)
            else:
                log = review.run([ROOT/'tools/bin/ti-cycles','--max','40',exe.with_suffix('.89z')])
                (OUT/(name+'-native.log')).write_text(log)
                found = values(log)[-3:]
                if name in ('migration','progress'):
                    match = re.search(r'([0-9A-F]+) failures (\d+) checks (\d+)',(OUT/(name+'-pc.log')).read_text())
                    assert found==[int(match[1],16),0,int(match[3])], log
                elif name=='blit':
                    match = re.search(r'(\d+) checks failures(\d+) digest([0-9A-F]+)',(OUT/'blit-pc.log').read_text())
                    assert found==[int(match[1]),0,int(match[3],16)], log
                else: assert found==[14,0,4684], log
                results[name]=found
    result_path.write_text(json.dumps(results,indent=2)+'\n')
    print('Selected native save/supervisor/compositor/VAT/render checks passed:', results)


if __name__=='__main__':main()
