#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "minish.h"
#include "generated.h"
static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL %d: %s\n",__LINE__,#c); failures++; } } while(0)

static void combat_pixels(void)
{
    FILE *f=fopen("fixtures/combat_pixels.bin","rb");
    unsigned n,t,x,y,bad=0;
    CHECK(f!=NULL);if (!f) return;
    n=fgetc(f);n|=fgetc(f)<<8;
    for (t=0;t<n;t++) {
        unsigned scenario=fgetc(f);scenario|=fgetc(f)<<8;sw_init(scenario);CHECK(sw_step(0));
        for (y=0;y<RT_H;y++) for (x=0;x<RT_W;x++) {
            unsigned expected=fgetc(f);
            if (!(y<8 && x>=128) && sw_level(x,y)!=expected) {
                if (bad++<3) printf("combat pixel scenario%u %u,%u %u/%u\n",scenario,x,y,sw_level(x,y),expected);
            }
        }
    }
    CHECK(!bad);CHECK(fgetc(f)==EOF);fclose(f);
    printf("Combat pixel oracle: %u pixels, %u differences\n",n*160*100,bad);
}

static void effects_pixels(void)
{
    FILE *f=fopen("fixtures/effects_pixels.bin","rb");
    unsigned n,t,x,y,bad=0;
    CHECK(f!=NULL);if (!f) return;
    n=fgetc(f);n|=fgetc(f)<<8;
    for (t=0;t<n;t++) {
        unsigned scenario=fgetc(f);scenario|=fgetc(f)<<8;sw_init(scenario);CHECK(sw_step(0));
        for (y=0;y<RT_H;y++) for (x=0;x<RT_W;x++) {
            unsigned expected=fgetc(f);
            if (!(y<8 && x>=128) && sw_level(x,y)!=expected) {
                if (bad++<3) printf("Effect pixel scenario%u %u,%u %u/%u\n",scenario,x,y,sw_level(x,y),expected);
            }
        }
    }
    CHECK(!bad);CHECK(fgetc(f)==EOF);fclose(f);
    printf("Roll/effect pixel oracle: %u pixels, %u differences\n",n*160*100,bad);
}

static void roll_fixtures(void)
{
    FILE *f=fopen("fixtures/roll.txt","r");unsigned n,t,i,steps=0,bad=0;
    CHECK(f!=NULL);if (!f) return;
    CHECK(fscanf(f,"%u",&n)==1);
    for (t=0;t<n;t++) {
        unsigned face,count;long px,py;
        CHECK(fscanf(f,"%u %ld %ld %u",&face,&px,&py,&count)==4);
        minish_place(px>>8,py>>8);st.x=px;st.y=py;st.anim_face=face;st.facing=face<<3;
        st.pose=st.display_pose=idle_pose[face];
        for (i=0;i<count;i++) {
            unsigned keys,active;long x,y;
            CHECK(fscanf(f,"%u %ld %ld %u",&keys,&x,&y,&active)==4);
            minish_step(keys);
            if (st.x!=x || st.y!=y || !!st.roll!=active) {
                if (bad++<12) printf("Roll trial%u step%u: %ld,%ld/%ld,%ld active%u/%u\n",t,i,(long)st.x,(long)st.y,x,y,!!st.roll,active);
            }
            steps++;
        }
    }
    fclose(f);CHECK(!bad);
    printf("Original roll fixtures: %u updates, %u differences\n",steps,bad);
    minish_place(320,184);minish_step(K_B);CHECK(!st.roll);
    minish_step(0);minish_step(K_A|K_RIGHT|K_B);CHECK(st.attack && !st.roll);
    minish_place(320,184);minish_step(K_B|K_RIGHT);
    {long x=st.x;for (i=0;i<12;i++) minish_step(K_LEFT|K_A|K_B);
     CHECK(st.roll && st.x>x && !st.attack);
     CHECK(!sw_save_state("captures/roll_state.bin"));
     {u32 hash=minish_hash();for(i=0;i<50;i++) minish_step(0);
      CHECK(!sw_load_state("captures/roll_state.bin"));CHECK(minish_hash()==hash);}
     remove("captures/roll_state.bin");}
    minish_place(320,184);
    for (i=0;i<5;i++) minish_fx_spawn(320+i*8,184,i&1);
    {unsigned live=0;for(i=0;i<4;i++) live+=st.effects[i].age!=0;CHECK(live==4);}
    minish_step(0);game_render();
    {u32 hash=minish_hash();u16 screen=sw_checksum();
     CHECK(!sw_save_state("captures/effects_state.bin"));
     for(i=0;i<120;i++) minish_step(0);
     CHECK(!sw_load_state("captures/effects_state.bin"));CHECK(minish_hash()==hash);
     game_render();CHECK(sw_checksum()==screen);remove("captures/effects_state.bin");}
    for (i=0;i<130;i++) minish_step(0);
    for(i=0;i<4;i++) CHECK(!st.effects[i].age && st.effects[i].display_pose==255);
    minish_fx_spawn(320,184,0);CHECK(st.effects[0].age);
    minish_place(320,184);for(i=0;i<4;i++) CHECK(!st.effects[i].age);
    minish_combat_start();st.enemies[1].hp=0;
    st.enemies[0].x=320*256L;st.enemies[0].y=184*256L;st.enemies[0].timer=200;
    st.roll=20;st.roll_guard=1;st.anim_face=1;
    minish_step(0);CHECK(st.health==24 && st.roll==21 && !st.iframes);
    st.roll=5;st.roll_guard=0;st.x=320*256L;st.y=184*256L;
    st.enemies[0].recoil=0;
    minish_step(0);CHECK(st.health==22 && !st.roll && st.iframes==30);
    sw_init(432);minish_step(K_A);
    for(i=0;i<5;i++) minish_step(0);
    CHECK(st.kills==1 && !st.enemies[0].hp && st.enemies[0].recoil && st.enemies[0].fade);
    for(i=0;i<100;i++) minish_step(0);
    CHECK(!st.enemies[0].recoil && !st.enemies[0].fade && st.health==24);
}

static void combat_fixtures(void)
{
    FILE *f=fopen("fixtures/combat.txt","r");
    unsigned trials,t,i,steps=0,bad=0;
    CHECK(f!=NULL);if (!f) return;
    CHECK(fscanf(f,"%u",&trials)==1);
    for (t=0;t<trials;t++) {
        unsigned kind,face,n,action,timer,hp,ifr,recoil,dir,i;
        long px,py,ex,ey;
        CHECK(fscanf(f,"%u %u %u %ld %ld %ld %ld %u %u %u %u %u %u",&kind,&face,&n,&px,&py,&ex,&ey,&action,&timer,&hp,&ifr,&recoil,&dir)==13);
        minish_place(px>>8,py>>8);minish_combat_start();
        st.x=px;st.y=py;st.anim_face=face;st.facing=face<<3;
        st.pose=st.display_pose=idle_pose[face];
        st.health=hp;st.iframes=ifr;st.recoil=recoil;st.recoil_dir=dir;
        st.enemies[0].x=ex;st.enemies[0].y=ey;
        st.enemies[0].action=action;st.enemies[0].timer=timer;st.enemies[0].face=face;
        st.enemies[1].hp=0;
        if (kind==1) {
            st.health=24;st.iframes=st.recoil=0;
            minish_step(0);
            CHECK(st.health==hp && st.iframes==ifr && st.recoil==recoil);
            st.x=px;st.y=py;st.health=hp;st.iframes=ifr;st.recoil=recoil;st.recoil_dir=dir;
            st.enemies[0].x=ex;st.enemies[0].y=ey;
            st.enemies[0].action=action;st.enemies[0].timer=timer;st.enemies[0].recoil=0;
        }
        for (i=0;i<n;i++) {
            unsigned k,ehp,flight,life;long x,y,ax,ay,rx,ry;
            CHECK(fscanf(f,"%u %ld %ld %ld %ld %u %u %u %u %u %ld %ld %u",&k,&x,&y,&ax,&ay,&hp,&ifr,&recoil,&ehp,&flight,&rx,&ry,&life)==13);
            minish_step(k);
            if ((kind==0 && (st.enemies[0].x!=ax || st.enemies[0].y!=ay)) ||
                (kind==1 && (st.health!=hp || st.iframes!=ifr || st.recoil!=recoil || st.x!=x || st.y!=y)) ||
                (kind==2 && st.enemies[0].hp!=ehp) ||
                (kind==3 && (st.health!=hp || st.iframes!=ifr || st.recoil!=recoil ||
                    (flight && (st.rocks[0].x!=rx || st.rocks[0].y!=ry || st.rocks[0].life!=life))))) {
                if (bad++<12) printf("combat trial %u kind %u step %u: enemy %ld,%ld/%ld,%ld hp%u/%u player %ld,%ld/%ld,%ld h%u/%u ifr%u/%u recoil%u/%u\n",
                    t,kind,i,(long)st.enemies[0].x,(long)st.enemies[0].y,ax,ay,st.enemies[0].hp,ehp,(long)st.x,(long)st.y,x,y,st.health,hp,st.iframes,ifr,st.recoil,recoil);
            }
            steps++;
        }
    }
    CHECK(!bad);fclose(f);
    printf("Original combat fixtures: %u updates, %u differences\n",steps,bad);
    /* Nearby Link is targeted even when the Octorok finishes its walk facing
       away. Follow the ordinary animation, release, flight and damage. */
    sw_init(420);
    {unsigned shot=0,moved=0; s32 released_x=0;
     for (i=0;i<120;i++) {
         minish_step(0);
         if (st.rocks[0].life) {
             if (!shot) {shot=1;released_x=st.rocks[0].x;CHECK(st.enemies[0].face==3);}
             else if (st.rocks[0].x<released_x) moved=1;
         }
         if (st.health<24) break;
     }
     CHECK(shot && moved && st.health==22 && st.iframes==30 && st.recoil==8);
     CHECK(!st.rocks[0].life);
     printf("Native aimed projectile: released, moved and dealt damage\n");}
    sw_init(258);CHECK(sw_step(0));CHECK(!st.health);
    {u32 h=minish_hash();sw_step(K_A|K_RIGHT);CHECK(!st.health && st.x==280*256L);CHECK(minish_hash()!=h);}
    sw_step(K_ENTER);CHECK(st.health==24 && st.enemies[0].hp==2 && st.enemies[1].hp==2);
    sw_init(256);
    {u32 h=minish_hash();CHECK(!sw_save_state("captures/combat_state.bin"));
     for (i=0;i<80;i++) sw_step(i&1 ? 0 : K_A);
     CHECK(!sw_load_state("captures/combat_state.bin"));CHECK(minish_hash()==h);
     remove("captures/combat_state.bin");}
    {
        static SwScript script;
        sw_init(256);CHECK(!sw_load_script(&script,"keys/combat.txt"));
        for (i=0;i<300;i++) CHECK(sw_step(sw_script_keys(&script,i)));
        CHECK(st.kills==2 && st.health>0 && !st.enemies[0].hp && !st.enemies[1].hp);
        printf("Native combat route: both Octoroks defeated, health %u/24\n",st.health);
        sw_init(260);CHECK(sw_step(0));
        {u32 h=minish_hash();u16 screen=sw_checksum();
         CHECK(st.rocks[0].life && !sw_save_state("captures/combat_state.bin"));
         for (i=0;i<80;i++) sw_step(i&1 ? K_A : K_RIGHT);
         CHECK(!sw_load_state("captures/combat_state.bin"));CHECK(minish_hash()==h);
         game_render();CHECK(sw_checksum()==screen);remove("captures/combat_state.bin");}
    }
}

static void action_pixels(void)
{
    FILE *f=fopen("fixtures/action_pixels.bin","rb");
    unsigned n,t,x,y,bad=0,pixels=0;
    CHECK(f!=NULL);if (!f) return;
    n=fgetc(f);n|=fgetc(f)<<8;
    for (t=0;t<n;t++) {
        unsigned scenario=fgetc(f);scenario|=fgetc(f)<<8;
        sw_init(scenario);CHECK(sw_step(0));
        for (y=0;y<RT_H;y++) for (x=0;x<RT_W;x++) {
            unsigned expected=fgetc(f);
            /* HUD/endpoint are independently cross-checked on PC and TI. */
            if (!(y<8 && x>=128) && sw_level(x,y)!=expected) {
                if (!bad) printf("action oracle scenario %u pixel %u,%u: %u/%u\n",scenario,x,y,sw_level(x,y),expected);
                bad++;
            }
            pixels++;
        }
    }
    CHECK(!bad);CHECK(fgetc(f)==EOF);fclose(f);
    printf("Sword/cut pixel oracle: %u pixels, %u differences\n",pixels,bad);
}

static void action_fixtures(void)
{
    FILE *f=fopen("fixtures/actions.txt","r");
    unsigned trials,t,steps=0,bad=0;
    CHECK(f!=NULL);if (!f) return;
    CHECK(fscanf(f,"%u",&trials)==1);
    for (t=0;t<trials;t++) {
        unsigned x,y,face,n,i;
        CHECK(fscanf(f,"%u %u %u %u",&x,&y,&face,&n)==4);
        minish_place(x,y);st.anim_face=face;st.facing=face<<3;
        st.pose=st.display_pose=idle_pose[face];
        for (i=0;i<n;i++) {
            unsigned k,px,py,active,flags[7],j;int pose;
            CHECK(fscanf(f,"%u %u %u %u %d",&k,&px,&py,&active,&pose)==5);
            for (j=0;j<7;j++) CHECK(fscanf(f,"%u",&flags[j])==1);
            { unsigned previous=st.cut_count;
            minish_step(k);
            CHECK(st.display_cut_count==previous);
            }
            if (st.x!=(s32)px || st.y!=(s32)py || !!st.attack!=active ||
                (active && st.pose!=pose)) {
                if (bad++<8) printf("action trial %u step %u: XY %ld,%ld/%u,%u active %u/%u pose %u/%d\n",
                   t,i,(long)st.x,(long)st.y,px,py,!!st.attack,active,st.pose,pose);
            }
            for (j=0;j<7;j++) if (st.cut_flags[j]!=flags[j]) {
                if (bad++<8) printf("action trial %u step %u cut byte %u: %u/%u\n",t,i,j,st.cut_flags[j],flags[j]);
            }
            steps++;
        }
    }
    CHECK(!bad);fclose(f);
    printf("Original sword and bush fixtures: %u steps, %u differences\n",steps,bad);
    sw_init(140);CHECK(minish_solid(424,136));
    for (t=0;t<10;t++) sw_step(t==0 ? K_A : 0);
    CHECK(st.cut_count>0 && !minish_solid(424,136));
    {
        u32 hash=minish_hash();u16 screen=sw_checksum();
        CHECK(!sw_save_state("captures/action_state.bin"));
        sw_step(K_ENTER);CHECK(!st.cut_count);
        CHECK(!sw_load_state("captures/action_state.bin"));
        CHECK(minish_hash()==hash);game_render();CHECK(sw_checksum()==screen);
        CHECK(!minish_solid(424,136));
        remove("captures/action_state.bin");
    }
    sw_step(0);sw_step(K_ENTER);CHECK(!st.cut_count);
    sw_init(140);CHECK(minish_solid(424,136));
}

static void fixtures(void)
{
    FILE *f = fopen("fixtures/traversal.txt","r");
    unsigned trials, t, frames = 0, failed_trials = 0;
    CHECK(f != NULL); if (!f) return;
    CHECK(fscanf(f,"%u",&trials) == 1);
    for (t = 0; t < trials; t++) {
        unsigned sx, sy, n, i, bad = 0;
        CHECK(fscanf(f,"%u %u %u",&sx,&sy,&n) == 3);
        minish_place(sx,sy);
        for (i = 0; i < n; i++) {
            unsigned key, x, y, action, direction, contacts;
            CHECK(fscanf(f,"%u %u %u %u %u %u",&key,&x,&y,&action,&direction,&contacts) == 6);
            minish_step(key);
            if (st.x != (s32)x || st.y != (s32)y || st.direction != direction || st.collisions != contacts) {
                if (!bad) printf("trial %u frame %u key %u: native (%ld,%ld,d%u,c%x) source (%u,%u,d%u,c%x)\n",
                    t,i,key,(long)st.x,(long)st.y,st.direction,st.collisions,x,y,direction,contacts);
                bad++; failures++;
            }
            frames++;
        }
        if (bad) failed_trials++;
    }
    fclose(f);
    printf("original walking fixtures: %u trials / %u source steps, %u failing trials\n",trials,frames,failed_trials);
}

static void animation_fixtures(void)
{
    FILE *f = fopen("fixtures/animation.txt","r");
    unsigned trials,t,steps=0;
    CHECK(f != NULL); if (!f) return;
    CHECK(fscanf(f,"%u",&trials)==1);
    for (t=0;t<trials;t++) {
        unsigned key,n,i;
        CHECK(fscanf(f,"%u %u",&key,&n)==2);
        minish_place(320,184);
        for (i=0;i<n;i++) {
            unsigned pose;
            CHECK(fscanf(f,"%u",&pose)==1);
            st.x=320*256L;st.y=184*256L;
            minish_step(key);
            CHECK(st.display_pose==pose);
            steps++;
        }
        minish_step(0); minish_step(0);
        CHECK(st.display_pose==idle_pose[t]);
    }
    fclose(f);
    printf("original animation fixtures: %u displayed poses\n",steps);
}

static unsigned word_le(FILE *f)
{
    unsigned lo=fgetc(f),hi=fgetc(f);
    return lo|(hi<<8);
}

static void zoom_fixtures(void)
{
    FILE *f=fopen("fixtures/zoom_pixels.bin","rb");
    unsigned cases,t,x,y,pixels=0,bad=0;
    CHECK(f!=NULL);if (!f) return;
    cases=word_le(f);
    for (t=0;t<cases;t++) {
        unsigned scenario=word_le(f);
        sw_init(scenario); CHECK(sw_step(0));
        minish_zoom_render(); /* Compare scene pixels independently of HUD/flag. */
        for (y=0;y<RT_H;y++) for (x=0;x<RT_W;x++) {
            unsigned expected=fgetc(f);
            if (sw_level(x,y)!=expected) {
                if (!bad) printf("zoom scenario %u pixel (%u,%u): got %u, expected %u\n",scenario,x,y,sw_level(x,y),expected);
                bad++;
            }
            pixels++;
        }
    }
    CHECK(!bad);CHECK(fgetc(f)==EOF);fclose(f);
    printf("70-percent scene/actor oracle: %u pixels, %u differences\n",pixels,bad);
}

static void depth_fixtures(void)
{
    FILE *f=fopen("fixtures/depth.txt","r");
    unsigned trials,t,steps=0;
    CHECK(f != NULL); if (!f) return;
    CHECK(fscanf(f,"%u",&trials)==1);
    for (t=0;t<trials;t++) {
        unsigned x,y,key,n,i;
        CHECK(fscanf(f,"%u %u %u %u",&x,&y,&key,&n)==4);
        minish_place(x,y); minish_step(0);
        for (i=0;i<n;i++) {
            unsigned cover;
            CHECK(fscanf(f,"%u",&cover)==1);
            st.x=(s32)x<<8;st.y=(s32)y<<8; minish_step(key);
            CHECK(st.display_cover==cover);steps++;
        }
    }
    fclose(f);
    printf("original terrain/actor depth: %u source steps\n",steps);
}

int main(int argc, char **argv)
{
    unsigned i;
    sw_init(0); CHECK(st.ready);
    if (argc >= 4 && !strcmp(argv[1],"--hash")) {
        static SwScript script;
        if (argc > 4) sw_init(atoi(argv[4]));
        CHECK(!sw_load_script(&script,argv[2]));
        for (i = 0; i < (unsigned)atoi(argv[3]); i++) {
            sw_step(sw_script_keys(&script,i)); printf("%lu\n",(unsigned long)minish_hash());
        }
        return failures != 0;
    }
    fixtures();
    animation_fixtures();
    action_fixtures();
    action_pixels();
    zoom_fixtures();
    depth_fixtures();
    combat_fixtures();
    combat_pixels();
    roll_fixtures();
    effects_pixels();
    {
        static SwScript script;
        sw_init(0); CHECK(!sw_load_script(&script,"keys/opening.txt"));
        for (i = 0; i < 400; i++) sw_step(sw_script_keys(&script,i));
        CHECK(st.x >= 688*256L && st.x <= 696*256L);
        CHECK(st.y >= 132*256L && st.y <= 140*256L);
        printf("native opening endpoint: (%ld/256,%ld/256)\n",(long)st.x,(long)st.y);
    }
    sw_init(0);
    CHECK(st.x == 248*256L && st.y == 88*256L);
    CHECK(!minish_solid(248,88));
    CHECK(minish_solid(176,104));
    for (i = 0; i < 500; i++) sw_step(K_RIGHT | K_DOWN);
    CHECK(st.x >= 8*256L && st.x <= (WOODS_W-8)*256L);
    CHECK(st.y >= 12*256L && st.y <= (WOODS_H-4)*256L);
    CHECK(st.camx >= 0 && st.camx <= WOODS_W-RT_W);
    CHECK(st.camy >= 0 && st.camy <= WOODS_H-RT_H);
    sw_step(K_ENTER); CHECK(st.x == 248*256L && st.y == 88*256L);
    for (i = 0; i < 7; i++) { sw_init(i); CHECK(st.ready && sw_step(0)); }
    CHECK(!sw_step(K_ESC));
    printf("%s\n", failures ? "FAILED" : "all tests passed");
    return failures != 0;
}
