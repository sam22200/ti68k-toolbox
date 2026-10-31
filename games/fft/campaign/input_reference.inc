// Prior direct input/AI-cursor policy; immutable native relocation control.
static void move_cursor(s16 du, s16 dv)
{
    s16 u, v, x, z;
#ifdef FFT_SOURCE_CAMERA
    s16 swap=du;du=dv;dv=swap;
#endif
    view_of(st.rot, st.cx, st.cz, &u, &v);
    u += du; v += dv;
    if ((u16)u >= view_w(st.rot) || (u16)v >= view_h(st.rot)) return;
    world_of(st.rot, u, v, &x, &z);
    {
        u16 cell=node_at(x,z,node_layer(CURSOR_CELL));
        if(cell>=MAP_N)cell=node_at(x,z,0);
        if(cell<MAP_N)cursor_place((u8)cell);
    }
}

// a step (du, dv) in the view as a facing (world +x, -x, +z, -z), and back
static u8 face_of(s16 du, s16 dv)
{
    s16 x0, z0, x, z;
#ifdef FFT_SOURCE_CAMERA
    s16 swap=du;du=dv;dv=swap;
#endif
    world_of(st.rot, 1, 1, &x0, &z0);
    world_of(st.rot, 1 + du, 1 + dv, &x, &z);
    return x > x0 ? 0 : x < x0 ? 1 : z > z0 ? 2 : 3;
}

static void to_menu(void)
{
    st.mode = M_MENU;
    cursor_place(UNIT_CELL(&st.unit[st.act]));
}

// Manual / this unit / all allies. Never reset Move/Act or advance the CT clock.
// Cancellation lets a committed walk, blow or loot message complete normally.
static void auto_control(u8 choice)
{
    u8 i;
    if (!choice) {
        st.auto_units=0; st.ai_all=0;
        if (st.unit[st.act].team!=TEAM_PLAYER || !st.ai) return;
    } else if (choice==2) {
        for (i=0;i<NUNIT;i++) if (st.unit[i].team==TEAM_PLAYER) st.auto_units|=(u16)(1U<<i);
    } else st.auto_units|=(u16)(1U<<st.act);
    st.ai=choice ? AI_THINK:AI_OFF; st.ai_t=0;
    W->ai_k=W->frz=W->frz_n=W->hold=0; W->frz_h=0;
    if (choice) st.mode=M_BROWSE;
    else if (st.mode!=M_WALK && st.mode!=M_ACT && st.mode!=M_PICK) to_menu();
    hl_seq++;
}

// the turn menu: Move and Act once each (a move undone with [shift] before acting), Wait
// (then the facing), Status
static void menu_keys(void)
{
    Unit *u = &st.unit[st.act];
    st.menu=choice_move(st.menu,MN_N,rt_keys&~rt_prev);
    if (input_pressed(K_A | K_ENTER)) {
        if (st.menu == MN_MOVE && !st.moved
#ifdef FFT_CHAPTER
            && unit_can_move(u)
#endif
        ) {
            st.mode = M_TARGET; st.sel = st.act;
            compute_reach(st.act);
        } else if (st.menu == MN_ACT && !st.acted
#ifdef FFT_CHAPTER
            && unit_can_act(u)
#endif
        ) {
            st.mode = M_ACTS; st.sub = 0;
        } else if (st.menu == MN_WAIT) {
            st.mode = M_FACE; st.wface = u->face;
        } else if (st.menu == MN_STATUS) {
            st.mode = M_STATUS; W->status_i = 0; st.page = 0; st.sel = st.act; st.back = M_MENU;
            st.status_col = st.status_row = st.status_info = 0;
        } else if (st.menu == MN_AUTO) {
            st.mode=M_AUTO; st.sub=1;
        }
    } else if (input_pressed(K_ESC | K_B)) {
        if (st.moved && !st.acted && !st.pick_t) {   // the move undone (not after a pick-up)
#ifdef FFT_CHAPTER
            unit_place(u,st.move_cell);u->face=st.mface;
#else
            u->x = st.mx; u->z = st.mz; u->face = st.mface;
#endif
            st.moved = 0; st.menu = MN_MOVE;
            to_menu();
        } else st.mode = M_BROWSE;                 // looking around
    }
}

// Act's list (Attack, the unit's skill set) and the abilities it knows (an item without any
// left greyed); [shift] goes back a level
static void acts_keys(void)
{
    u8 n = 0;
#ifdef FFT_CHAPTER
    if(!unit_can_act(&st.unit[st.act])){to_menu();return;}
#endif
    if (st.mode == M_ACTS) {
        u8 m = (u8)(2 + st.unit[st.act].defend);  // Attack, the skill set, Defend
        st.sub=choice_move(st.sub,m,rt_keys&~rt_prev);
        if (input_pressed(K_A | K_ENTER)) {
            if (!st.sub) aim(NO_ABIL);
            else if (st.sub == 2) { st.acted = 1; defend(st.act); }
            else if (known(st.act, 0) != NO_ABIL) { st.mode = M_SKILLS; st.skill = 0; }
        } else if (input_pressed(K_ESC | K_B)) st.mode = M_MENU;
        return;
    }
    while (known(st.act, n) != NO_ABIL) n++;
    st.skill=choice_move(st.skill,n,rt_keys&~rt_prev);
    if (input_pressed(K_A | K_ENTER)) {
        AbId ab = known(st.act, st.skill);
        if (can_use(st.act, ab)) aim(ab);
    } else if (input_pressed(K_ESC | K_B)) st.mode = M_ACTS;
}


static u8 ai_shown(void)
{
    u8 c=st.ai==AI_MOVE?st.ai_o:st.ai_tgt,x=TILE_X[c],z=TILE_Z[c];
    if (st.cx != x || st.cz != z || CURSOR_CELL!=c) {
        if (++st.ai_t >= AI_GLIDE) {
            st.ai_t = 0;
            s16 nextx=st.cx,nextz=st.cz;u16 cell;
            if (nextx != x) nextx+=nextx<x?1:-1;
            else if(nextz!=z)nextz+=nextz<z?1:-1;
            cell=node_at(nextx,nextz,node_layer(CURSOR_CELL));
            if(cell>=MAP_N)cell=node_at(nextx,nextz,0);
            if(cell<MAP_N)cursor_place((u8)cell);
            if(nextx==x&&nextz==z)cursor_place(c);
        }
        return 0;
    }
    if (!W->hud_ok) return 0;                   // the time shown counts once the HUD is drawn
    return ++st.ai_t >= (st.ai == AI_MOVE ? AI_SHOW : st.ai_ab == NO_ABIL ? AI_PREVIEW : AI_PREVIEW + AI_NAME);
}

