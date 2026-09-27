#!/usr/bin/env python3
"""MC68000 datasheet cycle count of straight-line functions in a GCC .s file (no loops, no branches).

Usage: datasheet_cycles.py FILE.s REGEX     e.g. datasheet_cycles.py r1.s 'spr_bw_[0-9]+'
Prints the cycles of each matching function (up to its rts, included) and the average.
Only the instruction forms the compiled sprites use are known; others are reported and stop the count.
TiEmu counts these instructions correctly (no movem, no multi-bit shift): the point is to have a
hardware figure to compare with ExtGraph's datasheet count.
"""
import re, sys

# (mnemonic, source kind, destination kind) -> cycles; kinds: D (Dn), A (An), I (#imm),
# ind ((An)), d16 (d(An) / (d,An)), pre (-(An)), post ((An)+)
T = {}
for sz, (rw, rl) in {"b": (0, 0), "w": (0, 0), "l": (0, 1)}.items():
    L = sz == "l"
    T[("move", sz, "ind", "D")] = 12 if L else 8
    T[("move", sz, "d16", "D")] = 16 if L else 12
    T[("move", sz, "D", "ind")] = 12 if L else 8
    T[("move", sz, "D", "d16")] = 16 if L else 12
    T[("move", sz, "I", "ind")] = 20 if L else 12
    T[("move", sz, "I", "d16")] = 24 if L else 16
    for op in ("and", "or", "eor"):
        T[(op, sz, "I", "D")] = 16 if L else 8
        T[(op, sz, "I", "ind")] = 28 if L else 16
        T[(op, sz, "I", "d16")] = 32 if L else 20
T[("move", "l", "A", "pre")] = 14
T[("move", "l", "post", "A")] = 12
T[("lea", "l", "d16", "A")] = 8


def kind(o):
    o = o.strip()
    if o.startswith("#"): return "I"
    if re.fullmatch(r"%d\d", o): return "D"
    if re.fullmatch(r"%(a\d|sp)", o): return "A"
    if re.fullmatch(r"\((%a\d|%sp)\)", o): return "ind"
    if re.fullmatch(r"-\((%a\d|%sp)\)", o): return "pre"
    if re.fullmatch(r"\((%a\d|%sp)\)\+", o): return "post"
    if re.fullmatch(r"-?\d+\((%a\d)\)|\(-?\d+,%a\d\)", o): return "d16"
    return "?" + o


def main(path, rx):
    funcs, cur = {}, None
    for line in open(path):
        m = re.match(r"^([A-Za-z_]\w*):", line)
        if m:
            cur = m.group(1) if re.fullmatch(rx, m.group(1)) else None
            if cur: funcs[cur] = 0
            continue
        if not cur or not line.startswith("\t") or line.strip().startswith("."):
            continue
        ins = line.split(None, 1)
        mn = ins[0]
        if mn == "rts":
            funcs[cur] += 16
            cur = None
            continue
        base, _, sz = mn.partition(".")
        ops = re.split(r",(?![^(]*\))", ins[1].strip())
        key = (base, sz or "l", kind(ops[0]), kind(ops[1]))
        if key not in T:
            sys.exit(f"unknown form {key} in {cur}: {line.strip()}")
        funcs[cur] += T[key]
    for f, c in funcs.items():
        print(f"{f:12s} {c}")
    print(f"average {sum(funcs.values()) / len(funcs):.0f} over {len(funcs)} functions")


main(sys.argv[1], sys.argv[2])
