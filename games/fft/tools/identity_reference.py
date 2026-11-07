#!/usr/bin/env python3
"""Independent legacy portrait/name bytes, pinned to the reviewed pre-change bank hash."""
from pathlib import Path
import hashlib,struct,sys
import extract,interface,units,opening_assets,management_data
from common_portraits import PORTRAITS
from identity_assets import write_changed
EXPECTED='5a67bc8d4612e5a4a79154e0bf7129aa0fb98bc4b368ca42d6b72b0dac59c663'

def build(cue):
    disc,files=extract.disc_files(cue)
    def face(name):return bytes(interface.planes(management_data.portrait(disc.read(*files['BATTLE/'+name+'.SPR'])),32))
    faces=[face(n)for _,n,_ in units.CHARS]
    faces +=[face(opening_assets.CHARS[i][1])for i in(0,2,3,4,5,6,7,8)]
    for k in range(management_data.CAREERS+1):
        for g in('MW'if k<management_data.CAREERS else ' '):
            n=management_data.SHEETS[k]+'_'+g if k<management_data.CAREERS else 'RAMUZA'
            if 'BATTLE/'+n+'.SPR'not in files:n=n.replace('_W','_M')if g=='W'else n.replace('_M','_W')
            faces.append(face(n))
    faces +=[face(n)for _,n in PORTRAITS]
    small=disc.read(*files['EVENT/SMALL.OUT']);entries=small[0xec+struct.unpack_from('<I',small,0x70)[0]:].replace(b'\xff',b'\xfe').split(b'\xfe')
    names=[interface.decode(raw).encode('ascii')+b'\0'for first in(256,512)for raw in entries[first:first+255]]
    blob=b'FIR1'+b''.join(faces)+b''.join(names)
    assert len(faces)==61 and len(names)==510 and hashlib.sha256(blob).hexdigest()==EXPECTED
    return blob
if __name__=='__main__':
    dest=Path(sys.argv[2]);dest.parent.mkdir(parents=True,exist_ok=True);write_changed(dest,build(sys.argv[1]));print('Identity reference:61 approved portraits and510 names, pinned legacy-bank SHA256')
