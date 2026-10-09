/* ti-cycles: runs a TI-89 NOSTUB program (.89z) on the PC under Musashi (68000 core with the
 * datasheet cycle counts, including movem, shifts and data-dependent mulu/muls), headless, and
 * reports the cycles of the zones the program marks. No calculator hardware: the program must
 * not touch the I/O ports (grayscale, interrupts); AMS ROM calls are emulated on the host (the
 * few a benchmark needs) and charged an estimated cost, reported apart.
 *
 * Usage: ti-cycles [options] prog.89z
 *   --arg N           value read by BENCH_ARG (the scenario number, say)
 *   --png FILE        at each BENCH_SHOT, write the 240x128 two-plane screen given to it as PNG
 *                     (FILE may contain %d: shot number)
 *   --max N           stop after N million cycles (default 2000)
 *   --trace-rom       log every ROM call
 *   --file F.89y      put the variable of F (any .89y/.89z) in the VAT, archived (repeatable)
 *   --save-dir D      at exit, write the variables the program created (SymAdd) as D/NAME.89y
 *   --keys F          input script for BENCH_KEYS, the PC format ("<frame> <keys...>" lines)
 *   --frames N        value read by BENCH_FRAMES (default -1: no limit)
 * Protocol (include tools/m68kbench/bench.h):  0xE00000..0xE000FF, word/long writes
 *   BENCH_NAME(id, "text")  names zone id (1..63)
 *   BENCH_BEGIN(id) / BENCH_END(id): accumulates the cycles of zone id
 *   BENCH_VALUE(v)          prints a long
 *   BENCH_SHOT(ptr)         PNG of the virtual screen at ptr (light plane, dark plane at +0xF00)
 *   BENCH_DARK_FIRST(f)     f != 0: the dark plane is at ptr, the light one at +0xF00 (GrayDBuf)
 *   BENCH_CYCLES            reads the cycle counter (long)
 *   BENCH_ARG               reads the --arg value (short)
 *   BENCH_KEYS(frame)       the rt.h key mask of that frame from --keys (0 without a script)
 *   BENCH_FRAMES            reads the --frames value (long)
 * AMS emulated for the Portable Game Runtime's RT_CYCLES build: the VAT (SymFindPtr, SymAdd,
 * DerefSym, EM_moveSym*), the fonts of OO_CondGetAttr (runtime/platform-sw/amsfont.h, extracted
 * from the TI OS; blank glyphs without it), FontGetSys/FontSetSys, GKeyFlush.
 * Each BENCH_SHOT also prints the checksum of the 160x100 view, as the PC --headless run does.
 * Output: one line per zone: id name calls total average (cycles), then ROM calls, then the
 * whole program's cycles. Exit status 0 when _main returned.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "m68k.h"
#if __has_include("amsfont.h")                 /* runtime/platform-sw, made by a runtime game's make */
#include "amsfont.h"
#else
static const unsigned char ams_f4x6[1536] = { 0 }, ams_f6x8[2048] = { 0 };
#endif

#define RAM_SIZE   0x400000u                 /* 4 MB flat, plenty (the TI has 256 KB) */
#define LOAD_ADDR  0x040000u                 /* program block (size word, then code) */
#define HEAP_ADDR  0x100000u                 /* host heap for HeapAlloc/malloc */
#define HEAP_END   0x380000u
#define STACK_TOP  0x03FFF0u
#define ROMTAB     0x3F0000u                 /* ROM call table, pointed to by 0xC8 */
#define ROMSTUB    0x3E0000u                 /* ROM call i = address ROMSTUB + 2 * i */
#define NROM       0x800
#define SENTINEL   0x3DFFF0u                 /* _main returns here */
#define SCRRECT    0x3DFF00u
#define EXC_ADDR   0x3DFFE0u                 /* every exception vector points here */
#define IO_BASE    0xE00000u
#define VAT_ADDR   0x3D0000u                 /* SYM_ENTRY table, 14 bytes each */
#define FONT_ADDR  0x3D8000u                 /* F_4x6 (6 bytes per glyph), then F_6x8 (8) */
#define ZERO_WORD  0x3DFF80u                 /* EV_runningApp: no running app */
#define NVAR       64

static uint8_t ram[RAM_SIZE];
static unsigned long long total;             /* cycles before the current timeslice */
static int done, trace_rom;
static long long max_cycles = 2000000000LL;
static const char *png_pattern;
static int shots, dark_first;
static uint32_t heap_ptr = HEAP_ADDR, handles[4096];   /* handle h -> address (h >= 1) */
static uint32_t handle_size[4096];
static int nhandles = 1;

static struct { char name[64]; unsigned long long sum, begin; long calls; int open; } zone[64];
static struct { long calls; unsigned long long cyc; } romstat[NROM];
static const char *romname[NROM];
static uint32_t pending_name_id;
static int io_warn;
static int nvars;
static int var_new[NVAR];                    /* created by the program (SymAdd): --save-dir */
static const char *save_dir;
static long bench_frames = -1;
static uint32_t key_frame;
static struct { int frame; uint32_t keys; } script[32768];
static int nscript;

static unsigned long long now(void) { return total + (unsigned long long)m68k_cycles_run(); }

/* ------------------------------------------------------------------------ memory */
static unsigned rd8(uint32_t a) { a &= 0xFFFFFF; return a < RAM_SIZE ? ram[a] : 0; }
static unsigned rd16(uint32_t a) { return rd8(a) << 8 | rd8(a + 1); }
static uint32_t rd32(uint32_t a) { return (uint32_t)rd16(a) << 16 | rd16(a + 2); }
static void wr8(uint32_t a, unsigned v) { a &= 0xFFFFFF; if (a < RAM_SIZE) ram[a] = v; }
static void wr16(uint32_t a, unsigned v) { wr8(a, v >> 8); wr8(a + 1, v); }
static void wr32(uint32_t a, uint32_t v) { wr16(a, v >> 16); wr16(a + 2, v); }

static void write_png(uint32_t base);

static void io_write(uint32_t a, uint32_t v, int size)
{
    unsigned off = a - IO_BASE;
    (void)size;
    switch (off) {
    case 0x00: if (v < 64) { zone[v].begin = now(); zone[v].open = 1; } break;   /* BEGIN */
    case 0x02: if (v < 64 && zone[v].open) {                                    /* END */
                   zone[v].sum += now() - zone[v].begin; zone[v].calls++; zone[v].open = 0; }
               break;
    case 0x04: printf("value: %ld (0x%lx)\n", (long)(int32_t)v, (unsigned long)v); break;
    case 0x08: pending_name_id = v; break;
    case 0x0C: if (getenv("TIC_DEBUG")) fprintf(stderr, "name %u at %06x\n", pending_name_id, v);
               if (pending_name_id < 64) {                                      /* NAME */
                   int i; for (i = 0; i < 63 && rd8(v + i); i++) zone[pending_name_id].name[i] = rd8(v + i);
                   zone[pending_name_id].name[i] = 0; }
               break;
    case 0x10: write_png(v); break;                                            /* SHOT */
    case 0x24: key_frame = v; break;                                           /* KEYS frame */
    case 0x14: done = 2; m68k_end_timeslice(); break;                          /* STOP */
    case 0x20: dark_first = v != 0; break;                                     /* PLANES */
    }
}

/* the keys of the last script line at or before frame f (rt_sw.c sw_script_keys) */
static uint32_t script_keys(uint32_t f)
{
    int i;
    uint32_t k = 0;
    for (i = 0; i < nscript && (uint32_t)script[i].frame <= f; i++) k = script[i].keys;
    return k;
}

unsigned int m68k_read_memory_8(unsigned int a)  { return rd8(a); }
static int bench_arg;
unsigned int m68k_read_memory_16(unsigned int a)
{
    if ((a & 0xFFFFFF) == IO_BASE + 0x1C) return bench_arg & 0xFFFF;                /* ARG */
    return rd16(a);
}
unsigned int m68k_read_memory_32(unsigned int a)
{
    if ((a & 0xFFFFFF) == IO_BASE + 0x18) return (uint32_t)now();              /* CYCLES */
    if ((a & 0xFFFFFF) == IO_BASE + 0x24) return script_keys(key_frame);       /* KEYS */
    if ((a & 0xFFFFFF) == IO_BASE + 0x28) return (uint32_t)bench_frames;       /* FRAMES */
    return rd32(a);
}
unsigned int m68k_read_disassembler_16(unsigned int a) { return rd16(a); }
unsigned int m68k_read_disassembler_32(unsigned int a) { return rd32(a); }
static void hw_write(uint32_t a, uint32_t v, int size)
{
    a &= 0xFFFFFF;
    if (a >= IO_BASE && a < IO_BASE + 0x100) { io_write(a, v, size); return; }
    if (a >= 0x600000 && a < 0x800000) { if (io_warn++ < 5) fprintf(stderr, "warning: write to I/O port %06x\n", a); return; }
    if (size == 1) wr8(a, v); else if (size == 2) wr16(a, v); else wr32(a, v);
}
void m68k_write_memory_8(unsigned int a, unsigned int v)  { hw_write(a, v, 1); }
void m68k_write_memory_16(unsigned int a, unsigned int v) { hw_write(a, v, 2); }
void m68k_write_memory_32(unsigned int a, unsigned int v) { hw_write(a, v, 4); }

/* ------------------------------------------------------------------------ ROM calls */
static uint32_t halloc(uint32_t size)
{
    uint32_t p = (heap_ptr + 2 + 1) & ~1u;      /* AMS blocks start with a size word */
    if (p + size > HEAP_END || nhandles >= 4096) return 0;
    wr16(p - 2, size > 0xFFFF ? 0xFFFF : size);
    handles[nhandles] = p; handle_size[nhandles] = size;
    heap_ptr = p + size;
    return nhandles++;
}

/* ------------------------------------------------------------------------ VAT */
static void sym_name(uint32_t symstr, char *name)       /* SYMSTR: a pointer to the final 0 */
{
    uint32_t p = symstr;
    int i = 0;
    while (p > 0 && rd8(p - 1) && symstr - p < 8) p--;
    while (p < symstr && i < 8) name[i++] = rd8(p++);
    name[i] = 0;
}

static int find_var(uint32_t symstr)
{
    char name[9];
    int v, i;
    sym_name(symstr, name);
    for (v = 0; v < nvars; v++) {
        for (i = 0; i < 8 && rd8(VAT_ADDR + 14 * v + i) == (uint8_t)name[i] && name[i]; i++) ;
        if ((i == 8 || !name[i]) && (i == 8 || !rd8(VAT_ADDR + 14 * v + i))) return v;
    }
    return -1;
}

static int add_var(const char *name, uint32_t handle)
{
    uint32_t e = VAT_ADDR + 14 * nvars;
    int i;
    for (i = 0; i < 8; i++) wr8(e + i, i < (int)strlen(name) ? name[i] : 0);
    wr16(e + 8, 0); wr16(e + 10, 0); wr16(e + 12, handle);                /* compat, flags, handle */
    return nvars++;
}

/* a .89y/.89z/... file: one variable, at the offset its entry gives; kept archived (read in place) */
static void load_var(const char *path)
{
    static uint8_t buf[1 << 17];
    FILE *f = fopen(path, "rb");
    long n;
    uint32_t off, size, h, i;
    char name[9];
    if (!f) { perror(path); exit(2); }
    n = fread(buf, 1, sizeof buf, f); fclose(f);
    if (n < 0x5A || memcmp(buf, "**TI89**", 8)) { fprintf(stderr, "%s: not a TI-89 file\n", path); exit(2); }
    off = buf[0x3C] | buf[0x3D] << 8 | buf[0x3E] << 16 | (uint32_t)buf[0x3F] << 24;
    size = buf[off + 4] << 8 | buf[off + 5];
    memcpy(name, buf + 0x40, 8); name[8] = 0;
    if (off + 6 + size > (uint32_t)n || nvars >= NVAR || !(h = halloc(2 + size))) { fprintf(stderr, "%s: cannot load\n", path); exit(2); }
    for (i = 0; i < 2 + size; i++) wr8(handles[h] + i, buf[off + 4 + i]);
    wr16(VAT_ADDR + 14 * add_var(name, h) + 10, 0x0200);                     /* archived */
}

/* the created variables, in the .89y layout of ttbin2oth (an OTH_TAG variable) */
static void save_vars(void)
{
    int v;
    for (v = 0; v < nvars; v++) {
        uint32_t e = VAT_ADDR + 14 * v, h = rd16(e + 12), p, size, i, sum;
        uint8_t hd[0x56] = { '*', '*', 'T', 'I', '8', '9', '*', '*', 1, 0, 'm', 'a', 'i', 'n' };
        char path[1024], name[9];
        FILE *f;
        if (!var_new[v] || !h || h >= (uint32_t)nhandles) continue;
        p = handles[h]; size = rd16(p);
        for (i = 0; i < 8; i++) name[i] = hd[0x40 + i] = rd8(e + i);
        name[8] = 0;
        hd[0x3A] = 1; hd[0x3C] = 0x52;
        hd[0x48] = 0x1C;                                                      /* OTH_TAG variable */
        i = 0x52 + 4 + 2 + size + 2;                                          /* file size */
        hd[0x4C] = i; hd[0x4D] = i >> 8; hd[0x4E] = i >> 16; hd[0x4F] = i >> 24;
        hd[0x50] = 0xA5; hd[0x51] = 0x5A;
        snprintf(path, sizeof path, "%s/%s.89y", save_dir, name);
        if (!(f = fopen(path, "wb"))) { perror(path); continue; }
        fwrite(hd, 1, 0x56, f);
        for (sum = 0, i = 0; i < 2 + size; i++) { uint8_t b = rd8(p + i); fputc(b, f); sum += b; }
        fputc(sum & 0xFF, f); fputc(sum >> 8 & 0xFF, f);
        fclose(f);
        fprintf(stderr, "ti-cycles: wrote %s (%u bytes)\n", path, size);
    }
}

static int load_script(const char *path)
{
    static const struct { const char *n; uint32_t k; } names[] = {
        { "UP", 1 }, { "LEFT", 2 }, { "DOWN", 4 }, { "RIGHT", 8 }, { "A", 0x10 }, { "B", 0x20 },
        { "C", 0x40 }, { "D", 0x80 }, { "ENTER", 0x100 }, { "ESC", 0x200 },
        { "F1", 0x400 }, { "F5", 0x800 },
    };
    FILE *f = fopen(path, "r");
    char line[256], w[16];
    if (!f) return -1;
    while (fgets(line, sizeof line, f) && nscript < 32768) {
        int fr, n, m;
        const char *s;
        uint32_t k = 0;
        if (sscanf(line, " %d%n", &fr, &n) != 1) continue;               /* blank or comment */
        for (s = line + n; sscanf(s, " %15s%n", w, &m) == 1; s += m) {
            size_t j;
            if (w[0] == '#') break;
            if (w[0] >= '1' && w[0] <= '9' && !w[1]) { k |= 0x8000u << (w[0] - '0'); continue; }
            for (j = 0; j < sizeof names / sizeof names[0] && strcmp(w, names[j].n); j++) ;
            if (j < sizeof names / sizeof names[0]) k |= names[j].k;
            else fprintf(stderr, "unknown key %s\n", w);
        }
        script[nscript].frame = fr; script[nscript++].keys = k;
    }
    fclose(f);
    return 0;
}

#define SP m68k_get_reg(NULL, M68K_REG_SP)
static uint32_t arg32(int off) { return rd32(SP + 4 + off); }
static uint32_t arg16(int off) { return rd16(SP + 4 + off); }

/* returns the estimated cycles of the call; -1 = not implemented */
static long rom_call(int n)
{
    uint32_t d0 = m68k_get_reg(NULL, M68K_REG_D0), d1 = m68k_get_reg(NULL, M68K_REG_D1);
    uint32_t r;
    switch (n) {
    case 0x26a: case 0x26b: {                                                  /* memcpy, memmove */
        uint32_t dst = arg32(0), src = arg32(4), len = arg32(8), i;
        if (dst < src) for (i = 0; i < len; i++) wr8(dst + i, rd8(src + i));
        else for (i = len; i--; ) wr8(dst + i, rd8(src + i));
        m68k_set_reg(M68K_REG_A0, dst); m68k_set_reg(M68K_REG_D0, dst);
        return 60 + len * 6; }                                                 /* ~23,400 per 3840 (TiEmu) */
    case 0x270: {                                                              /* memcmp */
        uint32_t a = arg32(0), b = arg32(4), len = arg32(8), i; int r = 0;
        for (i = 0; i < len && !r; i++) r = rd8(a + i) - rd8(b + i);
        m68k_set_reg(M68K_REG_D0, r); return 60 + len * 8; }
    case 0x27c: {                                                              /* memset(p, short c, len) */
        uint32_t dst = arg32(0), c = arg16(4) & 0xFF, len = arg32(6), i;
        for (i = 0; i < len; i++) wr8(dst + i, c);
        m68k_set_reg(M68K_REG_A0, dst);
        return 60 + len * 4; }                                                 /* ~15,900 per 3840 */
    case 0x27e: {                                                              /* strlen */
        uint32_t p = arg32(0), n = 0;
        while (rd8(p + n)) n++;
        m68k_set_reg(M68K_REG_D0, n);
        return 40 + n * 10; }
    case 0xa2: r = halloc(arg32(0)); m68k_set_reg(M68K_REG_A0, r ? handles[r] : 0); return 1000;   /* malloc */
    case 0x90: case 0x92: case 0x94:                                           /* HeapAlloc(High)(Throw) */
        r = halloc(arg32(0)); m68k_set_reg(M68K_REG_D0, r); return 1000;
    case 0x96: case 0x99:                                                      /* HeapDeref, HLock */
        r = arg16(0); m68k_set_reg(M68K_REG_A0, r < (uint32_t)nhandles ? handles[r] : 0); return 100;
    case 0x9a: m68k_set_reg(M68K_REG_D0, arg16(0)); return 100;               /* HeapLock */
    case 0x97: case 0x9f: case 0xa3: return 200;                               /* HeapFree, HeapUnlock, free */
    case 0x2a8: case 0x2aa: {                                                  /* _ds32s32, _du32u32: d1 /= d0 */
        if (!d0) return -1;
        r = n == 0x2a8 ? (uint32_t)((int32_t)d1 / (int32_t)d0) : d1 / d0;
        m68k_set_reg(M68K_REG_D1, r); return 460; }
    case 0x2a9: case 0x2ab: {                                                  /* _ms32s32, _mu32u32: d1 %= d0 */
        if (!d0) return -1;
        r = n == 0x2a9 ? (uint32_t)((int32_t)d1 % (int32_t)d0) : d1 % d0;
        m68k_set_reg(M68K_REG_D1, r); return 460; }
    case 0x6c: case 0x6d:                                                      /* SymFindFirst/Next: empty VAT */
        m68k_set_reg(M68K_REG_A0, 0); return 0;
    case 0x283: {                                                              /* SymFindPtr(SYMSTR, flags) */
        int v = find_var(arg32(0));
        m68k_set_reg(M68K_REG_A0, v < 0 ? 0 : VAT_ADDR + 14 * v); return 2000; }
    case 0x5c: {                                                               /* SymAdd(SYMSTR): HSym in d0 */
        uint32_t s = arg32(0);
        int v = find_var(s);
        if (v < 0 && nvars < NVAR) {
            char name[9];
            sym_name(s, name);
            v = add_var(name, 0);
        }
        if (v >= 0) var_new[v] = 1;
        m68k_set_reg(M68K_REG_D0, v < 0 ? 0 : 0x10000u | (14 * v)); return 3000; }   /* folder 1 */
    case 0x79:                                                                 /* DerefSym(HSym) */
        m68k_set_reg(M68K_REG_A0, VAT_ADDR + arg16(2)); return 100;
    case 0x161: case 0x162: {                                                  /* EM_moveSymFrom/ToExtMem */
        int v = find_var(arg32(0));
        if (v >= 0) {
            uint32_t f = VAT_ADDR + 14 * v + 10;
            wr16(f, n == 0x162 ? rd16(f) | 0x0200 : rd16(f) & ~0x0200);
        }
        m68k_set_reg(M68K_REG_D0, v >= 0); return 20000; }                     /* a Flash write: slow */
    case 0x3fa: {                                                              /* OO_CondGetAttr(frame, attr, &p) */
        uint32_t attr = arg32(4), p = arg32(8);
        if (attr != 0x300 && attr != 0x301) { m68k_set_reg(M68K_REG_D0, 0); return 200; }
        wr32(p, attr == 0x300 ? FONT_ADDR : FONT_ADDR + sizeof ams_f4x6);        /* OO_SFONT, OO_LFONT */
        m68k_set_reg(M68K_REG_D0, 1); return 200; }
    case 0x18e: m68k_set_reg(M68K_REG_D0, 1); return 50;                      /* FontGetSys: F_6x8 */
    case 0xb4: return 200;                                                     /* GKeyFlush */
    case 0x53: case 0x1a9: case 0x19e: case 0x18f: case 0x1a2: case 0x1a3:     /* sprintf, DrawStr, ClrScr, */
    case 0xe6: case 0x51:                                                      /* FontSetSys, Port*, ST_helpMsg, ngetchx */
        m68k_set_reg(M68K_REG_D0, 0); return 0;                                /* not emulated: no cost */
    }
    return -1;
}

static void rom_trap(uint32_t pc)
{
    int n = (pc - ROMSTUB) / 2;
    uint32_t sp = SP, ret = rd32(sp);
    long cyc = rom_call(n);
    if (cyc < 0) {
        fprintf(stderr, "ti-cycles: ROM call 0x%x (%s) not emulated (called from %06x)\n",
                n, romname[n] ? romname[n] : "?", ret);
        done = 3; m68k_end_timeslice(); return;
    }
    if (trace_rom) fprintf(stderr, "rom 0x%x from %06x\n", n, ret);
    romstat[n].calls++; romstat[n].cyc += cyc;
    total += cyc + 16;                              /* the rts */
    m68k_set_reg(M68K_REG_SP, sp + 4);
    m68k_set_reg(M68K_REG_PC, ret);
}

void instr_hook(unsigned int pc)
{
    pc &= 0xFFFFFF;
    if (pc >= ROMSTUB && pc < ROMSTUB + 2 * NROM) rom_trap(pc);
    else if (pc == SENTINEL) { done = 1; m68k_end_timeslice(); }
    else if (pc == EXC_ADDR) {
        uint32_t sp = SP;                    /* group 0 (bus/address error): 7 words; others: sr, pc */
        fprintf(stderr, "ti-cycles: CPU exception, stacked words %04x %04x %04x %04x %04x %04x %04x\n",
                rd16(sp), rd16(sp + 2), rd16(sp + 4), rd16(sp + 6), rd16(sp + 8), rd16(sp + 10), rd16(sp + 12));
        done = 4; m68k_end_timeslice();
    }
}

/* ------------------------------------------------------------------------ PNG (no zlib: stored) */
static uint32_t crc_table[256];
static uint32_t crc(uint32_t c, const uint8_t *p, size_t n)
{
    size_t i;
    if (!crc_table[1]) for (i = 0; i < 256; i++) { uint32_t k = i; int j; for (j = 0; j < 8; j++) k = k & 1 ? 0xEDB88320u ^ (k >> 1) : k >> 1; crc_table[i] = k; }
    for (i = 0; i < n; i++) c = crc_table[(c ^ p[i]) & 255] ^ (c >> 8);
    return c;
}
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t n)
{
    uint8_t h[8] = { n >> 24, n >> 16, n >> 8, n, type[0], type[1], type[2], type[3] };
    uint32_t c = crc(0xFFFFFFFFu, h + 4, 4); c = ~crc(c, data, n);
    fwrite(h, 1, 8, f); fwrite(data, 1, n, f);
    { uint8_t t[4] = { c >> 24, c >> 16, c >> 8, c }; fwrite(t, 1, 4, f); }
}
static void write_png(uint32_t base)
{
    enum { W = 240, H = 128 };
    static uint8_t raw[H * (W + 1)], z[H * (W + 1) + 64 + 6 * H];
    char name[512];
    uint32_t a = 1, b = 0, i, n = 0;
    int x, y;
    FILE *f;
    {                                           /* rt_sw.c sw_checksum: light then dark, 160x100 */
        unsigned s1 = 0, s2 = 0, p, o;
        for (p = 0; p < 2; p++)
            for (o = 0; o < 100 * 30; o++) {
                if (o % 30 >= 20) continue;
                s1 += rd8(base + (dark_first ? !p : p) * 0xF00 + o); if (s1 >= 255) s1 -= 255;
                s2 += s1; if (s2 >= 255) s2 -= 255;
            }
        printf("shot %d checksum %04X\n", shots, s2 << 8 | s1);
    }
    if (!png_pattern) { shots++; return; }
    for (y = 0; y < H; y++) {
        raw[y * (W + 1)] = 0;
        for (x = 0; x < W; x++) {
            int bit = 7 - (x & 7), o = y * 30 + (x >> 3);
            int l = rd8(base + o) >> bit & 1, d = rd8(base + 0xF00 + o) >> bit & 1;
            if (dark_first) { int t = l; l = d; d = t; }
            raw[y * (W + 1) + 1 + x] = 255 - (l * 85 + d * 170);      /* light 1/3, dark 2/3 */
        }
    }
    /* zlib stream of stored blocks, one per row */
    z[n++] = 0x78; z[n++] = 0x01;
    for (y = 0; y < H; y++) {
        uint16_t len = W + 1;
        z[n++] = y == H - 1; z[n++] = len & 255; z[n++] = len >> 8; z[n++] = ~len & 255; z[n++] = (uint16_t)~len >> 8;
        memcpy(z + n, raw + y * (W + 1), len); n += len;
    }
    for (i = 0; i < sizeof raw; i++) { a = (a + raw[i]) % 65521; b = (b + a) % 65521; }
    z[n++] = b >> 8; z[n++] = b; z[n++] = a >> 8; z[n++] = a;
    snprintf(name, sizeof name, png_pattern, shots++);
    if (!(f = fopen(name, "wb"))) { perror(name); return; }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    { uint8_t ihdr[13] = { 0, 0, 0, W, 0, 0, 0, H, 8, 0, 0, 0, 0 }; chunk(f, "IHDR", ihdr, 13); }
    chunk(f, "IDAT", z, n); chunk(f, "IEND", NULL, 0);
    fclose(f);
}

/* ------------------------------------------------------------------------ loader */
static uint32_t load_89z(const char *path)
{
    FILE *f = fopen(path, "rb");
    static uint8_t buf[1 << 17];
    long n;
    uint32_t size, i, base = LOAD_ADDR + 2;
    if (!f) { perror(path); exit(2); }
    n = fread(buf, 1, sizeof buf, f); fclose(f);
    if (n < 0x5A || memcmp(buf, "**TI89**", 8)) { fprintf(stderr, "%s: not a TI-89 file\n", path); exit(2); }
    size = buf[0x56] << 8 | buf[0x57];                  /* variable size word */
    wr16(LOAD_ADDR, size);
    for (i = 0; i < size; i++) wr8(base + i, buf[0x58 + i]);
    if (buf[0x58 + size - 1] != 0xF3) { fprintf(stderr, "%s: not an ASM program (tag %02x)\n", path, buf[0x58 + size - 1]); exit(2); }
    /* AMS relocation table, read backwards from the tag: (target, location) word pairs, 0 = end */
    {
        uint32_t p = base + size - 1, loc, tgt, nrel = 0;
        for (;;) {
            p -= 2; loc = rd16(p);
            if (!loc) break;
            p -= 2; tgt = rd16(p);
            wr32(base + loc, rd32(base + loc) + base + tgt);   /* the long holds 0: base + target */
            nrel++;
        }
        (void)nrel;
    }
    return base;
}

int main(int argc, char **argv)
{
    const char *prog = NULL, *files[16];
    int i, nfiles = 0;
    uint32_t entry, sp;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--arg") && i + 1 < argc) bench_arg = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--png") && i + 1 < argc) png_pattern = argv[++i];
        else if (!strcmp(argv[i], "--max") && i + 1 < argc) max_cycles = atoll(argv[++i]) * 1000000LL;
        else if (!strcmp(argv[i], "--trace-rom")) trace_rom = 1;
        else if (!strcmp(argv[i], "--file") && i + 1 < argc) files[nfiles++ & 15] = argv[++i];
        else if (!strcmp(argv[i], "--save-dir") && i + 1 < argc) save_dir = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) bench_frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--keys") && i + 1 < argc) { if (load_script(argv[++i])) { perror(argv[i]); return 2; } }
        else if (argv[i][0] == '-') { fprintf(stderr, "usage: ti-cycles [--arg N] [--png F] [--max Mcycles] [--trace-rom] [--file F.89y] [--save-dir D] [--keys F] [--frames N] prog.89z\n"); return 2; }
        else prog = argv[i];
    }
    if (!prog) { fprintf(stderr, "usage: ti-cycles [--arg N] [--png F] [--max Mcycles] [--trace-rom] [--file F.89y] [--save-dir D] [--keys F] [--frames N] prog.89z\n"); return 2; }
    romname[0x26a] = "memcpy"; romname[0x27c] = "memset"; romname[0x27e] = "strlen"; romname[0x2a8] = "_ds32s32";
    romname[0x2aa] = "_du32u32"; romname[0x90] = "HeapAlloc"; romname[0x99] = "HLock";
    romname[0x283] = "SymFindPtr"; romname[0x5c] = "SymAdd"; romname[0x162] = "EM_moveSymToExtMem";
    romname[0x3fa] = "OO_CondGetAttr"; romname[0xa2] = "malloc"; romname[0x96] = "HeapDeref";

    entry = load_89z(prog);
    /* AMS environment: ROM call table (count at -4), ScrRect (a TI-89 screen) */
    wr32(0xC8, ROMTAB);
    wr32(ROMTAB - 4, NROM);
    for (i = 0; i < NROM; i++) { wr32(ROMTAB + 4 * i, ROMSTUB + 2 * i); wr16(ROMSTUB + 2 * i, 0x4E75); }
    wr32(ROMTAB + 4 * 0x2f, SCRRECT); wr8(SCRRECT + 2, 159); wr8(SCRRECT + 3, 99);
    wr32(ROMTAB + 4 * 0x45d, ZERO_WORD);                                 /* EV_runningApp = 0 */
    for (i = 0; i < (int)sizeof ams_f4x6; i++) wr8(FONT_ADDR + i, ams_f4x6[i]);
    for (i = 0; i < (int)sizeof ams_f6x8; i++) wr8(FONT_ADDR + sizeof ams_f4x6 + i, ams_f6x8[i]);
    for (i = 0; i < nfiles && i < 16; i++) load_var(files[i]);
    wr16(SENTINEL, 0x4E71);
    wr16(EXC_ADDR, 0x4E71);
    for (i = 2; i < 64; i++) if (4 * i != 0xC8) wr32(4 * i, EXC_ADDR);   /* 0xC8: the ROM table */
    /* reset vectors, then _main(arg) called from the sentinel */
    sp = STACK_TOP;
    sp -= 4; wr32(sp, SENTINEL);
    wr32(0, sp); wr32(4, entry);

    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_set_instr_hook_callback(instr_hook);
    m68k_pulse_reset();
    m68k_set_reg(M68K_REG_SR, 0x2700);
    while (!done && (long long)total < max_cycles) total += m68k_execute(100000);
    if (!done) { fprintf(stderr, "ti-cycles: stopped after %lld cycles (PC %06x)\n", (long long)total, m68k_get_reg(NULL, M68K_REG_PC)); }

    printf("%-3s %-28s %8s %12s %10s\n", "id", "zone", "calls", "cycles", "average");
    for (i = 1; i < 64; i++)
        if (zone[i].calls)
            printf("%-3d %-28s %8ld %12llu %10llu\n", i, zone[i].name[0] ? zone[i].name : "-",
                   zone[i].calls, zone[i].sum, zone[i].sum / zone[i].calls);
    for (i = 0; i < NROM; i++)
        if (romstat[i].calls)
            printf("rom %-3x %-24s %8ld %12llu   (estimated)\n", i, romname[i] ? romname[i] : "", romstat[i].calls, romstat[i].cyc);
    if (save_dir) save_vars();
    printf("total %llu cycles, %s\n", total, done == 1 ? "_main returned" : done == 2 ? "BENCH_STOP" : done == 4 ? "CPU exception" : "not finished");
    return done == 1 || done == 2 ? 0 : 1;
}
