#!/usr/bin/env python3
"""The FFT campaign battle read from the live TiEmu RAM: no screenshot, no code in fftbat.

  fftmem.py battle            one snapshot: mode, active unit, every unit (team, job, HP, MP,
                              CT, tile, death counter)
  fftmem.py watch [SECONDS]   events as they happen (turns, modes, moves, HP/MP changes, KOs,
                              outcome), polling RAM every 50 ms until the battle ends
  fftmem.py set god|fast on|off   the battle's debug flags (st.dbg, debug_battle.inc), live

State/Unit offsets come from the TI compiler itself (state_layout.c compiled with tigcc -S,
cached in campaign/x/state_layout.json and rebuilt when fft.h changes). The battle's State is
the AMS heap block of sizeof(State)+sizeof(Save) bytes + malloc's handle word (battle_state).
"""
import importlib.machinery, importlib.util, json, re, struct, subprocess, sys, time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
CACHE = HERE.parent / 'campaign/x/state_layout.json'
MODES = ('BROWSE TARGET WALK MENU FACE STATUS AIM ACT ACTS SKILLS PICK END ORDER QUIT AUTO '
         'CAST_RANGE AREA CLOCK EQUIP').split()
AI = 'OFF THINK GO MOVE AIM FACE'.split()
TEAMS = 'PEG'                                   # player, enemy, guest
END = ('', 'VICTORY', 'DEFEAT')


def timem():
    loader = importlib.machinery.SourceFileLoader('timem', str(ROOT / 'tools/bin/ti-mem'))
    spec = importlib.util.spec_from_loader('timem', loader)
    mod = importlib.util.module_from_spec(spec)
    loader.exec_module(mod)
    return mod


def layout():
    sources = [HERE / 'state_layout.c', HERE.parent / 'fft.h', HERE.parent / 'campaign/save_types.h',
               HERE.parent / 'campaign/native_battle.c']
    if CACHE.exists() and CACHE.stat().st_mtime > max(p.stat().st_mtime for p in sources):
        return json.loads(CACHE.read_text())
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    out = CACHE.with_suffix('.s')
    tigcc = ROOT / 'tools/gcc4ti-bin'
    env = {'PATH': f'{tigcc}/bin:/usr/bin:/bin', 'TIGCC': str(tigcc)}
    subprocess.run(['tigcc', '-S', '-Os', '-mno-bss', '-mregparm=5', '-I' + str(ROOT / 'tools/extgraph/lib'),
                    str(HERE / 'state_layout.c'),
                    '-o', str(out)], cwd=CACHE.parent, env=env, check=True, capture_output=True)
    text = out.read_text()                      # a zero is emitted as `.skip 2`
    lay = {k: int(v) for k, v in re.findall(r'^layout_(\w+):\s*\n\s*\.word\s+(\d+)', text, re.M)}
    lay.update({k: 0 for k in re.findall(r'^layout_(\w+):\s*\n\s*\.skip\s+2', text, re.M)})
    CACHE.write_text(json.dumps(lay, indent=0))
    return lay


class Battle:
    def __init__(self):
        self.tm, self.lay = timem(), layout()
        self.mem, self.base = None, None
        self.size = self.lay['State'] + self.lay['Save']

    def find(self, data):
        """TI address of the battle State, or None (no battle running)."""
        tm, lay = self.tm, self.lay
        table = struct.unpack_from('>I', data, tm.HANDLE_TABLE)[0]
        for h in range(1, 4000):
            at = table + 4 * h
            if at + 4 > tm.RAM:
                break
            p = struct.unpack_from('>I', data, at)[0]
            if not 0x100 <= p < tm.RAM - self.size:
                continue
            w = struct.unpack_from('>H', data, p - 2)[0]
            # malloc (HeapAllocPtr) keeps the handle in the block's first word: data at +2.
            if ((w & 0x7FFF) - 1) * 2 == self.size + 2 and self.plausible(data, p + 2):
                return p + 2
        return None

    def plausible(self, d, s):
        """Another block of the same size is possible: check State's shape (the Save tail
        is scratch, not a save image)."""
        L = self.lay
        if d[s + L['State__rot']] > 3 or d[s + L['State__mode']] >= len(MODES) or d[s + L['State__end']] > 2:
            return False
        live = 0
        for i in range(L['UNITS']):
            u = s + L['State__unit'] + i * L['Unit']
            hp, mhp = struct.unpack_from('>HH', d, u + L['Unit__hp'])
            if hp > mhp or mhp > 9999:
                return False
            live += mhp > 0
        return live > 0

    def write_dbg(self, bit, on):
        if self.mem is None:
            self.mem, self.base = self.tm.locate(self.tm.tiemu_pid())
        self.mem.seek(self.base)
        at = self.find(self.mem.read(self.tm.RAM))
        if at is None:
            sys.exit('fftmem: no battle State in RAM')
        where = self.base + at + self.lay['State__dbg']
        self.mem.seek(where); old = self.mem.read(1)[0]
        new = old | bit if on else old & ~bit
        self.mem.seek(where); self.mem.write(bytes((new,)))
        return new

    def read(self):
        if self.mem is None:
            self.mem, self.base = self.tm.locate(self.tm.tiemu_pid())
        self.mem.seek(self.base)
        data = self.mem.read(self.tm.RAM)
        at = self.find(data)
        return None if at is None else self.decode(data, at)

    def decode(self, d, s):
        L = self.lay
        b = lambda f: d[s + L['State__' + f]]
        w = lambda f: struct.unpack_from('>H', d, s + L['State__' + f])[0]
        units = []
        for i in range(L['UNITS']):
            u = s + L['State__unit'] + i * L['Unit']
            ub = lambda f: d[u + L['Unit__' + f]]
            uw = lambda f: struct.unpack_from('>H', d, u + L['Unit__' + f])[0]
            if not uw('max_hp'):
                continue
            units.append(dict(i=i, team=TEAMS[ub('team')] if ub('team') < 3 else '?', job=ub('job'),
                              lv=ub('level'), hp=uw('hp'), mhp=uw('max_hp'), mp=uw('mp'), mmp=uw('max_mp'),
                              ct=ub('ct'), x=ub('x'), z=ub('z'), dc=ub('dc'), face=ub('face')))
        mode = b('mode')
        return dict(mode=MODES[mode] if mode < len(MODES) else str(mode), act=b('act'),
                    ai=AI[b('ai')] if b('ai') < len(AI) else str(b('ai')), end=END[b('end')] if b('end') < 3 else b('end'),
                    rounds=b('rounds'), moved=b('moved'), acted=b('acted'), rot=b('rot'),
                    cursor=(b('cx'), b('cz')), turns=list(d[s + L['State__turns']:s + L['State__turns'] + 6]),
                    dmg=w('dmg'), units=units)


def unit_line(u):
    return (f"u{u['i']:<2} {u['team']} job{u['job']:<3} Lv{u['lv']:<2} HP{u['hp']:>4}/{u['mhp']:<4} "
            f"MP{u['mp']:>3}/{u['mmp']:<3} CT{u['ct']:>3} ({u['x']},{u['z']}) dc{u['dc']}")


def snapshot(st):
    print(f"mode {st['mode']} act u{st['act']} ai {st['ai']} end {st['end'] or '-'} round {st['rounds']} "
          f"rot {st['rot']} cursor {st['cursor']} next {st['turns']}")
    for u in st['units']:
        print(('> ' if u['i'] == st['act'] else '  ') + unit_line(u))


def watch(seconds):
    bt, prev, t0, gone = Battle(), None, time.time(), 0
    while time.time() - t0 < seconds:
        st = bt.read()
        stamp = f'{time.time() - t0:7.2f}'
        if st is None:
            gone += 1
            if prev is not None and gone > 10:
                print(stamp, 'battle program left'); return
            time.sleep(0.05); continue
        gone = 0
        if prev is None:
            print(stamp, 'battle found'); snapshot(st)
        else:
            if st['act'] != prev['act']:
                u = next((u for u in st['units'] if u['i'] == st['act']), None)
                print(stamp, f"turn u{st['act']}", unit_line(u) if u else '')
            if (st['mode'], st['ai']) != (prev['mode'], prev['ai']):
                print(stamp, f"mode {prev['mode']}->{st['mode']} ai {st['ai']}")
            old = {u['i']: u for u in prev['units']}
            for u in st['units']:
                o = old.get(u['i'])
                if not o:
                    continue
                if (u['x'], u['z']) != (o['x'], o['z']):
                    print(stamp, f"u{u['i']} moves ({o['x']},{o['z']})->({u['x']},{u['z']})")
                if u['hp'] != o['hp']:
                    print(stamp, f"u{u['i']} HP {o['hp']}->{u['hp']} ({u['hp'] - o['hp']:+d})" + (' KO' if not u['hp'] else ''))
                if u['mp'] != o['mp']:
                    print(stamp, f"u{u['i']} MP {o['mp']}->{u['mp']}")
                if u['dc'] != o['dc']:
                    print(stamp, f"u{u['i']} death counter {o['dc']}->{u['dc']}")
            if st['end'] != prev['end']:
                print(stamp, f"END {st['end']} after {st['rounds']} rounds")
        prev = st
        sys.stdout.flush()
        time.sleep(0.05)


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('battle', 'watch', 'set'):
        sys.exit(__doc__)
    if sys.argv[1] == 'set':
        bits = {'god': 1, 'fast': 2}
        if len(sys.argv) != 4 or sys.argv[2] not in bits or sys.argv[3] not in ('on', 'off'):
            sys.exit(__doc__)
        print('dbg =', Battle().write_dbg(bits[sys.argv[2]], sys.argv[3] == 'on'))
        sys.exit(0)
    if sys.argv[1] == 'battle':
        st = Battle().read()
        if st is None:
            sys.exit('fftmem: no battle State in RAM')
        snapshot(st)
    else:
        watch(float(sys.argv[2]) if len(sys.argv) > 2 else 600)
