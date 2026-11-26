// The end of a chapter and the start of the next one, through the real fp_story.
//
// Stub children and in-memory progress: this checks what the story driver decides --
// the prerequisite gate, when the handover is announced, and that an uninstalled
// destination returns to the map instead of ending the program. What the map draws
// is check_pilot's job, on the real worldmap.c.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pack_story.h"
#include "save_progress.h"

typedef struct { char name[9]; unsigned char *p; unsigned short n; } File;
static File cache[96]; static unsigned char count;
static char dir[512];
static FpProgress progress_state;
static unsigned short map_reply[4], map_arg[8];
static unsigned char map_replies, map_next, map_calls, scene_flag;
static unsigned checks;

static const unsigned char *read_file(const char *name, unsigned short *size)
{
    unsigned char i; char path[768]; FILE *f; long n;
    for (i = 0; i < count; i++) if (!strcmp(cache[i].name, name)) { *size = cache[i].n; return cache[i].p; }
    snprintf(path, sizeof path, "%s/%s.payload", dir, name); f = fopen(path, "rb"); if (!f) return 0;
    fseek(f, 0, SEEK_END); n = ftell(f); rewind(f); assert(n > 0 && n < 65536 && count < 96);
    i = count++; strcpy(cache[i].name, name); cache[i].p = malloc((size_t)n); assert(cache[i].p);
    cache[i].n = (unsigned short)n; assert(fread(cache[i].p, 1, (size_t)n, f) == (size_t)n); fclose(f);
    *size = (unsigned short)n; return cache[i].p;
}
static void clear_files(void) { while (count) free(cache[--count].p); }
static void set_flag(unsigned short plus_one)
{ if (plus_one) progress_state.flags[(plus_one - 1) >> 3] |= (unsigned char)(1 << ((plus_one - 1) & 7)); }
static void read_progress(FpProgress *s) { *s = progress_state; }
static void reset(void) { memset(&progress_state, 0, sizeof progress_state); }

static unsigned short call(unsigned char program, unsigned short arg)
{
    if (program == FP_TITLE) return arg == 3 ? 3 : 1;      // New Game / Continue / saved
    if (program == FP_SCENE) { set_flag(scene_flag); return 1; }
    if (program == FP_MAP) {
        assert(map_calls < 8); map_arg[map_calls++] = arg;
        return map_next < map_replies ? map_reply[map_next++] : 0;
    }
    return 1;
}

#define CHECK(c) do { checks++; assert(c); } while (0)

static unsigned char run(const char *folder, const unsigned char *flags, unsigned char flag_scene,
                         const unsigned short *replies, unsigned char n, FpNotice *notice)
{
    unsigned char i;
    clear_files(); snprintf(dir, sizeof dir, "%s", folder);
    memset(&progress_state, 0, sizeof progress_state);
    if (flags) memcpy(progress_state.flags, flags, 32);
    scene_flag = flag_scene; map_calls = map_next = 0; map_replies = n;
    for (i = 0; i < n; i++) map_reply[i] = replies[i];
    memset(map_arg, 0, sizeof map_arg);
    return fp_story(read_file, call, read_progress, reset, 0, notice);
}

int main(int argc, char **argv)
{
    FpNotice notice; unsigned char done[32]; static const unsigned short quit[1] = { 0 };
    static const unsigned short stray[2] = { 200, 0 };
    char one[600], two[600];
    assert(argc == 3);
    snprintf(one, sizeof one, "%s", argv[1]); snprintf(two, sizeof two, "%s", argv[2]);
    memset(done, 0, sizeof done); done[0] = 2;             // Chapter I's completion flag (2)

    // A. Chapter II on a save that has not finished Chapter I: refused, and it names
    //    the chapter to install first rather than starting on a save it cannot use.
    CHECK(run(two, NULL, 3, quit, 0, &notice) == FP_PREVIOUS);
    CHECK(notice.requested == 1);
    CHECK(!map_calls);

    // B. Chapter II on a finished Chapter I: it runs, its scene completes it, and the
    //    map is told so on its very first call. No chapter follows, so FP_MAP_LAST.
    CHECK(run(two, done, 3, quit, 1, &notice) == FP_OK);
    CHECK(map_calls == 1);
    CHECK(map_arg[0] == FP_MAP_HANDOFF + FP_MAP_CHAPTER * FP_MAP_LAST);

    // C. A destination whose band this pack does not carry does not end the program:
    //    the map is reopened with the panel, and the player stays where they were.
    CHECK(run(two, done, 3, stray, 2, &notice) == FP_OK);
    CHECK(map_calls == 2);
    CHECK(map_arg[0] == FP_MAP_HANDOFF + FP_MAP_CHAPTER * FP_MAP_LAST);
    CHECK(map_arg[1] == FP_MAP_HANDOFF + FP_MAP_CHAPTER * FP_MAP_LAST);

    // D. Chapter I reopened on a save that already finished it: the handover is not
    //    announced again, but the chapter number still rides the call for the marker.
    CHECK(run(one, done, 1, quit, 1, &notice) == FP_OK);
    CHECK(map_calls == 1);
    CHECK(map_arg[0] == FP_MAP_CHAPTER * 2);

    clear_files();
    printf("handoff %u checks passed\n", checks);
    return 0;
}
