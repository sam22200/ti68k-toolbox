#!/usr/bin/env python3
"""Inspect five original 1-1 view widths with disclosed actor suppression.

This records scenery and streaming memory, not an unassisted winning route.
Collision extraction and the playable target remain later milestones.
"""
import json
import sys

from reference import ROOT, OUT, ROM, X, Y, MODE, STAGE
sys.path.insert(0, str(ROOT / 'tools/snes'))
from snesrun import SNES, PAD


def main():
    directory = OUT / 'survey'
    directory.mkdir(exist_ok=True)
    snes = SNES(ROM)
    try:
        snes.load(OUT / 'start.state')
        samples, events, captures = [], [], []
        previous = None
        for frame in range(1000):
            # Controlled terrain survey: suppress the 24 live sprite slots.
            # This is not an interaction test or an unassisted traversal.
            for slot in range(24):
                snes.write(0x700f00 + (slot << 2), 0, 2)
            buttons = ['RIGHT']
            if frame % 100 != 99:
                buttons.append('B')
            # The US map suggests this message state; on PAL the experiment
            # confirms A dismisses it. Fresh edges are needed while reading.
            if snes.read(0x7e0d0f) == 9 and frame % 10 == 0:
                buttons.append('A')
            keys = sum(1 << PAD[button] for button in buttons)
            if keys != previous:
                events.append(str(frame) + ' ' + ' '.join(buttons))
                previous = keys
            snes.pressed = keys
            snes.lib.retro_run()
            sample = {'frame': frame, 'x': snes.read(X, 2), 'y': snes.read(Y, 2),
                      'camera_x': snes.read(0x7e0039, 2),
                      'camera_y': snes.read(0x7e003b, 2),
                      'message': snes.read(0x7e0d0f),
                      'baby': snes.read(0x7001b2, 2)}
            samples.append(sample)
            assert snes.read(MODE, 2) == 0x13 and snes.read(STAGE) == 0
            if len(captures) < 5 and sample['camera_x'] >= len(captures) * 256:
                index = len(captures)
                snes.image().save(directory / f'view{index}.png')
                for name, data in zip(('vram', 'cgram', 'oam', 'regs', 'sram', 'wram'),
                                      (*snes.ppu(), snes.sram(), snes.dump())):
                    (directory / f'view{index}.{name}').write_bytes(data)
                captures.append(sample)
            if sample['x'] >= 1280:
                events.append(str(frame + 1))
                break
        else:
            raise RuntimeError('survey failed to reach X1280: ' + str(samples[-1]))
        assert len(captures) == 5
        (directory / 'keys.txt').write_text('\n'.join(events) + '\n')
        (directory / 'survey.json').write_text(json.dumps({
            'source': json.loads((OUT / 'start.json').read_text()),
            'pokes': {'base': '700f00', 'stride': 4, 'slots': 24,
                      'size': 2, 'value': 0, 'timing': 'before every frame'},
            'scope': 'instrumented scenery survey, not an unassisted traversal',
            'captures': captures, 'samples': samples}, indent=2) + '\n')
        print('Five 256-pixel reference views captured; reached', samples[-1])
    finally:
        snes.close()


if __name__ == '__main__':
    main()
