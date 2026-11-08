#ifndef FFT_RESOURCE_FILE_H
#define FFT_RESOURCE_FILE_H
/* Cold, single-hop references into immutable archived banks. Returned sizes
   exclude AMS's dat trailer on both CPUs. No handle/pointer survives a child. */
static u16 resource_size(const u8 *p,u16 n)
{
    if(p&&n>=6&&p[n-6]==0&&p[n-5]=='d'&&p[n-4]=='a'&&
       p[n-3]=='t'&&p[n-2]==0&&p[n-1]==248)n-=6;
    return n;
}
static const u8 *resource_file(const char *name,u16 *size)
{
    u16 n=0,off,end,i;const u8 *p=rt_file(name,&n);char target[9];
    n=resource_size(p,n);
    if(size)*size=0;
    if(!p)return RT_NULL;
    if(n>=4&&p[0]=='F'&&p[1]=='R'&&p[2]=='F'&&p[3]=='1'){
        if(n!=16)return RT_NULL;
        for(i=0;i<8;i++)target[i]=(char)p[4+i];
        target[8]=0;
        if(target[0]<'a'||target[0]>'z')return RT_NULL;
        for(i=1;i<8;i++){
            if(!target[i]){u16 j;for(j=i+1;j<8;j++)if(target[j])return RT_NULL;break;}
            if((target[i]<'a'||target[i]>'z')&&(target[i]<'0'||target[i]>'9'))return RT_NULL;
        }
        off=(u16)((u16)p[12]<<8|p[13]);end=(u16)((u16)p[14]<<8|p[15]);
        p=rt_file(target,&n);n=resource_size(p,n);
        if(!p||(off&1)||off>n||!end||end>n-off||
           (n>=4&&p[0]=='F'&&p[1]=='R'&&p[2]=='F'&&p[3]=='1'))return RT_NULL;
        p+=off;n=end;
    }
    if(size)*size=n;
    return p;
}
#endif
