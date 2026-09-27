"""Parse the FFA wall matrices (murK = room K+1) from ffa_en/ffa_decoded.txt.

Original room format (TI-Basic `mov`/`redess`, 1-based matrix indices):
- row 1 = [encounter rate frc, row index dr of the door table, ...]; mur[2,1] = C.
- walk grid: the cell under hero pixel (a, b) is mur[b/9+2, a/9+2]; 9-px cells. The logical grid
  is columns 1..C-1 (column 1 = left exits at a = -9) and rows 1..dr (row 1 = top exits at
  b = -9, the door-table row doubles as the bottom exit row): cell (x, y) = (a/9 + 1, b/9 + 1).
- values: 0 wall; > 0.9 walkable; 3..199 door to room p; 200..299 world map; 300 chocobo;
  450+ other map; >= 500 story script (scenar); <= -2 text (program text{int(|p|/10)+1}).
- door table (row dr): door ids in columns 1.., key flag clef[k] at column i+6 (0 none,
  -1 not on a chocobo), arrival b = mur[2i-1, C], a = mur[2i, C] (-1 = keep the coordinate).
"""
import re
import os
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
DECODED = os.path.join(HERE, '..', '..', '..', 'ffa_en', 'ffa_decoded.txt')


def _num(v):
    try:
        return Fraction(v.strip())
    except (ValueError, ZeroDivisionError):
        return Fraction(0)


def load(path=DECODED):
    txt = open(path, encoding='utf-8').read()
    mats = {}
    for m in re.finditer(r'rpg\\(\w+) \(matrice\) =+\n((?:\[.*\]\n)+)', txt):
        mats[m.group(1)] = [[_num(v) for v in r.strip()[1:-1].split(',')]
                            for r in m.group(2).strip().split('\n')]
    return mats


M = load() if os.path.exists(DECODED) else {}


def room(u):
    return M.get('mur%d' % (u - 1))


class Room:
    """Logical view of one room: grid[y][x] (x = column - 1, y = row - 2), doors, rate."""

    def __init__(self, u):
        m = room(u)
        if m is None:
            raise KeyError('no matrix for room %d' % u)
        self.u = u
        self.raw = m
        self.frc = m[0][0]
        dr = int(m[0][1])
        C = int(m[1][0])
        self.C, self.dr = C, dr
        at = lambda r, c: m[r - 1][c - 1] if r - 1 < len(m) and c - 1 < len(m[r - 1]) else Fraction(0)
        self.grid = [[at(r, c) for c in range(1, C)] for r in range(1, dr + 1)]
        self.w, self.h = C - 1, dr
        for (y, x) in ((0, 0), (0, 1), (1, 0)):          # header: frc, dr, C
            self.grid[y][x] = Fraction(0)
        for x in range(self.w):                           # door-table row: only exits count
            v = self.grid[dr - 1][x]
            if not (2 < v < 500):
                self.grid[dr - 1][x] = Fraction(0)
        self._sanitize()
        self.doors = []
        i = 1
        while i <= 6 and at(dr, i) != 0:
            b, a = at(2 * i - 1, C), at(2 * i, C)
            self.doors.append({'room': int(at(dr, i)), 'key': int(at(dr, i + 6)),
                               'b': int(b), 'a': int(a)})
            i += 1

    def walkable(self, v):
        return v > Fraction(9, 10) and not (2 < v < 500)

    def _sanitize(self):
        """Metadata (C, arrival coordinates, the door table doubling as the bottom exit row) sits
        on border cells: a non-wall cell with no walkable 4-neighbour is unreachable -> wall."""
        g = self.grid
        keep = [[False] * self.w for _ in range(self.h)]
        for y in range(self.h):
            for x in range(self.w):
                v = g[y][x]
                if v == 0:
                    continue
                if self.walkable(v):
                    keep[y][x] = True
                    continue
                keep[y][x] = any(0 <= y + j < self.h and 0 <= x + i < self.w and self.walkable(g[y + j][x + i])
                                 for i, j in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        for y in range(self.h):
            for x in range(self.w):
                if not keep[y][x]:
                    g[y][x] = Fraction(0)

    def door_index(self, p):
        for k, d in enumerate(self.doors):
            if d['room'] == p:
                return k
        return None


if __name__ == '__main__':
    import sys
    for u in map(int, sys.argv[1:]):
        r = Room(u)
        print('room', u, 'grid', r.w, 'x', r.h, 'frc', r.frc, 'doors', r.doors)
        for row in r.grid:
            print(' '.join('%4s' % ('%g' % float(v)) for v in row))
