#!/usr/bin/env python3
"""Extract the opening Highway collision band from the local ROM."""
import struct
from rom import level
from reference import GAME,OUT

WIDTH,TOP,HEIGHT=1024,256,256
def main():
    (GAME/'generated').mkdir(exist_ok=True)
    v=level();cols=WIDTH>>4;rows=HEIGHT>>4;stride=v['width']<<4
    collision=bytes(v['collision'][(y+(TOP>>4))*stride+x] for y in range(rows) for x in range(cols))
    header=b'MXM1'+struct.pack('>6H',WIDTH,TOP,HEIGHT,cols,rows,16+len(collision))
    bank=header+collision
    (GAME/'mmxmap.bin').write_bytes(bank);(GAME/'mmxmap.be.bin').write_bytes(bank)
    (GAME/'generated/terrain.json').write_text(__import__('json').dumps({'width':WIDTH,'top':TOP,'height':HEIGHT,'rom_level':0,'source_layout':[v['width'],v['height']],'classes':sorted(set(collision))},indent=2)+'\n')
    print('Highway band:',WIDTH,'original pixels; first gap800..831, climb wall832; bank',len(bank))
if __name__=='__main__':main()
