# FFA remake: PART I specification (castle and dungeon, up to the knighting and the CURE materia)

Source: `ffa_en/ffa_decoded.txt` (TI-Basic v1.07). Engine programs are cited as `prog:line`, meaning
`ffa_en/programs/prog.txt` line numbers. `story*` and `text*` exist only in the decoded file, so they
are cited as `dec:NNNN`, the line number in `ffa_decoded.txt`. In strings, `=` is a line break
(it is drawn by `dia`, an ASM program, see §8). `rand(n)` on the TI-89 returns an integer in 1..n.

---------------------------------------------------------------------------------------------------

## 0. Corrections and additions to the engine facts

| Fact given | Verdict / exact behaviour |
|---|---|
| Room u = `dec{u}` + `mur{u-1}` | Correct (`redess:138,174`, `ffa:227,235`). |
| Row 1 = [frc, door-table row] | Correct (`mur[1,1]`→frc `redess:194`; `mur[1,2]` = door-table row `redess:114`). Row 1 is **also the top-edge row**: moving up from b=0 reads `mur[1,·]` (`mov:25`), so the top exits of room 12 are stored in row 1. |
| Cell of (a,b) = `mur[b/9+2, a/9+2]` | Correct. a = x (column, 0..144), b = y (row, 0..63); sprites are drawn with `RclPic pic,b,a`. Neighbour cells: left `mur[b/9+2,a/9+1]`, up `[b/9+1,a/9+2]`, right `[b/9+2,a/9+3]`, down `[b/9+3,a/9+2]` (`mov:17,25,33,41`). Column 1 is the left edge (a=-9) and the row after the last grid row (b=72) is the bottom edge. Both edges hold exit cells, and the door-table row usually doubles as the bottom-edge row. |
| 0 wall, >0.9 walkable | Walkable means `p>zt`, where zt=0.9 on foot and 0.5 or 0.2 on a chocobo (`ffa:254`, `del:15-17`). A value v with 0.9<v≤2 is walkable (the value 2 in room 12 is one). |
| 3..199 door | The move code opens a door when `2<p<500` (`mov:17`). `redess:16` then splits by value: p<199 or p>499 is a normal door (Lbl bb); 200..298 is the world map `murt` (Lbl yy); 199 and 299..449 use the `murr` labyrinth; 450..499 use `murrr` (Lbl xx). |
| ≥500 script | The hero **walks onto** the cell (`mov:18`: 500>2 fails the door test, p>zt passes). `scenar()` then runs on **every loop iteration** while p stays ≥500 (`mov:6`), so only the scripts' flags stop repeats. |
| ≤-2 text → `text{int(|p/10|)+1}` | The exact condition is `-100<p<-1` (`mov:7`): -1.5 is a text, -1 is not (it acts as a wall). The cell blocks movement, and `texts` (`texts:4-11`) waits for a key: ENTER(13) or F1(268) runs the text, and any other key is replayed as a move. A value with no branch in its text program (-8 in room 15) does nothing. |
| Door table: id at col af, key `clef[cl]` at col af+6, arrival b=`mur[2af-1,mur[2,1]]`, a=`mur[2af,mur[2,1]]` | Correct, with these details. The arrival coordinates are read from the **source** room's matrix, before `mur` is reloaded (`redess:140-143` runs before `:174`). An arrival value of **-1 keeps the current coordinate**. A room can have at most 6 doors. A door value missing from the table makes the search loop run off the matrix (`redess:112-119`). cl=0 means free. cl>0 with `clef[cl]=0` shows "The door is locked." (`redess:125`). cl=-1 requires being on foot, otherwise "I must leave the=Chocobo :=ESC". |
| (new) monster table | Column `mur[2,1]+1`, rows 2..10: [N, n1, t1, n2, t2, n3, t3, n4, t4]. bb=rand(N), n = the first nk with bb≤tk (`choimon:17-25`). If no row matches, n keeps its old value (0). |
| Encounters `mc>co`, co=15+rand(5) | co=15+rand(5) holds for a **new game only** (`ffa:155`). A loaded game keeps co=20+rand(10) (`ffa:31`). After each **won** battle co=20+rand(16) (`fincomb:83`). mc+=frc on each step (`mov:21...`). The test `mc>co` runs every loop; mc is reset after any battle (`mov:13`) and while riding a chocobo (`mov:11`). **A room change does not reset mc** (castle rooms have frc=0, so mc just stays frozen there). Escaping does not re-roll co. |
| Start: room 8, a=27, b=27 | Correct (`ffa:102,155`). Also `mur7→mur` (`ffa:157`) and hero sprite `art10`. |

---------------------------------------------------------------------------------------------------

## 1. Walkthrough (guide, cross-checked with the code)

| # | Where | Action | Code effect |
|---|---|---|---|
| 0 | New game | Title dialog "Final Fantasy Alternative v 1.07" → New Game. You pick a growth stat (§4), then a name (default "Arthur", 1..8 chars). The intro text and a "20 years after..." fade follow. | `ffa:57-204` |
| 1 | 8 bedroom | Walking to the exit cell (b=9,a=36, value 500) makes Edouard come in (story1). | clef[9]=1; the hero moves to a=45 |
| 2 | 8 | Take the potion (desk, -19). The bed (-2.5) gives a free full heal at any time. | clef[40]=1, npot+1 |
| 3 | 6 throne | You arrive on 501: "Father, I don't see where my sword is..." (story2). | clef[10]=1, the hero is placed at b=18,a=108 |
| 4 | 5 courtyard | Talk to the potion seller (-12): Potion 50 g. Start gils = 100. | shop1 |
| 5 | 18 Olen/Jess | Olen (-10) gives the Dungeon Key. | clef[1]=1 |
| 6 | 7 bookcases | Crossing 504 below the dungeon door sets clef[11]=1. The top door leads to the dungeon (key clef[1]). | |
| 7 | 10 dungeon hall | The corpse on the upper wall (-18) holds the little key. | clef[2]=1 |
| 8 | 12 (south of 10) | Read the notice (-6): "...there were N injured. Among them, 3000 died." **N=devi is re-rolled 5000+rand(100) at every reading.** The east door to 13 needs the little key. | devi |
| 9 | 13 | Push the upper-wall switch (-4). It toggles clef[17], which unlocks the 11→16 door. The chest at b=9,a=72 (-2) is a trap and starts a random fight. The real chest (-18) gives an Antidote. Stepping on 502 before the east exit asks for the number: answer devi-3000. | clef[3]=1 on a right answer |
| 10 | 14 prison hall | Chest (-18): Fire materia (it cannot be used without a slotted weapon). There are 3 cell doors: cell 1 (door 14, clef[4] is never set, so it stays **locked forever**), cell 2 (door 17, clef[5]), cell 3 (door 15, clef[6]=1 from the start). | maglist[2,7]=1 |
| 11 | 15 cell 3 | The corpse on the bed (-18) holds the Power Wrist (Strength+5). | armat[9,1]=1 |
| 12 | (optional) | Go back, sleep, buy Potions (the guide advises ≥10). | |
| 13 | 11 → 16 weapon room | The north door of 11 opens if clef[17]=1. Stepping on the 503 ring around the chest starts the boss (story4, n=3, 1000 HP, no escape). Reward: "Cell 2 Key". | clef[5]=1 |
| 14 | 16 | Chest (-18): Buster Sword. Equip it (APPS › EQUIP › Weapon) and put Fire in a slot (MATERIA). | armat[1,1]=1, **clef[8]=0** (locks door 5→18) |
| 15 | 14 → 17 cell 2 | Chest (-18): Bronze Bangle (armor). | armat[25,1]=1 |
| 16 | 6 throne | Entering from the courtyard puts you on 505: the knighting ceremony (story5). | clef[7]=1 (unlocks courtyard→front), clef[8]=1 (unlocks Olen's room) |
| 17 | 18 | Olen: "Take this..." gives the Cure materia. | maglist[3,7]=1 |
| 18 | 5 → 4 → 19 | The courtyard south exit is now open. Room 4 exits south to room 19, the outside crossroads (monsters n=4/5). **End of Part I.** | |

---------------------------------------------------------------------------------------------------

## 2. Rooms

Notation: a cell is (row,col) in the 1-based matrix, with pixel b=(row-2)·9 and a=(col-2)·9.
"Arrive" gives (b,a) in the **destination** room; -1 means that coordinate is kept.
Pictures: background `dec{u}`, 153×72 px at (0,0) unless noted. NPCs are **drawn into the background
picture**. They are not sprites: a text cell sits on each NPC.

### Room 8: hero's bedroom (left half) + Larc's room (right half, closed in Part I)
Matrix `mur7` 9×18, frc 0 (no fights). Door row 9, arrival column 18, no monster column. Walkable rows 3-8, cols 3-17. Start (b27,a27).

| Exit | Cells | → room | Key | Arrive |
|---|---|---|---|---|
| door 1 | (2,6) top wall at a=36 | 6 | 0 | (18,117) = the 501 cell of room 6 |

| Cell | Value | Handler | Effect |
|---|---|---|---|
| (3,6) b9 a36 | 500 | scenar:3 → story1 if clef[9]=0 | Edouard enters (sprites per1/per3/per2 at 9,36). Dialogue D1. Then a=45, b=9, clef[9]=1 |
| (3,4),(4,4) | -2.5 | text1 dec:6086 | Bed: "Sleep?" Yes/No; Yes sets hp=hpm, mp=mpm, wipe effect, redraws dec8 |
| (3,9),(3,10) | -3.5 | text1 dec:6101 | Plaque: `name&" STRIFE,born=in this Castle,son=of Edouard STRIFE=and Larc's Brother."` |
| (8,8) | -19 | text2 dec:6410 `recu(40,1,"npot")` | "Found 1 Potion(s) !" once (clef[40]) |
| (6,11) | -3 | text1 dec:6210 | Door to Larc's room: while clef[64]=0 it shows "My brother Larc's=room.He always=locked it before=leaving." Late game (clef[64]≠0) it teleports the hero across (a 72↔90). |
| (3,12),(3,13) | -18 | text2 dec:6411 | Larc's diary (story15). Unreachable in Part I |
| (8,15) | -24 | text3 dec:6664 | Apocalypse chest: "Closed.The lock is =strange shaped." (clef[42]). Unreachable in Part I |

### Room 6: throne hall
`mur5` 11×18, frc 0. Door row 11, arrival column 18.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,8..12) bottom edge | 5 | 0 | (54,72) |
| door 2 | (3,15) b9 a117 | 8 | 0 | (9,36) = the 500 cell |

| Cell | Value | Effect |
|---|---|---|
| (4,15) b18 a117 | 501 | story2 if clef[10]=0 (`scenar:5`): Edouard sprite per3 at (27,46); dialogue D2; clef[10]=1; hero set to (18,108) |
| (5,7) b27 a45 | -9 | Edouard is drawn in dec6 at (27,45). `redess:175-179` writes this cell to -9 while clef[67]=0 and to 1 once he is dead (then `XorPic per1,27,45` erases him). Text: D3 |
| (9,10) b63 a72 | 505 | The arrival cell from the courtyard. story5 (ceremony) if `armat[1,1]=1 and clef[8]=0 and clef[7]=0` (`scenar:30`). Later branches (clef[56], clef[64]) are out of scope |

### Room 5: courtyard
`mur4` 11×19, frc 0, door row 11, arrival column 19. Walkable rows 5-9.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,9..11) bottom | 4 | **clef[7]** (set by the ceremony) | (45,72) |
| door 2 | (7,10) b45 a72 | 6 | 0 | (63,72) = the 505 cell |
| door 3 | (7,4) b45 a18 | 7 | 0 | (63,72) |
| door 4 | (7,16) b45 a126 | 18 | **clef[8]** (1 at start, 0 from sword pickup until the ceremony) | (63,27) |

| Cell | Value | Effect |
|---|---|---|
| (5,5) b27 a27 | -7.1 | NPC: `"I love this Castle:I=feel safe,behind these=solid walls..."` (dec:6004) |
| (6,13) b36 a99 | -12 | Potion seller NPC: `"Hello Sir "&name&",=I'm the «official»=potion seller in the=Castle."` then shop1 (§6) |
| (9,10) b63 a72 | 514 | Arrival cell from room 4. Its text needs clef[56]=1 (late: "Potion Seller:Finally you come back...") |

### Room 4: castle front
`mur3` 10×19, frc 0. Door row 10, arrival column 18. Column 19 = [·,5,4,3,5,5] looks like a monster table, but frc=0 so no fight happens here (room 19 has the same table).

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,9..11) bottom | 19 (outside crossroads) | 0 | (0,-1) |
| door 2 | (6,10) b36 a72 | 5 | -1 (must be on foot) | (63,72) |

| Cell | Value | Effect |
|---|---|---|
| (6,2..4) b36 a0-18 | -7.4 | House on the left: `"Message:We left to play=at the Chocodome!"` (dec:6010, generic text without a u check) |

### Room 7: bookcase room (door to the dungeon)
`mur6` 10×14, frc 0, door row 10, arrival column 14. Picture 116×72.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,9..11) bottom | 5 | 0 | (54,18) |
| door 2 | (2,10) b0 a72 | 10 (dungeon) | **clef[1]** Dungeon Key | (9,72) |

| Cell | Value | Effect |
|---|---|---|
| (3,10) b9 a72 | 504 | `scenar:15`: if clef[11]=0, set clef[11]=1 (silent; this changes Edouard's line). The clef[56]/[58] branch is late |
| (4,4),(4,5) | -2 | `"EXCALIBUR is the=Legendary Sword with=an unbelievable=Power.It's kept=somewhere in this=castle.It seems you=must know the Ancient=Language to get it."` |
| (4,7),(4,8) | -1.5 | `"The Ancient Language has=disapeared,but it seems=some scholars still know=it..."` |
| (7,4),(7,5) | -4 | `"Cloud embodies the=strength and the=power of the Strifes.=He saved the world by=killing SEPHIROTH,=1000 years ago."` |
| (7,7),(7,8) | -3 | `"Since the 30 years=War,BRAMANA kingdom=is the potential=enemy of Milunia.=Yet,the two kingdoms=got well along until=that outstanding war."` |

### Room 18: Olen and Jess's room
`mur17` 10×14, frc 0, door row 10, arrival column 14. Picture 117×72.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,5..6) bottom | 5 | 0 | (54,126) |

| Cell | Value | Effect |
|---|---|---|
| (5,5) b27 a27 | -10 | Olen NPC (drawn). Dialogue D4 (key, then Cure) |
| (4,11) b18 a81 | -11 | Jess NPC (drawn). D5 |
| (7,11) b45 a81 | -8.5 | Carrots on the table: `"Fresh carrots.."` while clef[62]=0 (the pickup is late game) |

### Room 10: dungeon hall (first dungeon room)
`mur9` 11×21, **frc 6/5**, door row 11, arrival column 20, monster column 21 = [5,1,1,2,5]: **n=1 20 %, n=2 80 %**.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (2,10) b0 a72 | 7 | 0 | (9,72) = the 504 cell |
| door 2 | (5,19) right edge at b27 | 11 | 0 | (27,0) |
| door 3 | (10,7),(10,8),(10,12) bottom | 12 | 0 | (0,-1) |

| Cell | Value | Effect |
|---|---|---|
| (2,6) b0 a36 | -18 | Corpse on the wall: `boit(1,1,"It holds a key.")`, `"Found little Key!"`, clef[2]=1 (dec:6419) |

### Room 11: east corridor (narrow, picture 54×71)
`mur10` 9×9, **frc 3/2**, door row 9, arrival column 8, monster column 9 = [5,1,3,2,5]: **n=1 60 %, n=2 40 %**.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (2,5) b0 a27 | 16 weapon room | **clef[17]** switch | (63,72) |
| door 2 | (5,1) left edge at b27 | 10 | 0 | (27,144) |

| Cell | Value | Effect |
|---|---|---|
| (8,5) b54 a27 | -18 | Chest `recu(43,1,"npot")`: "Found 1 Potion(s) !" |

### Room 12: notice room (south of 10)
`mur11` 9×21, frc 6/5, door row 9 (it is also grid row b=63; the "2" at (9,8) is its key flag and cannot be reached), arrival column 20, monster column 21 = [5,1,3,2,5] (60/40).

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (1,7),(1,8),(1,12) top edge | 10 | 0 | (63,-1) |
| door 2 | (3,19),(4,19),(7,19),(8,19) right edge | 13 | **clef[2]** little key | (-1,0) |

| Cell | Value | Effect |
|---|---|---|
| (2,4) b0 a18 | -6 | Notice: `5000+rand(100)→devi`, then `"The Thirty-Years War=was very deadly,there=were "&string(devi)&"= injured.Among=them,3000 died."` (dec:6133) |
| (8,3) b54 a9 | -18 | Chest `recu(44,1,"net")`: "Found 1 Ether(s) !" |

### Room 13: switch and riddle room
`mur12` 10×21, frc 6/5, door row 10, arrival column 20, monster column 21 = [5,1,3,2,5].

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (3,1),(4,1),(7,1),(8,1) left edge | 12 | 0 | (-1,144) |
| door 2 | (7,19) right edge b45 | 14 | **clef[3]** riddle | (45,0) |
| door 3 | (2,15) b0 a117 | 73 | clef[56] (late) | (63,81) |

A locked door with p=73 also prints `"The Weapon Room is=not behind this door,=anyway..."` (`redess:126-127`).

| Cell | Value | Effect |
|---|---|---|
| (2,6) b0 a36 | -4 | Switch: `"Push the switch?"` Yes/No; Yes **toggles** clef[17] (0↔1), then `"There's a lock noise."` (dec:6175). Pressing it twice relocks door 11→16 |
| (3,10) b9 a72 | -2 | Trap chest: `boit(1,1,"Oh no!...")`, p=0, `combat()` with n=0, i.e. a random room monster that can be escaped (dec:6193). It re-triggers at every ENTER |
| (8,11) b54 a81 | -18 | Chest `recu(45,1,"anti")`: "Found 1 Antidote(s) !" |
| (7,17) b45 a135 | 502 | story3 if clef[3]=0 (§3 riddle, D6) |

### Room 14: prison hall
`mur13` 10×20, frc 6/5, door row 10, arrival column 19, monster column 20 = [5,1,3,2,5].

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (7,1) left edge b45 | 13 | 0 | (45,144) |
| door 2 | (4,4),(4,5) cell 1 | 14 (itself) | **clef[4], never set: always "The door is locked."** | (0,0), never used |
| door 3 | (4,10),(4,11) cell 3 | 15 | clef[6] (=1 from the start) | (63,81) |
| door 4 | (4,7),(4,8) cell 2 | 17 | **clef[5]** Cell 2 Key (boss) | (63,63) |

| Cell | Value | Effect |
|---|---|---|
| (4,16) b18 a126 | -18 | Chest: `"Found Materia=Fire!"`, maglist[2,7]=1 (dec:6427) |
| (6,5) b36 a27 | -3 | Sign: `"In front of you,the=prison cells."` |

### Room 15: cell 3
`mur14` 10×17, frc 6/5, door row 10, arrival column 16, monster column 17 = [5,1,3,2,5]. Picture 133×77.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,10..12) bottom | 14 | 0 | (27,81) |

| Cell | Value | Effect |
|---|---|---|
| (3,14) b9 a108 | -7 | Plaque: `"John BARNARD=Condamned to Death=Sentence."` |
| (4,8),(4,9) | -18 | Corpse on the bed: `boit(1,1,"It holds something...")`, `"Found Accessory=Power Wrist!"`, armat[9,1]=1 |
| (8,8),(8,9) | -8 | text1 has no p=-8 branch, so **nothing happens** (unclear intent) |

### Room 16: weapon room (boss)
`mur15` 10×19, frc 6/5 (random fights are possible here too), door row 10, arrival column 18, monster column 19 = [5,1,3,2,5].

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,9..11) bottom | 11 | 0 | (9,27) |

| Cell | Value | Effect |
|---|---|---|
| (4,10) b18 a72 | -18 | Chest: `boit(1,0,"Found Weapon=Buster Sword !")`, armat[1,1]=1, **clef[8]=0** (dec:6437) |
| ring (4,8),(4,12),(5,8),(5,12),(6,8..12) | 503 | story4 if clef[5]=0. The ring fully encloses the 3 cells below the chest |
| (7,4) b45 a18 | -19 | Comet chest: `"Locked by Gold Seal."` while clef[54]=0 (late) |

### Room 17: cell 2
`mur16` 10×15, **frc 0** (safe). Door row 10, arrival column 14. Its monster column [5,1,3,2,2] is unused and inconsistent: a draw of 4..5 would leave n unchanged.

| Exit | Cells | → | Key | Arrive |
|---|---|---|---|---|
| door 1 | (10,8..10) bottom | 14 | 0 | (27,45) |

| Cell | Value | Effect |
|---|---|---|
| (4,10) b18 a72 | -18 | Chest: `"Found Armor=Bronze Bangle!"`, armat[25,1]=1 |

### Out of scope
- **Room 73** (door (2,15) of room 13, key clef[56] = late game): frc 6/5, monsters [5,14,3,15,5] (n=14 60 %, n=15 40 %). Doors lead to 13, 75 and 76.
- **Room 19** (south of room 4): the outside crossroads. frc 6/5, monsters [5,4,3,5,5] (n=4 60 %, n=5 40 %), sign -14 "? Castle=? Milunia", exits to 4, 20, 22 and 25. This is the boundary of Part I.
- The BFS over doors from room 8 finds no other room in the castle and dungeon cluster.

---------------------------------------------------------------------------------------------------

## 3. Story flags and state used in Part I

### clef[k,1] (matrix 120×1, zeroed at a new game except for the initial values)
| k | Init | Meaning | Set by | Read by |
|---|---|---|---|---|
| 1 | 0 | Dungeon Key | Olen -10 (dec:6241) | door 7→10; Edouard/Olen texts |
| 2 | 0 | little key | corpse, room 10 | door 12→13 |
| 3 | 0 | riddle solved | story3 | door 13→14; scenar 502 |
| 4 | 0 | cell 1 key | **never** | door 14 cell 1 (always locked) |
| 5 | 0 | Cell 2 Key (boss beaten) | story4 end | door 14→17; scenar 503 |
| 6 | **1** | cell 3 open | ffa:80 | door 14→15 |
| 7 | 0 | knighted | story5 | door 5→4; scenar 505 |
| 8 | **1** | Olen room open / ceremony gate | ffa:81 =1; sword chest =0; story5 =1 | door 5→18; scenar 505 needs 0 |
| 9 | 0 | story1 done | story1 (=1) | scenar 500 |
| 10 | 0 | story2 done | story2 | scenar 501 |
| 11 | 0 | visited the dungeon door (504) | scenar:15 | Edouard -9 |
| 14 | 0 | read only (Edouard "Good Luck Knight", Jess "glad for you") and **never set**, so those lines never appear | none | text1/2 |
| 17 | 0 | switch 13 (toggle) | -4 room 13 | door 11→16 |
| 40,43,44,45 | 0 | one-shot chests (`recu` n): potion room 8, potion 11, ether 12, antidote 13 | recu | recu |
| 54,56,58,61,62,64,65,67 | 0 | late-game gates tested in Part I cells: gold seal (16), room 73, 514 text, Edouard/Olen/Jess later lines | later parts | text1/text2/scenar |
| 42 | 0 | Apocalypse chest (room 8, late) | | text3 |
| 66 | 0 | War placement mode (menu SAVE resets it to 0) | | |
| 72,75,76,89,90 | 9,9,5,5,8 | world-map and labyrinth cursors (ffa:82-86) | redess | redess |

### Other state
| Var | Meaning | Part I use |
|---|---|---|
| `devi` | riddle number, 5000+rand(100) (ffa:72) | **Re-rolled at every reading of the notice** (dec:6133), deleted after a right answer. It is **not saved** (`sav:17`): after a Continue it is undefined, `expr(re)=devi-3000` errors inside Try, and the answer shows "False". You must read the notice again. |
| `armat[k,1]` | equipment owned (32×8, column 1 zeroed at a new game, ffa:73) | 1 Buster Sword, 9 Power Wrist, 25 Bronze Bangle |
| `maglist[k,·]` | materia 18×8, a copy of matrix `new` (ffa:146). Columns: 1 equipped, 2 level, 3 AP, 4-6 AP thresholds for lv2/lv3/"Master", 7 owned, 8 name | 2 Fire (chest 14), 3 Cure (Olen) |
| `matp1..6` | maglist row placed in each weapon slot | MATERIA menu |
| `chmat` 12×7 | chocobos and greens (column 7 = counts) | unused in Part I (the ITEM list reads chmat[2..7,7]) |
| `eqarm`, `eqarmu`, `eqacc1/2` | equipped weapon row (1..8), armor (row = eqarmu+24), accessories (row = eqacc+8) | |
| `jl` | limit gauge 127..157 (starts at 127, it is saved) | §5 |

---------------------------------------------------------------------------------------------------

## 4. Hero

### Initial state (`ffa:98-155`)
| Stat (var) | Value | Stat (var) | Value |
|---|---|---|---|
| HP hp/hpm | 80/80 | Level nv | 1 |
| MP mp/mpm | 15/15 | Exp exp / expt / expn | 0 / 370 / 370 |
| Strength forc | 10 | Gils | 100 |
| Magic mag | 12 | Items | Potion npot 3, Antidote anti 1, all others 0 |
| Vitality def | 5 | Unarmed | atq 7, rate 100, orif (slots) 0, croiss (AP growth) 0 |
| Spirit defm | 6 | Armor | adef 0, adefm 0, esq (evade) 0 |
| Speed vit | 15 | Limit | lim 1 (Braver), jl 127 |
| Luck chan | 3 | Options | vcomb 4 (battle speed), mode 0 (wait) |

The `speci` choice ("Choose a capacity that will increase more=during the game.": Strength→forc, Magic→mag, Vitality→def, Spirit→defm, Speed→vit, Luck→chan) adds **+0.25 to that stat right away** (ffa:154) and +0.25 at each level up (fincomb:49). Stats are stored as fractions and shown with `int()`.

### Level up (`fincomb:36-54`)
The victory screen adds `expn/20` to exp per animation step until exp ≥ exp1+expe; at the end exp = exp1+expe exactly (`fincomb:56`). Several level ups in a row are possible. When exp ≥ expt:
```
expn = int(1.1*expn); expt = expt+expn; nv = nv+1
forc += 1/2; mag += 1/2; chan += 1/5; defm += 2/5; def += 2/5; vit += 1/4; #speci += 1/4
hpm = int((1.105 - nv/1000)*hpm); mpm = int(mpm*(1.105 - nv/1000))    (nv = new level)
```
A level up does not heal. Resulting table without the speci bonus (expt = total exp needed for the next level):

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| expt | 370 | 777 | 1224 | 1715 | 2255 | 2849 | 3502 | 4220 | 5009 | 5876 |
| hpm | 80 | 88 | 96 | 105 | 115 | 126 | 138 | 151 | 165 | 180 |
| mpm | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 23 | 25 | 27 |
| forc | 10 | 10.5 | 11 | 11.5 | 12 | 12.5 | 13 | 13.5 | 14 | 14.5 |
| def | 5 | 5.4 | 5.8 | 6.2 | 6.6 | 7 | 7.4 | 7.8 | 8.2 | 8.6 |

mag = forc+2, defm = def+1, vit = 15+0.25(nv-1), chan = 3+0.2(nv-1).

### Equipment found in Part I (`matmake:3`; armat columns after ffa:73 = [owned, c2..c8])
| Item | Row | Data | Use |
|---|---|---|---|
| Buster Sword | 1 | [·,92,2,1,"",10,"Buster Sword",0] | rate (hit %) 92, **slots 2**, AP growth ×1, atq 10 (`armch:34-37`). Battle sprite `bcomb11` (unarmed `bcomb10`) |
| Power Wrist | 9 | [·,"Power Wrist",0,"forc",5,"Strength+5",0,0] | type 0 = additive: forc += 5 on equip, -5 on removal (`acce:43-47,33`) |
| Bronze Bangle | 25 | [·,"Bronze Bangle",5,2,2,"",0,0] | adef 5, **esq (evade %) 2**, adefm 2 (`amuch:30-32`) |
| Fire materia | maglist 2 | lv2 at 1000 AP, lv3 at 5000, then Master | Fire (4 MP); Fire 2 needs lv2 |
| Cure materia | maglist 3 | lv2 at 900, lv3 at 4000 | Cure (5 MP) |

Accessory types (col 3): 0 = add col5 to the stat named in col4; 3 = multiply; 2 = two additive stats (col4/col5 and col7/col8). The same accessory cannot be in both slots. Unequipping a weapon clears the materia in slots > orif (`armch:39`). MATERIA without a slotted weapon shows "No Slot".

---------------------------------------------------------------------------------------------------

## 5. Battle system (Part I subset)

### Setup (`combat`, `choimon`)
- Random battle: n=0 before `combat` (`mov:13`), then the room table picks n (§0) and qu=4, so escape is possible. Scripted battle: n is preset and qu=1 (`choimon:15`), so escape is impossible.
- Monster stats come from `mons[n,·]` (`choimon:89-95`): hpe=c1, vie (speed)=c4, expe=c5, ge (gils)=c6, ape=c7, dec (x offset)=c8. vol=1 if c12≠0. Hero vit1=int(vit), forc1=forc, rate1=rate. Monster sprite `monst{n}` (`combat:18`).
- Keys: F1 Attack (Limit when the gauge is full), F2 Magic (toolbar), F3 Item, ESC Run (`combat:143-165`).

### Turn order: ATB (`combat:31-32,82-95`)
ja=129+rand(10) and jae=129+rand(10). Each tick adds `ja += vit1/(11-vcomb)` (capped at 157) and `jae += vie/(11-vcomb)`. ja≥157 opens the command menu; jae≥157 makes the monster act (`atqen`), then jae=127. After the hero acts, ja=127 (`combat:219`). Wait mode (mode≠1) blocks the whole loop at the menu. Active mode (mode=1) keeps running the monster's gauge while the menu is shown (`combat:167-168`). vcomb changes the speed of both sides, not their ratio. A full turn is 30 gauge units: hero 30·7/15 = 14 ticks, monster 1 15, monster 2 13.1, boss 17.5.

### Hero attack (`at:15-29`)
```
cou  = 0.5*forc1*atq * (255 - mons[n,3])/255
coup = int(0.95*cou - 1 + rand(int(0.1*cou)+1));  if rand(100) < chan: coup *= 2   (critical, counted in cc)
miss if rand(100) - int(nv/4) - chan/2 - 40*tran > rate1 - mons[n,28]      (coup = 0, "Miss")
hpe -= coup
```
Line 25, `If l=2 and forc1≠forc: Goto nn`, skips the miss test. It tests `l`, which looks like a typo for `li`. Unclear.
Examples at Lv1, forc 10, unarmed (atq 7): 31-34 vs monster 1 and 29-32 vs monster 2. With Power Wrist (forc 15): 47-52 vs monster 1, 41-45 vs the boss. Miss rate with the Buster Sword (rate 92) at Lv1: 7 % vs monster 1, 11 % vs monster 2, 13 % vs the boss. Unarmed (rate 100): 0 %, 0 %, 5 %.

### Monster physical attack (`atqen:37-64,254-263`)
```
coup = 0.95*mons[n,2] - 1 + rand(int(0.1*mons[n,2])+1);  if rand(30) <= 1: coup *= 2
coup = int(coup*(100 - (adef+def))/100)
special: if mons[n,11]≠0 and rand(10) <= mons[n,11]: coup = int(1.5*coup)  (the sprite dashes)
hero evades if rand(100) <= esq + chan/2  ("Miss")
```
The monster's AI order (`atqen:13-35`): low-HP spell c17 (if c20≠0, hpe<c1/c20 and rand(100)<c16); self-cure c26 (if hpe<c1/c21 and c26 starts with "s"); support c25 (h1/l1/st); regular spell c22 (if c20≠0, rand(c21)≤c20 and **hero** hp > 0.6·c2); spell c15 (if rand(100)≤c14); otherwise the physical attack.

### Magic (`magi`, `magie`)
MP costs are shown in the F2 toolbar: Fire 4, Cure 5 (Ice 4, Fire2 20, Cure2 22...).
If the materia is not equipped or MP is short, magie returns without effect **but the turn is still consumed** (qm=0 → `combat:155-156`).
- **Fire (hero)**, `magie:63,388-397`: degm=9, `coup = mag*9`; `coup = 0.95*coup-1+rand(int(0.1*coup)+1)`; `coup = int(coup*(255-mons[n,19])/255)`. Damage is doubled if `left(ma,1)=mons[n,23]` (weakness), and heals the monster if it equals mons[n,24] (absorb). mp -= 4. At mag 12: 102-112 vs mdef 0, 94-103 vs the boss.
- **Cure (hero)**, `magie:71-72,425-429`: `recup = 9*mag`, `recup = int(0.9*recup-1+rand(int(0.2*recup)+1))`, then hp += recup capped at hpm; mp -= 5. At mag 12: 97-118. Cure always targets the hero, so monster 2's weakness "s" never matters.
- **Monster spell on the hero**, `magie:410-412`: `coup = mons[n,18]*degm`, same ±5 % variance, `coup = int(coup*(100-adefm-defm)/100)`. Fire and Ice are halved by armors 6/7 (not in Part I).

### Limit break (`atqen:278-282`, `at:10-13`, `limite:34`)
- Every hit taken adds `jl += (35-4*lim)*coup/hpm` (lim 1: 31·dmg/hpm). Enemy spells count too. The gauge is full at 157 ("Max LIMIT!!"), so it takes about 0.97·hpm of total damage taken. It persists between battles and is saved.
- When jl=157, pressing F1 casts the limit instead (tran=0, cont=0). **Braver** (lim 1): li=1, forc1=2.5·forc, rate1=200 (it cannot miss), then the normal attack formula runs, criticals included. Afterwards forc1 and rate1 are restored and jl=127. Braver with Power Wrist vs the boss: 102-112.

### Items in battle (`objet`, obj=2); each use costs the turn
| Item (var) | Effect | Condition |
|---|---|---|
| Potion npot | hp+100 (capped) | hp≠hpm |
| Hi Potion nspot | hp+500 | hp≠hpm |
| Ether net | mp+50 | mp≠mpm |
| Turbo Ether nett | mp+200 | |
| X Potion npx | hp=hpm | |
| Elixir nel | hp=hpm, mp=mpm | |
| Antidote anti | cures poison (pois1) or shows "Miss" | battle only ("Can't use outside the battle.") |
`Lbl el` is defined twice in `objet` (lines 80, 89). The first one wins, so Elixir works.

### Escape (`combat:221-225`)
If `rand(4) < qu`, the hero escapes (random battles 3/4, bosses never). He goes back to the map with no reward and fui+1. Otherwise "Can't run away" shows and the turn is lost.

### Victory (`aa`, `fincomb`)
The monster sprite blinks 5 times. Then: gils += c6, exp += c5. Each equipped materia gets `maglist[m,3] += croiss*ape`; it levels up when AP ≥ the threshold of the next column ("<name> LV+1!"; an unarmed hero gets croiss 0, hence 0 AP). The drop happens if `rand(c13) ≤ c12`: +1 of the item var c9, shown as "Found:"&c10. The screen shows "Battle is over" / Gils / Ap / Exp (the Ap shown is the raw c7). co is re-rolled.
**Defeat**: hp≤0 shows "Game Over", then `delet()` deletes every variable and **exits to HOME** (`combat:59-66`, `delet`). The game must be reloaded.

### mons columns (decoded from atqen/at/magie/choimon/fincomb/voler/aa)
| c | Meaning | c | Meaning |
|---|---|---|---|
| 1 | HP | 16 | % chance of the low-HP spell c17 |
| 2 | attack | 17 | low-HP spell (when hpe < c1/c20) |
| 3 | physical defense (/255) | 18 | magic power (spell dmg = c18·degm) |
| 4 | speed (ATB) | 19 | magic defense (/255) |
| 5 | EXP | 20 | spell chance numerator (also the low-HP divisor) |
| 6 | gils | 21 | spell chance denominator (also the self-cure divisor) |
| 7 | AP | 22 | regular spell |
| 8 | sprite x offset `dec` | 23 | weakness letter (×2 dmg) |
| 9 | drop/steal item var | 24 | absorb letter (heals; "b..." = immune to poison) |
| 10 | drop/steal item name | 25 | support spell h1/l1/st |
| 11 | special attack chance (rand(10)≤c11 → ×1.5) | 26 | self-cure spell s1..s3 |
| 12 | drop numerator; ≠0 = stealable | 27 | Morph result var |
| 13 | drop denominator | 28 | evade/level: hit penalty and status resistance |
| 14 | % chance of spell c15 | 29 | Morph result name |
| 15 | spell | | |

### Part I monsters
| n | Sprite | HP | Atk | Def | Spd | EXP | Gils | AP | Drop | Special | Magic | MDef | Evade | Weak | Where |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | monst1 (small winged creature, 18×24) | 90 | 7 | 10 | 14 | 85 | 50 | 7 | Potion 2/4 = 50 % | 30 % ×1.5 | none | 0 | 0 | none | rooms 10-16 (20 % in 10, 60 % elsewhere) |
| 2 | monst2 (skeleton, 16×44) | 120 | 10 | 30 | 16 | 125 | 70 | 10 | Potion 2/5 = 40 % | none | Ice "g1" 1/3 (rand(3)≤1), power 2 → 16-17 dmg | 15 | 4 | "s" (unused) | rooms 10-16 (80 % in 10, 40 % elsewhere) |
| 3 | monst3 (prisoner with spear, 21×49) | 1000 | 30 | 45 | 12 | 330 | 1000 | 0 | Hi Potion 1/1 = 100 % | none | Fire "f1" 2/5, power 4.5 → 36-39 dmg | 20 | 6 | none | boss, room 16 (story4) |
Monster damage on a Lv1 hero (def 5): n1 6 (special 9, critical 12); n2 9; n3 27-29 (critical 54-59). Morph results (c27/c29) are Potion, Potion and Hi Potion.

### Boss: story4 (dec:5695), n=3
The trigger is the 503 ring when clef[5]=0. The sprite `bos2` appears at (63,72): "A Voice:...". The boss walks to the hero's column (`bos3` walking right or `bos4` walking left, 9 px steps) and then up to b+9. Then n=3 and `combat()` (qu=1, no escape). His behaviour per turn: if hero hp > 18 he casts Fire with probability 2/5, otherwise he attacks physically. He has no low-HP behaviour (c16=0, c26=""). Rewards: 330 EXP, 1000 gils, 0 AP, 1 Hi Potion, then "Found:Cell 2 Key!" and clef[5]=1. The hero is unarmed at this point (the sword is in the chest he guards), so only Power Wrist and Braver help.

---------------------------------------------------------------------------------------------------

## 6. Menus, shop, bed

### Map keys (`mov`)
Arrows move by 9 px and redraw with `RplcPic #t` plus the facing sprite: art1x down, art2x up, art3x right, art4x left (x=0 on foot). ENTER/F1 while facing a text cell reads it. **APPS** (265) opens the main menu (on foot only). **ESC** (264) opens `del`: a popup {"Continue","Quit"}. Quit calls `delet()` (exit without saving).

### Main menu (`menu`)
The header shows Level, Gils, HP a/b, MP a/b, "Next Lv:" expt-exp plus a bar, Steps (pas), then Weapon, Armor, Acc.1 and Acc.2. The toolbar:
| F | Title | Content |
|---|---|---|
| F1 | ITEM | `objet` obj=1: Potion, Hi Potion, Ether, Turbo Ether, X Potion, Elixir (Antidote refused) and chocobo greens (info text only) |
| F2 | EQUIP | Weapon (`armch`: Attack, %attack, Slot(s), Growth ×, "NB:"), Armor (`amuch`: Defense, Magic Def., %Evade), Accessory 1, Accessory 2 (`acce`: "Choose an accessory", Effect) |
| F3 | MATERIA | `materia`: slots 1..orif, ▸ cursor up/down, ENTER opens a popup of the owned, unequipped materia; it shows Lv, Ap and "Nxt Lv:". Bug: it compares to `"master"` in lower case (`materia:37`) while the data holds `"Master"`, so the "Nxt Lv:master" line never shows |
| F4 | STATUS | Lv, Exp, Next Lv, Strength, Magic, Vitality, Spirit, Luck, Speed, Limit n + gauge. Attack = int(forc·atq/2). Phys.Def = adef+def (≤100). Mag.Def = adefm+defm (≤100). %Evade = esq+chan/2. "See INFORMATIONS": Killed, Flight, Limits used, Critical Cuts, Magics used, Items used, Attacks (total), Missed Attacks %, Total Received Cuts, Missed Received Cuts % |
| F5 | SAVE | clef[66]=0, then `sav(2)`: "PassWord,6 letters max". It writes `sv{pw}` (a 63-item list, `sav:17`), `sa{pw}` = [armat column 1; clef], `sm{pw}` = maglist and `sh{pw}` = chmat. "Game Saved". Continue from the title screen reads them (`ffa:206-241`) |
| F6 | OPTIONS | Battle Speed 1-7 (vcomb), Battle Mode Active/Wait, Limit (Braver only in Part I) |
| F7 | ESC | back to the map |

### Potion seller (`shop1`, `buy`)
The screen shows `"Gils=N g"`, `"_[F1]=Potions   50g"` and `"[ESC]=quit"`. F1 opens a dialog "Nb of Potions:k / Gils=Ng". ENTER buys **one** potion for 50 g; ESC in the dialog cancels. Without enough gils: "Sorry,not enough gils". ESC on the shop screen shows "See you later." and redraws the room. Prices in `buy:5`: Potion 50, Hi Potion 250, Ether 800, Turbo Ether 1500, Elixir 5000; shop1 enables F1 only (ee=1).

### Bed (room 8, -2.5)
"Sleep?" Yes sets hp=hpm and mp=mpm. It is free and unlimited, with a horizontal-line wipe (`dec:6086-6100`).

---------------------------------------------------------------------------------------------------

## 7. Dialogue texts of Part I, verbatim (`boit(n,portrait,text)`)

Portraits `perso{k}`: 1 hero, 2 Edouard, 3 Olen, 4 Larc, 6 Jess (0 = none). The first argument is 0 or 1; its meaning is in `dia` (ASM, not decoded): unclear, probably the box position or style.

**Intro** (`ffa:167-195`, PxlText, each block ends with Pause): "In a poor Milunian" / "family..." — "LIONHEART" / "My poor baby,you" / "can't even walk yet," / "and all the matters" / "are falling on you..." / "Mary,my dear wife,is" / "caught by the Bramanian" / "soldiers,and I must" — "LIONHEART" / "soldier,and I must return" / " to this cursed War..." / "I'll give the priest to" / "look after you,wishing I" / "will come back alive" / "from this War..." — "20 years after...". Name prompt: "Enter your name,8 letters max.".

**D1 story1** (room 8, 500): (2) `"EDOUARD:My son...,as=you know,today is a=great day for you,=you'll be named=KNIGHT.As soon as=you're ready,come=downstairs."`

**D2 story2** (room 6, 501): (1) `name&":Father,I=don't see where my=sword is..."` — (2) `"Edouard:Your Sword!=Oh that's right!It is=in the dungeon,in the=weapon room.It's a=ritual,each future=Knight must beat=some monsters to find=his sword...So good=luck!"`

**D3 Edouard -9** (dec:6111), the first matching branch:
- clef[11]=0: (2) `"Your sword is in the=dungeon."`
- clef[11]=1 and clef[1]=0: (2) `"Closed?=Olen is the last who=went there.."`
- clef[14]=1 and clef[61]=0: `"EDOUARD:Well,Good Luck=Knight "&name&"!!"`. Unreachable, since clef[14] is never set. In Part I he therefore says nothing once you hold the key.

**D4 Olen -10** (dec:6238):
- clef[1]=0: (3) `"OLEN:"&name&"!How=are you!Ready for=the Big Day!Here is=the Dungeon Key...=Good Luck!"`, then `"Found:Dungeon Key!"`
- armat[1,1]=1 and maglist[3,7]=0: (3) `"Take this,"&name&",=it will be useful."`, then `"Found: Materia  :=Cure!"`
- maglist[3,7]=1: (3) `"OLEN:Good Luck!"`
- otherwise, while clef[67]=0 (key taken, no sword yet): (3) `"Olen:"&name&",you=mustn't get=discouraged!"`

**D5 Jess -11** (dec:6257): armat[1,1]=0: (6) `"JESS:A lot of=prisoner's corpses=are remained in the=dungeon...It scares=me so much!"`. After the sword, the next branch needs clef[14], so she is silent for the rest of Part I.

**D6 story3** (room 13, 502): `"This door will open if=you know the number of=injuried who stayed=alive."`, then a Request dialog. Right: `"That's right...=You can enter."`. Wrong or cancelled: `"False"` (Cancel returns silently).

**D7 story4** (room 16, boss): `"A Voice:Ya there...=What're ya doing here?"` — (1) `name&":I ask you the=same question."` — `"I'm send by..well,I'm a=prisoner,because of your=father!You'll pay for=him!"` — [battle] — `"Found:Cell 2 Key!"` — (1) `"Cell 2!This prisoner=must have came from=there..."`

**D8 story5**, the knighting (room 6, 505). The hero sprite walks from (63,63) to (36,45), and Olen/Jess (`ole2`,`bon12`) then Larc (`fre2`) appear at the bottom:
1. (2) `"You found your sword="&name&".."`
2. (1) `"Yes..and I was=attacked by a=prisoner,but nothing=important."`
3. (2) `"Strange...Ha!Here are=Olen and Jess!Only=Larc is missing..."`
4. (2) `"It's him..You were=at the village?"`
5. (4) `"LARC:Yes...You're=also here,"&name&"..."`
6. (2) `"Well.Let's start,="&name&"..."`
7. (2) `"I,Edouard STRIFE,Lord=of Milunia Kingdom,=declares "&name&"=STRIFE,my son,Knight.=You must defend your=castle and your=people.Now swear=respect and obedience=to your Lord."`
8. (1) `"I swear it.."`
9. (2) `name&",you are now=a Milunia Knight!"`
10. (6) `"Jess:Long life the=Knight "&name&"!!!"`
11. (3) `"Olen:Long life the=Lord STRIFE!!!"`
12. (2) `"Thank you,my friends.=Now,I want to speak=with my sons..."`
13. (6) `"Yes my Lord..."` (Olen and Jess leave)
14. (4) `"LARC:Father,..I have=to go to the village=again..."`
15. (2) `"But...,well...Go.=...but that's not=polite to leave like=that!"` (Larc leaves, `fre1`)
16. (2) `name&".."` (a villager, sprite `bon12` at 63,72)
17. (0) `"A villager:My Lord!=My Lord!The village's=been attacked again,some=Chocobos from the Ranch='ve been killed!=We need help!"`
18. (2) `"Strange..Don't worry,=go say the Village's=Chief we gonna help=him."`
19. (0) `"All Right.Thanks=My Lord!"`
20. (2) `"Well..."&name&".I want=you to help Milunia=Village,it's your=first mission."`
21. (1) `"Yes,I'll go."`
22. (2) `"I could have asked=your brother for some=help,but he's just an=incompetent!=Well,good luck,=Knight "&name&"!!"`
23. (2) `"By the way,...don't go=to BRAMANA,we have no=good relations with=them nowadays."`
End: clef[7]=1, clef[8]=1, hero placed at (36,45) facing down, p=0.

**Cells** (all quoted in §2): the bed "Sleep?"; plaque -3.5; room 8 -3; room 7 -2, -1.5, -4, -3; room 5 -7.1, -12; room 4 -7.4; room 12 -6; room 13 -4 ("Push the switch?", "There's a lock noise."), -2 ("Oh no!..."); room 14 -3; room 15 -7 and the Power Wrist ("It holds something...", "Found Accessory=Power Wrist!"); room 10 key ("It holds a key.", "Found little Key!"); chests "Found Materia=Fire!", "Found Weapon=Buster Sword !", "Found Armor=Bronze Bangle!"; `recu` chests `"Found "&i&" "&name&"(s) !"`, for example "Found 1 Potion(s) !"; room 18 "Fresh carrots.."; room 16 "Locked by Gold Seal.".

**System**: "The door is locked." (1); "The Weapon Room is=not behind this door,=anyway..." (door 73); "I must leave the=Chocobo :=ESC"; battle: "Can't run away", "Miss", "Max LIMIT!!", "Braver", "Battle is over", "Level UP!", "Fire LV+1!", "Found:Potion", "Game Over"; shop: see §6.

---------------------------------------------------------------------------------------------------

## 8. Remaining unknowns
- `dia(n,ni,ch,pic)` is an **ASM program** (type 0x21 in the `programs.89g` folder table, offset 368) and is not decoded. The box layout, the meaning of n, and the handling of portrait 0 are unknown. It wraps text at `=`.
- `at:25` tests `l=2`, probably meant as `li`.
- Room 15 value -8 has no handler.
- Room 17's monster column is inconsistent, and the column in room 4 is unused (frc 0).
- clef[4] (cell 1) and clef[14] are never set: dead content.
- Monster names do not exist in the data (only the sprites `monst1..3`).
