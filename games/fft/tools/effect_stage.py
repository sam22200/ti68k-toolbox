#!/usr/bin/env python3
"""Which ability dims the scene, named, with how long it dims for.

Earlier passes recorded that *an* effect dimmed without knowing which ability
played, and then a table was filled in as if every Holy Sword had been watched.
One measured effect is not eight. This reads the ability the acting unit just
used -- `+0x170` of its record, which oracle.py's AI trace already reports as
"last ability" -- at the moment the picture darkens.

The AI plays both sides (the player flag cleared every frame, as --turns does),
so nothing is driven by hand and nothing is assumed about menus.

usage: effect_stage.py STATE FRAMES [out_dir]
"""
import sys
import os

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '../../../.claude/skills/ti-port-ps1/scripts'))
sys.path.insert(0, HERE)
import battle  # noqa: E402
import core_audit  # noqa: E402
import management_data as md  # noqa: E402
import oracle  # noqa: E402
from psxrun import PSX  # noqa: E402

LAST_ABILITY = 0x170            # u16, per unit: what it just used
TURN = 0x8018f520


def main():
    state, frames = sys.argv[1], int(sys.argv[2])
    out = sys.argv[3] if len(sys.argv) > 3 else None
    if out:
        os.makedirs(out, exist_ok=True)
    names = md.names(battle.Disc(str(core_audit.DEFAULT_DISC)).bat, md.ABILITY_NAMES, 512)
    psx = PSX(str(core_audit.DEFAULT_DISC))
    psx.load(state)

    def ability_of(unit):
        a = psx.read(oracle.UNITS + oracle.UNIT_SIZE * unit + LAST_ABILITY, 2)
        return a, (names[a] if a < len(names) and names[a] else '?')

    # The baseline is the median of recent frames that are NOT in an effect, and it is
    # frozen while one plays. A first version raised it with max() every frame, so it
    # climbed during the effect and the exit condition went incoherent: the same Night
    # Sword measured 0.03 s, 98 s and 0.03 s. The metric was never the problem --
    # luminance separates a staged frame from a normal one cleanly, 32 against 55..60,
    # red wash included. The moving baseline was.
    from statistics import median
    recent = []
    dim_at = None
    seen = []
    for f in range(frames):
        for i in range(21):                      # the AI plays both sides
            a = oracle.UNITS + oracle.UNIT_SIZE * i + 5
            psx.write(a, psx.read(a) & ~0x08)
        psx.pressed = oracle.key('CIRCLE') if f % 150 < 4 else 0
        psx.run()
        if f % 2:
            continue
        im = psx.image()
        lum = sum(im.convert('L').resize((80, 60)).getdata()) / 4800.0
        if dim_at is None:
            recent.append(lum)
            del recent[:-60]
        if len(recent) < 20:
            continue
        bright = median(recent)
        if dim_at is None and lum < bright * 0.70:
            unit = psx.read(TURN)
            aid, name = ability_of(unit)
            dim_at = (f, lum, unit, aid, name)
            if out:
                im.save('%s/dim-%06d.png' % (out, f))
        elif dim_at is not None and lum > bright * 0.88:
            f0, l0, unit, aid, name = dim_at
            held = f - f0
            dim_at = None
            if held < 10:          # a one-frame dip is not a staged effect
                continue
            print('%-18s id 0x%03x  unit %2d  %3d frames  %.2f s' %
                  (name, aid, unit, held, held / 60.0), flush=True)
            seen.append(name)
    print('%d staged effects: %s' % (len(seen), ', '.join(seen)))


if __name__ == '__main__':
    main()
