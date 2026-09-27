// Bytecode VM for game logic: a 17-opcode register VM (8 short registers per entity, byte
// operands, rel8 branches, YIELD, CALL native) runs the walker behaviour of behav.vms (assembled
// by vmasm.py) for 50 entities, against the same behaviour in native C (the state machine of
// coro.c). Dispatch by switch vs GCC computed goto; the script embedded in the program vs read in
// place from the archived data file "vmscr" (archived by this program on first run).
// Checks all variants give the native trajectory; cycles per entity update and per opcode.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "behav.h"

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
volatile unsigned short sink;

#define NENT 50
#define FRAMES 400
#define N ((unsigned long)NENT * FRAMES)
#define CHECK_FRAMES 300

enum { OP_HALT, OP_LDI, OP_MOV, OP_ADD, OP_SUB, OP_ADDI, OP_ANDI, OP_LDB, OP_JMP, OP_JZ, OP_JNZ,
       OP_DJNZ, OP_JLTI, OP_JEQ, OP_JLT, OP_YIELD, OP_CALL };
#define R(k) (*(short *)(v + (k)))          // k = register * 2, pre-scaled by the assembler

typedef struct { short x, y; unsigned short st; unsigned char dir, len, wait, i; } Ent;
typedef struct { unsigned short pc; short v[8]; } VEnt;   // v: x y dir len wait i tmp -
static const signed char dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };

static void nat_step(short *v) { v[0] += dx[v[2]]; v[1] += dy[v[2]]; }
static void (*const natives[1])(short *) = { nat_step };

// ---- native: the state machine from coro.c ----
enum { S_START, S_WALK, S_WAIT };
static __attribute__((noinline)) void fsm_update(Ent *e)
{
    switch (e->st) {
    case S_WAIT:
        if (--e->i) return;
        e->dir = (e->dir + 1) & 3;
        if (!e->dir && ++e->len > 8) e->len = 2;
        goto walk;
    case S_WALK:
        if (--e->i) break;
        e->i = e->wait; e->st = S_WAIT;
        return;
    default:
    walk:
        e->i = e->len; e->st = S_WALK;
        break;
    }
    e->x += dx[e->dir]; e->y += dy[e->dir];
}
static __attribute__((noinline)) void empty_update(Ent *e) { sink = e->st; }

// ---- switch interpreter (plain, and counting opcodes) ----
static unsigned long opcount;
#define VM_NAME vm_sw
#define VM_COUNT
#include "vmsw.h"
#undef VM_NAME
#undef VM_COUNT
#define VM_NAME vm_count
#define VM_COUNT opcount++
#include "vmsw.h"

// ---- computed-goto interpreter: no range check, one indirect jump per opcode ----
static __attribute__((noinline)) void vm_goto(VEnt *e, const unsigned char *base)
{
    static void *const lbl[17] = { &&op_halt, &&op_ldi, &&op_mov, &&op_add, &&op_sub, &&op_addi,
        &&op_andi, &&op_ldb, &&op_jmp, &&op_jz, &&op_jnz, &&op_djnz, &&op_jlti, &&op_jeq, &&op_jlt,
        &&op_yield, &&op_call };
    const unsigned char *pc = base + e->pc;
    char *v = (char *)e->v;
#define NEXT goto *lbl[*pc++]
    NEXT;
op_ldi:  R(pc[0]) = (signed char)pc[1]; pc += 2; NEXT;
op_mov:  R(pc[0]) = R(pc[1]); pc += 2; NEXT;
op_add:  R(pc[0]) += R(pc[1]); pc += 2; NEXT;
op_sub:  R(pc[0]) -= R(pc[1]); pc += 2; NEXT;
op_addi: R(pc[0]) += (signed char)pc[1]; pc += 2; NEXT;
op_andi: R(pc[0]) &= (signed char)pc[1]; pc += 2; NEXT;
op_ldb:  R(pc[0]) = (signed char)base[pc[2] + R(pc[1])]; pc += 3; NEXT;
op_jmp:  pc += 1 + (signed char)pc[0]; NEXT;
op_jz:   pc += 2; if (!R(pc[-2])) pc += (signed char)pc[-1]; NEXT;
op_jnz:  pc += 2; if (R(pc[-2])) pc += (signed char)pc[-1]; NEXT;
op_djnz: pc += 2; if (--R(pc[-2])) pc += (signed char)pc[-1]; NEXT;
op_jlti: pc += 3; if (R(pc[-3]) < (signed char)pc[-2]) pc += (signed char)pc[-1]; NEXT;
op_jeq:  pc += 3; if (R(pc[-3]) == R(pc[-2])) pc += (signed char)pc[-1]; NEXT;
op_jlt:  pc += 3; if (R(pc[-3]) < R(pc[-2])) pc += (signed char)pc[-1]; NEXT;
op_call: natives[*pc++](e->v); NEXT;
op_yield: e->pc = pc - base; return;
op_halt: e->pc = pc - 1 - base; return;
#undef NEXT
}

// ---- data ----
static Ent *nat;
static VEnt *vms[5];                     // sw pure, sw native, goto pure, goto native, sw archive
static const unsigned char *arch;        // script read in place from the archived "vmscr"

static void init(void)
{
    unsigned short k, j;
    for (k = 0; k < NENT; k++) {
        Ent *t = &nat[k];
        t->x = k * 3; t->y = k; t->st = 0; t->dir = k & 3;
        t->len = 2 + k % 5; t->wait = 1 + k % 3; t->i = 0;
        for (j = 0; j < 5; j++) {
            VEnt *w = &vms[j][k];
            w->pc = script[j & 1];                          // entry table: [0] pure, [1] native
            w->v[0] = t->x; w->v[1] = t->y; w->v[2] = t->dir;
            w->v[3] = t->len; w->v[4] = t->wait; w->v[5] = w->v[6] = w->v[7] = 0;
        }
    }
}

static void b_empty(void) { unsigned short f = FRAMES; while (f--) { Ent *e = nat; unsigned short n = NENT; while (n--) empty_update(e++); } }
static void b_nat(void)   { unsigned short f = FRAMES; while (f--) { Ent *e = nat; unsigned short n = NENT; while (n--) fsm_update(e++); } }
#define BVM(name, fun, j, img) static void name(void) { unsigned short f = FRAMES; \
    while (f--) { VEnt *e = vms[j]; unsigned short n = NENT; while (n--) fun(e++, img); } }
BVM(b_swp, vm_sw, 0, script)
BVM(b_swn, vm_sw, 1, script)
BVM(b_gop, vm_goto, 2, script)
BVM(b_gon, vm_goto, 3, script)
BVM(b_swa, vm_sw, 4, arch)

#define NB 7
static void (*const fn[NB])(void) = { b_empty, b_nat, b_swp, b_swn, b_gop, b_gon, b_swa };
static const char *const nm[NB] = { "loop+call", "native C", "switch", "switch+CALL", "goto", "goto+CALL", "switch arch" };

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long t[NB], s, ops[2];
    unsigned short k, f, j, same = 1, archived = 0, size = 0;
    SYM_ENTRY *se;
    char *blk = malloc(NENT * sizeof(Ent) + 5 * NENT * sizeof(VEnt));
    if (!blk) return;
    nat = (Ent *)blk;
    for (j = 0; j < 5; j++) vms[j] = (VEnt *)(blk + NENT * sizeof(Ent)) + j * NENT;
    ClrScr();
    se = SymFindPtr(SYMSTR("vmscr"), 0);
    if (se && !se->flags.bits.archived) {                  // first run: archive the data file
        EM_moveSymToExtMem(SYMSTR("vmscr"), HS_NULL);
        se = SymFindPtr(SYMSTR("vmscr"), 0);             // the SYM_ENTRY moved
    }
    if (!se) { printf("vmscr missing"); ngetchx(); free(blk); return; }
    archived = se->flags.bits.archived;
    arch = HeapDeref(se->handle);
    size = *(const unsigned short *)arch;
    arch += 2;                                            // skip the size word
    if (memcmp(arch, script, SCRIPT_SIZE)) same = 0;

    // correctness: every VM variant against native C, every entity, every frame
    init();
    for (f = 0; f < CHECK_FRAMES; f++)
        for (k = 0; k < NENT; k++) {
            Ent *e = &nat[k];
            fsm_update(e);
            vm_sw(&vms[0][k], script); vm_sw(&vms[1][k], script);
            vm_goto(&vms[2][k], script); vm_goto(&vms[3][k], script);
            vm_sw(&vms[4][k], arch);
            for (j = 0; j < 5; j++) {
                VEnt *w = &vms[j][k];
                if (w->v[0] != e->x || w->v[1] != e->y || w->v[2] != e->dir) same = 0;
            }
        }
    // opcode counts per update (pure / native-call scripts)
    init();
    for (j = 0; j < 2; j++) {
        opcount = 0;
        for (f = 0; f < FRAMES; f++) for (k = 0; k < NENT; k++) vm_count(&vms[j][k], script);
        ops[j] = opcount;
    }
    init();
    printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < NB; k++) {
        unsigned long best = 0xFFFFFFFF, r;
        short rep;
        for (rep = 0; rep < 2; rep++) {
            s = ticks; while (ticks == s); s = ticks;
            fn[k]();
            r = ticks - s; if (r < best) best = r;
        }
        t[k] = best;
    }
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "cyc/update  -call  cyc/op  (%u x %u)", NENT, FRAMES);
    for (k = 0; k < NB; k++) {
        unsigned long c = t[k] * 46875UL / N, w = c - t[0] * 46875UL / N;
        printf_xy(0, 7 + 7 * k, "%-11s %4lu %5lu", nm[k], c, w);
        if (k >= 2) {                                     // work cycles / opcodes per update
            unsigned long o = ops[k == 3 || k == 5];
            printf_xy(84, 7 + 7 * k, "%4lu", w * N / o);
        }
    }
    printf_xy(0, 58, "ops/update x100: pure %lu, CALL %lu", ops[0] * 100 / N, ops[1] * 100 / N);
    printf_xy(0, 65, "same trajectory: %s", same ? "yes" : "NO");
    printf_xy(0, 72, "vmscr archived %u size %u @%lx", archived, size, (unsigned long)arch);
    printf_xy(0, 79, "script %u bytes", (unsigned short)SCRIPT_SIZE);
    GKeyFlush();
    ngetchx();
    FontSetSys(F_6x8);
    free(blk);
}
