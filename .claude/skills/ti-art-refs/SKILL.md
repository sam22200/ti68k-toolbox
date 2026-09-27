---
name: ti-art-refs
description: "Reference sprite banks (The Spriters Resource sheets of SNES, GB/GBC and GBA RPGs) to draw the art of a TI-89 game: scenery and tilesets, characters and NPCs, UI, HUD, menus, dialogue boxes, portraits and facial expressions. Gives the final pick per category, why it fits 160×100 in 4 greys, and how to turn a reference into our own art. Use it before drawing or restyling any sprite, tile, window, menu, HUD or portrait (FFA, ti-port-sdl or ti-port-tibasic graphics step), or when the user asks for art or style references."
---

# ti-art-refs (sprite banks for the art)

The sheets are **references, not assets**: they are ripped from commercial games and copyrighted.
Study them for proportions, poses, animation frames, tile structure and layout, then **redraw**
at the TI's scale. Never commit a downloaded sheet or a traced copy: keep them in
`sources/art/<game>/` (not in git, like all of `sources/`). Converting a capture (native
resolution, grey thresholds, masks): `docs/resources.md`, § Graphics assets.

Target, always: 160×100, 4 greys (2 planes, `COLOR_*`), 16×16 tiles, white outline on the main
sprites, nothing that matters under 2 pixels (`CLAUDE.md`, visibility). **Game Boy / GBC sheets
first**: 4 shades, 8×8 tiles, 160×144 screen, almost 1:1. SNES and GBA sheets are style
references only: their palette goes to 4 levels by luminance and their sprites are too big.

## Final picks

| Category | Pick | Why it fits the TI | Complement |
|---|---|---|---|
| Scenery, tilesets | **Link's Awakening DX**, [Overworld Tileset](https://www.spriters-resource.com/game_boy_gbc/thelegendofzeldalinksawakeningdx/asset/9445/) | Born on the 4-shade Game Boy: 16×16 blocks of 8×8 tiles, top-down 3/4 like our TileMap engine, readable on a 160-pixel-wide screen as is | Seiken Densetsu 3 and Sword of Mana for the richness of vegetation and interiors (the 16-bit look of FFA), reduced to 4 greys |
| Characters, NPCs | **Mystic Quest / Final Fantasy Adventure**, [Sumo](https://www.spriters-resource.com/game_boy_gbc/ffadv/asset/33364/) | Game Boy 4 shades, 16×16 top-down characters, the Final Fantasy (and Mana) universe: the closest to FFA; readable without colour | [Link's Awakening DX — Link](https://www.spriters-resource.com/game_boy_gbc/thelegendofzeldalinksawakeningdx/asset/9436/) for animations (walk, carry, shield, 4 directions); Mother 3 for expressive poses |
| UI, HUD, menus, dialogue | **Pokémon Crystal** | Made for 160×144 with few colours: window frames from 8×8 border tiles (cheap on the TI), arrow cursor, text boxes, HP bars and numbers readable at a glance | Final Fantasy VI for the structure of an FF menu (Item, Magic, Equip, Status, party, gauges); Golden Sun [Icons and HUD](https://www.spriters-resource.com/game_boy_advance/gs/asset/40088/) for item and status icons |
| Faces, expressions in dialogue | **Golden Sun** | Small bust portraits beside the text box, and emotion bubbles (!, ?, sweat, anger) drawn above the field sprite: a bubble costs one small masked sprite and works without a portrait | [FFTA — Portraits and Miscellaneous](https://www.spriters-resource.com/game_boy_advance/fftacticsadv/asset/10395/) for more expressive faces; Phantasy Star IV for dramatic scenes (manga panels) |

Portrait budget on the TI: the FFA dialogue box is 3 lines of 24 characters of the 6×8 font
(144 pixels). A portrait beside it takes about 24–32 pixels: the text drops to 20–22 columns,
so check the longest line of `texts.h` first. Draw the portrait (2 planes, one sprite per
expression) once per page, not per frame. Measure the portrait sizes on the sheet before choosing
them.

## All the banks

| Game (platform) | Where its sprites shine | Sheets |
|---|---|---|
| Seiken Densetsu 3 (SNES) | natural scenery and vegetation | https://www.spriters-resource.com/snes/seikendensetsu3/ |
| Chrono Trigger (SNES) | map and town building; field characters | https://www.spriters-resource.com/snes/chronotrigger/, [Characters](https://www.spriters-resource.com/snes/chronotrigger/asset/3642/) |
| Treasure of the Rudras (SNES) | battle sprites and animations | https://www.spriters-resource.com/snes/treasurerudras/ |
| Final Fantasy VI (SNES) | classic RPG menus and interface | https://www.spriters-resource.com/snes/ff6/ |
| Phantasy Star IV (Genesis) | portraits, manga-style storytelling | https://www.spriters-resource.com/genesis_32x_scd/phantasystar4/ |
| Final Fantasy Tactics Advance (GBA) | tactical RPG interface, portraits | https://www.spriters-resource.com/game_boy_advance/fftacticsadv/ |
| Mother 3 (GBA) | character animations | https://www.spriters-resource.com/game_boy_advance/mother3/ |
| Golden Sun (GBA) | special effects, summons; icons and HUD | https://www.spriters-resource.com/game_boy_advance/gs/ |
| Sword of Mana (GBA) | nature, characters, action-RPG animations; [Hero](https://www.spriters-resource.com/game_boy_advance/som/asset/6168/), [Hot House](https://www.spriters-resource.com/game_boy_advance/som/asset/6181/) (interior) | https://www.spriters-resource.com/game_boy_advance/som/ |
| Magical Vacation (GBA) | small expressive sprites, fantasy villages; [Entrance](https://www.spriters-resource.com/game_boy_advance/magicalvacationjpn/asset/122473/), [Tapioca Tea Village](https://www.spriters-resource.com/game_boy_advance/magicalvacationjpn/sheet/122307/) (map layout, proportions) | https://www.spriters-resource.com/game_boy_advance/magicalvacationjpn/ |
| Pokémon Crystal (GBC) | readability on a small screen | https://www.spriters-resource.com/game_boy_gbc/pokemoncrystal/ |
| Dragon Warrior / Quest III (GBC) | a complete 8-bit JRPG: classes, monsters, readable towns and dungeons | https://www.spriters-resource.com/game_boy_gbc/dragonwarrior3/ |
| Lufia: The Legend Returns (GBC) | detailed monsters and bosses, portraits, battle layout | https://www.spriters-resource.com/game_boy_gbc/lufiathelegendreturns/ |
| Star Ocean: Blue Sphere (GBC) | dungeon and village scenery, rich GBC pixel art | https://www.spriters-resource.com/game_boy_gbc/staroceanbs/ |
| Link's Awakening / DX (GB/GBC) | Link's animations, world tilesets, dungeons, items, interface | https://www.spriters-resource.com/game_boy_gbc/thelegendofzeldalinksawakeningdx/ |
| Mystic Quest / Final Fantasy Adventure (GB) | hero, enemies, bosses and monochrome scenery | https://www.spriters-resource.com/game_boy_gbc/ffadv/ |

Other sprite sites (Sprite Database, VideoGameSprites, VGMaps for whole maps): `docs/resources.md`.

## From a reference to our art

1. Pick the reference from the table above for the category; download its sheet into
   `sources/art/<game>/`.
2. Take the structure from it: the grid (tile and metatile sizes), the character's footprint on
   the tiles, the number of animation frames, the window layout. Scale the SNES/GBA ones to our
   units (16×16 tiles, a 16×16 or 16×24 character on 160×100).
3. Redraw in 4 greys: contrast the sprite against its background, white outline on the main
   sprites, 2 pixels minimum for any detail that matters. The tools in `tools/pyenv` (PIL) can
   reduce a sheet to 4 greys as a starting sketch, never as the final art.
4. Compare 2 to 4 variants on the same headless PC screenshot (grey and `RT_MONO`), pick one,
   keep only the winner (`ti-port-sdl` step 5, `ti-port-tibasic` §2).
5. Measure the cost without UI (`make cycles`, `make xcheck`), then the TI last.

A new bank worth keeping: add a row above, in its category (game, where its sprites shine, URL).
