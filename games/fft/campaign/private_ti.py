#!/usr/bin/env python3
"""Private Titanium milestone tools: scoped PID/display/profile, never live TiEmu.

Use start once after PC acceptance, then keys/log/heap/battle/var, and stop.
Every RAM write is restricted to this private process and explicit debug fields.
"""
import argparse
import contextlib
import importlib.machinery
import importlib.util
import json
import io
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
OUT=HERE/'x/chapter1-ready/ti'
NAME='tiemu-chapter1-ready'
sys.path.insert(0,str(HERE.parent/'tools'))
import chapter_packages as packages
import fftmem


def run(args,**kw):
    p=subprocess.run(list(map(str,args)),text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,**kw)
    if p.returncode:raise RuntimeError(p.stdout)
    return p.stdout.strip()


def module(name,path):
    loader=importlib.machinery.SourceFileLoader(name,str(path));spec=importlib.util.spec_from_loader(name,loader)
    m=importlib.util.module_from_spec(spec);loader.exec_module(m);return m


def start():
    OUT.mkdir(parents=True,exist_ok=True);profile=OUT/'home/.tiemu';profile.mkdir(parents=True,exist_ok=True)
    assert not run(['docker','ps','-q','--filter','name=^/'+NAME+'$']),'Private instance already running'
    state=ROOT/'tools/tiemu/89t/states/hw3/.tiemu/state.sav'
    shutil.copy2(state,profile/'state.sav')
    ini=(state.parent/'tiemu.ini').read_text(encoding='latin1')
    ini=re.sub(r'^sav_file=.*','sav_file=/tmp/fftqa/home/.tiemu/state.sav',ini,flags=re.M)
    ini=re.sub(r'^screen_folder=.*','screen_folder=/tmp/fftqa/',ini,flags=re.M)
    (profile/'tiemu.ini').write_text(ini,encoding='latin1')
    image=ROOT/'tools/tiemu/89t/.tiemu/images/TI89Titanium_OS.img';shutil.copy2(image,OUT/'os.img')
    auth=OUT/'xauth';auth.touch();shim=OUT/'bin';shim.mkdir(exist_ok=True)
    (shim/'xdotool').write_text('#!/bin/sh\nif [ "$1" = windowactivate ]; then shift; [ "$1" = --sync ] && shift; exec /usr/bin/xdotool windowfocus --sync "$@"; fi\nexec /usr/bin/xdotool "$@"\n');(shim/'xdotool').chmod(0o755)
    display=next(n for n in range(99,199) if not Path('/tmp/.X11-unix/X'+str(n)).exists())
    log=(OUT/'xvfb.log').open('wb');server=subprocess.Popen(['Xvfb',':'+str(display),'-screen','0','800x900x24','-ac'],stdout=log,stderr=log,start_new_session=True)
    for _ in range(100):
        if Path('/tmp/.X11-unix/X'+str(display)).exists():break
        assert server.poll() is None;time.sleep(.03)
    variables={}
    for name in ('fft-core.89g','chapter-01.89g'):variables.update(packages.read_group(OUT.parent/'packages'/name))
    tm=module('private_timem',ROOT/'tools/bin/ti-mem');tm.cmd_mkvar([str(OUT/'tilog.89y'),'512'])
    variables.update(packages.read_group(OUT/'tilog.89y'));(OUT/'boot.89g').write_bytes(packages.group_bytes(variables.values()))
    meta=dict(display=':'+str(display),xvfb_pid=server.pid,container=NAME,profile=str(profile),started=time.time())
    (OUT/'session.json').write_text(json.dumps(meta,indent=2)+'\n')
    try:
        run(['docker','run','-d','--rm','--name',NAME,'--network','host','-u',f'{os.getuid()}:{os.getgid()}',
             '-e','DISPLAY='+meta['display'],'-e','XAUTHORITY=/tmp/fftqa/xauth','-e','HOME=/tmp/fftqa/home',
             '-v',str(ROOT)+':'+str(ROOT)+':ro','-v',str(OUT)+':/tmp/fftqa',
             '-v',str(OUT/'os.img')+':'+str(image),'tiemu-jammy','tiemu','-send=/tmp/fftqa/boot.89g'])
        meta['pid']=int(run(['docker','inspect','-f','{{.State.Pid}}',NAME]));(OUT/'session.json').write_text(json.dumps(meta,indent=2)+'\n')
        print('Private Titanium started PID',meta['pid'],'display',meta['display'],flush=True)
        tm.tiemu_pid=lambda:meta['pid'];deadline=time.monotonic()+240;last=0
        while time.monotonic()<deadline:
            try:
                d=tm.ram()[2]
                if all(tm.landed(d,v.name.encode(),bool(v.attr&2),len(v.payload)) for v in variables.values()):
                    print('All',len(variables),'matching variables received',flush=True);return
            except (OSError,SystemExit):pass
            if time.monotonic()-last>15:print('Waiting for archived boot transfer...',flush=True);last=time.monotonic()
            time.sleep(.3)
        raise TimeoutError('Private boot transfer did not finish')
    except BaseException:
        print('Private evidence retained in',OUT,flush=True);raise


def session():
    meta=json.loads((OUT/'session.json').read_text());actual=int(run(['docker','inspect','-f','{{.State.Pid}}',NAME]))
    assert actual==meta['pid'] and meta['container']==NAME
    tm=module('private_timem',ROOT/'tools/bin/ti-mem');tm.tiemu_pid=lambda:actual
    return meta,tm


def read_address(tm,address,size):
    mem,base=tm.locate(tm.tiemu_pid())
    if address<0x80000:
        mem.seek(base+(address&0x3ffff));return mem.read(size)
    for row in Path(f'/proc/{tm.tiemu_pid()}/maps').read_text().splitlines():
        p=row.split();lo,hi=[int(v,16) for v in p[0].split('-')]
        if not p[1].startswith('rw') or len(p)>5 or hi-lo<0x400000:continue
        for at in range(lo,hi-0x400000,4096):
            mem.seek(at);b=mem.read(24)
            if len(b)<24:continue
            n=struct.unpack_from('<Q',b,8)[0]&~15;ssp,reset=struct.unpack_from('>II',b,16)
            if ssp==0x4c00 and 0x400000<=n<0x402000:
                origin=0x800000 if reset>=0x800000 else 0x200000
                assert origin<=address<origin+0x400000;mem.seek(at+16+address-origin);return mem.read(size)
    raise RuntimeError('Private ROM mapping missing')


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('command',choices=['start','stop','keys','log','heap','battle','set','var','dump'])
    ap.add_argument('args',nargs='*');a=ap.parse_args()
    if a.command=='start':start();return
    meta,tm=session()
    if a.command=='stop':
        run(['docker','stop','-t','2',NAME]);os.kill(meta['xvfb_pid'],15);print('Only the private instance stopped');return
    if a.command=='keys':
        env={**os.environ,'DISPLAY':meta['display'],'XAUTHORITY':str(OUT/'xauth'),'PATH':str(OUT/'bin')+':'+os.environ['PATH']}
        with (OUT/'keys.log').open('a') as f:f.write(str(time.time())+' '+' '.join(a.args)+'\n')
        print(run([ROOT/'tools/bin/ti-key','--hold','0.3',*a.args],env=env));return
    if a.command in ('log','heap'):
        stream=io.StringIO()
        with contextlib.redirect_stdout(stream):
            if a.command=='log':tm.cmd_log(a.args)
            else:tm.cmd_heap(a.args)
        text=stream.getvalue()
        with (OUT/(a.command+'-transcript.log')).open('a') as f:f.write('\n'+str(time.time())+'\n'+text)
        print(text,end='');return
    if a.command=='dump':(OUT/(a.args[0] if a.args else 'ram.bin')).write_bytes(tm.ram()[2]);return
    if a.command in ('battle','set'):
        fftmem.CACHE=OUT/'state_layout.json';b=fftmem.Battle();b.tm.tiemu_pid=lambda:meta['pid']
        if a.command=='battle':
            text=json.dumps(b.read(),indent=2);(OUT/'battle.json').write_text(text+'\n');print(text);return
        allowed={'dbg','mode','ai','banner','config','config_row','want','turn'}
        field,value=a.args[0],int(a.args[1]);assert field in allowed and 0<=value<=255
        mem,base=tm.locate(meta['pid']);mem.seek(base);at=b.find(mem.read(tm.RAM));assert at is not None
        key='State__'+field
        if key not in b.lay:
            # Ask the real native translation unit for extra debug-field offsets.
            src=OUT/'debug_layout.c';src.write_text('#include "'+str(ROOT/'games/fft/tools/state_layout.c')+'"\n'+''.join('F(State,'+n+')\n' for n in ('config','config_row','want','turn')))
            sdk=ROOT/'tools/gcc4ti-bin';env={'PATH':str(sdk/'bin')+':/usr/bin:/bin','TIGCC':str(sdk)}
            run(['tigcc','-S','-Os','-mno-bss','-mregparm=5','-I'+str(ROOT/'tools/extgraph/lib'),'-o',OUT/'debug_layout.s',src],env=env)
            text=(OUT/'debug_layout.s').read_text();b.lay.update({k:int(v) for k,v in re.findall(r'^layout_(\w+):\s*\n\s*\.word\s+(\d+)',text,re.M)})
        mem.seek(base+at+b.lay[key]);mem.write(bytes((value,)));print('Private debug',field,value);return
    d=tm.ram()[2];name=a.args[0];table=struct.unpack_from('>I',d,tm.HANDLE_TABLE)[0]
    for at in range(0,len(d)-14,2):
        if d[at:at+8]!=name.encode().ljust(8,b'\0'):continue
        flags,handle=struct.unpack_from('>HH',d,at+10)
        if not 0<handle<4000 or table+4*handle+4>len(d):continue
        address=struct.unpack_from('>I',d,table+4*handle)[0]
        if not address:continue
        raw=read_address(tm,address,2);n=struct.unpack('>H',raw)[0]
        if n<6 or n>65520:continue
        payload=read_address(tm,address+2,n)
        if payload[-1]!=248:continue
        dest=OUT/(name+('-'+a.args[1] if len(a.args)>1 else '')+'.payload');dest.write_bytes(payload);print(name,'flags',hex(flags),'address',hex(address),'bytes',n,'prefix',payload[:8].hex());return
    raise RuntimeError('Variable not found: '+name)


if __name__=='__main__':main()
