// Private PC visual-review hooks. Never included by a shipping source file.
static const u8 *review_art;
static u8 review_enabled;
static const u8 *review_record(u8 job)
{return review_art+8+48+255U*16U+(u16)review_art[8+job-94]*660U;}
static const char *review_name(u8 id)
{return (const char *)(review_art+8+48+(u16)id*16U);}
static void review_actor(s16 x,s16 y,const SaveUnit *u)
{
    const u8 *p=review_record(u->job);u32 rows[3][26];u8 a,b;
    RtSprite sp={32,26,rows[0],rows[1],rows[2]};
    for(a=0;a<3;a++)for(b=0;b<26;b++){
        const u8 *q=p+(u16)a*104+(u16)b*4;
        rows[a][b]=(u32)q[0]<<24|(u32)q[1]<<16|(u16)q[2]<<8|q[3];
    }
    draw_sprite(x-8,y,&sp);
}
