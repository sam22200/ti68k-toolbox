#ifndef FFT_STORY_CHOICE_FILE_H
#define FFT_STORY_CHOICE_FILE_H
#ifndef __m68k__
#include <stdio.h>
#ifndef SC_FILE_WRITE
#define SC_FILE_WRITE fwrite
#endif
#ifndef SC_FILE_CLOSE
#define SC_FILE_CLOSE fclose
#endif
#ifndef SC_FILE_RENAME
#define SC_FILE_RENAME rename
#endif
// PC session replacement: short writes/close/rename failure preserve the old
// record. The temporary file is in the same directory/filesystem as the save.
static u8 story_choice_store_file(const Save *s)
{
    FILE *f=fopen(SAVE_SESSION_NAME ".tmp","wb");u8 ok;
    if(!f)return 0;
    ok=SC_FILE_WRITE(s,1,sizeof *s,f)==sizeof *s;
    if(SC_FILE_CLOSE(f))ok=0;
    if(ok)ok=!SC_FILE_RENAME(SAVE_SESSION_NAME ".tmp",SAVE_SESSION_NAME ".sav");
    if(!ok)remove(SAVE_SESSION_NAME ".tmp");
    return ok;
}
#undef SC_FILE_WRITE
#undef SC_FILE_CLOSE
#undef SC_FILE_RENAME
#endif
#endif
