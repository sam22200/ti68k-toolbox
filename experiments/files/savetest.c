// Save file round trip: an OTH variable "tsave" with extension "sav", magic + version check.
// Each run reads the counter, increments it and writes the file back.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

#define EXT "sav"
#define MAGIC 0x54534156UL                  // 'TSAV'
typedef struct { unsigned long magic; unsigned short version, runs; } SAVE;
// data bytes after the size word: SAVE, 0, "sav", 0, OTH_TAG
#define DATA_LEN (sizeof(SAVE) + 1 + sizeof(EXT) + 1)

static short load(SAVE *out)
{
    SYM_ENTRY *se = SymFindPtr(SYMSTR("tsave"), 0);
    const unsigned char *p;
    if (!se) return 0;
    p = HeapDeref(se->handle);
    if (*(const unsigned short *)p != DATA_LEN || p[2 + DATA_LEN - 1] != OTH_TAG) return 0;
    memcpy(out, p + 2, sizeof(SAVE));
    return out->magic == MAGIC && out->version == 1;
}

static short save(const SAVE *in)
{
    SYM_ENTRY *se = SymFindPtr(SYMSTR("tsave"), 0);
    HANDLE h;
    HSym hs;
    unsigned char *p;
    if (se && se->flags.bits.archived)      // SymAdd would keep the archived flag: unarchive first
        EM_moveSymFromExtMem(SYMSTR("tsave"), HS_NULL);
    h = HeapAlloc(2 + DATA_LEN);
    if (h == H_NULL) return 0;
    hs = SymAdd(SYMSTR("tsave"));           // replaces an existing variable
    if (hs.folder == 0) { HeapFree(h); return 0; }
    DerefSym(hs)->handle = h;
    p = HeapDeref(h);
    *(unsigned short *)p = DATA_LEN;
    memcpy(p + 2, in, sizeof(SAVE));
    p += 2 + sizeof(SAVE);
    *p++ = 0;
    memcpy(p, EXT, sizeof(EXT));            // "sav" + its NUL
    p += sizeof(EXT);
    *p = OTH_TAG;
    return 1;
}

void _main(void)
{
    SAVE s;
    SYM_ENTRY *se = SymFindPtr(SYMSTR("tsave"), 0);
    short arch = se ? se->flags.bits.archived : -1;
    short ok = load(&s);
    if (!ok) { s.magic = MAGIC; s.version = 1; s.runs = 0; }
    s.runs++;
    ClrScr();
    printf_xy(0, 0, "load: %s", ok ? "ok" : "none/invalid");
    printf_xy(0, 10, "runs = %u", s.runs);
    printf_xy(0, 20, "save: %s", save(&s) ? "ok" : "FAILED");
    se = SymFindPtr(SYMSTR("tsave"), 0);
    printf_xy(0, 30, "archived before %d, after %d", arch, se ? se->flags.bits.archived : -1);
    ngetchx();
}
