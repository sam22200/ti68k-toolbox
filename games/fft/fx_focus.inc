// Focus dimming for combat effects: step 1 of campaign/COMBAT_FX.md -- the palette,
// the focus and the restoration. No effect, primitive or asset lives here.
//
// The finished picture is dimmed so that one unit reads against it, and that unit is
// then drawn again at normal brightness through its own cover mask, so the scenery
// still occludes it. Redrawing it over everything would be wrong, and the spec says so.
// The HUD is composed afterwards and stays bright.
//
// rt.h: grey = light + 2 * dark, which makes the dimming bitwise, with no lookup table
// and no per-pixel branch -- v -> min(v+1, 3) is exactly
//     d' = d | l        l' = d | ~l
// checked over the four levels and measured in experiments/gray/darken.c at 46,638
// datasheet cycles over the whole 160x100 picture and both planes, with the row
// unrolled: a five-iteration inner loop loses to the unrolled form, and widening the
// work to the full 30-byte stride to flatten the loop costs more than it saves.
//
// The spec's second, deeper level was built, reviewed on the picture and dropped
// (2026-11-27): it leaves the scenery on dark grey and black alone and the relief
// stops reading. One level is therefore all there is, so the dimming arrives in a
// single step; four greys offer no fade between "normal" and "one level down".
//
// Nothing here touches W->bg. The kept screen copy stays the undimmed scene, so the
// picture needs no repair when an effect ends: the next frame simply stops dimming.
// That also leaves copy_view()'s PC invariant -- the copy equals the view -- intact.

// The whole state is one byte, and it has to stay one byte: sizeof(Work) feeds MOD_SIG
// (module.h), so growing Work makes the core refuse every already-built module image and
// the battle never starts. One u8 fits the padding the struct already had; two did not.
//   0        off
//   1..n     on, unit n-1 stays bright
//   FX_ALL   on, nothing stays bright
#define FX_ALL 255
#define FX_NO_UNIT 255                  // what a caller passes for "dim everything"

// The 160 visible pixels of a row are its first five long words (screen_scene()).
static void fx_dark(void)
{
    u8 *lp = rt_light, *dp = rt_dark;
    s16 y;
    for (y = 0; y < RT_H; y++, lp += RT_PBYTES, dp += RT_PBYTES) {
        PlaneWord *l = (PlaneWord *)lp, *d = (PlaneWord *)dp;
        PlaneWord a0 = l[0], a1 = l[1], a2 = l[2], a3 = l[3], a4 = l[4];
        PlaneWord b0 = d[0], b1 = d[1], b2 = d[2], b3 = d[3], b4 = d[4];
        d[0] = b0 | a0; d[1] = b1 | a1; d[2] = b2 | a2; d[3] = b3 | a3; d[4] = b4 | a4;
        l[0] = b0 | ~a0; l[1] = b1 | ~a1; l[2] = b2 | ~a2; l[3] = b3 | ~a3; l[4] = b4 | ~a4;
    }
}

// Ask for a focus: unit stays bright, or FX_NO_UNIT for the whole picture. on = 0 restores.
// The caller owns the timing, so the spec's anticipation and focus phases belong to the
// effect scheduler, not here.
static void fx_focus(u8 unit, u8 on)
{
    W->fx_sel = !on ? 0 : unit >= NUNIT ? FX_ALL : (u8)(unit + 1);
}

// After the units are drawn and before the HUD. Returns whether it dimmed, so the frame
// budget is charged for what happened rather than for what was asked.
static u8 fx_apply(const Feet *f, const u8 *vis)
{
    u8 sel = W->fx_sel;
    if (!sel) return 0;
    fx_dark();
    // The focused unit returns to normal brightness, still occluded by the scenery:
    // its cover mask is the one the ordinary pass just used. A unit out of the view has
    // no mask and nothing to redraw, which is why vis is checked and not just the index.
    if (sel != FX_ALL && --sel < NUNIT && vis[sel])
        draw_unit_covered(f[sel].x - st.camx, f[sel].y - st.camy, st.rot, sel, &W->cover[sel]);
    return 1;
}
