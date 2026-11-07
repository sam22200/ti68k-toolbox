// A position-independent module (-mpcrel): no relocation, copied as is into a zone of the
// program and called with a context (picrun.c). It must call nothing but the context: no
// library, no core symbol (picmod.sh checks the object).
typedef unsigned short u16; typedef short s16; typedef unsigned char u8;
typedef struct { u16 *counter; void (*rect)(s16, s16, s16, s16, u8); void (*text)(s16, s16, const char *, u8, u8); } ModCtx;
static const u8 table[8] = { 3, 1, 4, 1, 5, 9, 2, 6 };
static u16 helper(u16 v) { return (u16)(v * 3 + table[v & 7]); }
u16 mod_entry(const ModCtx *c)
{
    char s[8]; u16 v = helper(++*c->counter), i = 5;
    s[i] = 0;
    do s[--i] = (char)('0' + v % 10); while ((v /= 10) && i);
    c->rect(20, 30, 120, 40, 3);
    c->text(30, 44, "module", 0, 3);
    c->text(30, 54, s + i, 0, 3);
    return v;
}
