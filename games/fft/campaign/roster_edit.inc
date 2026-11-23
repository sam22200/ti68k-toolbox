// Roster edits shared by the management screens: FFT's Remove Unit and Order Unit.
// A party slot is a key: the egg/birthday/colour record, the current-raw-stats bit and the
// deployment's slot references all follow the unit, so every edit moves them together.
#ifndef FFT_ROSTER_EDIT_INC
#define FFT_ROSTER_EDIT_INC

static void roster_slot_swap(Save *s, u8 a, u8 b)
{
    SaveUnit t;
    u8 i, ca, cb, ra, rb, e, d0, d1;
    if (a == b || a >= s->n || b >= s->n) return;
    if(s->missions.report.ready)return;
    {u8 x=save_generic(s,a),y=save_generic(s,b),j,k;save_generic_set(s,a,y);save_generic_set(s,b,x);
     for(j=0;j<s->missions.dispatches.n;j++)for(k=0;k<s->missions.dispatches.job[j].n;k++){u8 *v=&s->missions.dispatches.job[j].unit[k];if(*v==a)*v=b;else if(*v==b)*v=a;}}
    t = s->unit[a]; s->unit[a] = s->unit[b]; s->unit[b] = t;
    ra = save_raw_current(s, a); rb = save_raw_current(s, b);
    save_raw_set(s, a, rb); save_raw_set(s, b, ra);
    ca = world_egg_color(&s->world, a); cb = world_egg_color(&s->world, b);
    e = s->world.egg[a]; s->world.egg[a] = s->world.egg[b]; s->world.egg[b] = e;
    d0 = s->world.birthday[a][0]; d1 = s->world.birthday[a][1];
    s->world.birthday[a][0] = s->world.birthday[b][0];
    s->world.birthday[a][1] = s->world.birthday[b][1];
    s->world.birthday[b][0] = d0; s->world.birthday[b][1] = d1;
    wb_color_set(&s->world, a, cb); wb_color_set(&s->world, b, ca);
    for (i = 0; i < s->deploy_n && i < 5; i++) {
        if (s->deploy[i][0] == a) s->deploy[i][0] = b;
        else if (s->deploy[i][0] == b) s->deploy[i][0] = a;
    }
}

// The dismissed unit's equipment goes back to the bag, as the source returns it.
static u8 roster_remove(Save *s, u8 slot)
{
    u8 i, k;
    if (slot >= s->n || s->unit[slot].type == 2 || s->missions.report.ready || prop_away(&s->missions.dispatches,slot)) return 0;      // never Ramza
    for (k = 0; k < 5; k++) {
        u8 id = s->unit[slot].eq[k];
        if (id && s->inv[id] < 99) s->inv[id]++;
        s->unit[slot].eq[k] = 0;
    }
    for (i = slot; i + 1 < s->n; i++) roster_slot_swap(s, i, (u8)(i + 1));
    i = (u8)(s->n - 1);
    memset(&s->unit[i], 0, sizeof s->unit[i]);
    save_raw_set(s, i, 0);save_generic_set(s,i,0);
    world_party_clear_slot(&s->world, i);
    for (k = 0; k < s->deploy_n && k < 5; k++) if (s->deploy[k][0] == i) {
        u8 j;
        for (j = k; j + 1 < s->deploy_n; j++) memcpy(s->deploy[j], s->deploy[j + 1], 4);
        s->deploy_n--; k--;
    }
    s->n--;
    return 1;
}

// Order Unit's criteria, in the source's menu order.
enum { ORD_JOB, ORD_NUMBER, ORD_LEVEL, ORD_MAX_MP, ORD_MAX_HP, ORD_FAITH, ORD_BRAVE, ORD_N };

// The sort key, descending for every criterion but Job and Number (the source lists
// the strongest first); Number restores the roster's own order, so it is a no-op.
static u16 roster_key(const Save *s, u8 slot, u8 by, u16 hp, u16 mp)
{
    const SaveUnit *u = &s->unit[slot];
    switch (by) {
    case ORD_JOB:   return u->job;
    case ORD_LEVEL: return (u16)(255 - u->level);
    case ORD_MAX_MP: return (u16)(9999 - mp);
    case ORD_MAX_HP: return (u16)(9999 - hp);
    case ORD_FAITH: return (u16)(255 - u->faith);
    default:        return (u16)(255 - u->brave);
    }
}
#endif
