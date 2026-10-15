# Tenchu / perspective roadmap

- [x] Read the highest-priority height raycaster and FAT SDK architecture.
- [x] Study historical and supplied renderer references; separate license status.
- [x] Original HW2 memory/layout/toolchain constraints; stock AMS launch check.
- [x] Shared portable C harness, injection door, four common logical scenes.
- [x] Phase 1: height DDA, variable elevations, steps, doors, masked billboards,
  camera height, horizon pitch, height-sensitive column occlusion and aiming.
- [x] Phase 2: near/screen clipping, yaw/pitch, flat triangles/quads, slopes,
  bridges, world-plane/backface rejection, face/sprite painter ordering.
- [x] Phase 3: bounded convex-room tree, doorway spans, visibility rejection,
  parent/child clipping; closed/open door comparison in the MGS fixture.
- [x] Phase 4: pitched horizontal plane projection, elevated flat faces,
  full/two-pixel/flat floor comparisons; sloped roofs remain flat shaded.
- [x] Phase 5: hidden lines, full wireframe, silhouettes, sparse dither,
  mixed near fills/distant lines; ledge markers with an opaque white border.
- [x] Eight sprite sizes, two poses, baked/runtime NN/four-slot LRU comparisons,
  isolated five/ten visible sprite fixture and height-occlusion timings.
- [x] Datasheet stage/call/frame profiling, min/mean/worst render samples,
  buffer/workspace/table/file sizes, PC/TI equality and framebuffer exports.
- [x] Static inspection cache; no dirty-rectangle claim during camera motion.
- [x] Local original training archive, RAM-verified TMD vertices, exact planar
  merges, native stream bank, opening traversal/jump/ledge study.

## Next production milestones (not completed)

1. Find the writable original player state and measure collision/ledge/hook rules.
   Validate a whole training traversal, including gaps and landings. A straight
   grapple tween and snapped barriers are insufficient evidence of fidelity.
2. Partition the actual training mesh into cells or an offline visibility/order
   structure. The synthetic portal tree has no native-map performance claim.
   Resolve painter-order failures before treating the rasterizer as production.
3. Compare splitting large faces, actual room ordering and a tiny depth/coverage
   structure on identical pixels. Full Z buffering and BSP have not been timed.
4. Use measured hot spots to select an existing fast span/line primitive, then
   ask before writing any new 68000 assembly. C first; no unverified asm shortcut.
5. Original character/pose art and readable materials, after engine/traversal.
   Evaluate true four-tone drawing only after meeting the monochrome budget.
6. Define mission completion and the needed target/weapon mechanics, then bot-play
   the entire course, PC/TI replays, one final emulator milestone and physical HW2.
7. Integrate contextual adapters into real MGS / FFT / Vagrant Story gameplay.
   Current named scenes are fixtures; they do not implement those games' rules.

Do not replace the working height path or static cache while improving geometry.
The recommendation is mode-specific and provisional until real map visibility,
visual feedback and physical timing are measured.
