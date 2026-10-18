# Camera configuration

`make camera-config` builds the full PC game and an interactive native TI camera
review with the same configuration code. The feature uses `FFT_CONFIG`; historical
research/control builds keep their original defaults and checksums.

Press **CATALOG** (PC **F4**) in a settled player view to open the menu. APPS still
opens the turn list. Use up/down to select one of two checkboxes, then ENTER,
2nd or left/right to toggle it. CATALOG, ESC or SHIFT closes the menu and applies
the view. Labels change with the selected value.

| Section | Unchecked (default) | Checked |
|---|---|---|
| Perspective view | Wide view | Close view |
| Perspective view | Textured | Filled wireframe (untextured mesh) |

Both checkboxes start unchecked on a new battle. Textured rotations and their
archive banks were retired to save space; F1/F5 always animate in native160x100 low poly.
Choices persist through
inspection entry/exit and camera switches during that battle. MODE still changes
close/wide inspection, and ALPHA toggles textured/filled mesh in this build; the menu
reflects those shortcuts. F1/F5 rotations remain animated; perspective entry,
return and zoom remain direct cuts, and automatic attack shots remain disabled.

The opaque menu freezes tactical state, CT, RNG and actor animation. It does not
run a blocking geometry render when an option changes. After sixteen quiet frames,
the selected forward inspection geometry is prepared cooperatively in a separate cache.
Closing and entering inspection consumes a ready result or finishes a matching partial
job; mismatches use ordinary rendering. See [idle preparation](PREFETCH.md). Opening is blocked during walking, attacks, camera turns,
banners, AI processing and battle results. Closing consumes the key so it cannot
trigger another action underneath. Three state bytes store the menu and filled-mesh
choice; close/wide uses the existing preference byte. Alignment leaves total State
size unchanged. The idle geometry cache adds 20,284 optional heap bytes in several blocks;
allocation failure retains synchronous inspection. It preserves the ISO scene/depth/HUD
buffers and does not store stale actor poses or tactical snapshots. Inactive Status
storage holds temporary vertices; Status/AT-list/rotation cancels a partial job before
reusing that storage. A matching inspection finishes it before clearing FWork.

The user's "filled wireframe" means complete flat-shaded, untextured polygons
with visible mesh edges. Faces remain filled and retain their material contrast;
this is not the earlier white-face hidden-line experiment. The unchecked mode
uses scenery textures. Both retain triangle depth, solid architecture and character
occlusion. Two view caches are invalidated together when necessary so a cached
textured opposite view cannot reappear after selecting untextured geometry.

## Run

From the repository root:

```sh
make -C games/fft camera-config
cd games/fft/x/config/data
../../../fftcfg_pc --scenario 65
```

The PC game retains its complete tactical menus and filtered perspective texture
reference. Those filtered PC samples do not establish native TI mapping performance.
Its F1/F5 path uses the same three native160x100 low-poly intermediate frames as the TI.

Native review: send `games/fft/x/config/fftcfg.89z` and **every** archived `.89y` bank
in `x/config/data/` on a clean boot; run `fftcfg()`. The current native size and hash
are recorded in `x/config/checked.json`. The adjacent PC-only `ffttex.bin`/`fftiso.bin` files are not sent to the TI.
The review retains the existing compact isometric presentation, native actors and
real camera/turn/configuration input handling. Its two forward filled inspection
backgrounds are offline textured fixture banks; other camera descriptors use the
native geometry renderer. The menu does not remove these pre-existing prototype
limits or claim a full tactical calculator release.

The full tactical+camera native packaging remains separate work; the earlier
79,380-byte link log predates storage cleanup. After Auto-battle, the bounded review is 62,515 file
bytes and sends 158,468 raw data bytes in nine banks. See the
[single-facing portraits and bounded blade cache](BLADE_CACHE.md).
The [live rotation contours](TURN_EDGES.md) add 552 executable bytes without new banks
or Work allocation; their CPU cost is measured separately from the historical turns.
Auto-battle's tactical UI is available in the full PC build; the compact native camera
review remains a presentation fixture. See [Auto-battle](AUTO_BATTLE.md) for the separate
ordinary tactical TI executable and tests.
It is built and checked independently. To avoid the
previous TiEmu frame-order issue, it opts into `RT_GRAY_COPY_PRESENT`: draw in the
existing hidden allocation, copy finished planes to fixed grayscale addresses.
This presentation control does not change the runtime default for other games or
reuse previous buffer-flip timing claims. It has no new framebuffer allocation.

## Checks

`make camera-config-check` exercises the actual full game: default wide entry,
dynamic choices, held/open/close keys, frozen tactical data/RNG, APPS versus CATALOG,
animated low-poly turns, textured/filled-mesh cache changes and inspection re-entry.
`make camera-config-ti` cross-checks484 consecutive native/PC pictures through both
rotations, all checkboxes, close/wide, opposite views and returning to the AT list.
Results, source views, native variables and hashes are in `x/config/`. A separate
scenery-only checksum excludes the HUD and checks that textured and untextured
filled modes really differ at the same camera/actor pose; returning restores the
exact textured image.

The menu render zone is reported as zone3 in `x/config/cycles.log`. These are
CPU-only costs, excluding the grayscale ISR, fixed-plane copies and waits, rather
than emulator FPS measurements.
The ordinary FFT tests, legacy fixed-camera/cut tests and runtime input tests pass.

**Verified in isolated Titanium TiEmu:** the actual CATALOG key opens the menu,
ENTER toggled the former three options in the historical build.
The historical native screenshot is `x/config/menu-tiemu.png` and predates the
"Textured / Filled wireframe" label correction. Current menu images and native
frame checks are rebuilt headlessly; the previous screenshot and its binary hash
remain historical keyboard evidence. This milestone did not drive the user's
desktop or emulator session.

The runtime exposes CATALOG as a new `K_CATALOG` bit while retaining its legacy
`K_APPS` alias for other games. The configuration handler consumes CATALOG first.
No runtime key bits or scripts used by existing games are reassigned.
