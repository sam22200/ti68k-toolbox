/* Preloaded into TiEmu (Dockerfile.tiemu): lets the host user read the emulator's memory
   (/proc/PID/mem) despite Yama ptrace_scope=1, so tools/bin/ti-mem can dump the emulated
   calculator RAM (logs, VAT, LCD) without screenshots. Same uid only; nothing else changes. */
#include <sys/prctl.h>
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif
#ifndef PR_SET_PTRACER_ANY
#define PR_SET_PTRACER_ANY ((unsigned long)-1)
#endif
__attribute__((constructor)) static void allow_reader(void)
{
    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
}
