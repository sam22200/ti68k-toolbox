# Measured renderer comparison

MC68000 datasheet-cycle emulation; AMS costs estimated. These are CPU estimates, not physical FPS. The live application is capped at 12.8 FPS; add the verified approximately 17,500-cycle LCD copy and interrupt costs. All moving runs use the same 48-frame yaw/pitch sweep, five/ten actors and three resolutions.

## Tenchu common fixture, 80x50, five actors

| Method | Mean cycles incl. update | Mean ms at 12 MHz | CPU FPS ceiling | Conservative minimum CPU FPS | Worst render ms |
|---|---:|---:|---:|---:|---:|
| Height DDA | 1,499,872 | 124.989 | 8.0 | 7.22 | 138.392 |
| Filled polygons | 1,210,747 | 100.896 | 9.91 | 9.13 | 109.513 |
| Portal polygons | 1,244,762 | 103.73 | 9.64 | 8.89 | 112.379 |
| Plane floor hybrid | 1,556,074 | 129.673 | 7.71 | 7.02 | 142.381 |
| Hidden line | 1,578,526 | 131.544 | 7.6 | 6.98 | 143.299 |

Minimum CPU FPS and worst-frame estimates use worst render plus the reported steady-frame worst update, which need not occur on the same frame; initialization is outside these frame zones. Per-frame minimum/maximum render samples and stage totals are in each CSV; nested zones overlap. Worst render includes the first frame; ti-cycles zone maxima alone deliberately exclude that call.

## All common scenes at 80x50

| Scene / actors | Height | Filled | Portal | Floor | Lines |
|---|---:|---:|---:|---:|---:|
| Tenchu B / 5 | 8.0 | 9.91 | 9.64 | 7.71 | 7.6 |
| Tenchu B / 10 | 7.69 | 9.42 | 9.17 | 7.41 | 7.31 |
| FFT A / 5 | 8.79 | 14.95 | 14.37 | 10.43 | 10.94 |
| FFT A / 10 | 8.41 | 13.95 | 13.42 | 9.93 | 10.39 |
| MGS C / 5 | 8.63 | 12.73 | 19.34 | 9.32 | 9.15 |
| MGS C / 10 | 8.24 | 11.94 | 19.3 | 8.89 | 8.73 |
| Vagrant D / 5 | 9.1 | 15.98 | 15.31 | 11.0 | 12.18 |
| Vagrant D / 10 | 8.69 | 14.85 | 14.25 | 10.45 | 11.51 |

Geometry differences: Height DDA flattens the Tenchu roof, thickens the bridge into a single column, and cannot represent the Vagrant bridge underside. The other four share identical source faces. Portal cells are authored for the MGS fixture only; the other three are single cells. These are synthetic fixtures, not extracted FFT/MGS/Vagrant game levels.

## Original training

| Mode / resolution | Mean cycles | CPU FPS ceiling | Worst render ms |
|---|---:|---:|---:|
| Lossy heightfield / 160x100 | 1,751,900 | 6.85 | 145.581 |
| Lossy heightfield / 80x50 | 797,208 | 15.05 | 66.851 |
| Lossy heightfield / 80x100 | 1,074,565 | 11.17 | 89.75 |
| Polygon camera motion / 160x100 | 2,618,871 | 4.58 | 231.038 |
| Polygon camera motion / 80x50 | 2,247,605 | 5.34 | 195.347 |
| Polygon camera motion / 80x100 | 2,687,268 | 4.47 | 235.395 |
| Portal path / single cell / 160x100 | 2,685,309 | 4.47 | 236.61 |
| Portal path / single cell / 80x50 | 2,284,837 | 5.25 | 198.465 |
| Portal path / single cell / 80x100 | 2,753,600 | 4.36 | 240.966 |
| Base plane floor hybrid / 160x100 | 3,944,746 | 3.04 | 374.161 |
| Base plane floor hybrid / 80x50 | 2,615,211 | 4.59 | 234.507 |
| Base plane floor hybrid / 80x100 | 3,439,674 | 3.49 | 315.307 |
| Hidden-line camera motion / 160x100 | 4,301,376 | 2.79 | 384.208 |
| Hidden-line camera motion / 80x50 | 3,148,630 | 3.81 | 275.967 |
| Hidden-line camera motion / 80x100 | 3,900,102 | 3.08 | 346.935 |
| Polygon static inspection / 160x100 | 148,043 | 81.06 | 219.182 |
| Polygon static inspection / 80x50 | 249,364 | 48.12 | 188.895 |
| Polygon static inspection / 80x100 | 257,807 | 46.55 | 225.046 |

Static averages include one expensive first render and 47 cached frames. They do not imply camera motion at that rate. The unmodified mesh renderer is too slow for a 10–15 FPS moving inspection mode. The ray adapter is faster but cannot be treated as geometrically equivalent: walls are thickened and slopes/layers are lost. 29,638 bank bytes, 221 vertices, 156 planar-merged faces, 100 ledges; no stored room screens.

The native portal path has one cell, so its overhead does not buy visibility savings. Native floor mapping textures only the base plane; elevated native faces remain flat shaded. Separate elevated mappings are implemented and tested in the common box fixtures.

## Sprite scaling (all five/ten visible; no background reuse)

| Method / resolution / actors | Sprite stage cycles | Scale stage cycles (included) | Cache hits / misses |
|---|---:|---:|---:|
| Baked A / 160x100 / 5 | 359110 | 0 | 0 / 0 |
| Baked A / 160x100 / 10 | 719342 | 0 | 0 / 0 |
| Baked A / 80x50 / 5 | 107450 | 0 | 0 / 0 |
| Baked A / 80x50 / 10 | 211458 | 0 | 0 / 0 |
| Baked A / 80x100 / 5 | 190290 | 0 | 0 / 0 |
| Baked A / 80x100 / 10 | 373696 | 0 | 0 / 0 |
| Runtime NN B / 160x100 / 5 | 718188 | 357048 | 0 / 0 |
| Runtime NN B / 160x100 / 10 | 1438712 | 715310 | 0 / 0 |
| Runtime NN B / 80x50 / 5 | 213498 | 104018 | 0 / 0 |
| Runtime NN B / 80x50 / 10 | 424128 | 208610 | 0 / 0 |
| Runtime NN B / 80x100 / 5 | 296338 | 104018 | 0 / 0 |
| Runtime NN B / 80x100 / 10 | 586366 | 208610 | 0 / 0 |
| LRU C / 160x100 / 5 | 363974 | 2980 | 238 / 2 |
| LRU C / 160x100 / 10 | 725930 | 2980 | 478 / 2 |
| LRU C / 80x50 / 5 | 110203 | 869 | 238 / 2 |
| LRU C / 80x50 / 10 | 215935 | 869 | 478 / 2 |
| LRU C / 80x100 / 5 | 193043 | 869 | 238 / 2 |
| LRU C / 80x100 / 10 | 378173 | 869 | 478 / 2 |

All methods deliberately use the same eight scale bins and two animation poses; their screens match. The cache test is a warm, repetitive workload, not a promise about animated production actors. Mixed moving workloads can be rerun with --sprite-mode 2 to expose misses/thrashing. Baked pixels cost 2,208 Flash bytes; the shared popcount table costs 256; metadata/raw source costs 137. Raw ink+mask costs 32 bytes/pose versus 1,104 baked bytes/pose. Four cache slots allocate 2,064 bytes. The all-method harness allocates that cache in A/B too so methods can be switched; this is actual harness RAM, not a claimed per-method saving. A specialized A/B build could omit it, but that saving is not measured here.

The additional moving-camera cache fixture has four misses at each resolution: 141 hits / 4 misses for five actors and 381 / 4 for ten at 80x50 (97.2% / 99.0% hit rates). Some actors leave the view during that sweep; the static fixture above is the all-visible comparison. Neither workload exhausts all eight sizes and many animation poses simultaneously.

## Floor and presentation variants

| Variant / Tenchu B / 80x50 / five actors | Mean cycles | CPU FPS ceiling |
|---|---:|---:|
| Two-pixel floor sampling | 1468868 | 8.17 |
| Flat floor | 1212229 | 9.9 |
| Full wireframe | 3022484 | 3.97 |
| Black silhouette | 1225621 | 9.79 |
| Sparse dither + outlines | 1582861 | 7.58 |

Flat ground wins this implementation. Outline/hidden-line work is not inherently cheaper than filled spans. Full wireframe shows hidden structures and is unsuitable for depth-valid aiming. Silhouettes lose interior ledge cues. Readability observations are qualitative engineering review, not a user preference study.

## Memory, executable and tables

All-method transfer file: 24,591 bytes; executable variable data: 24,501; stock AMS limit: 24,576. Current cycle binary SHA-256: `50a1c1346ef1bfc06e7cffab193fa516821c89dcea63dc5926b7561343ba2e88`.

Tracked heap workspaces: world 1,028; framebuffer/cache/column spans 8,042; polygon/portal 8,644; sprite cache 2,064 = **19,778 bytes**. Live mono runtime adds 3,840 bytes: **23,618**, excluding program, AMS allocations, globals and stack. Reserve roughly 2 KB for nested rendering stack as a conservative allowance, not a measured stack high-water mark. ti-cycles allocates two planes for its generic screenshot path, so its runtime allocation is larger than live mono. Archive banks consume no copied data RAM; unarchived banks additionally occupy their full payloads. The 4,808-byte sine/reciprocal/row tables live inside the executable and are included in its size.

Standalone height transfer file: 18,809 bytes. Polygon workspace is zero; live tracked workspace including runtime plane is **14,974 bytes**. Remaining polygon methods share the 8,644-byte workspace in this implementation, so measured RAM efficiency ranks height first and the other four tied.

## Ranking and production recommendation

Measured performance, average of the four common half-resolution five-actor scenes: Portal polygons, Filled polygons, Hidden line, Plane floor hybrid, Height DDA. Original training ordering is different: lossy heightfield, filled polygons, single-cell portal path, base-floor hybrid, hidden line.

| Criterion | Ranking / qualification |
|---|---|
| Geometry | Filled / portal / floor / hidden-line tied for expressible polygons; heightfield last. All painter paths share sorting limitations. |
| Readability | Filled+clear ledge markers preferred in review; hidden lines clarify ledges but cost more; silhouette and wireframe sacrifice cues; busy checker floors add noise. |
| Complexity | Height, filled, hidden-line, floor hybrid, portals (graph/general preprocessing increases complexity). |
| Reuse | Filled geometry best baseline; portal optimization where real room cells exist; height useful for restricted MGS modes; floor/lines remain optional styles. |

Use compact archived geometry, monochrome flat spans, early world-plane rejection, quantized baked sprites and last-frame reuse. For Tenchu, retain true geometry for ledge/roof inspection and implement native-map visibility before production; use the cheaper height adapter only where its losses are acceptable. For MGS-like interiors, portals can reduce work; the native training has no such measured optimization yet. Prefer simple flat floors over software Mode 7. Hidden line is an optional presentation, not the speed winner. No recommendation assumes full Z buffering, pre-rendered room orientations, modern caches or SIMD. Physical timing and a complete training route remain prerequisites for a production claim.
