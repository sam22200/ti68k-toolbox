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
    return palette(table, nibble - 1, frame) if nibble else 0


def palette(table, row, frame):
    """g_effect_palette_table[row].factor[frame], read straight.

    The colour curves are **not** READ_CURVE: the source indexes the table with
    the nibble itself, so row 0 is a real row and not "no curve". Subtracting
    one there turned every ring stack black, Holy's beam included.
    """
    if row < 0 or row >= len(table):
        return 0
    r = table[row]
    return r[frame % len(r)] if r else 0


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
    """One live ring stack: the work block the overlay keeps between frames.

    `RingMesh` is the same renderer with other counts, so the lattice is built
    from these three: nine rows of eight for the stack, three rows of
    thirty-two for the mesh, and the mesh shifts its height step left 2.
    """

    ROWS = RING_ROWS
    SEGMENTS = RING_SEGMENTS
    HEIGHT_SHIFT = 0

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
        gy = lerp(e.height_start, e.height_end, j) << self.HEIGHT_SHIFT
        gz = lerp(e.row_radius_start, e.row_radius_end, j)
        j = curve(self.curves, (e.wave_curves >> 4) & 0xf, frame)
        sx = lerp(e.growth_step_start, e.growth_step_end, j)
        sy = lerp(e.height_step_start, e.height_step_end, j) << self.HEIGHT_SHIFT
        sz = lerp(e.row_radius_step_start, e.row_radius_step_end, j)
        j = curve(self.curves, e.scroll_curves >> 28, frame)
        self.spin = (self.spin + lerp(e.spin_speed_start, e.spin_speed_end, j)) & 0xfff
        step = 0x1000 // self.SEGMENTS
        pts, n, k = [], 0, 0
        for _ in range(self.ROWS):
            r = radius + ((k + self.radius_offset) >> 8)
            y = n >> 8
            # The source builds one quadrant and mirrors or rotates it into the
            # other three; rotating by a quarter turn is adding 0x400 to the
            # angle, so walking the whole turn in `step` gives the same points.
            row = [(((rcos(self.spin + step * i) * r) >> 12), y,
                    ((rsin(self.spin + step * i) * r) >> 12))
                   for i in range(self.SEGMENTS)]
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
        return (palette(self.curves, k & 0xf, frame),
                palette(self.curves, (k >> 4) & 0xf, frame),
                palette(self.curves, k >> 8, frame))

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
        for j in range(self.ROWS - 1):
            for i in range(self.SEGMENTS):
                nxt = (i + 1) % self.SEGMENTS
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


class RingMesh(RingStack):
    """`effect_e015_update_ring_mesh_state`: the ring that turns around the target.

    The ring stack's sibling, and the same geometry view. Three lattice rows of
    nine points cover one quadrant 0x80 apart; the source builds that quadrant
    and rotates it into the other three, which is the same 32 points as walking
    the whole turn in 0x80 steps, so they are walked. Two rings of 32 quads, and
    **the height step is shifted left 2** where the stack takes it raw.
    """

    ROWS = 3
    SEGMENTS = 32
    HEIGHT_SHIFT = 2


class QuadGrid4x4:
    """`effect_e015_update_textured_quad_grid_4x4`: the wash over the whole scene.

    Sixteen POLY_GT4 in a 4x4 grid of 64-pixel cells, **in screen space** -- the
    source writes their corners once in phase 1 and never projects them -- with
    the texture window scrolling along a speed and an angle and each quad row
    tinted from `g_effect_gfx_brightness_table`. That is Holy's halo: it does not
    move with the camera because it never meets it.
    """

    CELL = 0x40
    ORIGIN_X = 0x80

    def __init__(self, raw, bright, curves=()):
        self.raw = raw
        self.bright = bright          # g_effect_gfx_brightness_table, 5 wide
        self.curves = curves
        self.scroll_u = 0
        self.scroll_v = 0
        f = lambda fmt, off: struct.unpack_from(fmt, raw, off)[0]
        self.flags = f('<H', 0x06)
        self.curve0 = f('<I', 0x08)
        self.colour_curves = f('<I', 0x10)
        self.texture_u = f('<H', 0x14)
        self.texture_v = f('<H', 0x16)
        self.angle_start = f('<h', 0x2e)
        self.angle_end = f('<h', 0x34)
        self.page_flags = f('<h', 0x4c)
        self.brightness_row = f('<h', 0x4e)
        self.ot_depth = f('<h', 0x54)
        self.speed_start = f('<h', 0x5c)
        self.speed_end = f('<h', 0x60)
        self.tex_w = f('<h', 0xa8) // 2
        self.tex_h = f('<h', 0xaa) // 2

    def step(self, frame):
        """Advance the texture window; (u, v) of its top left."""
        j = curve(self.curves, self.curve0 >> 28, frame)
        speed = lerp(self.speed_start, self.speed_end, j) << 4
        j = curve(self.curves, (self.curve0 >> 8) & 0xf, frame)
        angle = lerp(self.angle_start, self.angle_end, j)
        self.scroll_u += (rcos(angle) * speed) >> 12
        while self.tex_w and self.scroll_u >= self.tex_w << 12:
            self.scroll_u -= self.tex_w << 12
        while self.tex_w and self.scroll_u < 0:
            self.scroll_u += self.tex_w << 12
        self.scroll_v += (rsin(angle) * speed) >> 12
        while self.tex_h and self.scroll_v >= self.tex_h << 12:
            self.scroll_v -= self.tex_h << 12
        while self.tex_h and self.scroll_v < 0:
            self.scroll_v += self.tex_h << 12
        return (self.scroll_u >> 12) + self.texture_u, (self.scroll_v >> 12) + self.texture_v

    def colour(self, frame):
        if not (self.flags & GEOMETRY_FLAG_COLOUR_CURVES):
            return 0x80, 0x80, 0x80
        k = self.colour_curves & 0xfff
        return (palette(self.curves, k & 0xf, frame),
                palette(self.curves, (k >> 4) & 0xf, frame),
                palette(self.curves, (k >> 8) & 0xf, frame))

    def quads(self, frame):
        """The sixteen quads, all at the entry's own ordering-table depth."""
        u, v = self.step(frame)
        red, green, blue = self.colour(frame)
        row = self.bright[self.brightness_row % len(self.bright)]
        depth = self.ot_depth
        depth = 1 if depth <= 0 else min(depth, OT_DEPTH_LIMIT - 1)
        out = []
        for qr in range(4):
            for qc in range(4):
                x0 = (qc << 6) + self.ORIGIN_X
                y0 = qr << 6
                top, bot = row[qr], row[qr + 1]
                def tint(k):
                    return ((k * red) >> 12, (k * green) >> 12, (k * blue) >> 12)
                out.append(dict(
                    pts=[(x0, y0), (x0 + self.CELL, y0),
                         (x0, y0 + self.CELL), (x0 + self.CELL, y0 + self.CELL)],
                    uv=[(u, v), (u + self.tex_w, v),
                        (u, v + self.tex_h), (u + self.tex_w, v + self.tex_h)],
                    rgb=[tint(top), tint(top), tint(bot), tint(bot)],
                    depth=depth, screen=True,
                    abe=(self.page_flags & 4) == 0,
                    mode=self.page_flags & 3))
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
