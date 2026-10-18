# Original-resolution sprites: PC-only visual experiment

The existing perspective renderer already uses FFT battle sprites, through the isometric
16x26 bank. That bank reduces the source to 0.6 scale and adjusts contours/eyes for the
small LCD. Enlarging it for the close camera also enlarges those reduced pixels.

This review instead composes the original SPR/SHP idle frames before reduction, then
samples them directly at the final projected size. Feet anchors, physical scale, frame
selection, draw order, opaque scenery depth, camera and automatic cutaway come from the
actual portable C renderer. A one-screen-pixel white outline is added after scaling.
The original bank's synthetic oval shadows are not copied into the new source variants.

Two source variants are compared: the existing palette luminance thresholds in four
greys (without the reduced bank's face edits), and original RGB palette colours. Colour
is only a PC reference. Billboards retain the original oblique sprite artwork; these
are not new rear-camera-specific or pitch-specific character images.

Run `make fixed-original-sprites` in `games/fft`. The local disc is required; extracted
art, captures and GIFs stay ignored under `x/original-sprites/`:

- `current-original.gif`: existing reduced sprites versus original-resolution four greys,
  four camera rows (close/front, close/back, wide/front, wide/back).
- `grey-color.gif`: original-resolution four greys beside the original colours.
- `close-front.gif`, `close-back.gif`, `wide-front.gif`, `wide-back.gif`: native 160x100
  original-resolution four-grey animations.
- Corresponding PNG montages and `manifest-pc.json` (source/binary/disc-file hashes,
  per-frame poses, anchors, pixel counts and reference checks), plus per-view depth files.

This is an offline Python sprite compositor over frames and depth exported by C. It is
a reviewable visual proposal, not an integrated calculator sprite renderer. Production
code and asset banks are unchanged. No TI build, ti-cycles, xcheck or TiEmu run is involved.
GIF playback at 10 FPS is for review, not a performance measurement. Source-resolution
storage, scaling and cache costs must be assessed separately if this variant is chosen.

Verification covers 128 C source frames and 384 resulting variants. Every background-only
capture restores the original C framebuffer checksum afterward. When previous visibility
review captures are present, all 128 current frames must match them exactly. Complete
foreground depth rejection must leave the background unchanged, both HUD strips stay
unchanged, and the four-grey variant may contain only the four target tones.

Visual assessment: direct source sampling improves the close camera's contour and internal
detail. The wide view still has very few pixels per character; the reduced bank's manual
contrast/face treatment may remain preferable there. Original colours help identify the
artwork on PC but do not establish readability on the calculator's monochrome LCD.
