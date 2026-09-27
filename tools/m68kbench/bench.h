/* bench.h: markers for tools/bin/ti-cycles (the PC 68000 cycle counter). Only for programs run
 * under ti-cycles: on a calculator 0xE00000 is Flash, the writes do nothing useful. */
#ifndef TI_BENCH_H
#define TI_BENCH_H
#define BENCH_IO(off, type) (*(type volatile *)(0xE00000 + (off)))   /* the lvalue itself volatile */
#define BENCH_BEGIN(id)     (BENCH_IO(0x00, short) = (id))
#define BENCH_END(id)       (BENCH_IO(0x02, short) = (id))
#define BENCH_VALUE(v)      (BENCH_IO(0x04, long) = (long)(v))
#define BENCH_NAME(id, s)   (BENCH_IO(0x08, long) = (id), BENCH_IO(0x0C, const char *) = (s))
#define BENCH_SHOT(p)       (BENCH_IO(0x10, const void *) = (p))
#define BENCH_DARK_FIRST(f) (BENCH_IO(0x20, long) = (f))
#define BENCH_STOP()        (BENCH_IO(0x14, long) = 0)
#define BENCH_CYCLES        BENCH_IO(0x18, unsigned long)
#define BENCH_ARG           BENCH_IO(0x1C, short)
#define BENCH_KEYS(frame)   (BENCH_IO(0x24, long) = (frame), BENCH_IO(0x24, unsigned long))
#define BENCH_FRAMES        BENCH_IO(0x28, long)
#endif
