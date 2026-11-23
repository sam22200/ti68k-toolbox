// Host-only route probes. All transactions use actual menu inputs on the same
// save produced by combat; no funds, inventory, unlock or roster injection.
#if PILOT_KIND==5 || PILOT_KIND==6
static u8 pilot_marker(const char *name)
{FILE *f=fopen(name,"rb");if(!f)return 0;fclose(f);return 1;}
#endif
#if PILOT_KIND==5
static void pilot_travel(u8 destination)
{
    u32 frames=0;u16 day=save.world.day;
    while(abs(wm.handx-loc_x(destination))>1&&frames++<2000)step(wm.handx<loc_x(destination)?K_RIGHT:K_LEFT);
    while(abs(wm.handy-loc_y(destination))>1&&frames++<2000)step(wm.handy<loc_y(destination)?K_DOWN:K_UP);
    step(0);assert(wm.sel==destination&&!wm.moving);tap(K_ENTER);assert(wm.moving);
    while(wm.moving&&frames++<4000)step(0);
    assert(!wm.moving&&wm.at==destination&&save.world.day==day+1);
    printf("town-travel loc%u day%u frames%u\n",wm.at,save.world.day,frames);
}
static void pilot_town(void)
{
    u8 item,count,n;u32 funds;FILE *f;
    assert(wm.at==6);snapshot("before-town.sav");
    pilot_travel(24);tap(K_ENTER);assert(wm.note==NOTE_LATER);tap(K_ENTER); // formation not ripped yet
    pilot_travel(6);
    tap(K_ENTER);assert(wm.screen==SC_TOWN);
    tap(K_ENTER);assert(wm.screen==SC_STORE);
    tap(K_ENTER);assert(wm.screen==SC_SHOP&&wm.buy);
    item=shop_nth(0);funds=gil();count=save.inv[item];assert(item&&item_price(item)&&funds>=2*item_price(item)+office_price(0));
    tap(K_MODE);assert(wm.detail);tap(K_MODE);assert(!wm.detail);
    tap(K_ENTER);tap(K_ENTER);assert(save.inv[item]==count+2&&gil()==funds-2*item_price(item));
    tap(K_LEFT);assert(!wm.buy);
    {u16 limit=0;while(shop_nth(wm.trow)!=item&&limit++<256)tap(K_DOWN);assert(shop_nth(wm.trow)==item);}
    tap(K_ENTER);assert(save.inv[item]==count+1&&gil()==funds-2*item_price(item)+(item_price(item)>>1));
    f=fopen("pilot-town-item","wb");assert(f);fputc(item,f);fclose(f);
    printf("town-shop item%u inventory%u->%u gil%u->%u\n",item,count,save.inv[item],funds,gil());
    tap(K_ESC);tap(K_ESC);assert(wm.screen==SC_TOWN);
    tap(K_DOWN);tap(K_ENTER);assert(wm.screen==SC_OFFICE);
    n=save.n;funds=gil();tap(K_ENTER);assert(wm.screen==SC_HIRE);
    tap(K_DOWN);tap(K_DOWN);tap(K_ENTER);assert(wm.card);tap(K_B);assert(!wm.card);
    tap(K_UP);tap(K_UP);tap(K_ENTER);assert(wm.screen==SC_OFFICE&&save.n==n+1&&gil()==funds-office_price(0)&&save_generic(&save,n));
    printf("town-office party%u->%u gil%u->%u\n",n,save.n,funds,gil());
    tap(K_ESC);assert(wm.screen==SC_TOWN);
    tap(K_DOWN);tap(K_DOWN);tap(K_ENTER);assert(wm.note==NOTE_LATER);tap(K_ENTER); // III stays locked
    tap(K_DOWN);tap(K_ENTER);assert(wm.screen==SC_BAR&&!service_open(&save,service_gates,0));
    tap(K_ENTER);assert(wm.screen==SC_RUMORS&&rumor_count());
    {u8 i,longest=0;u16 pages=0;
        for(i=0;i<rumor_count();i++){tap(K_ENTER);assert(wm.screen==SC_RUMOR);if(wm.rlast>longest)longest=wm.rlast;
            while(wm.screen==SC_RUMOR&&pages++<256)tap(K_ENTER);
            assert(wm.screen==SC_RUMORS);tap(K_DOWN);}
        printf("town-bar topics%u pages%u longest%u propositions-locked\n",rumor_count(),pages,longest+1);
    }
    tap(K_ESC);tap(K_ESC);tap(K_ESC);assert(wm.screen==SC_MAP&&!wm.note);
    snapshot("town-last-durable.sav"); // world leave below commits all pending changes
}
#elif PILOT_KIND==6
static void pilot_formation(void)
{
    Save before=save,back;FILE *f=fopen("pilot-town-item","rb");u8 item=0,who=0,slot=0,old=0;u16 limit=0;
    snapshot("before-formation.sav");
    if(f){int value=fgetc(f);fclose(f);assert(value>0&&value<256);item=(u8)value;
        for(who=0;who<save.n;who++)for(slot=0;slot<SLOTS;slot++)if(save.inv[item]&&can_equip(unit(who),slot,item))goto wearer;
        assert(0);
wearer:
        while(mg.unit!=who&&limit++<100){tap(K_RIGHT);while(mg.slide)step(0);}assert(mg.unit==who);
    }
    tap(K_ENTER);assert(mg.screen==S_EQUIP);
    tap(K_RIGHT);assert(mg.screen==S_SET);tap(K_LEFT);assert(mg.screen==S_EQUIP);
    if(item){u8 i;old=unit(who)->eq[slot];assert(old!=item);
        while(slot_order[mg.row]!=slot&&limit++<200)tap(K_DOWN);assert(slot_order[mg.row]==slot);
        tap(K_ENTER);assert(mg.screen==S_PICK);
        while(mg.pick[mg.top+mg.row]!=item&&limit++<500)tap(K_DOWN);assert(mg.pick[mg.top+mg.row]==item);
        tap(K_ENTER);assert(mg.screen==S_EQUIP&&unit(who)->eq[slot]==item&&save.inv[item]==before.inv[item]-1);
        if(old)assert(save.inv[old]==before.inv[old]+1);
        for(i=0;i<save.n;i++){SaveUnit expected=before.unit[i];if(i==who)expected.eq[slot]=item;assert(!memcmp(&expected,&save.unit[i],sizeof expected));}
    }
    tap(K_B);assert(mg.screen==S_FORM);tap(K_ESC);assert(!running);
    assert(save_load(&back)&&!memcmp(&back,&save,sizeof back));
    if(!item)assert(!memcmp(&before,&save,sizeof save));
    snapshot("after-formation.sav");f=fopen("pilot-managed.done","wb");assert(f);fputc(1,f);fclose(f);
    printf("formation unit%u slot%u item%u old%u unchanged-progress saved\n",who,slot,item,old);
}
#endif
