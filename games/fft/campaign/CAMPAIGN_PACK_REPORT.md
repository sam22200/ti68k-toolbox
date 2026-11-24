# FFT campaign: measured storage and accepted pack plan

Date:2026-11-24. Target: TI-89 Titanium, retail PS1 US campaign.

## Decisions

- Keep the user-approved **eight main packs**: I whole; II split after Goug; III after Orbonne; IV after Bethla and after Igros.
- **Budget against the measured archive ceiling of 2,555,826 bytes**, not TI's2.7MB. That figure is `allExceptBaseCode` (2,686,976) and is not archivable. Add 23 bytes per archived variable; the reserve absorbs it.
- **native30 is the official pose profile for every budget here.** It is the list the shipping renderer already uses. extended_regular stays a stress control and is now ruled out: it breaks the archive ceiling and the actor-bank ceiling at once.
- Plan separate optional quest and Deep Dungeon packs. Content-pack phase never changes the original story chapter or town progression.
- Use one orientation per scripted scene by default. Keep extra source-required directions, camera crops, pans and gestures.
- Keep complete unique appearances for recruits and major story actors. Pool identical sprites, poses and mirrored frames physically.
- Keep complete sprites in the combat renderer. Naive lossless two/three-part preparation is slower; it does not save space in the native30 corpus.
- **One canonical body per person, Ramza and Delita included.** The user dropped optional costume changes. Their measured cost, 47,232 bytes for Ramza2/3 + Delita2/3 and 58,973 for the six-outfit set, is now free budget, not a reserved allowance.
- Costume variants share no sprite band: the measured two/three-part increments are the same size as the whole-sprite ones, so a second outfit always costs a second complete body.
- Reduce scene-only actors to their actual scripted poses when authoring. No generic head/body replacement has been visually approved or integrated.

This report measures archive encodings and a bounded pose-preparation prototype. It does not install new banks, rip the complete campaign, change the shipping renderer, or certify final Flash/RAM fit. ABI99 and all reviewed art remain unchanged.

## 1. Available storage and measured release

The ceiling is now measured, not assumed. `experiments/archfree/` calls `EM_survey` and logs its six figures through tilog, so `ti-mem log` reads them without a screenshot. On the project's clean Titanium `.sav`, `inUse + free` is constant at **2,555,826 bytes**: that is what archived variables may actually occupy. The2.7MB TI quotes is `allExceptBaseCode` (2,686,976), which also counts one permanently unused65,536-byte sector and the archive's own bookkeeping. The former 2,700,000 planning ceiling overstated capacity by 144,174 bytes on an empty calculator, before any other app or file. [TI capacity table](https://education.ti.com/en/customer-support/knowledge-base/ti-83-84-plus-family/general-information/12174).

Archive overhead was measured too, and it is small and linear: 36 archived variables holding694,890 stored bytes occupied 695,696 (+806,22.39 each);62 variables holding989,381 occupied 990,766 (+1,385,22.34 each). Budget 23 bytes per archived variable. There is no sector-fragmentation term to model: AMS packs archived blocks contiguously and only garbage collection works by sector. An installed set of 150 variables therefore costs about3,450 bytes, which the 131,072-byte reserve already absorbs; it is not budgeted again per pack. Variables sent unarchived cost RAM instead, like the core's5,381-byte `fftstory`.

| Item | Bytes | Status |
|---|---:|---|
| Promoted ABI99 core, stored variables/size words | 700,273 | Measured |
| Same core, transport file | 701,169 | Measured; excluded from archive sums |
| Archivable ceiling, empty Titanium | 2,555,826 | **Measured**, EM_survey |
| TI's quoted capacity (allExceptBaseCode) | 2,686,976 | Measured; NOT archivable |
| Archive overhead per archived variable | 23 | Measured22.39/22.34, rounded up |
| Core installed, 36 archived variables | 695,696 | Measured in the archive |
| Core + Chapter I pilot,62 variables | 990,766 | Measured in the archive |
| Flash/VAT/save reserve | 131,072 | Retained;128KiB |
| Dropped wardrobe increment, Ramza+Delita | 47,232 | Measured; not retained, not budgeted |

MB/KB in this report are decimal. KiB means1,024 bytes. Actor-study stored counts include compressed streams, u16 offsets, mirrored logical mappings, recipe/part headers and bounded variable DAT/size overhead. Archive headers are now measured and budgeted; other installed software on the user's own calculator is still not, and comes off the same ceiling.

## 2. Human bodies, parts and costume measurements

The corpus reads52 named-human identity records (38 logical people) and38 generic job bodies from the local disc. Each pose uses the existing16x26,208-byte four-plane converter. Preserve white outlines, transparency and the internal hit mask. The study changes storage only and reconstructs every retained input pose byte for byte.

Two coverage controls are reported:

- **native30:** the previous thirty-pose proxy, using the same existing source-frame list.
- **extended_regular:** all regular-body SHP frames plus native30 for named bodies; generic jobs remain native30. This includes many unused gestures as a wardrobe stress case. It is not a complete source-SEQ/attack/SP2 animation rip.

There are5,085 converted model/pose pairs. 685 empty source-shape attempts are explicit;2 other attempts fail the existing converter (Rudvich pose24 and male Summoner pose86, both eye-position bounds). They are not repaired or silently treated as certified animations. The source-sized model profiles are storage controls, not approved final sprite sheets.

### 2.1 Lossless whole sprites versus row parts

The two-part control splits rows0..7 /8..25. The three-part control splits rows0..7 /8..17 /18..25. These fixed bands are **not anatomical heads, torsos and feet for arbitrary attacks or KO poses**. Parts keep all four planes and follow the canonical whole-frame orientation. Each unique complete pose has an explicit2/3-ID recipe; no free descriptors are assumed.

| Corpus:38 canonical named +38 generic bodies | Whole | Two parts | Three parts |
|---|---:|---:|---:|
| native30 | 312,656 | 333,032 | 340,824 |
| extended_regular | 539,728 | 552,522 | 530,595 |

For native30, splitting adds20,376 /28,168 bytes. For extended_regular, two parts add12,794 bytes and three save9,133 bytes. The larger named-only library has more repeated rows, but that does not make all combat actors cheaper or faster. Important characters retain exactly their source-converted appearance in every lossless case.

### 2.2 Hybrid scopes

| Profile | Actors split; everyone else whole | Two-part saving | Three-part saving |
|---|---|---:|---:|
| native30 | named_only | -8,689 | -10,444 |
| native30 | scene_only | -2,996 | -4,637 |
| native30 | secondary_only | -4,214 | -4,068 |
| extended_regular | named_only | -1,132 | 26,832 |
| extended_regular | scene_only | -2,778 | -270 |
| extended_regular | secondary_only | 229 | 14,428 |

Negative savings mean a larger bank. Hybrid counts include separate whole/part dictionaries. Do not add their savings together: these are alternative encodings.

### 2.3 Head substitution: archive potential, not approved art

| Corpus | Full canonical bodies + jobs | Count-only shared-body option | Potential difference |
|---|---:|---:|---:|
| native30 | 312,656 | 256,990 | 55,666 |
| extended_regular | 539,728 | 398,931 | 140,797 |

This counterfactual keeps18 important/recruitable people fully unique and counts unique upper bands for20 secondary people against an already required generic body catalogue. It does not select compatible body templates, rig head anchors, repair outline seams or approve altered silhouettes. No changed character picture was integrated or used in the CPU benchmark. **No head-substitution saving is credited to the pack budgets.**

### 2.4 Costume variants: measured, then dropped

The user dropped optional costume changes. Every person keeps one canonical body. The table prices what was given up; none of it is budgeted anywhere else in this report.

| Dropped set | native30 body increment | Extended body increment | Extended + portrait/reference allowance |
|---|---:|---:|---:|
| ramza_only | 8,761 | 22,745 | 23,457 |
| delita_only | 8,673 | 23,063 | 23,775 |
| ramza_delita | 17,434 | 45,808 | 47,232 |
| wiegraf_only | 3,844 | 10,705 | 11,061 |
| All six story-outfit candidates | 21,398 | 56,837 | 58,973 |
| All fourteen same-name variants | 24,749 | 62,597 | Not all are clothing variants |

The optional source identity17 control adds only324 bytes in extended_regular (mapping records; retained pictures deduplicate). This does **not** price an independently authored hooded executioner: special scene-only shapes and attack/SP2 pages still require their actual source extraction.

A second outfit cannot reuse the first one's legs and torso. The same corpus measured with the row-band encodings gives the increments below: splitting the sprite recovers nothing, because an alternate chapter sheet redraws the whole figure, head band included.

| Dropped set, native30 | Whole | Two parts | Three parts |
|---|---:|---:|---:|
| ramza_only | 8,761 | 8,729 | 8,982 |
| delita_only | 8,673 | 8,882 | 8,963 |
| ramza_delita | 17,434 | 17,621 | 17,935 |

Ramza2/3 costs8,761 bytes whole and8,729 split in two: the shared lower band saves32 bytes,0.4%. Delita2/3 is worse split (+209). Head-only substitution is measured in2.3 for *different people* sharing one generic body, not for one person's wardrobe. Keep every same-name wounded, guest or undead identity canonical as well.

## 3. Native preparation cost and RAM

The standalone C prototype reconstructs45 diverse poses, ordinary and mirrored, through all three encodings:270 cases. PC ASan/UBSan and the actual68000 binary under ti-cycles report zero byte differences and signature 0x7199. The native path calls the existing verified ZX0 and ExtGraph mirror routines. No new assembly, I/O driver or live TiEmu test.

| Preparation | Calls | Mean cycles | Maximum cycles | Mean vs whole |
|---|---:|---:|---:|---:|
| Whole | 90 | 26,804 | 42,336 | +0.0% |
| Two parts | 90 | 30,335 | 45,976 | +13.2% |
| Three parts | 90 | 30,705 | 46,408 | +14.6% |

These are pose-cache-miss microbenchmarks, not complete game frames or hardware FPS. Per-mode maxima can occur on different poses. The warm complete-sprite blit would remain the same, but extra preparation still consumes the frame budget. Current selected Zeklaus13 rendering has only 3,168 cycles of tightest measured margin, so the average3.5..3.9K extra preparation cannot be assumed to fit.

The simple prototype needs an additional144-byte part workspace for two parts, or80 bytes if compiled specifically for three. Both bench modes share144 bytes. A208-byte assembly temporary can use the role of the current mirror temporary; this is not an installed RAM optimization. The actual game has not been linked with the new path, its stack/lifetimes are not certified, and the42-entry cache is not reduced. Installed ABI99 Zeklaus13 previously had436 bytes free in the settled diagnostic. Keep the shipping whole-pose path.

## 4. Scene-only actors and character lifetime

Appearance uniqueness and storage ownership are separate. A major enemy can keep a unique body without remaining in every later pack. A recruited person must keep their body available after their last scripted line. Names and speaker portraits remain in shared identity data.

The eleven scene-only human candidates are Ovelia, Larg, Goltana, Funeral, Simon, Teta, Barinten, Balmafula, Rudvich, human Draclau and Ajora. Human Draclau is separate from the Queklain boss body. These candidates still need an authored source-action inventory; no required injury/death/gesture may be removed merely because the person does not fight.

| Same eleven people, whole sprites | Stored bytes | Meaning |
|---|---:|---|
| standing_walking10 | 14,744 | Minimum motion probe; does not cover all scenes |
| with_hurt_kneel_ko16 | 21,891 | Adds hit/kneel/KO controls; gestures remain |
| native30 | 39,029 | Previous compact pose proxy |
| extended_regular | 88,100 | Broad gesture/body library control |

The16-pose scope is17,138 bytes smaller than native30, and66,209 smaller than the broad library. Those are alternative corpus measurements, **not savings already earned in the campaign**. Extract exactly the poses used by each scene, including rare deaths and gestures.

### 4.1 All38 named-human lifetime candidates

Chapters below come from the bounded static ENTD/message census. Silent actors, script graphics replacements and later party use can extend them. They are not proof of an actor's last possible use.

| Person | Source narrative chapters | Appearance plan | Lifetime policy |
|---|---|---|---|
| Agrias | I, II | Unique | Retain after joining |
| Ajora | IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Algus | I | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Alma | I, III, IV | Unique | Active narrative packs; core only if useful across packs |
| Balk | IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Balmafula | IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Barinten | III | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Beowulf | optional | Unique | Retain after joining |
| Celia | III, IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Cloud | IV, optional | Unique | Retain after joining |
| Delita | I, II, III, IV | Unique | Active narrative packs; core only if useful across packs |
| Draclau | II | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Dycedarg | I, II, IV | Unique | Active narrative packs; core only if useful across packs |
| Elmdor | I, III, IV | Unique | Active narrative packs; core only if useful across packs |
| Funeral | IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Gafgarion | I, II | Unique | Active narrative packs; core only if useful across packs |
| Goltana | II, III, IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Izlude | III | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Kletian | IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Larg | I, IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Lede | III, IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Malak | III, IV | Unique | Retain after joining |
| Meliadoul | IV | Unique | Retain after joining |
| Mustadio | II, optional | Unique | Retain after joining |
| Olan | III, IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Orlandu | II, III, IV | Unique | Retain after joining |
| Ovelia | I, II, III, IV | Unique | Active narrative packs; core only if useful across packs |
| Rafa | III, IV | Unique | Retain after joining |
| Ramza | I, II, III, IV, optional | Unique | Retain after joining |
| Reis | optional | Unique | Retain after joining |
| Rofel | IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Rudvich | II | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Simon | I, II, III | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Teta | I | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Vormav | II, III, IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |
| Wiegraf | I, III | Unique | Active narrative packs; core only if useful across packs |
| Zalbag | I, III, IV | Unique | Active narrative packs; core only if useful across packs |
| Zalmo | III, IV | Canonical; shared-body candidate only | Active narrative packs; core only if useful across packs |

Clear chapter-confined non-recruits include Algus/Teta(I), Draclau/Rudvich(II), Barinten/Izlude(III). Named generic-job enemies such as Gustav/Miluda/Golagros are separate from this dedicated-human corpus; a proper name does not imply a dedicated physical sprite bank.

### 4.2 Human-bank controls per active pack

The following counts include38 generic job controls, all13 final stable human banks and the active named NPCs, one body per person. They are exact counts of these converted profiles, excluding portraits, nonhuman bosses and missing poses. Early packs need not front-load future recruits; doing so here is conservative.

| Pack | native30 | Extended named bodies / native30 jobs |
|---|---:|---:|
| I | 253,813 | 394,601 |
| II-a | 240,023 | 358,434 |
| II-b | 235,982 | 349,023 |
| III-a | 249,800 | 385,556 |
| III-b | 248,257 | 382,283 |
| IV-1 | 250,579 | 386,839 |
| IV-2 | 254,037 | 397,129 |
| IV-3 | 249,914 | 387,190 |

Both columns are measured. The native30 column is the budget; the extended column is the stress control that4.3 rules out.

### 4.3 Actor-bank ceiling, and why native30 is the budget

Actor frames live in `ffca..ffcf`. Each is one archive variable that `tools/common_actors.py` `split()` caps at 65,000 bytes (header + u16 directory + ZX0 streams; the in-bank offsets are u16, so the cap is a format limit, not a preference). `actors.h` declares ACTOR_BANKS 6, giving a hard ceiling of **390,000 bytes of actor frames resident at once** for any pack. The builder asserts rather than truncating, so the limit cannot be crossed silently.

| Profile | Whole corpus | Banks needed | Worst pack | Fits 6 slots |
|---|---:|---:|---:|---|
| native30 | 312,656 | 5 | 254,037 | yes, 135,963 spare |
| extended_regular | 539,728 | 9 | 397,129 | **no, 7,129 over** |

Two independent limits therefore agree. extended_regular needs 9 banks for the whole corpus against 6 slots, and its worst pack (397,129) exceeds the 390,000-byte ceiling outright, while PackI at 394,601 is also over. The same profile is what pushes the high archive forecast past the measured ceiling in6. **native30 is adopted as the budget basis.**

That choice buys a measurable gesture budget rather than closing the door. A stored pose costs about141 bytes (303,628 stream bytes over2,216 frames, plus about4 bytes of mapping per logical pose). The worst pack leaves 135,963 bank bytes free, roughly 964 extra poses spread over that pack's cast, before any bank-directory change is needed. Author the actual scripted gestures against that budget.

Raising ACTOR_BANKS is cheap in itself: the banks are read in place from the archive, the extra pointers cost four bytes each, the name is built as `'a'+k` up to26 banks, and the miss-path bank walk is linear against a 26,804-cycle decode. It is not free in practice: ACTOR_BANKS is part of the core/chapter contract (`contract_layout.c`), so changing it bumps the ABI and forces matching core and chapter packages. Do it when an authored pose inventory proves it necessary, not pre-emptively.

## 5. Scene and text compression

Scenes already decompress ZX0 pictures once at entry. Their low processing load permits cold decompression; it does not create a new entropy saving by itself. Keep the existing codec and all six picture planes.

| Picture width x100 | Raw bytes | Compressed minimum | Mean | Maximum |
|---|---:|---:|---:|---:|
| 160 | 12,000 | 1,842 | 3,836 | 4,910 |
| 256 | 19,200 | 3,230 | 5,528 | 7,699 |
| 320 | 24,000 | 3,360 | 5,870 | 8,942 |

These are six representative rooms, two tested directions per room and three widths. Testing two directions did not require shipping both. They are source-texture compression samples, not approved new C2 scene art or an inventory of original cameras. Row-XOR preprocessing saves only about4% at320px; do not add its reader just for this small difference.

Default to one source-appropriate direction per scene. Same-direction camera crops/pans may share a panorama; different scenes in one room may still need different compositions. The one-direction planning case allows5..10KB per scene-only location plus up to30KB per pack for extra framing/pictures. Keep the older8..20KB/location allowance as a separate conservative control until the source camera list exists.

| Pack | Referenced event text, raw |4KiB ZX0 chunks + directory |
|---|---:|---:|
| I | 30,995 | 18,432 |
| II-A | 19,967 | 11,583 |
| II-B | 11,866 | 6,932 |
| III-A | 15,423 | 9,196 |
| III-B | 19,429 | 11,465 |
| IV-A | 16,222 | 9,535 |
| IV-B | 13,143 | 7,868 |
| IV-C | 10,377 | 6,125 |

Whole chapters I/II/III are30,995/31,833/34,852 raw bytes and 18,432/18,535/20,567 with4KiB chunks, directories included. The full linear source scan finds2,230 entries,167,838 raw bytes and96,972 chunked bytes, including material outside the selected main route. It has zero cursor/instruction failures; unmapped glyphs and dynamic placeholders remain explicit in text-cast.json. This is not a final faithful translation.

Decode only a bounded text block at a scene/dialogue boundary; do not expand the whole chapter or decompress the same line every frame. The native cold text format/reader is not implemented. Item/help/rumor catalogue text is separate, and already shared core catalogues must not be added twice.

## 6. Accepted eight-pack breakdown and budgets

All amounts below include the shared core, one installed active pack, characters/boss allowances, ten optional adapted effects and the 131,072-byte reserve. No wardrobe allowance remains. They are forecasts, not ripped/installed pack sizes. No hypothetical head-substitution or unproven scene-pose pruning is credited.

The eight boundaries stay accepted. One small terrain-list refinement is proposed: IV-C keeps Bed Desert as its sole additional random field instead of also shipping Doguola. After the final Orbonne entry there is no world travel. This preserves more margin for late bosses and scene framing. Earlier extra-field choices remain illustrative until formations are authored.

| Pack | End boundary | Maps | Scenes | One-direction low..high | Margin at high | Conservative high | Margin |
|---|---|---:|---:|---:|---:|---:|---:|
| I | Zeakden complete | 10 +1 | 9 | 2.155..2.579MB | -22,918 | 2.639MB | -82,918 |
| II-A | Goug rescue complete, before Warjilis | 7 +2 | 9 | 1.976..2.367MB | 188,764 | 2.427MB | 128,764 |
| II-B | Lionel complete | 4 +2 | 4 | 1.742..2.097MB | 458,729 | 2.107MB | 448,729 |
| III-A | Orbonne sequence complete, before Grog Hill | 5 +2 | 3 | 1.819..2.159MB | 396,530 | 2.159MB | 396,530 |
| III-B | Riovanes complete | 6 +2 | 5 | 1.920..2.313MB | 243,209 | 2.333MB | 223,209 |
| IV-A | Bethla sluice/rescue/aftermath complete | 8 +2 | 6 | 2.085..2.509MB | 47,016 | 2.539MB | 17,016 |
| IV-B | Igros complete | 6 +2 | 6 | 1.973..2.433MB | 122,523 | 2.463MB | 92,523 |
| IV-C | St. Murond through the ending | 8 +1 | 4 | 2.061..2.553MB | 3,002 | 2.563MB | -6,998 |

Margins are against the measured 2,555,826-byte ceiling; a negative margin is an overflow. **Every pack fits at its low forecast** (worst low: 2.155MB, 400,952 spare). At the unconstrained high forecast the one-direction peak is 2.579MB and PackI is 22,918 bytes over; the conservative high peak is 2.639MB, with PackI over by 82,918 and PackIV-C by 6,998.

The high column is the sum of every range's upper bound at once, so it is a worst case, not a forecast. Adopting native30 (4.3) relieves it from below the low column, not merely toward it: PackI's human bodies were estimated at 345,361..476,231 bytes, while the measured native30 cast for that pack is 253,813, 91,548 bytes under the low estimate. The measured extended cast (394,601) sits inside the old range but over the 390,000-byte actor-bank ceiling, so the profile that would justify the high column cannot ship. **PackI is the only pack with no slack under either reading, and it is the one the user asked to keep whole.** Author its actual map, scene and pose inventory before the others: if the authored figures land above the low forecast, splitting PackI after Dorter or the Sand Mouse cellar is the lever, since battle maps (780,736..890,736 bytes for11 four-view maps) dominate every other item in the pack.

These margins assume nothing else is installed. The user's own calculator carries HW3Patch, AMS files and any other software, so measure it with `experiments/archfree/` before committing to a boundary.

### 6.1 Core and content ownership

- Current measured core:700,273 stored bytes; installed, its36 archived variables occupy 695,696 archive bytes and `fftstory` stays in RAM. The broader shared core projection, including generic/stable humans and ten optional effects, is roughly1.04..1.19MB; late optional nonhuman recruits bring it to1.07..1.25MB.
- Core: rules/catalogues, shared UI/portraits/names, ordinary monster families, generic job art, retained recruits/common actors and selected reusable effects.
- Pack: battle maps/depth, scene backdrops, narrative text/choreography, encounters/objectives/deployment, local NPC body poses, local large boss forms and source progression bindings. Pool every resource across that pack's consumers.
- Moving an active bank between core and pack saves zero combined installed bytes. Dropping an expired NPC bank from later packs is a real installed-content reduction.
- Character appearance is independent of job/team/identity. Costume/body sharing must not change saved stats, recruit ownership or known-speaker portraits.

### 6.2 Map lists

Source IDs distinguish physical terrains, not battle count. Both Bethla approaches are retained even though one playthrough chooses only one. Repeated fights on the same map do not add another bank. Scene-only lists are working static census groups, not a completed event reachability proof.

#### I

- Story battle terrains: 022 Magic City Gariland; 032 Slums in Dorter; 034 Cellar of Sand Mouse; 049 Fort Zeakden; 056 Orbonne Monastery; 074 Sweegy Woods; 077 Lenalia Plateau; 085 Mandalia Plains; 091 Thieves Fort; 103 Windmill Shed.
- Additional random examples: 076 Zeklaus Desert.
- Scene-only locations: 009 Citadel of Igros Castle; 011 Office of Igros Castle; 023 Beoulve residence; 024 Military Academy's Auditorium; 062 Chapel of Orbonne Monastery; 073 Inside of Windmill Shed; 093 Broke down shed - Wooden building; 094 Broke down shed - Stone building; 104 Beoulve Residence Room.

#### II-A

- Story battle terrains: 031 Dorter Trade City; 035 Zaland Fort City; 040 Slums in Goug; 078 Zigolis Swamp; 080 Araguay Woods; 083 Zirekile Falls; 084 Bariaus Hill.
- Additional random examples: 074 Sweegy Woods; 085 Mandalia Plains.
- Scene-only locations: 011 Office of Igros Castle; 012 Gate of Lionel Castle; 014 Office of Lionel Castle; 037 Ruins outside Zaland; 038 Goug Machine City; 041 Besrodio's House; 056 Orbonne Monastery; 093 Broke down shed - Wooden building; 094 Broke down shed - Stone building.

#### II-B

- Story battle terrains: 012 Gate of Lionel Castle; 013 Inside of Lionel Castle; 063 Golgorand Execution Site; 087 Bariaus Valley.
- Additional random examples: 074 Sweegy Woods; 085 Mandalia Plains.
- Scene-only locations: 014 Office of Lionel Castle; 020 Inside of Zeltennia Castle; 043 Port of Warjilis; 094 Broke down shed - Stone building.

#### III-A

- Story battle terrains: 002 Back Gate of Lesalia Castle; 027 Goland Coal City; 057 Underground Book Storage, First Floor; 058 Underground Book Storage, Second Floor; 059 Underground Book Storage, Third Floor.
- Additional random examples: 080 Araguay Woods; 084 Bariaus Hill.
- Scene-only locations: 004 Office of Lesalia Castle; 056 Orbonne Monastery; 068 Bethla Garrison.

#### III-B

- Story battle terrains: 005 Roof of Riovanes Castle; 006 Gate of Riovanes Castle; 007 Inside of Riovanes Castle; 025 Yardow Fort City; 079 Yuguo Woods; 081 Grog Hill.
- Additional random examples: 080 Araguay Woods; 084 Bariaus Hill.
- Scene-only locations: 008 Riovanes Castle; 026 Weapon storage of Yardow; 031 Dorter Trade City; 045 Ruins of Zeltennia Castle's church; 094 Broke down shed - Stone building.

#### IV-A

- Story battle terrains: 036 Church outside the town; 044 Bervenia Free City; 064 In front of Bethla Garrison's Sluice; 066 South Wall of Bethla Garrison; 067 North Wall of Bethla Garrison; 082 Bed Desert; 086 Doguola Pass; 088 Finath River.
- Additional random examples: 079 Yuguo Woods; 081 Grog Hill.
- Scene-only locations: 007 Inside of Riovanes Castle; 021 Zeltennia Castle; 065 Granary of Bethla Garrison; 068 Bethla Garrison; 095 Church; 115 Banished fort.

#### IV-B

- Story battle terrains: 010 Inside of Igros Castle; 016 Inside of Limberry Castle; 017 Underground Cemetery of Limberry Castle; 019 Gate of Limberry Castle (2); 089 Poeskas Lake; 090 Germinas Peak.
- Additional random examples: 082 Bed Desert; 086 Doguola Pass.
- Scene-only locations: 001 Main Gate of Igros Castle; 018 Office of Limberry Castle; 020 Inside of Zeltennia Castle; 046 Cemetery of Heavenly Knight, Balbanes; 047 Zarghidas Trade City; 092 Igros - Beoulve residence.

#### IV-C

- Story battle terrains: 003 Hall of St. Murond Temple; 050 St. Murond Temple; 052 Chapel of St. Murond Temple; 054 Lost Sacred Precincts; 055 Graveyard of Airships; 060 Underground Book Storage, Fourth Floor; 061 Underground Book Storage, Fifth Floor; 069 Murond Death City.
- Additional random examples: 082 Bed Desert.
- Scene-only locations: 045 Ruins of Zeltennia Castle's church; 051 St. Murond Temple; 053 Entrance to Death City; 100 Public Cemetery.

### 6.3 Optional packs and revisits

- **Quest pack:** Colliery/Goug/Nelveska/Zarghidas, with its own scenes and source unlock conditions. The working census has six quest battle terrains; Cloud's street/slum map distinction still needs the authored source route audit.
- **Deep Dungeon:** ten floors105..114 in a separate pack, with its own optional boss/scene allowance. No world-random field bank is required while inside.
- These are separately proposed packs, not two fully extracted size-certified deliveries. Do not force all optional content into IV-C.
- Returning for an old encounter or Brave Story scene replay requires the corresponding content pack. Do not silently remove old travel nodes. Source random destinations can route to selected active terrains, but formations need legal authored placements; changing only a bank name is insufficient.

### 6.4 Pack replacement and saves

Save after the boundary's battle chain, rewards and following scenes settle. Return to HOME before replacing content variables from the previous pack's generated list. Preserve ffsave/ffsave1/ffsave2/ffsave3/ffslot/ffcfg. Use matching core/package keys and dependency lengths/checksums. A missing next pack must be detected before partial progression/rewards are committed. This report does not implement automatic pack switching.

Content phase must be distinct from original chapter: II-A/II-B share ChapterII shop/rumor/proposition rules; all IV packs keep ChapterIV rules. Preserve inventory, XP/JP/levels, recruitment, deaths, breeding, poach stock and durable source flags across swaps. Current FSV9 PC/TI save compatibility is existing evidence; future pack-boundary/Continue flows still need their actual authored PC replay and selected native checks.

## 7. Remaining gates and next work

1. **Done for storage** ([PACK1_INVENTORY.md](PACK1_INVENTORY.md)): PackI's eleven maps and nine rooms measured from the disc total1,894,549 bytes with today's core,530,205 inside the budget, so PackI stays whole and needs no split. Two maps need a shorter scene: Sweegy Woods202->165 rows losing43 of sky, Slums in Dorter202->172 losing54 of foreground. Orbonne is not an overrun; the18,488 cap was sized on it. Still extract scene camera/pose references defaulting to one orientation, keeping every required story gesture and source staging; do not use this report to certify unreviewed cropping or a generic head-swap appearance.
2. Keep one canonical body per person. Reopening any costume variant needs a new measurement and a user decision, not this report's freed margin.
3. Keep complete battle sprites. A future secondary-body or scene-only parts prototype needs a visual verdict if it changes pixels, and whole-frame/stack gates if integrated. Microbenchmarks are not those gates.
4. Author remaining battle/scene maps using approved C2 treatment, full geometry and source handedness. Current320/352px/row/depth ceilings are planning constraints, not proof of future full-map fit. Finath and BookStorage5 still need envelope work; the old Sweegy depth excess remains a ChapterI gate.
5. Author poses against the native30 budget and its 135,963 spare bank bytes (4.3). Raise ACTOR_BANKS only when an authored inventory needs it, and then bump the ABI and rebuild matching core/chapter packages. Cold text ownership still needs its own pass. Keep42 actor entries,18488 depth bytes, existing allocation lifetimes and ordinary360k cycle gates.16KiB free remains deferred.
6. Build the actual eight data-only packs, pool resources, measure final stored bytes and available Flash, replay saves/Continue on PC, then use selected native/hardware checks only where changed content requires them.
7. Run `experiments/archfree/` on the target calculator before fixing any boundary. Every margin here assumes an otherwise empty Titanium; HW3Patch, AMS files and other installed software all come off the same ceiling.

## 8. Evidence and reproduction

Tracked measurement tools:

- tools/actor_storage_study.py: local-disc conversion, exact split/join, optimal ZX0 roundtrips, bank accounting and native fixtures.
- tools/actor_costume_increment_study.py: wardrobe subsets, hybrid scopes and per-pack whole-human controls, reusing the measured corpus.
- tools/actor_parts_bench.c: PC/native byte reconstruction and existing ZX0/mirror preparation costs; no game changes.
- tools/campaign_pack_report.py: this report and pack-projection.json.
- experiments/archfree/archfree.c: the EM_survey archive measurement, run in TiEmu on the clean Titanium `.sav` and read back with `ti-mem log`, no screenshot.

Private local evidence (art/fixtures stay ignored): x/actor-storage/results.json, costume-choices.json, pc.json, native.log, pack-projection.json and verification.json. Existing pack/text/scene baseline: x/pack-refinement/{plan.json,text-cast.json,scenes.json}. The report contains the scope/IDs/assumptions needed to review these estimates even when the private extracted art is not distributed.

Run from the repository root after preparing the existing read-only baseline caches:

```sh
rtk proxy tools/pyenv/bin/python games/fft/tools/actor_storage_study.py
rtk proxy tools/pyenv/bin/python games/fft/tools/actor_costume_increment_study.py
rtk proxy cc -std=c99 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DSTUDY_FIXTURES_FILE='"/home/samuelcoz/workspace/ti/games/fft/campaign/x/actor-storage/fixtures.h"' \
  games/fft/tools/actor_parts_bench.c -o games/fft/campaign/x/actor-storage/parts-pc
rtk proxy games/fft/campaign/x/actor-storage/parts-pc > games/fft/campaign/x/actor-storage/pc.json
rtk proxy tools/bin/ti-cc \
  -DSTUDY_FIXTURES_FILE='"/home/samuelcoz/workspace/ti/games/fft/campaign/x/actor-storage/fixtures.h"' \
  -o games/fft/campaign/x/actor-storage/apartb games/fft/tools/actor_parts_bench.c lib/unpack68k.s
rtk proxy tools/bin/ti-cycles --max 80 games/fft/campaign/x/actor-storage/apartb.89z \
  > games/fft/campaign/x/actor-storage/native.log 2>&1
rtk proxy tools/pyenv/bin/python games/fft/tools/campaign_pack_report.py
```

The fixture include path is workspace-specific; adjust it if the checkout moves. No emulator restart, input, screenshot, installed-group replacement or save write is part of these commands. Preserve all historical protected proof/save bytes and mtimes. Current release remains ABI99, core key `10965478d3a2c348c22d2dea8e6ac51d`. The user subsequently requested ti-commit for these measurement tools and documentation; extracted art and private proof files are excluded from Git.
