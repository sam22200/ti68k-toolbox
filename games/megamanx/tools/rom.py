#!/usr/bin/env python3
"""Local LoROM level hierarchy, kept separate from observed collision rules."""
from reference import ROM

DATA=ROM.read_bytes()
def pc(a):return ((a&0x7f0000)>>1)|(a&0x7fff)
def word(p):return int.from_bytes(DATA[p:p+2],'little')
def pointer(a):return int.from_bytes(DATA[pc(a):pc(a)+3],'little')

def level(n=0):
    ptr={key:pc(pointer(base+n*3)) for key,base in
         [('layout',0x868d24),('scenes',0x868d93),('blocks',0x868e02),
          ('maps',0x868e71),('collision',0x868ee0)]}
    p=ptr['layout'];width,height,scenes=DATA[p:p+3];p+=3;layout=[]
    while DATA[p]!=255:
        control,value=DATA[p:p+2];p+=2
        layout.extend(value if control&128 else value+k for k in range(control&127))
    assert len(layout)==width*height
    cells=[]
    for y in range(height*16):
        for x in range(width*16):
            scene=layout[(y>>4)*width+(x>>4)]
            block=word(ptr['scenes']+scene*128+(((y&15)>>1)*8+((x&15)>>1))*2)
            ident=word(ptr['blocks']+block*8+((y&1)*2+(x&1))*2)
            cells.append(ident)
    return {'width':width,'height':height,'scenes':scenes,'layout':layout,
            'ptr':ptr,'cells':cells,'collision':[DATA[ptr['collision']+m] for m in cells]}
