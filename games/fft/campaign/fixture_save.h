// PC-only readers for ignored historical preview/test fixtures. Production
// migration is exercised separately through save_load on both PC and68000.
#include <stdio.h>
static u8 read_save_fixture(FILE *f,Save *s)
{
    size_t n;
    memset(s,0,sizeof *s);n=fread(s,1,sizeof *s,f);
    if(n==sizeof *s&&!memcmp(s->magic,SAVE_MAGIC,4))return 1;
    if(n==SAVE_V4_BYTES&&!memcmp(s->magic,"FSV4",4)){memcpy(s->magic,SAVE_MAGIC,4);return 1;}
    if(n==SAVE_V3_BYTES&&!memcmp(s->magic,"FSV3",4)){memcpy(s->magic,SAVE_MAGIC,4);return 1;}
    if(n!=SAVE_V2_BYTES||memcmp(s->magic,"FSV2",4))return 0;
    memcpy(s->magic,SAVE_MAGIC,4);s->money[2]=7;s->money[3]=208;return 1;
}
