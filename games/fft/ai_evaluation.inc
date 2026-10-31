// Shared immediate-state evaluator and stable ranking; no image loads or RNG.
static s16 worth(const Unit *u, u16 hp, u8 status)
{
    s16 s;
    if (!hp) return -192;
    s = divs32_16((long)hp << 7, (short)u->max_hp);
    if ((u16)(hp * 5) <= u->max_hp) s -= 32;
    if (status & ST_POISON) s -= 26;
    if (status & ST_DARK) s -= 64;
    return s;
}

static u8 ai_range(u8 a, AbId ab)
{
#ifdef FFT_CHAPTER
    if (ab == NO_ABIL) return W->ai_weapon;
#endif
    if (ab == NO_ABIL) return 1;
    if (abilities[ab].flags & AF_ITEM && st.unit[a].move < abilities[ab].range) return st.unit[a].move;
    return abilities[ab].range;
}

static u8 ai_reaches(u8 a, AbId ab, u8 o, u8 c)
{
    const Tile *s = &map_tiles[o], *t = &map_tiles[c];
    s16 dx = (s16)TILE_X[c] - TILE_X[o], dz = (s16)TILE_Z[c] - TILE_Z[o], h = (s16)t->stand - s->stand;
    u8 d = (u8)((dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz));
    if (!t->walk) return 0;
#ifdef FFT_CHAPTER
    if (ab == NO_ABIL && W->ai_weapon > 1) return ranged_reach(a, o, c);   // a bow
#endif
    if (ab == NO_ABIL) return d == 1 && h >= -6 && h <= 5;
    if (d > ai_range(a, ab) || (o==c && !(abilities[ab].flags & AF_SELF))) return 0;
    return abilities[ab].vert == 0xFF || (h < 0 ? -h : h) <= abilities[ab].vert;
}

static u8 ai_better(s16 pri, u8 hit, u8 o, const AiAct *e)
{
    if (pri != e->pri) return pri > e->pri;
    if (hit != e->hit) return hit > e->hit;
    return W->ai_prox[o] < W->ai_prox[e->o];
}

static void ai_insert(const AiAct *c)
{
    AiAct *r = W->ai_rank;
    u8 i, j;
    if (c->pri <= W->ai_base) return;
    for (i = 0; i < W->ai_n && !ai_better(c->pri, c->hit, c->o, &r[i]); i++) ;
    if (i >= 8) return;
    for (j = W->ai_n < 8 ? W->ai_n : 7; j > i; j--) r[j] = r[j - 1];
    r[i] = *c;
    if (W->ai_n < 8) W->ai_n++;
}
