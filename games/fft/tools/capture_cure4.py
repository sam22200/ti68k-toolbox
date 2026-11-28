#!/usr/bin/env python3
"""Cast Cure 4 on the original and keep the frames: a reference for its colour.

Gariland has no Priest, so the spell is staged the way `oracle.py` stages the
others -- the caster's primary skill set poked to White Magic (0x0a, whose list
opens Cure, Cure 2, Cure 3, **Cure 4**), everything in it learned and the MP
filled -- and then the menu is driven to it.

usage: capture_cure4.py OUT [ABILITY_INDEX]
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '../../../.claude/skills/ti-port-ps1/scripts'))
sys.path.insert(0, HERE)
import core_audit        # noqa: E402
import oracle            # noqa: E402
from psxrun import PSX   # noqa: E402

STATE = os.path.join(HERE, '../../../sources/fft_ps1/gariland_t1.state')
WHITE_MAGIC = 0x0A


def stage(psx, mp=99):
    """Ramza with White Magic as his own set, all of it learned, and MP to spend."""
    a = oracle.UNITS + oracle.UNIT_SIZE * oracle.ATTACKER
    psx.write(a + 0x12, WHITE_MAGIC)
    lo = int(os.environ.get('CURE4_LEARN_LO', '0x99'), 0)
    hi = int(os.environ.get('CURE4_LEARN_HI', '0x9f'), 0)
    for k in range(lo, hi):
        psx.write(a + k, 0xFF)
    psx.write(a + 0x91, psx.read(a + 0x91) | 0x08)
    for off in (0x2c, 0x2e):                     # MP and its maximum, beside HP at 0x28
        psx.write(a + off, mp, 2)


def main():
    out = sys.argv[1]
    index = int(sys.argv[2]) if len(sys.argv) > 2 else 3     # Cure 4 is the fourth entry
    os.makedirs(out, exist_ok=True)
    psx = PSX(str(core_audit.DEFAULT_DISC))
    psx.load(STATE)
    stage(psx)
    a = oracle.UNITS + oracle.UNIT_SIZE * oracle.ATTACKER
    print('caster: set 0x%02x  hp %d/%d  mp %d/%d'
          % (psx.read(a + 0x12), psx.read(a + 0x28, 2), psx.read(a + 0x2a, 2),
             psx.read(a + 0x2c, 2), psx.read(a + 0x2e, 2)), flush=True)
    # The turn menu is Move / Attack / White Magic / Auto-battle, with no Act
    # submenu: two downs reach the set, `index` more the spell inside it.
    script = os.environ.get('CURE4_KEYS') or (
        'DOWN:40 DOWN:40 CIRCLE:60 ' + 'DOWN:30 ' * index
        + 'CIRCLE:80 CIRCLE:80 CIRCLE:80 CIRCLE:60 ')
    keys = oracle.taps(script)
    print('keys: %s' % script, flush=True)
    frames = len(keys) + 420
    for f in range(frames):
        # The engine recomputes MP from the unit's stats, so a single poke is
        # undone before the menu opens: hold it up every frame instead.
        psx.write(a + 0x2c, 99, 2)
        psx.write(a + 0x2e, 99, 2)
        psx.pressed = keys[f] if f < len(keys) else 0
        psx.run()
        if f % int(os.environ.get('CURE4_EVERY', '1')) == 0:
            psx.image().save('%s/%04d.png' % (out, f))
    print('%d frames -> %s' % (frames, out), flush=True)


main()
