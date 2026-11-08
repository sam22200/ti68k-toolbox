// Event log readable from the live emulator without screenshots (tools/bin/ti-mem log).
// Opt-in: entries go to the unarchived variable "tilog" only when it exists (send the
// tilog.89y made by `ti-mem mkvar`); without it every call returns at once, so the
// calls can stay in release builds. The variable is a ring allocated once: logging never
// allocates or moves heap memory, so it cannot disturb what it observes.
// Layout after AMS's size word: "TLG1", u16 head (next write offset), u16 capacity,
// u16 wraps, then capacity bytes of '\n'-terminated text. On the PC, entries go to stderr
// when the environment has TILOG set.
#ifndef TILOG_H
#define TILOG_H
#include <stdarg.h>
#ifdef __m68k__
static void tilog(const char *fmt, ...)
{
    char line[96];va_list ap;unsigned short n,i,head,cap;unsigned char *p;
    SYM_ENTRY *e=SymFindPtr(SYMSTR("tilog"),0);
    if(!e||e->flags.bits.archived)return;
    p=(unsigned char *)HeapDeref(e->handle)+2;
    if(p[0]!='T'||p[1]!='L'||p[2]!='G'||p[3]!='1')return;
    va_start(ap,fmt);vsprintf(line,fmt,ap);va_end(ap);
    n=(unsigned short)strlen(line);line[n++]='\n';
    head=*(unsigned short *)(p+4);cap=*(unsigned short *)(p+6);
    for(i=0;i<n;i++){
        p[10+head]=(unsigned char)line[i];
        if(++head>=cap){head=0;(*(unsigned short *)(p+8))++;}
    }
    *(unsigned short *)(p+4)=head;
}
#else
#include <stdio.h>
#include <stdlib.h>
static void tilog(const char *fmt, ...)            // PC: only with TILOG set (tests compare output)
{
    va_list ap;if(!getenv("TILOG"))return;
    va_start(ap,fmt);fputs("tilog: ",stderr);vfprintf(stderr,fmt,ap);fputc('\n',stderr);va_end(ap);
}
#endif
#endif
