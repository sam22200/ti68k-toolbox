// C Header File
// Created 26/05/2002; 13:51:47

#define OPTIMIZE_ROM_CALLS
#include <tigcclib.h> 

#define PIXOFFSET(x,y)  ((y<<5)-(y<<1)+(x>>3))
#define PIXADDR(p,x,y)  (((unsigned char*)(p))+PIXOFFSET(x,y))
#define PIXMASK(x)      ((unsigned char)(0x80 >> ((x)&7)))


// Return 1 if there is at least one collision 
// betwwen the sprite and pixels already drawn 
// Sprite shouldn'tbe larger than 24 pixels
int test_sprite(int x,int y,long sprite[],int height,unsigned char* plane)
{
int i;
unsigned char* addr=PIXADDR(plane,x,y);//Pixel Adress
unsigned long mask1 =PIXMASK(x),mask2,data;
for (i=0;i<8;i++)
{
if (mask1&(1<<i))
break;
mask1|=1<<i;
}
mask2=(~(mask1&255))|0xFFFFFF00;
mask1=((mask1<<24)|0xFFFFFF)&mask2;
//Mask1 contains your sprite line

for(i=0;i<height;i++,addr+=30)
{
data=((unsigned long)(*addr)<<24)+((unsigned long)(*(addr+1))<<16)
+((unsigned long)(*(addr+2))<<8)+(*(addr+3));
//Data contains the Screen line

data=data&mask1&((sprite[i])>>(x&7));
if (data!=0)
return 1;
}
return 0;
}

