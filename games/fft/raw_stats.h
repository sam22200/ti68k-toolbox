#ifndef FFT_RAW_STATS_H
#define FFT_RAW_STATS_H
// Unit.raw has a spare high byte; serialized raw values remain exactly24 bits.
// Legacy records store level-one seeds. A level-down materializes current raw
// values once, then subsequent levels/jobs retain those values as in FFT.
#define RAW_CURRENT 0x80000000UL
#define RAW_MAX 0x00ffffffUL
static u32 core_raw_grown(u32 value,u8 growth,u8 level)
{
    u8 k,g=growth?growth:1;
    for(k=2;k<=level;k++)value+=value/(u16)(g+k-1);
    return value>RAW_MAX?RAW_MAX:value;
}
static u32 core_raw_level(u32 value,u8 growth,u8 level,u8 down)
{
    u16 divisor=(u16)(growth?growth:1)+level;
    value=down?value-value/divisor:value+value/divisor;
    return value>RAW_MAX?RAW_MAX:value;
}
#endif
