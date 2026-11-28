#!/usr/bin/env python3
"""Report, from the decompilation, which native renderer each overlay carries.

All 110 EFFECT overlays load at the same address, 0x801c2500, so a routine
pointer in `g_battle_effect_file_ptrs` (0x801b50d0 in BATTLE.BIN, 198 entries)
is really an **offset into whichever overlay is loaded**. `target/effect.yaml`
then says which function sits at that offset in a given overlay, and which of
its own data symbols is that renderer's brightness table -- every overlay has
its own, at its own address.

That pair is what lets a family be wired to an effect without guessing:

    offset = g_battle_effect_file_ptrs[id] - 0x801c2500
    family, table = overlays[effect][offset]

Run it to refresh `effect_handlers.json`, which is committed so the decoder
does not need the decompilation checked out:

    effect_handlers.py ../../../sources/fft_decomp DISC.cue
"""
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
LOAD = 0x801C2500
FILE_PTRS = 0x801B50D0
FILE_PTR_COUNT = 198

# The five families transcribed in effect_mesh.py, and the width of the
# brightness table each one reads (effect.h's own array bounds).
FAMILIES = {
    'update_ring_stack_mesh_state': ('ring_stack', 'ring_stack_mesh_brightness_table', 9),
    'update_ring_mesh_state': ('ring_mesh', 'ring_mesh_brightness_table', 3),
    'update_summon_mesh_state': ('summon_mesh', 'summon_mesh_brightness_table', 9),
    'update_textured_quad_grid_4x4': ('quad_grid_4x4', 'gfx_brightness_table', 5),
    'update_polar_mesh_state': ('polar_mesh', 'polar_mesh_brightness_table', 6),
}


def table_for(suffix, data):
    """The overlay's own table for one family, by exact suffix.

    `ring_stack_mesh_brightness_table` ends in `ring_mesh_brightness_table`, so
    the ring mesh has to be matched on the whole suffix and not on a substring.
    """
    for name, off in data.items():
        if name.endswith(suffix) and not (suffix == 'ring_mesh_brightness_table'
                                          and name.endswith('stack_mesh_brightness_table')):
            return off
    return None


def parse(decomp):
    """{effect number: {routine offset: [family, table offset, width]}}."""
    text = (Path(decomp) / 'target' / 'effect.yaml').read_text()
    out = {}
    for block in text.split('---'):
        m = re.search(r'module: effect-e(\d+)', block)
        if not m:
            continue
        data = {name: int(addr, 16) - LOAD
                for addr, name in re.findall(r'\{addr: (0x[0-9a-f]+), name: (g_effect_\w+)\}', block)}
        found = {}
        for addr, name in re.findall(r'\{addr: (0x[0-9a-f]+), size: \d+, name: effect_e\d+_(\w+?), hash', block):
            if name not in FAMILIES:
                continue
            family, suffix, width = FAMILIES[name]
            table = table_for(suffix, data)
            if table is None:
                continue
            found[int(addr, 16) - LOAD] = [family, table, width]
        if found:
            out[int(m.group(1))] = found
    return out


def routines(cue):
    """`g_battle_effect_file_ptrs`, as offsets into the loaded overlay."""
    sys.path.insert(0, str(HERE))
    import battle
    d = battle.Disc(cue)
    at = FILE_PTRS - battle.BAT
    return [v - LOAD for v in struct.unpack_from('<%dI' % FILE_PTR_COUNT, d.bat, at)]


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    out = dict(routines=routines(sys.argv[2]), overlays=parse(sys.argv[1]))
    path = HERE / 'effect_handlers.json'
    path.write_text(json.dumps(out, indent=1, sort_keys=True) + '\n')
    n = sum(len(v) for v in out['overlays'].values())
    print('%s: %d handlers over %d overlays' % (path.name, n, len(out['overlays'])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
