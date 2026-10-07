#!/usr/bin/env python3
"""Controlled PAL tongue/ingestion timelines; disclose every intervention."""
import json
import sys
from reference import ROOT, OUT, ROM
sys.path.insert(0, str(ROOT / 'tools/snes'))
from snesrun import SNES, PAD

GAME = ROOT / 'games/yoshi'
DIRECTORY = OUT / 'actors'


def prepare(snes, capture=False, left=False):
    snes.pressed = 0
    snes.load(OUT / 'start.state')
    if capture:
        # Natural actor loading, retaining both Shy Guys and Baby Mario.
        # Only the flower introduction is excluded from this controlled door.
        for frame in range(170):
            for slot in range(24):
                if snes.read(0x701360 + slot * 4, 2) == 0xad:
                    snes.write(0x700f00 + slot * 4, 0, 2)
            snes.pressed = (1 << PAD['RIGHT']) | ((1 << PAD['B']) if frame % 100 != 99 else 0)
            snes.lib.retro_run()
        assert snes.read(0x701360 + 92, 2) == 0x1e
        assert snes.read(0x7010e2 + 92, 2) == 496
        assert snes.read(0x701182 + 92, 2) == 1904
        assert snes.read(0x701a96 + 92, 2) == 20
        # Park on the verified flat top at Y1920, facing the loaded actor.
        for address, value in ((0x70008c, 464), (0x700090, 1888),
                               (0x70008a, 0), (0x70008e, 65535),
                               (0x7000b4, 0), (0x7000a8, 0), (0x7000aa, 0),
                               (0x7000c0, 0), (0x7000c4, 0), (0x7000d2, 1)):
            snes.write(address, value, 2)
    elif left:
        snes.write(0x7000c4, 2, 2)
    snes.pressed = 0


def sample(snes):
    mouth, tx, ty, timer, block, slot, holding, swallow, eggs = [snes.read(a, 2) for a in
        (0x700150, 0x700152, 0x700154, 0x7001e0, 0x70015e, 0x700168,
         0x700162, 0x7001ee, 0x701df6)]
    assert slot in (0, 93) and holding in (0, 93) and not block
    return {'mouth': mouth, 'length': max(abs((tx ^ 32768) - 32768), abs((ty ^ 32768) - 32768)),
            'timer': timer, 'up': int(bool(ty)), 'slot': int(bool(slot)),
            'holding': int(bool(holding)), 'swallow': swallow, 'eggs': eggs >> 1,
            'x': snes.read(0x7010e2 + 92, 2), 'y': snes.read(0x701182 + 92, 2),
            'vx': snes.read(0x701220 + 92, 2, True), 'sub': snes.read(0x7010e1 + 92),
            'actor_state': snes.read(0x700f00 + 92, 2)}


def census(snes):
    """Streaming census only; Baby Mario restoration deliberately bypasses damage."""
    snes.pressed = 0
    snes.load(OUT / 'start.state')
    seen = {}
    for frame in range(2400):
        for slot in range(24):
            offset = slot << 2
            state = snes.read(0x700f00 + offset, 2)
            identity = snes.read(0x701360 + offset, 2)
            if state and identity != 0x61:
                stage = snes.read(0x7014a0 + offset)
                seen.setdefault((identity, stage), {'id': identity, 'stage': stage, 'frame': frame,
                    'x': snes.read(0x7010e2 + offset, 2), 'y': snes.read(0x701182 + offset, 2)})
            if identity == 0xad:
                snes.write(0x700f00 + offset, 0, 2)
        snes.write(0x7001b2, 0x8000, 2)
        buttons = ['RIGHT'] + (['B'] if frame % 100 < 80 else []) + \
                  (['Y'] if frame % 20 == 0 else []) + (['A'] if frame % 10 == 0 else [])
        snes.pressed = sum(1 << PAD[b] for b in buttons)
        snes.lib.retro_run()
        if snes.read(0x70008c, 2) >= 1280:
            break
    else:
        raise AssertionError('actor census failed to traverse selected band')
    actors = [entry for entry in seen.values() if entry['x'] < 1280]
    shy_guys = sorted((a['x'], a['y']) for a in actors if a['id'] == 0x1e)
    assert shy_guys == [(496,1904),(560,1904),(688,1840),(848,1824),(1152,1808)]
    (DIRECTORY / 'census.json').write_text(json.dumps({
        'source': json.loads((OUT / 'start.json').read_text()), 'actors': actors,
        'protocol': 'Before each frame: suppress flower ID00AD and restore Baby Mario state8000. '
                    'Keys: RIGHT; B for80/100 frames, Y every20, A every10. '
                    'This identifies placements only, not an unassisted or damage reference replay.',
        'endpoint_frame': frame}, indent=2) + '\n')
    print('Actor census:', len(actors), 'placements in the band; five Shy Guys verified', flush=True)
    return shy_guys


def main():
    DIRECTORY.mkdir(exist_ok=True)
    cases = []
    snes = SNES(ROM)
    try:
        placements = census(snes)
        (GAME / 'generated/actors_map.h').write_text(
            '/* Local ROM-derived placements; generated, ignored. */\n' +
            'static const u16 ya_spawn_x[5] = {' + ','.join(str(x) for x, y in placements) + '};\n' +
            'static const u16 ya_spawn_y[5] = {' + ','.join(str(y) for x, y in placements) + '};\n')
        for up in (False, True):
            for hold in (1, 4, 20):
                events = {0: ['Y'] + (['UP'] if up else []), hold: []}
                cases.append((f'{"up" if up else "right"}_{hold}', 50, 60, False, False, events))
        cases += [('left_held', 51, 60, False, True, {0: ['Y'], 20: []}),
                  ('retap', 50, 60, False, False, {0: ['Y'], 1: [], 12: ['Y'], 13: []}),
                  ('capture', 52, 70, True, False, {0: ['Y'], 1: []}),
                  ('swallow', 52, 70, True, False, {0: ['Y'], 1: [], 20: ['DOWN']}),
                  ('early_swallow', 52, 70, True, False, {0: ['Y'], 1: [], 4: ['DOWN']}),
                  ('automatic_swallow', 52, 1240, True, False, {0: ['Y'], 1: []}),
                  ('spit', 52, 70, True, False, {0: ['Y'], 1: [], 20: ['Y'], 21: []}),
                  ('walk', 52, 90, True, False, {0: []})]
        outputs = []
        key_names = {'Y': 'B', 'UP': 'UP', 'DOWN': 'DOWN'}
        for name, scenario, frames, capture, left, events in cases:
            prepare(snes, capture, left)
            snes.save(DIRECTORY / f'{name}.state')
            samples, buttons = [], []
            for frame in range(frames):
                if frame in events: buttons = events[frame]
                snes.pressed = sum(1 << PAD[b] for b in buttons)
                snes.lib.retro_run()
                samples.append(sample(snes))
                assert snes.read(0x70008c, 2) == (464 if capture else 119)
                assert snes.read(0x700090, 2) == (1888 if capture else 1904)
                assert snes.read(0x7001b2, 2) == 0x8000
            keys = '\n'.join(str(f) + (' ' + ' '.join(key_names[b] for b in bs) if bs else '')
                             for f, bs in events.items()) + '\n'
            (DIRECTORY / f'{name}.txt').write_text(keys)
            outputs.append({'name': name, 'scenario': scenario, 'events': events, 'samples': samples})
            print(name, len(samples), 'states; eggs:', samples[-1]['eggs'], flush=True)
        result = {'source': json.loads((OUT / 'start.json').read_text()),
                  'protocol': 'Empty tongue: natural spawn, optional direction=2 poke. Capture/walk: '
                              '170-frame natural loader with flower slots suppressed; one position/velocity '
                              'injection to X464/Y1888. No per-frame anchoring or actor suppression in samples. '
                              'Slot93 normalized to1; egg byte offset/2 normalized to egg count.',
                  'cases': outputs}
        (DIRECTORY / 'reference.json').write_text(json.dumps(result, indent=2) + '\n')
        header = ['/* Generated from local PAL RAM timelines; do not distribute ROM-derived data. */',
                  'typedef struct { u16 swallow; u8 mouth,length,timer,up,slot,holding,eggs; } ActorSample;']
        for case in outputs:
            header.append(f'static const ActorSample ar_{case["name"]}[] = {{')
            header += ['{%d,%d,%d,%d,%d,%d,%d,%d},' % tuple(sample[k] for k in
                       ('swallow', 'mouth', 'length', 'timer', 'up', 'slot', 'holding', 'eggs'))
                       for sample in case['samples']]
            header.append('};')
        header += ['typedef struct { const ActorSample *samples; u16 count,scenario; const char *name; } ActorCase;',
                   'static const ActorCase actor_cases[] = {']
        header += [f'{{ar_{c["name"]},{len(c["samples"])},{c["scenario"]},"{c["name"]}"}},' for c in outputs]
        header.append('};')
        (GAME / 'generated/actors_ref.h').write_text('\n'.join(header) + '\n')
    finally:
        snes.close()


if __name__ == '__main__':
    main()
