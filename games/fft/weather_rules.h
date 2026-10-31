// Source weather semantics, independent of scripts, particles or map-ID exceptions.
#ifndef FFT_WEATHER_RULES_H
#define FFT_WEATHER_RULES_H
enum { WEATHER_MAX_STRENGTH=4,WEATHER_SNOW=1,WEATHER_INDOORS=2 };
static inline u8 core_weather(u8 strength,u8 flags)
{
    if(flags&WEATHER_INDOORS)return 0;
    return strength>=2&&(flags&WEATHER_SNOW)?strength+3:strength;
}
#endif
