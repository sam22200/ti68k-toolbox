#!/bin/sh
# usage: sh bench_proj.sh [-DSP_PROFILE]: datasheet cycles per zone, Gariland then Orbonne.
# The printed checksums must equal those of x/proj/<map><rot>.raw (rotations 0,1,2,3,0).
R=../../..
$R/tools/bin/ti-cc -o x/spprof -DRT_CYCLES "$@" bench_proj.c proj.c proj68k.s bank.c $R/runtime/core/rt_core.c $R/runtime/platform-ti68k/rt_ti.c $R/lib/unpack68k.s $R/tools/extgraph/lib/tilemap.a >/dev/null 2>&1 || { echo build failed; exit 1; }
for a in 0 1; do $R/tools/bin/ti-cycles --frames 1 --max 1000 --arg $a --file x/proj/spgeo.89y --file x/proj/spmgar.89y --file x/proj/spmorb.89y x/spprof.89z | grep -E '^ ?(1[0-4]|2[0-9]) |value' | awk '/value/{v=v" "$2;next}{print}END{print "values"v}'; done
