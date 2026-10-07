"""Offline planar PPU decoding shared by ROM-art pipelines.

Consumes snesrun's four PPU buffers. Scroll/HDMA/color math are intentionally
left to each game's measured extraction; register mirrors are not latches.
"""
from functools import lru_cache
import struct
import numpy as np

def palette(raw):
    words=np.frombuffer(raw,dtype='<u2').astype(np.uint32)
    return np.array([((words>>n)&31)*255//31 for n in (0,5,10)],dtype=np.uint8).T

def composite(canvas,rgba,x,y):
    h,w=rgba.shape[:2]
    x0,y0,x1,y1=max(0,x),max(0,y),min(canvas.shape[1],x+w),min(canvas.shape[0],y+h)
    if x1<=x0 or y1<=y0:return
    src=rgba[y0-y:y1-y,x0-x:x1-x];dst=canvas[y0:y1,x0:x1]
    opaque=src[:,:,3]!=0;dst[opaque]=src[opaque]

class PPU:
    def __init__(self,vram,cgram,oam,regs):
        self.vram,self.colors,self.oam,self.regs=vram,palette(cgram),oam,regs
    @lru_cache(maxsize=None)
    def tile(self,address,bpp=4):
        pixels=np.zeros((8,8),dtype=np.uint8)
        for y in range(8):
            for plane in range(bpp):
                bits=self.vram[(address+(plane//2)*16+y*2+(plane&1))&65535]
                for x in range(8):pixels[y,x]|=((bits>>(7-x))&1)<<plane
        return pixels
    def bg_tile(self,word,layer=0):
        if self.regs[5]&7!=1:raise ValueError('background decoder currently supports Mode1 only')
        bpp=2 if layer==2 else 4
        base=((self.regs[11+layer//2]>>(4*(layer&1)))&15)<<13
        pixels=self.tile((base+(word&1023)*bpp*8)&65535,bpp)
        if word&0x4000:pixels=pixels[:,::-1]
        if word&0x8000:pixels=pixels[::-1]
        colors=self.colors[pixels+((word>>10)&7)*(1<<bpp)]
        return np.dstack((colors,(pixels!=0).astype(np.uint8)*255))
    def background(self,layer):
        setting=self.regs[7+layer];base=(setting&252)<<9
        width,height=(64 if setting&1 else 32),(64 if setting&2 else 32)
        size=16 if self.regs[5]&(16<<layer) else 8
        result=np.zeros((height*size,width*size,4),dtype=np.uint8)
        for y in range(height):
            for x in range(width):
                block=(y//32)*(width//32)+x//32
                word=struct.unpack_from('<H',self.vram,(base+block*2048+((y&31)*32+(x&31))*2)&65535)[0]
                tile=self.bg_tile(word,layer)
                if size==16:
                    tile=np.zeros((16,16,4),dtype=np.uint8)
                    for dy in range(2):
                        for dx in range(2):
                            sub=(word&0x3c00)|(((word&1023)+dx+16*dy)&1023)
                            tile[dy*8:dy*8+8,dx*8:dx*8+8]=self.bg_tile(sub,layer)
                    if word&0x4000:tile=tile[:,::-1]
                    if word&0x8000:tile=tile[::-1]
                result[y*size:y*size+size,x*size:x*size+size]=tile
        return result
    def objects(self):
        base=(self.regs[1]&7)<<14
        sizes=((8,16),(8,32),(8,64),(16,32),(16,64),(32,64),(16,32),(16,32))
        for n in range(127,-1,-1):
            x,y,tile,flags=self.oam[n*4:n*4+4]
            high=(self.oam[512+n//4]>>(2*(n&3)))&3;x|=(high&1)<<8
            if x>=256:x-=512
            if y>=224:continue
            size=sizes[self.regs[1]>>5][high>>1]
            address=base+((self.regs[1]>>3&3)+1)*8192*(flags&1)
            pixels=np.zeros((size,size),dtype=np.uint8)
            for dy in range(size//8):
                for dx in range(size//8):
                    number=((tile+dx)&15)|((tile+dy*16)&240)
                    pixels[dy*8:dy*8+8,dx*8:dx*8+8]=self.tile((address+number*32)&65535)
            if flags&64:pixels=pixels[:,::-1]
            if flags&128:pixels=pixels[::-1]
            colors=self.colors[128+((flags>>1)&7)*16+pixels]
            yield n,x,y,(flags>>1)&7,np.dstack((colors,(pixels!=0).astype(np.uint8)*255))
