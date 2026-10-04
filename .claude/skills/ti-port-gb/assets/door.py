#!/usr/bin/env python3
"""The memory of the ROM at the injection door, per hall, for the trace tests: door.py ROM
PRE_STATE OUTDIR HALL...  ->  OUTDIR/door_NN.bin = WRAM C000-DFFF, VRAM 8000-9FFF, HRAM
FF80-FFFF, I/O FF00-FF4F, OAM FE00-FE9F, taken when the CPU reaches play_hall (0223) after the
door's pokes (hall_id, travel_dir, intro). The C port starts the hall from exactly that memory,
so the traces can compare whole RAM regions. HALL 0 = a new game (no pokes, the intro)."""
import os, sys, warnings
warnings.filterwarnings('ignore')
from pyboy import PyBoy

ENTRY7 = {5, 6, 17, 18, 29, 30}       # halls the door enters from the east (travel_dir 7)


def pokes(hall):
    if hall == 0:
        return {}
    if hall == 1:
        return {0xC0AC: 2, 0xC0B0: 0, 0xC0B3: 0}
    return {0xC0AC: hall + 1, 0xC0B0: 7 if hall in ENTRY7 else 3, 0xC0B3: 0}


def door(rom, state, hall):
    gb = PyBoy(rom, window='null', sound_emulated=False)
    with open(state, 'rb') as f:
        gb.load_state(f)
    got = []

    def hook(_):
        if not got:
            for a, v in pokes(hall).items():
                gb.memory[a] = v
            m = gb.memory
            got.append(bytes(m[0xC000:0xE000]) + bytes(m[0x8000:0xA000]) + bytes(m[0xFF80:0x10000])
                       + bytes(m[0xFF00:0xFF50]) + bytes(m[0xFE00:0xFEA0]))
    gb.hook_register(0, 0x0223, hook, None)
    gb.button_press('a')
    n = 0
    while not got and n < 3000:
        gb.tick()
        n += 1
    gb.stop(save=False)
    return got[0] if got else None


if __name__ == '__main__':
    rom, state, out = sys.argv[1:4]
    os.makedirs(out, exist_ok=True)
    for h in (int(x) for x in sys.argv[4:]):
        d = door(rom, state, h)
        if d is None:
            sys.exit('door: hall %d never reached play_hall' % h)
        open(os.path.join(out, 'door_%02d.bin' % h), 'wb').write(d)
