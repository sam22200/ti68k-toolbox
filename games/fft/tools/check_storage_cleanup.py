#!/usr/bin/env python3
"""Compare compact formats and actual game screens to the pre-cleanup snapshot."""
from pathlib import Path
import json
import re
import struct
import subprocess
import tempfile
from PIL import Image

HERE=Path(__file__).resolve().parents[1]
OUT=HERE/'x/storage'
BEFORE=OUT/'before-cleanup'


def sections(header, bank):
    entries=[(n,int(v)) for n,v in re.findall(r'^#define UI_(\w+) (\d+)UL',header,re.M)]
    return {n:bank[start:end] for (n,start),end in zip(entries,[v for _,v in entries[1:]]+[len(bank)])}


def main():
    old=sections((BEFORE/'interface.h').read_text(),(BEFORE/'ffti.bin').read_bytes())
    new=sections((HERE/'interface.h').read_text(),(HERE/'ffti.bin').read_bytes())
    for name in ('ADV','SLOPES','TILES','BANNERS'):
        assert old[name]==new[name], name
    for i in range(6):
        assert new['PORTRAITS'][i*348:(i+1)*348]==old['PORTRAITS'][i*696:i*696+348]
    glyphs=bytearray()
    for char in range(95):
        for shift in range(16):
            glyphs+=struct.pack('>5I',*(v<<(24-shift) for v in new['FONT'][char*5:char*5+5]))
    assert bytes(glyphs)==old['FONT'] and 'MARKER' not in new
    for gender in ('MALE','FEMALE'):
        for i in range(255):
            offset=struct.unpack_from('>H',new[gender],2*i)[0]
            assert new[gender][offset:].split(b'\0',1)[0]==old[gender][12*i:12*i+12].split(b'\0',1)[0]
    old_weapons=(BEFORE/'fftw.bin').read_bytes(); new_weapons=(HERE/'fftw.bin').read_bytes()
    index=19968
    header=(HERE/'weapons.h').read_text()
    zoff=int(re.search(r'WEAPON_ZOFF (\d+)',header)[1])
    start=int(re.search(r'WEAPON_OFF (\d+)',header)[1])
    assert old_weapons[:index]==new_weapons[:index]
    from storage_audit import zx0pack
    with tempfile.TemporaryDirectory() as tmp:
        packed=Path(tmp)/'packed'; decoded=Path(tmp)/'decoded'
        for i in range(48):
            id=new_weapons[index+i]; a,b=struct.unpack_from('>2H',new_weapons,zoff+id*2)
            packed.write_bytes(new_weapons[start+a:start+b])
            subprocess.run([zx0pack.DZX0,'-f',str(packed),str(decoded)],check=True,stdout=subprocess.PIPE)
            assert old_weapons[index+i*432:index+(i+1)*432]==decoded.read_bytes()
    screens=json.loads((BEFORE/'screens.json').read_text())
    changed_portraits=0
    for key, expected in screens.items():
        case,frames=key.split(':')
        log=subprocess.check_output(['./fft_pc','--headless','--scenario',case,'--frames',frames],cwd=HERE,text=True)
        if re.search(r'checksum (\w+)',log)[1]!=expected:
            with tempfile.TemporaryDirectory() as tmp:
                pictures=[]
                for binary,cwd,label in [(BEFORE/'fft_pc',BEFORE,'before'),(HERE/'fft_pc',HERE,'now')]:
                    shot=Path(tmp)/(label+'.png')
                    subprocess.run([str(binary),'--headless','--scenario',case,'--frames',frames,
                                    '--scale','1','--shot',str(shot)],cwd=cwd,check=True,stdout=subprocess.PIPE)
                    image=Image.open(shot).convert('L')
                    assert image.width%160==0 and image.height%100==0
                    image=image.resize((160,100),Image.Resampling.NEAREST)
                    pictures.append(image)
                for y in range(100):
                    for x in range(160):
                        if not (x<19 and y>=71):
                            assert pictures[0].getpixel((x,y))==pictures[1].getpixel((x,y)),(key,x,y)
            changed_portraits+=1
    config=json.loads((HERE/'x/config/checked.json').read_text())
    manifest=set(config['native_banks'])
    assert not manifest.intersection({'fftat0','fftat1','fftat2','fftsurf','fftuv','fftshot'})
    assert {p.stem for p in (HERE/'x/config/data').glob('*.89y')}==manifest
    from storage_audit import decode_bench
    data=HERE/'x/config/data'; raw=(data/'fftlook.raw').read_bytes()
    packed=(data/'fftlook.bin').read_bytes()
    near,wide=struct.unpack_from('>2H',packed,8)
    (OUT/'look_near.zx0').write_bytes(packed[near:wide])
    (OUT/'look_wide.zx0').write_bytes(packed[wide:])
    decode=decode_bench({'look_near':raw[8:6008], 'look_wide':raw[6008:12008]})
    report=dict(before_after_screens=len(screens),glyph_shift_samples=95*16,
                screens_changed_only_within_left_portrait=changed_portraits,
                original_names=510,original_blade_frames=48,
                native_data_bytes=config['data_bytes'],
                removed_banks=['fftat0','fftat1','fftat2','fftsurf','fftuv','fftshot'],
                current_fixture_decode=decode)
    (OUT/'cleanup-checked.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__': main()
