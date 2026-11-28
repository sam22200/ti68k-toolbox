#!/usr/bin/env python3
"""Cast one ability on the original and keep the frames: a reference for its look.

Gariland has only Squires and Chemists, so anything else is staged the way
`oracle.py` stages its own -- the caster's primary skill set poked, everything
in it learned, the MP filled -- and then the menu is driven to the entry.

Three things cost three runs each and are worth keeping:

* **the learned bitmap is wider than `oracle.skill_setup` writes.** Filling
  +0x99..+0x9e leaves "No Abilities can be used" and the set greyed out; +0x90
  to +0xb0 opens it;
* **the turn menu is Move / Act / Wait / Status / Auto-battle.** The sets are
  one level in, behind Act. Two downs from the top is Wait, which ends the turn
  and throws the run away without a word;
* **read the costs off the spell list** rather than guessing at MP. Cure 4
  costs 10, and the caster already had 16.

usage: capture_ability.py OUT SET INDEX     e.g. 0x0a 3 for Cure 4
                                                 0x1d 0 for Stasis Sword
                                                 0x20 0 for Night Sword
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


def stage(psx, skill_set, mp=99):
    """Ramza with that set as his own, all of it learned, and MP to spend."""
    a = oracle.UNITS + oracle.UNIT_SIZE * oracle.ATTACKER
    psx.write(a + 0x12, skill_set)
    lo, hi = 0x90, 0xb0            # wider than skill_setup: see the module note
    for k in range(lo, hi):
        psx.write(a + k, 0xFF)
    psx.write(a + 0x91, psx.read(a + 0x91) | 0x08)
    for off in (0x2c, 0x2e):                     # MP and its maximum, beside HP at 0x28
        psx.write(a + off, mp, 2)


def main():
    out = sys.argv[1]
    skill_set = int(sys.argv[2], 0)
    index = int(sys.argv[3], 0)
    os.makedirs(out, exist_ok=True)
    psx = PSX(str(core_audit.DEFAULT_DISC))
    psx.load(STATE)
    stage(psx, skill_set)
    a = oracle.UNITS + oracle.UNIT_SIZE * oracle.ATTACKER
    print('caster: set 0x%02x  hp %d/%d  mp %d/%d'
          % (psx.read(a + 0x12), psx.read(a + 0x28, 2), psx.read(a + 0x2a, 2),
             psx.read(a + 0x2c, 2), psx.read(a + 0x2e, 2)), flush=True)
    script = os.environ.get('CAPTURE_KEYS') or (
        'DOWN:30 CIRCLE:40 DOWN:20 CIRCLE:80 ' + 'DOWN:20 ' * index
        + 'CIRCLE:80 CIRCLE:80 LEFT:40 CIRCLE:80 CIRCLE:80 CIRCLE:600 ')
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
        if f % int(os.environ.get('CAPTURE_EVERY', '1')) == 0:
            psx.image().save('%s/%04d.png' % (out, f))
    print('%d frames -> %s' % (frames, out), flush=True)


main()
