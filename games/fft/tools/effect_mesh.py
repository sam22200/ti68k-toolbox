#!/usr/bin/env python3
"""The native mesh renderers of the EFFECT overlays, transcribed from fft_decomp.

110 of the 401 effect files are MIPS overlays: their data sections carry only the
accompaniment -- sparks, small sprites, particles -- and the shape you actually
remember is built by the overlay's own code. Holy is three of them, and nothing
in `effect_anim.py` could ever draw its beam, its halo or its ring, because none
of the three is in the file.

They are not 110 renderers. `target/effect.yaml` has 198 handler instances over
**25 families**, and the three Holy needs are the three most reused:

    update_ring_mesh_state           32 overlays
    update_ring_stack_mesh_state     31 overlays
    update_textured_quad_grid_4x4    27 overlays

This module takes them one at a time, as a transcription of the decompiled C and
not as a lookalike. First: `ring_stack`, from
`src/effect/effect_e015_update_ring_stack_mesh_state.c`.

**The ring stack is the beam.** Nine lattice rows of eight points, 45 degrees
apart, each row growing in radius and height by its own step, make 8 rings of 8
textured quads; the texture scrolls in v, which is what draws the vertical
stripes down the column. Every vertex takes its brightness from the overlay's
own table, so the beam fades along its length without a single sprite.
"""
import struct

RING_ROWS = 9              # lattice rows
RING_SEGMENTS = 8          # points around one ring
OT_DEPTH_LIMIT = 0x180
GEOMETRY_FLAG_COLOUR_CURVES = 0x40


def rsin(a):
    """libgte's rsin: 4096 is a whole turn, the result 12-bit fixed."""
    import math
    return int(round(math.sin(a * 2 * math.pi / 4096) * 4096))


def rcos(a):
    import math
    return int(round(math.cos(a * 2 * math.pi / 4096) * 4096))


def lerp(start, end, factor):
    """battle_effect_lerp_linear."""
    return (((end - start) * factor) >> 8) + start


def curve(table, nibble, frame):
    """READ_CURVE: nibble 0 is no curve and reads 0, else row nibble - 1."""
    i = nibble - 1
    if i == -1:
        return 0
    row = table[i] if i < len(table) else None
    if not row:
        return 0
    return row[frame % len(row)]


class RingMeshView:
    """effect_ring_mesh_geometry_view_t over one 196-byte section-3 record."""

    _F = {'placement_flags': ('<H', 0x02), 'flags': ('<H', 0x06),
          'scroll_curves': ('<I', 0x08), 'wave_curves': ('<I', 0x0c),
          'colour_curves': ('<I', 0x10), 'u_span': ('<h', 0x20), 'v_span': ('<h', 0x22),
          'texture_page_flags': ('<H', 0x24), 'u_start': ('<h', 0x2c), 'v_start': ('<h', 0x2e),
          'v_scroll_speed': ('<H', 0x30), 'u_end': ('<h', 0x32), 'v_end': ('<h', 0x34),
          'brightness_row': ('<h', 0x3c), 'spin_speed_start': ('<h', 0x5c),
          'spin_speed_end': ('<h', 0x60), 'growth_start': ('<h', 0x64),
          'height_start': ('<h', 0x68), 'row_radius_start': ('<h', 0x6c),
          'growth_end': ('<h', 0x70), 'height_end': ('<h', 0x74),
          'row_radius_end': ('<h', 0x78), 'growth_step_start': ('<h', 0x7c),
          'height_step_start': ('<h', 0x80), 'row_radius_step_start': ('<h', 0x84),
          'growth_step_end': ('<h', 0x88), 'height_step_end': ('<h', 0x8c),
          'row_radius_step_end': ('<h', 0x90), 'radius_start': ('<h', 0xa8),
          'radius_end': ('<h', 0xac)}

    def __init__(self, raw):
        for name, (fmt, off) in self._F.items():
            setattr(self, name, struct.unpack_from(fmt, raw, off)[0])


class RingStack:
    """One live ring stack: the work block the overlay keeps between frames."""

    def __init__(self, entry, bright, curves=()):
        self.e = entry if isinstance(entry, RingMeshView) else RingMeshView(entry)
        self.bright = bright          # [row][0..8], 12-bit fixed
        self.curves = curves          # g_effect_palette_table
        self.spin = 0
        self.v_scroll = 0
        self.radius_offset = 0
        self.radius_speed = 0

    def lattice(self, frame):
        """points[row][point] = (x, y, z), and the growth the row carried."""
        e = self.e
        j = curve(self.curves, (e.wave_curves >> 16) & 0xf, frame)
        radius = lerp(e.radius_start, e.radius_end, j)
        j = curve(self.curves, e.wave_curves & 0xf, frame)
        gx = lerp(e.growth_start, e.growth_end, j)
        gy = lerp(e.height_start, e.height_end, j)
        gz = lerp(e.row_radius_start, e.row_radius_end, j)
        j = curve(self.curves, (e.wave_curves >> 4) & 0xf, frame)
        sx = lerp(e.growth_step_start, e.growth_step_end, j)
        sy = lerp(e.height_step_start, e.height_step_end, j)
        sz = lerp(e.row_radius_step_start, e.row_radius_step_end, j)
        j = curve(self.curves, e.scroll_curves >> 28, frame)
        self.spin = (self.spin + lerp(e.spin_speed_start, e.spin_speed_end, j)) & 0xfff
        sin0, sin1 = rsin(self.spin), rsin(self.spin + 0x200)
        cos0, cos1 = rcos(self.spin), rcos(self.spin + 0x200)
        pts, n, k = [], 0, 0
        for _ in range(RING_ROWS):
            r = radius + ((k + self.radius_offset) >> 8)
            a = (cos0 * r) >> 12
            b = (sin0 * r) >> 12
            c = (cos1 * r) >> 12
            d = (sin1 * r) >> 12
            y = n >> 8
            # The eight points are two rsin/rcos pairs mirrored into four quadrants.
            row = [None] * RING_SEGMENTS
            row[0] = (a, y, b)
            row[1] = (c, y, d)
            row[2] = (-b, y, a)
            row[3] = (-d, y, c)
            row[4] = (-a, y, -b)
            row[5] = (-c, y, -d)
            row[6] = (b, y, -a)
            row[7] = (d, y, -c)
            pts.append(row)
            gy += sy
            gz += sz
            n += gy
            k += gz
        self.radius_speed += sx
        self.radius_offset += gx + self.radius_speed
        return pts

    def texture(self, frame):
        """(u, v) of the scrolling window, after this frame's scroll."""
        e = self.e
        j = curve(self.curves, (e.scroll_curves >> 8) & 0xf, frame)
        u = lerp(e.u_start, e.u_end, j)
        v = lerp(e.v_start, e.v_end, j)
        self.v_scroll += e.v_scroll_speed
        if self.v_scroll > e.v_span << 8:
            self.v_scroll -= e.v_span << 8
        if self.v_scroll < 0:
            self.v_scroll += e.v_span << 8
        return u, v

    def colour(self, frame):
        """The (r, g, b) the whole stack is tinted by; 0x80 is neutral."""
        e = self.e
        if not (e.flags & GEOMETRY_FLAG_COLOUR_CURVES):
            return 0x80, 0x80, 0x80
        k = e.colour_curves & 0xfff
        return (curve(self.curves, k & 0xf, frame),
                curve(self.curves, (k >> 4) & 0xf, frame),
                curve(self.curves, k >> 8, frame))

    def quads(self, frame, project):
        """The 64 POLY_GT4 of this frame, farthest first.

        `project(x, y, z) -> (sx, sy, depth)` stands for the battle camera.
        Quad `j * 8 + i` takes lattice row `j + 1` for its top corners (0, 1) and
        row `j` for its bottom ones (2, 3); segment 7 closes the ring on 0.
        """
        e = self.e
        pts = self.lattice(frame)
        u, v = self.texture(frame)
        red, green, blue = self.colour(frame)
        row = self.bright[e.brightness_row % len(self.bright)]
        scr = [[project(*p) for p in r] for r in pts]
        out = []
        for j in range(RING_SEGMENTS):
            for i in range(RING_SEGMENTS):
                nxt = (i + 1) % RING_SEGMENTS
                c = [scr[j + 1][i], scr[j + 1][nxt], scr[j][i], scr[j][nxt]]
                depth = (c[0][2] + c[1][2] + c[2][2] + c[3][2]) // 16
                if not 0 <= depth < OT_DEPTH_LIMIT:
                    continue
                top, bot = row[j + 1], row[j]
                def tint(k):
                    return ((k * red) >> 12, (k * green) >> 12, (k * blue) >> 12)
                out.append(dict(
                    pts=[(p[0], p[1]) for p in c],
                    uv=[(u, v + (self.v_scroll >> 8)), (u + e.u_span, v + (self.v_scroll >> 8)),
                        (u, v + e.v_span + (self.v_scroll >> 8)),
                        (u + e.u_span, v + e.v_span + (self.v_scroll >> 8))],
                    rgb=[tint(top), tint(top), tint(bot), tint(bot)],
                    depth=depth,
                    abe=(e.texture_page_flags & 4) == 0,
                    mode=e.texture_page_flags & 3))
        out.sort(key=lambda q: -q['depth'])        # the ordering table, back to front
        return out


def brightness_table(raw, base, rows=16, width=9):
    """The overlay's own vertex brightness rows, 12-bit fixed, u32 each."""
    out = []
    for r in range(rows):
        o = base + r * width * 4
        if o + width * 4 > len(raw):
            break
        out.append(list(struct.unpack_from('<%dI' % width, raw, o)))
    return out
