#ifndef FFT_IDENTITY_POOL_H
#define FFT_IDENTITY_POOL_H
static inline u16 identity_word(const u8 *p,u16 i)
{
#ifdef __m68k__
    return ((const u16 *)p)[i];
#else
    p+=i*2;return (u16)((u16)p[0]<<8|p[1]);
#endif
}
// FCP2: aliases at byte7; five words before the portrait data describe the
// Gariland/Orbonne/management ID directories, generic names and unique count.
static const u8 *identity_section(const u8 *bank,u8 which)
{
    const u8 *meta=bank+identity_word(bank,2)-10;
    return bank+identity_word(meta,which);
}
static const u8 *identity_portrait(const u8 *bank,u8 id)
{return bank+identity_word(bank,2)+(u16)id*348;}
static const char *identity_name(const u8 *bank,u16 id)
{
    const u8 *names=identity_section(bank,3);
    return (const char *)names+identity_word(names,id);
}
static u8 identity_valid(const u8 *bank,u16 size)
{
    const u8 *meta;u16 faces,n,i,k,at;u8 lengths[3];
    if(!bank||size<18||identity_word(bank,0)!=0x4643||identity_word(bank,1)!=0x5032)return 0;
    faces=identity_word(bank,2);if(faces<18||(faces&1)||faces>size)return 0;
    meta=bank+faces-10;n=identity_word(meta,4);
    if(!n||n>255||n>(size-faces)/348)return 0;
    lengths[0]=6;lengths[1]=8;lengths[2]=41;
    for(k=0;k<3;k++){
        at=identity_word(meta,k);if(at<7||at>faces-10||lengths[k]>faces-10-at)return 0;
        for(i=0;i<lengths[k];i++)if(bank[at+i]>=n)return 0;
    }
    at=identity_word(meta,3);
    if((at&1)||at<faces+n*348U||at>size||1020U>size-at)return 0;
    for(i=0;i<510;i++){
        u16 position=identity_word(bank+at,i);
        if(position<1020||position>=size-at)return 0;
        position+=at;while(position<size&&bank[position])position++;
        if(position==size)return 0;
    }
    // Alias records must terminate before the three ID directories.
    at=7;
    for(i=0;i<bank[6];i++){
        if(at>=identity_word(meta,0)||bank[at++]>=n)return 0;
        while(at<identity_word(meta,0)&&bank[at])at++;
        if(at++>=identity_word(meta,0))return 0;
    }
    return 1;
}
#endif
