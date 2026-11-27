#!/usr/bin/env python3
"""Capture the original around a staged effect: the frames, and WHEN the wash starts
relative to the blow.

The open question this answers is an ordering, so it is measured as one: the wash's
first frame against the frame a target's HP changes, both read per frame. Luminance
says when the picture darkens; HP says when the blow lands. Neither is an impression
of a screenshot.

usage: effect_capture.py STATE FRAMES OUT
"""
import os
import sys
from statistics import median

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '../../../.claude/skills/ti-port-ps1/scripts'))
sys.path.insert(0, HERE)
import battle            # noqa: E402
import core_audit        # noqa: E402
import management_data as md   # noqa: E402
import oracle            # noqa: E402
from psxrun import PSX   # noqa: E402

LAST_ABILITY, HP, TURN = 0x170, 0x28, 0x8018f520


def vram_dump(psx, stem):
    """The 1 MB VRAM at this instant, raw and as the PS1 sees it (15-bit, 1024x512).

    Which cells of an effect's atlas are actually resident, and where the GPU reads them
    from, is a fact the console holds and the file does not: the .BIN says what a frame
    COULD use, the VRAM says what is loaded while it plays.
    """
    from psxrun import vram_png
    if getattr(psx, 'vram', None) is None:
        print('vram: the core has no RETRO_MEMORY_VIDEO_RAM '
              '(needs scripts/pcsx_vram.patch)', flush=True)
        return
    raw = bytes(psx.vram)
    open(stem + '.bin', 'wb').write(raw)
    vram_png(raw, stem + '.png')
    print('vram -> %s.png' % stem, flush=True)
PRE, POST = 24, 150          # frames kept before the wash, and after it starts
TAIL = 90                    # and after the wash LIFTS: the sprites outlive it.
# Stasis Sword's wash holds 66 frames; its animation runs 72 ticks (36 frames at 30 fps)
# and keeps playing once the screen is bright again. Stopping at the wash cut the last
# third off the only recording of it, which is why the decoded tail -- the pillar
# melting -- looked like something the game does not do.


def main():
    state, frames, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    os.makedirs(out, exist_ok=True)
    names = md.names(battle.Disc(str(core_audit.DEFAULT_DISC)).bat, md.ABILITY_NAMES, 512)
    psx = PSX(str(core_audit.DEFAULT_DISC))
    psx.load(state)

    ring, recent, hp_prev = [], [], None
    events, cur = [], None
    for f in range(frames):
        for i in range(21):                        # the AI plays both sides
            a = oracle.UNITS + oracle.UNIT_SIZE * i + 5
            psx.write(a, psx.read(a) & ~0x08)
        psx.pressed = oracle.key('CIRCLE') if f % 150 < 4 else 0
        psx.run()

        hp = [psx.read(oracle.UNITS + oracle.UNIT_SIZE * i + HP, 2) for i in range(21)]
        hit = None
        if hp_prev is not None:
            for i, (a, b) in enumerate(zip(hp_prev, hp)):
                if a != b and b < 1000 and a < 1000:
                    hit = (i, a, b)
                    break
        hp_prev = hp

        im = psx.image()
        lum = sum(im.convert('L').resize((80, 60)).getdata()) / 4800.0
        if cur is None:
            recent.append(lum)
            del recent[:-60]
            ring.append((f, lum, im))
            del ring[:-PRE]
        base = median(recent) if len(recent) >= 20 else None

        if cur is None and base and lum < base * 0.70:
            unit = psx.read(TURN)
            aid = psx.read(oracle.UNITS + oracle.UNIT_SIZE * unit + LAST_ABILITY, 2)
            nm = names[aid] if aid < len(names) and names[aid] else '?'
            cur = {'start': f, 'unit': unit, 'aid': aid, 'name': nm,
                   'hits': [], 'frames': list(ring), 'base': base}
            vram_dump(psx, '%s/vram-%06d' % (out, f)) if out else None
            print('WASH  f%-6d %-16s id 0x%03x unit %2d' % (f, nm, aid, unit), flush=True)
        if cur is not None:
            if hit:
                cur['hits'].append((f, hit))
                print('  HIT f%-6d unit %2d  hp %d -> %d  (wash%+d)'
                      % (f, hit[0], hit[1], hit[2], f - cur['start']), flush=True)
            if f != cur['start']:
                cur['frames'].append((f, lum, im))
            if cur.get('lifted') is None and lum > cur['base'] * 0.88:
                cur['lifted'] = f
                print('  LIFT  f%-6d  wash%+d, %d more frames' % (f, f - cur['start'], TAIL),
                      flush=True)
            if (cur.get('lifted') is not None and f - cur['lifted'] >= TAIL) \
                    or f - cur['start'] > POST + TAIL:
                held = f - cur['start']
                print('  END f%-6d  held %d frames = %.2f s' % (f, held, held / 60.0), flush=True)
                if held >= 10:
                    events.append(cur)
                cur = None
                recent, ring = [], []
        elif hit:
            print('  hit f%-6d unit %2d  hp %d -> %d  (no wash)' % (f, hit[0], hit[1], hit[2]),
                  flush=True)

    for n, e in enumerate(events):
        d = os.path.join(out, '%02d-%s' % (n, e['name'].replace(' ', '_')))
        os.makedirs(d, exist_ok=True)
        for fr, lum, im in e['frames']:
            im.save('%s/%06d.png' % (d, fr))
        print('%s : %d frames, wash at %d, hits %s'
              % (d, len(e['frames']), e['start'], [h[0] for h in e['hits']]), flush=True)


main()
