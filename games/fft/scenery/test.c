// Test a streamed C compositor against an independent per-pixel Python oracle.
#include "bank.h"
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
    u16 bs,ms;u8 *bank=readfile("x/sbcommon.bin",&bs);unsigned map,rot;
    const char *names[]={"gariland","orbonne"};
    for(map=0;map<2;map++){
        char path[128];SbScene s;u8 *metadata,*patches,*data,*seen,*color,*cover,*gold;
        u8 scratch[256];u16 plane;
        sprintf(path,"x/sb%.3s.bin",names[map]);metadata=readfile(path,&ms);
        assert(!sb_bind(&s,NULL,bs,metadata,ms));
        assert(!sb_bind(&s,bank,19,metadata,ms));
        assert(!sb_bind(&s,bank,bs,metadata,15));
        assert(sb_bind(&s,bank,bs,metadata,ms));plane=s.plane;
        assert(!sb_bind(&s,bank,bs,metadata,ms-1));
        assert(sb_bind(&s,bank,bs,metadata,ms));
        data=malloc(6*plane+32);seen=malloc(plane+32);assert(data&&seen);
        color=data+16;cover=color+2*plane;
        sprintf(path,"x/sp%.3s.bin",names[map]);patches=readfile(path,NULL);
        for(rot=0;rot<4;rot++){
            memset(data,0xA5,6*plane+32);memset(seen,0xA5,plane+32);
            sb_compose(&s,rot,color,cover,seen+16);
            sprintf(path,"x/%s%u.raw",names[map],rot);gold=readfile(path,NULL);
            if(memcmp(color,gold,6*plane)){
                unsigned i;for(i=0;i<6*plane;i++)if(color[i]!=gold[i])break;
                fprintf(stderr,"%s rot%u byte%u C=%u oracle=%u\n",names[map],rot,i,color[i],gold[i]);
                return 1;
            }
            free(gold);
            assert(sb_patch(patches,rot,color,cover,plane,scratch));
            sprintf(path,"x/%s%u-original.raw",names[map],rot);gold=readfile(path,NULL);
            assert(!memcmp(color,gold,6*plane));
            // Rebuild only coverage, preserving an already patched color cache.
            sb_compose(&s,rot,NULL,cover,seen+16);
            assert(sb_patch(patches,rot,NULL,cover,plane,scratch));
            assert(!memcmp(color,gold,6*plane));free(gold);
            {unsigned i;for(i=0;i<16;i++){
                assert(data[i]==0xA5&&data[6*plane+16+i]==0xA5);
                assert(seen[i]==0xA5&&seen[plane+16+i]==0xA5);
            }}
            printf("%s rotation%u: six planes + exact residual + coverage-only OK\n",names[map],rot);
        }
        free(patches);free(seen);free(data);free(metadata);
    }
    free(bank);return 0;
}
