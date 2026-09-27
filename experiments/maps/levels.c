// Level-encoding ideas of KB §11-12, measured:
// 1. Object-coded level (Super Mario Bros. style): a 256x16 platformer level stored as 3-byte
//    objects (type | length, x delta, y | height) decoded on load into a byte grid, with an
//    auto-tiling pass for pipe and ground edges. Cycles per decode, checksum; the host build
//    writes the grid to level.bin so tools/sizes.py can compare with RLE / LZ4 / ZX0.
// 2. Seeded rooms (Elite style): a 20x12 room rebuilt from a 16-bit seed with wyhash16
//    (walls, doors from seed bits, pillars, loot). Same seed twice = same room (checksum), cycles
//    per room, 2 bytes stored instead of 240.
// Host: gcc -O2 -Wall -o levels levels.c && ./levels      TI: ti-cc -o levels levels.c
#include "../ai/ai.h"

// ------------------------------------------------------------------ object-coded level
#define LW 256
#define LH 16
enum { T_SKY, T_GROUND, T_GTOP, T_BRICK, T_QBLOCK, T_PIPE_L, T_PIPE_R, T_PIPE_TL, T_PIPE_TR, T_COIN, T_STEP };
enum { O_GROUND, O_GAP, O_BRICKS, O_QBLOCKS, O_PIPE, O_COINS, O_STAIRS, O_PLATFORM };
// type << 4 | (length - 1), x delta from the previous object, y << 4 | height (pipes, stairs)
static const u8 level[] = {
    O_GROUND << 4 | 15, 0, 14 << 4 | 2,      O_QBLOCKS << 4 | 0, 16, 9 << 4,
    O_BRICKS << 4 | 4, 4, 9 << 4,            O_QBLOCKS << 4 | 0, 2, 5 << 4,
    O_GROUND << 4 | 15, 2, 14 << 4 | 2,      O_PIPE << 4 | 1, 6, 12 << 4 | 2,
    O_PIPE << 4 | 1, 10, 11 << 4 | 3,        O_GROUND << 4 | 15, 6, 14 << 4 | 2,
    O_PIPE << 4 | 1, 8, 10 << 4 | 4,         O_COINS << 4 | 5, 6, 8 << 4,
    O_GROUND << 4 | 15, 10, 14 << 4 | 2,     O_PIPE << 4 | 1, 4, 10 << 4 | 4,
    O_GAP << 4 | 2, 12, 14 << 4 | 2,         O_GROUND << 4 | 15, 3, 14 << 4 | 2,
    O_BRICKS << 4 | 2, 8, 9 << 4,            O_QBLOCKS << 4 | 0, 3, 9 << 4,
    O_BRICKS << 4 | 7, 1, 5 << 4,            O_GROUND << 4 | 15, 13, 14 << 4 | 2,
    O_PLATFORM << 4 | 5, 6, 7 << 4,          O_COINS << 4 | 3, 1, 6 << 4,
    O_GROUND << 4 | 15, 9, 14 << 4 | 2,      O_STAIRS << 4 | 3, 6, 13 << 4 | 4,
    O_STAIRS << 4 | 3, 6, 13 << 4 | 4,       O_GROUND << 4 | 15, 4, 14 << 4 | 2,
    O_QBLOCKS << 4 | 2, 6, 9 << 4,           O_PIPE << 4 | 1, 10, 12 << 4 | 2,
    O_GROUND << 4 | 15, 6, 14 << 4 | 2,      O_BRICKS << 4 | 3, 4, 9 << 4,
    O_COINS << 4 | 3, 0, 8 << 4,             O_GROUND << 4 | 15, 12, 14 << 4 | 2,
    O_STAIRS << 4 | 7, 4, 13 << 4 | 8,       O_GROUND << 4 | 15, 16, 14 << 4 | 2,
    O_PIPE << 4 | 1, 6, 12 << 4 | 2,         O_GROUND << 4 | 15, 10, 14 << 4 | 2,
    O_PLATFORM << 4 | 3, 4, 8 << 4,          O_COINS << 4 | 2, 1, 7 << 4,
};
#define NOBJ (sizeof(level) / 3)
static u8 grid[LH][LW];

static void hrun(u16 y, u16 x, u16 n, u8 t) { u8 *p = &grid[y][x]; if (x + n > LW) n = LW - x; while (n--) *p++ = t; }

static void decode_level(void)
{
    const u8 *o = level;
    u16 k, x = 0;
    memset(grid, T_SKY, sizeof(grid));
    for (k = 0; k < NOBJ; k++, o += 3) {
        u16 type = o[0] >> 4, len = (o[0] & 15) + 1, y = o[2] >> 4, h = o[2] & 15, j;
        x += o[1];
        if (x >= LW) break;
        switch (type) {
        case O_GROUND: for (j = 0; j < h; j++) hrun(y + j, x, len, T_GROUND); break;
        case O_GAP:    for (j = 0; j < h; j++) hrun(y + j, x, len, T_SKY); break;
        case O_BRICKS: hrun(y, x, len, T_BRICK); break;
        case O_QBLOCKS: hrun(y, x, len, T_QBLOCK); break;
        case O_COINS:  hrun(y, x, len, T_COIN); break;
        case O_PLATFORM: hrun(y, x, len, T_BRICK); break;
        case O_PIPE:
            grid[y][x] = T_PIPE_TL; grid[y][x + 1] = T_PIPE_TR;
            for (j = 1; j < h; j++) { grid[y + j][x] = T_PIPE_L; grid[y + j][x + 1] = T_PIPE_R; }
            break;
        case O_STAIRS:                              // len columns rising 1 per column up to h
            for (j = 0; j < len; j++) {
                u16 hh = j + 1 > h ? h : j + 1, i;
                for (i = 0; i < hh; i++) grid[y - i][x + j] = T_STEP;
            }
            break;
        }
    }
    // auto-tiling: ground with sky above becomes a top tile
    for (k = 1; k < LH; k++) {
        u8 *p = grid[k], *q = grid[k - 1];
        u16 n = LW;
        while (n--) { if (*p == T_GROUND && *q == T_SKY) *p = T_GTOP; p++; q++; }
    }
}

// ------------------------------------------------------------------ seeded rooms
#define RW 20
#define RH 12
static u8 room[RH][RW];
enum { R_FLOOR, R_WALL, R_DOOR, R_PILLAR, R_LOOT };
static void make_room(u16 seed)
{
    u16 k, n;
    wy = seed;                                      // the room is a pure function of its seed
    memset(room, R_FLOOR, sizeof(room));
    memset(room[0], R_WALL, RW); memset(room[RH - 1], R_WALL, RW);
    for (k = 1; k < RH - 1; k++) room[k][0] = room[k][RW - 1] = R_WALL;
    if (seed & 1) room[0][RW / 2] = R_DOOR;         // doors: bit fields of the seed
    if (seed & 2) room[RH - 1][RW / 2] = R_DOOR;
    if (seed & 4) room[RH / 2][0] = R_DOOR;
    if (seed & 8) room[RH / 2][RW - 1] = R_DOOR;
    n = 2 + RND(6);                                 // pillars, 2x2, symmetric
    while (n--) {
        u16 x = 2 + RND(RW / 2 - 4), y = 2 + RND(RH - 5);
        room[y][x] = room[y + 1][x] = room[y][x + 1] = room[y + 1][x + 1] = R_PILLAR;
        room[y][RW - 2 - x] = room[y + 1][RW - 2 - x] = room[y][RW - 1 - x] = room[y + 1][RW - 1 - x] = R_PILLAR;
    }
    n = RND(4);                                     // loot on free floor cells
    while (n--) {
        u16 x = 1 + RND(RW - 2), y = 1 + RND(RH - 2);
        if (room[y][x] == R_FLOOR) room[y][x] = R_LOOT;
    }
}

static u16 fletcher16(const u8 *p, u16 n)
{
    u16 a = 0, b = 0;
    while (n--) { a += *p++; if (a >= 255) a -= 255; b += a; if (b >= 255) b -= 255; }
    return b << 8 | a;
}

#ifdef __m68k__
void _main(void)
#else
int main(void)
#endif
{
    u16 lsum, rsum = 0, rsum2 = 0, k, r1;
    u32 tl = 0, tr = 0;
    decode_level();
    lsum = fletcher16(&grid[0][0], sizeof(grid));
    for (k = 0; k < 64; k++) { make_room(k * 1237); rsum += fletcher16(&room[0][0], sizeof(room)); }
    for (k = 0; k < 64; k++) { make_room(k * 1237); rsum2 += fletcher16(&room[0][0], sizeof(room)); }
    make_room(4321); r1 = fletcher16(&room[0][0], sizeof(room));
#ifdef __m68k__
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    timer_on();
    tstart(); for (k = 0; k < 32; k++) decode_level(); tl = TICKS();
    tstart(); for (k = 0; k < 256; k++) make_room(k * 1237); tr = TICKS();
    timer_off();
    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "object level 256x16: %u objects = %u B", (u16)NOBJ, (u16)sizeof(level));
    printf_xy(0, 7, "decode + autotile: %lu cyc, sum %04X", CYC(tl) / 32, lsum);
    printf_xy(0, 17, "seeded room 20x12: %lu cyc, 2 B vs 240 B", CYC(tr) / 256);
    printf_xy(0, 24, "64 seeds twice: %04X %04X %s, 4321: %04X", rsum, rsum2, rsum == rsum2 ? "same" : "DIFF", r1);
    printf_xy(0, 34, "keys: level (left part), room 4321");
    GKeyFlush(); ngetchx();
    {                                               // views: one pixel per cell, 4x4 blocks
        short x, y;
        ClrScr();
        for (y = 0; y < LH; y++) for (x = 0; x < 160 / 4 && x < LW; x++)
            if (grid[y][x] != T_SKY) { short i, j; for (i = 0; i < 3; i++) for (j = 0; j < 3; j++)
                if (grid[y][x] != T_COIN || (i == 1 && j == 1)) DrawPix(x * 4 + j, y * 4 + i + 20, A_NORMAL); }
        GKeyFlush(); ngetchx();
        ClrScr();
        make_room(4321);
        for (y = 0; y < RH; y++) for (x = 0; x < RW; x++) {
            static const char ch[5] = { '.', '#', 'D', 'O', '$' };
            char s[2] = { ch[room[y][x]], 0 };
            DrawStr(x * 6 + 20, y * 8 + 2, s, A_NORMAL);
        }
        GKeyFlush(); ngetchx();
    }
    FontSetSys(F_6x8);
#else
    {
        FILE *f = fopen("level.bin", "wb");
        fwrite(grid, 1, sizeof(grid), f); fclose(f);
    }
    printf("level: %u objects = %u B, grid %u B, sum %04X\n", (unsigned)NOBJ, (unsigned)sizeof(level), (unsigned)sizeof(grid), lsum);
    printf("rooms: %04X %04X %s, 4321: %04X\n", rsum, rsum2, rsum == rsum2 ? "same" : "DIFF", r1);
    (void)tl; (void)tr;
    return 0;
#endif
}
