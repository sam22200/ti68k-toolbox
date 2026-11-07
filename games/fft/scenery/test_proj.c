// Runtime-projection compositor against proj.py's independent per-pixel oracle.
#include "proj.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 *readfile(const char *name,u16 *length)
{
    FILE *f=fopen(name,"rb");long n;u8 *data;
    assert(f);assert(!fseek(f,0,SEEK_END));n=ftell(f);assert(n>0&&n<65536);
    rewind(f);data=malloc(n);assert(data);assert(fread(data,1,n,f)==(size_t)n);
    fclose(f);if(length)*length=(u16)n;return data;
}

int main(void)
{
    u16 bs,ms;u8 *bank=readfile("x/proj/spgeo.bin",&bs);unsigned map,rot;
    const char *names[]={"gariland","orbonne"};
    static SpScene s;
    for(map=0;map<2;map++){
        char path[128];u8 *metadata,*data,*seen,*color,*cover,*gold;u16 plane;
        sprintf(path,"x/proj/spm%.3s.bin",names[map]);metadata=readfile(path,&ms);
        assert(!sp_bind(&s,bank,23,metadata,ms));
        assert(!sp_bind(&s,bank,bs,metadata,ms-1));
        assert(sp_bind(&s,bank,bs,metadata,ms));plane=s.plane;
        data=malloc(6*plane+32);seen=malloc(plane+32);assert(data&&seen);
        color=data+16;cover=color+2*plane;
        for(rot=0;rot<4;rot++){
            memset(data,0xA5,6*plane+32);memset(seen,0xA5,plane+32);
            sp_compose(&s,rot,color,cover,seen+16);
            sprintf(path,"x/proj/%s%u.raw",names[map],rot);gold=readfile(path,NULL);
            if(memcmp(color,gold,6*plane)){
                unsigned i;for(i=0;i<6*plane;i++)if(color[i]!=gold[i])break;
                fprintf(stderr,"%s rot%u byte%u C=%u oracle=%u\n",names[map],rot,i,color[i],gold[i]);
                return 1;
            }
            sp_compose(&s,rot,NULL,cover,seen+16);   // coverage-only rebuild
            assert(!memcmp(cover,gold+2*plane,4*plane));
            {unsigned i;for(i=0;i<16;i++){
                assert(data[i]==0xA5&&data[6*plane+16+i]==0xA5);
                assert(seen[i]==0xA5&&seen[plane+16+i]==0xA5);
            }}
            printf("%s rotation%u: six planes OK (%lu pieces)\n",names[map],rot,(unsigned long)s.pieces);
            free(gold);
        }
        free(seen);free(data);free(metadata);
    }
    free(bank);return 0;
}
