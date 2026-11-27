#!/usr/bin/env python3
"""Watch the original play an ability, and measure what it does to the picture.

Needs a Gariland state at Ramza's turn, the one `oracle.py DISC.cue OUT.state`
builds. This reuses oracle.py's own menu driving -- the sequences there are tuned
to that battle and that state, and replaying them anywhere else does not line up.

Two confirmations are needed to execute, not one: the targeting taps raise an
"Executing action. OK?" prompt and a second CIRCLE runs it. Capturing after the
first one photographs the prompt, which is a still screen.

Measuring on a still screen is how a wrong answer gets a clean number. A first
run of this reported "the game displays at 30 Hz" from perfectly uniform 2-frame
holds -- measured while that prompt was up, where only a cursor blinked. With the
action running the holds are mixed 1s and 2s: the game redraws at up to 60 Hz and
holds when nothing moves.

usage: effect_watch.py STATE OUT [name ...]
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '../../../.claude/skills/ti-port-ps1/scripts'))
sys.path.insert(0, HERE)
import core_audit  # noqa: E402
import oracle  # noqa: E402
from psxrun import PSX  # noqa: E402

# Gariland's own abilities all go through the effect system, which is enough to see
# the staging: no later save is needed. The cases come straight from oracle.SKILLS,
# pokes included, so a case is set up exactly as the oracle sets it up -- Phoenix
# Down needs its target dead, Antidote needs it poisoned, and so on.
CASES = {name: (menu, k, hp, sts) for name, menu, k, hp, sts in oracle.SKILLS}


def watch(psx, state, name, out, frames=400):
    from PIL import Image
    menu, k, hp, sts = CASES[name]
    psx.load(state)
    oracle.skill_setup(psx)
    t = oracle.UNITS + oracle.UNIT_SIZE * oracle.TARGET
    if hp is not None:
        psx.write(t + 0x28, hp, 2)
    for b, bit in sts:                       # as oracle.skills() does: current and inflicted
        for o in (0x58, 0x1bb):
            psx.write(t + o + b, psx.read(t + o + b) | bit)
    s = oracle.taps(menu + 'DOWN:20 ' * k + 'CIRCLE:60 CIRCLE:60 LEFT:40 CIRCLE:60')
    oracle.run(psx, len(s) + 1, s)
    s = oracle.taps('CIRCLE:60 CIRCLE:60')        # the OK? prompt, then execute
    oracle.run(psx, len(s) + 1, s)
    d = os.path.join(out, name)
    os.makedirs(d, exist_ok=True)
    lum, prev, holds, run_len = [], None, [], 0
    for f in range(frames):
        psx.pressed = 0
        psx.run()
        im = psx.image().convert('RGB')
        small = im.resize((80, 60))
        lum.append(sum(small.convert('L').getdata()) / 4800.0)
        cur = small.tobytes()
        if prev is not None and cur == prev:
            run_len += 1
        else:
            if prev is not None:
                holds.append(run_len + 1)
            run_len = 0
            im.save('%s/%03d.png' % (d, f))
        prev = cur
    holds.append(run_len + 1)
    bright = sum(lum[:5]) / 5.0
    dark = [i for i, v in enumerate(lum) if v < bright * 0.6]
    return holds, (dark[0], dark[-1]) if dark else None


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    state, out = sys.argv[1], sys.argv[2]
    names = sys.argv[3:] or list(CASES)
    psx = PSX(str(core_audit.DEFAULT_DISC))
    for name in names:
        holds, dark = watch(psx, state, name, out)
        if dark:
            n = dark[1] - dark[0] + 1
            print('%-8s dimmed frames %d..%d  (%d frames, %.2f s at 60 Hz)'
                  % (name, dark[0], dark[1], n, n / 60.0))
        else:
            print('%-8s no dimming' % name)
        print('%-8s holds: %s' % ('', holds[:24]))


if __name__ == '__main__':
    main()
