# Putting an original effect on the TI-89

Everything here is measured: the choreography from the disc with
`tools/effect_anim.py`, the drawing costs from
`.claude/skills/ti89-c-dev/reference/ti68k-performance.md`, which took them
with `ti-cycles` on datasheet timings.

## What changed: the console is no longer in the loop

The decoder now plays an effect's choreography straight from `EFFECT/Exxx.BIN`
-- cells, sequences, emitters, their children, the colour tracks and the effect
camera -- so **an effect can be rendered, measured and budgeted without running
the PlayStation**. `reading an original effect` (`effects.md`) holds what each
section means; this page is only the port.

That matters for budgeting more than for pictures: the numbers below come from
walking the real timeline, not from counting pixels in a video.

## The load, measured

Per 30 fps frame, over each effect's whole run, quads counted with their
`ticks` so a held cell counts on every frame it is up:

    effect   quads/frame   peak   filled px/frame   peak px
    Stasis          50.9    132            25,284    95,125
    Night           76.1    251           193,355   737,896
    Holy           124.8    438            78,089   420,724
    Cure 4          59.5    150            27,306    84,519
    Flare          171.8    237           246,978   351,451
    Haste           53.3    152            13,421    31,185
    Meteor         325.6    488           644,399 1,741,036

"Filled px" is the quads' total area with overlap, at the PlayStation's
256x240. A TI screen is 16,000 px, so Meteor paints **ten screenfuls a frame**
on average and twenty-eight at its peak. That is the real shape of the problem:
not the number of sprites, the fill rate.

## What a quad costs here

At the TI's 0.625 linear scale (160/256), sorted by the longest side:

    effect    <=4px    5-8    9-16   17-32   33-64    >64
    Stasis     2.2%  38.8%   47.9%    7.6%    2.2%   1.3%
    Night      0.1%   0.1%    3.0%   29.5%   64.7%   2.6%
    Holy       3.5%  31.0%   53.3%    4.4%    6.8%   0.9%
    Cure 4     6.9%  25.9%   51.4%   15.0%    0.8%   0.1%
    Flare     27.3%  13.6%   13.9%   21.7%   22.6%   0.9%
    Haste      0.0%  45.5%   51.1%    3.4%    0.0%   0.0%
    Meteor    33.3%  13.0%   20.0%   19.5%   10.4%   3.8%

Two families, and they want opposite treatments:

* **Stasis, Holy, Cure 4 and Haste are made of small quads** -- 80 to 97 % at
  16 px or under. Grey masked sprites: 4,600 cycles at 16x16, 2,700 at 8x8.
* **Night Sword is made of big ones** -- 94 % between 17 and 64 px. Those are
  its blade and its burst, the parts the port exists to show.
* **Flare and Meteor are both**: a third of their quads are 4 px or less
  (sparks) and a quarter are 17 px or more.

## The budget, and the three concessions that buy it

A frame is 360,000 cycles. The scene behind the effect is already on screen and
is restored by its dirty boxes, so call it **230,000 for the effect at 30 fps**.

Drawing every quad as a sprite, nothing fits -- 1.5x over for Haste, 19.8x for
Meteor. Three concessions, each measured:

**1. A spark is a pixel, not a sprite.** A quad of 4 px or less is a pixel OR
at ~200 cycles instead of a 2,700-cycle 8x8 sprite. That is a third of Flare's
and Meteor's quads for 7 % of the price.

**2. A quad over 16 px is a dithered flat, not a texture.** `GrayFastFillRect_R`
does 160x100 in 37,000 cycles -- **2.3 cycles a pixel**. A 32x32 flat costs
~2,400 against 10,900 for a 32-wide masked sprite, a 64x64 one ~9,400 against
~22,000. The texture inside these quads is a glow with a bright core; four greys
of dither carry that, and the white outline rule does not apply to light.

**3. Effects run at 15 fps.** The battle already redraws only what changes, and
an effect that steps every other frame has **720,000 cycles** a step. Nothing in
the choreography depends on 30 fps: the hold counts are in 60 Hz ticks and halve
cleanly.

With all three:

    effect     mean      peak   vs 230k (30fps)   vs 720k (15fps)
    Stasis   211,810   433,296            1.9x            0.6x   fits
    Cure 4   224,738   615,137            2.7x            0.9x   fits
    Flare    469,306   627,206            2.7x            0.9x   fits
    Haste    221,821   555,900            2.4x            0.8x   fits
    Night    317,699   830,651            3.6x            1.2x   trim
    Holy     549,142 1,499,568            6.5x            2.1x   trim
    Meteor 1,078,482 2,155,346            9.4x            3.0x   trim

**Four of the seven fit outright.**

## The three that do not, and what to do with each

* **Night Sword, 1.2x.** It is over on its peak frames only, and what is over is
  the blade. This is the one to let through: the user's standing note on this
  effect is that the blade must be *more* visible, not less. A 1.2x frame at
  15 fps is 830k against a 720k step -- one dropped frame at the burst, nothing
  a player reads as a stutter.
* **Holy, 2.1x.** Its 438-quad peak is the ring stack plus the beam plus the
  wash. Cap the per-frame quad count at ~200, dropping the smallest first: the
  beam and the rings are all in the 9-16 px band and survive, the 31 % of 5-8 px
  motes thin out. Its 360-degree camera turn is already out of scope.
* **Meteor, 3.0x.** Two separate problems. The rock itself is three polygons of
  260x364 -- at TI scale 162x227, a 37,000-cycle flat, and **worth paying for**:
  it is the effect. The other 325 quads a frame are the trail and the embers;
  cap them hard, one in three. The camera's pull-out to x0.40 cannot be
  reproduced, so the rock has to be authored smaller than the file says or it
  fills the screen for two seconds.

## What it costs in bytes

Only quads of 16 px or less become sprites; the bigger ones are flats and cost
nothing but their corners.

    effect   cells   polys   sprite bytes (2 planes + mask)
    Stasis     126     297     5.5 KB
    Haste       34      38     2.0 KB
    Cure 4      58      68     1.8 KB
    Holy        53      57     1.0 KB
    Night       54     112     0.6 KB
    Flare       34      34     0.5 KB
    Meteor      39      63     0.4 KB

**11.8 KB for all seven.** That is an archived data file read in place, not a
problem. The choreography itself -- keyframes, emitters, curves -- is a few
hundred bytes an effect and can ship decoded rather than as the original
section 7.

## What will not be ported, and why

* **The effect camera's rotation.** Five of the seven carry one, at least one a
  full yaw turn in twenty frames. The scene is a photograph composed offline per
  orientation; it cannot rotate. (This is the "Holy spins 360, we are not doing
  that" the port set aside from the start.)
* **The camera's zoom.** The tactical view is 0xC00 and effects ask for ONE, a
  x1.33 push-in, and Meteor pulls out to x0.40. A pre-composed scene cannot
  scale. Pan is the only camera move that transposes, as scrolling.
* **Additive blending.** Four greys have no accumulation; a stack of nineteen
  glows is one dithered shape. This is what the `base/2` colour gain already
  approximates on the preview side.
* **Per-particle colour curves.** They fade a particle over its life across
  r/g/b; with four greys that is at best three dither patterns deep.

## The order to build it in

1. **Haste** -- 0.8x, 38 polys, 2 KB, no big quad at all, and its two
   counter-rotating rings of roman numerals are the whole effect. The cheapest
   complete proof.
2. **Stasis Sword** -- 0.6x, already reviewed and approved on the preview.
3. **Cure 4** and **Flare** -- 0.9x each; Flare exercises the spark-as-pixel
   path on 27 % of its quads.
4. **Night Sword** -- the first one allowed to overrun, and the one whose big
   quads prove the dithered flat.
5. **Holy**, then **Meteor** -- both need the quad cap, and Meteor needs its
   rock re-authored for a camera that cannot pull out.
