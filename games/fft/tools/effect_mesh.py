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

# `placement_flags` bits 9-11 say what the emitter's position is measured from.
ORIGIN_MASK = 0xe00
ORIGIN_WORLD = 0x000
ORIGIN_CURSOR_TILE = 0x200
ORIGIN_CASTER = 0x400
ORIGIN_TARGET = 0x600
ORIGIN_PARENT_PARTICLE = 0x800
ORIGIN_MAP_CENTRE = 0xa00
ORIGIN_TRACKED_ENTITY = 0xc00
ORIGIN_NAMES = {ORIGIN_WORLD: 'world', ORIGIN_CURSOR_TILE: 'cursor tile',
                ORIGIN_CASTER: 'caster', ORIGIN_TARGET: 'target',
                ORIGIN_PARENT_PARTICLE: 'parent particle',
                ORIGIN_MAP_CENTRE: 'map centre', ORIGIN_TRACKED_ENTITY: 'tracked entity'}
# A preview has no battlefield, so the anchors that resolve to a unit or to the
# cursor's tile all land on the one point it draws about. Only these two are
# somewhere else, and nothing here can place them.
ORIGIN_UNPLACEABLE = (ORIGIN_PARENT_PARTICLE, ORIGIN_MAP_CENTRE)


def spawn_position(raw, factor):
    """`battle_effect_interpolate_emitter_spawn_position`: three lerped words.

    `spawn_position_start` at 0x14 and `spawn_position_end` at 0x1a, each lerped
    by the curve the caller read. Taking the start alone -- which is what came
    before -- pins an emitter that the original walks from one place to another
    across the effect.
    """
    a = struct.unpack_from('<3h', raw, 0x14)
    b = struct.unpack_from('<3h', raw, 0x1a)
    return tuple(lerp(a[i], b[i], factor) for i in range(3))


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
        self.raw = entry
        self.e = entry if isinstance(entry, RingMeshView) else RingMeshView(entry)
        self.bright = bright          # [row][0..8], 12-bit fixed
        self.curves = curves          # g_effect_palette_table
        self.spin = 0
        self.v_scroll = 0
        self.radius_offset = 0
        self.radius_speed = 0

    def origin(self, frame):
        """Where this mesh is measured from, and the point it is placed at.

        The source lerps `spawn_position` by the `scroll_curves` nibble 0 curve
        and then adds the anchor the placement flags name. A preview draws about
        the target, so every anchor that resolves to a unit or to the cursor's
        tile is already that point; `unplaceable` says when it is not.
        """
        e = self.e
        j = curve(self.curves, e.scroll_curves & 0xf, frame)
        mode = e.placement_flags & ORIGIN_MASK
        return spawn_position(self.raw, j), mode, mode in ORIGIN_UNPLACEABLE

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
        (ox, oy, oz), _, _ = self.origin(frame)
        u, v = self.texture(frame)
        red, green, blue = self.colour(frame)
        row = self.bright[e.brightness_row % len(self.bright)]
        scr = [[project(p[0] + ox, p[1] + oy, p[2] + oz) for p in r] for r in pts]
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


class SummonMesh(RingStack):
    """`effect_e071_update_summon_mesh_state`: a spinning dome of 8x16 quads.

    Nine lattice rows at 0x80 angle steps: each row's ring radius shrinks by the
    cosine of its own step and the row rises by the scaled sine, so the rows walk
    a quarter circle instead of a straight column. Seventeen points a row at
    0x100 steps close the ring. Its view is the ring mesh's but for the row
    radius terms, which a dome has no use for.
    """

    ROWS = 9
    SEGMENTS = 16

    def lattice(self, frame):
        e = self.e
        j = curve(self.curves, (e.wave_curves >> 16) & 0xf, frame)
        radius = lerp(e.radius_start, e.radius_end, j)
        j = curve(self.curves, e.wave_curves & 0xf, frame)
        gx = lerp(e.growth_start, e.growth_end, j)
        gy = lerp(e.height_start, e.height_end, j)
        j = curve(self.curves, (e.wave_curves >> 4) & 0xf, frame)
        sx = lerp(e.growth_step_start, e.growth_step_end, j)
        sy = lerp(e.height_step_start, e.height_step_end, j)
        j = curve(self.curves, e.scroll_curves >> 28, frame)
        self.spin = (self.spin + lerp(e.spin_speed_start, e.spin_speed_end, j)) & 0xfff
        pts = []
        for row in range(self.ROWS):
            r = radius + (self.radius_offset >> 8)
            k = (r * rcos(row << 7)) >> 12                   # the ring narrows with the row
            n = (((r * rsin(row << 7)) >> 12) * gy) >> 8     # and climbs by the scaled sine
            pts.append([(((rcos(self.spin + (i << 8)) * k) >> 12), -n,
                         ((rsin(self.spin + (i << 8)) * k) >> 12))
                        for i in range(self.SEGMENTS)])
            gy += sy
        self.radius_speed += sx
        self.radius_offset += gx + self.radius_speed
        return pts


class PolarMesh:
    """`effect_e004_update_polar_mesh_state`: a four-fold polar mesh, in screen space.

    Six rings of five points from a radius and an angle that each carry a step
    and a step of their own, mirrored into four quadrants of 5x4 quads -- 80 in
    all. The centre drifts by a velocity that gains an acceleration **between
    rings**, and like the 4x4 grid the corners are written straight to screen
    coordinates about 0x80, never projected.

    One detail worth keeping: every curve here is read at the handler's own
    frame counter, which advances once an update, **except the scroll**, which
    the source reads at the caller's frame.
    """

    RINGS = 6
    POINTS = 5
    ORIGIN_X = 0x80

    def __init__(self, raw, bright, curves=()):
        self.raw = raw
        self.bright = bright
        self.curves = curves
        self.frame = 0
        self.scroll_u = 0
        self.scroll_v = 0
        f = lambda fmt, off: struct.unpack_from(fmt, raw, off)[0]
        self.flags = f('<H', 0x06)
        self.c0 = f('<I', 0x08)
        self.c1 = f('<I', 0x0c)
        self.colour_curves = f('<I', 0x10)
        self.radius = (f('<h', 0x20), f('<h', 0x26))
        self.vel_angle = (f('<h', 0x2c), f('<h', 0x32))
        self.vel_speed = (f('<h', 0x2e), f('<h', 0x34))
        self.acc_angle = (f('<h', 0x38), f('<h', 0x3e))
        self.acc_speed = (f('<h', 0x3a), f('<h', 0x40))
        self.angle = (f('<h', 0x44), f('<h', 0x48))
        self.page_flags = f('<H', 0x4c)
        self.brightness_row = f('<h', 0x4e)
        self.ot_depth = f('<h', 0x54)
        self.radius_step = (f('<h', 0x5c), f('<h', 0x60))
        self.radius_step_delta = (f('<h', 0x64), f('<h', 0x70))
        self.scroll_u_speed = (f('<h', 0x7c), f('<h', 0x88))
        self.scroll_v_speed = (f('<h', 0x80), f('<h', 0x8c))
        self.angle_step = (f('<h', 0x9c), f('<h', 0xa2))
        self.half_w = f('<h', 0xa8) // 2
        self.half_h = f('<h', 0xaa) // 2
        self.angle_step_delta = (f('<h', 0xb4), f('<h', 0xb6))
        self.texture_u = f('<H', 0xb8)
        self.texture_v = f('<H', 0xba)

    def rings(self):
        """points[ring][0..4], the quadrant one ring covers."""
        n = self.frame
        c = lambda nib: curve(self.curves, nib, n)
        radius = lerp(*self.radius, c((self.c0 >> 4) & 0xf)) << 12
        r_step = lerp(*self.radius_step, c(self.c0 >> 28)) << 4
        r_delta = lerp(*self.radius_step_delta, c(self.c1 & 0xf)) << 4
        angle = lerp(*self.angle, c((self.c0 >> 16) & 0xf)) << 4
        a_step = lerp(*self.angle_step, c((self.c1 >> 12) & 0xf))
        a_delta = lerp(*self.angle_step_delta, c((self.c1 >> 24) & 0xf))
        out = []
        for _ in range(self.RINGS):
            p = [((rcos(angle + (i << 8)) * (radius >> 12)) >> 12,
                  (rsin(angle + (i << 8)) * (radius >> 12)) >> 12) for i in range(4)]
            p.append((-p[0][1], p[0][0]))     # the fifth point closes the quadrant
            out.append(p)
            radius += r_step
            r_step += r_delta
            angle += a_step
            a_step += a_delta
        return out

    def drift(self):
        """The centre, and the velocity and acceleration it gains between rings."""
        n = self.frame
        c = lambda nib: curve(self.curves, nib, n)
        centre = list(spawn_position(self.raw, c(self.c0 & 0xf))[:2])
        f = c((self.c0 >> 8) & 0xf)
        va = lerp(*self.vel_angle, f) << 4
        vs = lerp(*self.vel_speed, f) << 4
        vel = [(rcos(va) * vs) >> 12, (rsin(va) * vs) >> 12]
        f = c((self.c0 >> 12) & 0xf)
        aa = lerp(*self.acc_angle, f) << 4
        as_ = lerp(*self.acc_speed, f) << 4
        acc = [(rcos(aa) * as_) >> 12, (rsin(aa) * as_) >> 12]
        return centre, vel, acc

    def texture(self, frame):
        """The scroll window. This curve alone is read at the caller's frame."""
        j = curve(self.curves, (self.c1 >> 4) & 0xf, frame)
        self.scroll_u += lerp(*self.scroll_u_speed, j) << 4
        while self.half_w and self.scroll_u >= self.half_w << 12:
            self.scroll_u -= self.half_w << 12
        while self.half_w and self.scroll_u < 0:
            self.scroll_u += self.half_w << 12
        j = curve(self.curves, (self.c1 >> 8) & 0xf, frame)
        self.scroll_v += lerp(*self.scroll_v_speed, j) << 4
        while self.half_h and self.scroll_v >= self.half_h << 12:
            self.scroll_v -= self.half_h << 12
        while self.half_h and self.scroll_v < 0:
            self.scroll_v += self.half_h << 12
        return (self.scroll_u >> 12) + self.texture_u, (self.scroll_v >> 12) + self.texture_v

    def colour(self, frame):
        if not (self.flags & GEOMETRY_FLAG_COLOUR_CURVES):
            return 0x80, 0x80, 0x80
        k = self.colour_curves & 0xfff
        return (palette(self.curves, k & 0xf, frame),
                palette(self.curves, (k >> 4) & 0xf, frame),
                palette(self.curves, (k >> 8) & 0xf, frame))

    QUADRANTS = ((1, 0, 0, 1), (0, -1, 1, 0), (-1, 0, 0, -1), (0, 1, -1, 0))

    def quads(self, frame):
        """The eighty screen-space quads: four quadrants of five rings of four."""
        rings = self.rings()
        centre, vel, acc = self.drift()
        u, v = self.texture(frame)
        red, green, blue = self.colour(self.frame)
        row = self.bright[self.brightness_row % len(self.bright)]
        depth = max(4, min(self.ot_depth, OT_DEPTH_LIMIT - 1))
        out = []
        for r in range(self.RINGS - 1):
            for qx, qy, qz, qw in self.QUADRANTS:
                for i in range(4):
                    a, b = rings[r][i], rings[r][i + 1]
                    c2, d2 = rings[r + 1][i], rings[r + 1][i + 1]
                    def put(p):
                        return (centre[0] + qx * p[0] + qy * p[1] + self.ORIGIN_X,
                                centre[1] + qz * p[0] + qw * p[1])
                    def tint(k):
                        return ((k * red) >> 12, (k * green) >> 12, (k * blue) >> 12)
                    t, bo = row[r % len(row)], row[(r + 1) % len(row)]
                    out.append(dict(
                        pts=[put(a), put(b), put(c2), put(d2)],
                        uv=[(u, v), (u + self.half_w, v),
                            (u, v + self.half_h), (u + self.half_w, v + self.half_h)],
                        rgb=[tint(t), tint(t), tint(bo), tint(bo)],
                        depth=depth, screen=True,
                        abe=(self.page_flags & 4) == 0,
                        mode=self.page_flags & 3))
            centre[0] += vel[0] >> 12
            centre[1] += vel[1] >> 12
            vel[0] += acc[0]
            vel[1] += acc[1]
        self.frame += 1
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
