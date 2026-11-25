# Nice to have

Source mechanics this port does **not** implement, with what each would cost and
what it would touch. Nothing here is a bug or a regression: every line is a
feature of the original that was never built, and the point of the file is that
the next person does not have to re-derive whether it exists.

Opened 2026-11-25 from an audit of FFT's own tavern rumours
(`fft_quotes_game.txt`), each of which describes a mechanic and so asks whether
we have it. **What the audit found already implemented is listed at the bottom**,
so a question asked once is not asked again.

The two hard constraints decide everything below: the battle is short of RAM
(436 free bytes on the Zeklaus 13-unit diagnostic, `campaign/INSTALLED_RAM_ABI99.md`)
and short of program bytes. A feature that needs a new `State` field moves the
ABI and needs a rebuilt core.

---

## 1. Riding a Chocobo

A human unit that moves onto an allied Chocobo's tile mounts it. The **Chocobo**
becomes invulnerable, the rider can still be killed; the rider keeps every one of
their own abilities and loses access to the Chocobo's; Move becomes 6, and nearly
every status is immune except Charm. A black Chocobo flies, a red one ignores
height differences. If the rider reaches 0 HP they dismount and the Chocobo
survives, controllable again.

**Not implemented, and not small.** It needs:

- a rider/mount pair in `State` (two unit slots acting as one), which is a new
  field and therefore an ABI move;
- the movement graph to take the mount's Move, Jump and terrain rules rather than
  the rider's, including flight and height-ignoring, which is `step_geometry.h`
  and the 16-link graph;
- targeting, cover masks and the actor renderer to treat the pair as one tall
  actor at one tile;
- an invulnerability that applies to one half of the pair only.

**Worth it?** It is the single largest gameplay feature missing, and it is
entirely optional content in the original — a player can finish the game without
ever mounting one. Recommend deferring until the campaign is content-complete;
if it is ever taken, take it as its own milestone with a RAM budget agreed first.

## 2. Inheriting abilities from a crystal

When a KO unit's death counter reaches zero it becomes a crystal. Walking onto it
offers either HP/MP restoration **or** its abilities — and in the PS1 version
picking one teaches you **all** of the abilities listed, provided you have
unlocked the corresponding jobs. It is the cheapest way in the game to get
expensive Ninja, Samurai and Calculator skills without farming JP.

**Not implemented, deliberately.** `fft.c` says so where the crystal is handled:
*"a crystal gives back all HP and MP (FFT also offers its abilities: not kept)"*.

**Cost**: moderate and mostly UI. The crystal already exists, its owner's learned
set is already in the save, and the job-unlock test already exists for the
ability menus. What is missing is the choice panel and writing the learned bits
to the picker. No new `State` field is obviously required — the crystal's owner
is already tracked.

**Worth it?** Yes, and it is the best value in this file: it changes how a player
plays every battle, it reuses the ability-learning path we already have, and it
is a *menu* rather than an engine change. Recommend taking it before anything
else here.

## 3. Equip Change (Reequip) in battle -- RESOLVED, it was already there

**This entry was wrong and is kept as a warning about how.** The first audit
grepped for `EQUIP_CHANGE`, found only the chapter descriptor's `EQUIP_CHANGE_ID
0x1E0` and `EQUIP_CHANGE_POS 19`, and concluded there was no in-battle command.
There is. The turn menu adds its row from the support ability's **source id**:

```c
u8 m = (u8)(2 + st.unit[st.act].defend + cj_support(common,job,st.support[st.act],480));
...
else if(st.sub==2+st.unit[st.act].defend){equipment_control(1);}
```

and 480 is 0x1E0. Searching for the constant's *name* missed the one place that
uses its *value*.

It is complete, too. `equipment_runtime.inc` gates it on `who==st.act &&
!st.acted`, so it cannot follow an action, and ends with
`equipment_rebuild(who); st.acted=1; st.mode=M_MENU; st.menu=st.moved?MN_WAIT:MN_MOVE;`
-- it spends the turn's action and leaves movement alone, which is the source's
rule. The exception for items granting an initial status (the Angel Ring's
Reraise) holds by construction: `equipment_rebuild()` recomputes stats and
clamps HP and MP without re-seeding statuses.

**Lesson for the next audit in this file: grep the value as well as the name.**

## 4. Optimum Equipment's per-slot ordering

We have "equip to the maximum" (`best_fit_unit()` in `campaign/manage.c`, and the
shop's Best fit ranks with the same measure), but it is **not the source's rule**.
Ours maximises one aggregate score, `mg_fit()` = HP + MP + 10x(PA+MA+SP) + 8xWP
per item, greedily over two passes, weapon first.

The original picks per slot, by a different criterion each:

| slot | the source's order |
|---|---|
| weapon | additional attack power |
| shield | additional evade rate |
| helmet | gained HP |
| armour | gained HP |

**The gap that matters is the shield**: evade rate is not in `mg_fit()` at all, so
our pick ignores exactly the stat the source ranks shields by.

**Cost**: small and local, one function. **Worth it?** Yes if anyone cares that
the auto-equip chooses the same item the original would. It changes no save
format and no ABI. Note it would also need the shop's Best fit to follow, since
both read the same measure.

## 5. Jump's horizontal reach -- a gap of one already works; the blocker is one byte

Jump does not only set the height a unit can climb: it also sets how wide a gap
it can cross, at **floor(Jump / 2)** empty tiles, with retail clamping ordinary
Jump to 7 first. So Jump 2-3 crosses 1 tile, 4-5 crosses 2, 6-7 crosses 3.

**A gap of one is already implemented.** `movement_edges.inc` builds links at
`for(j=0;j<=(jump>=2)&&j<room;j++)`: j is the number of intervening cells, so
j=1 is a one-tile gap, exactly `long_jump_limit(2..3)`. It checks the span with
`surface_jump_layers()` and remembers the cell jumped over in `W->via[t][k]`, so
a unit standing there still blocks it. The earlier note in this file, that the
movement graph was the obstacle, was wrong.

**Space is not the obstacle either.** A node's edge row is `1 << W->edge_shift`
= 64 bytes at two bytes a link, so 32 links; the builder emits at most 4
directions x 2 distances x 2 layers = 16. **Half of every row is unused**, which
is room for exactly the 4 x 4 x 2 = 32 a gap of three would need. And the nodes
that want a long jump are the ones beside a gap, whose blocked directions emit
nothing at all.

**The obstacle is the `kind` byte, and it is full.** Every bit is spoken for:

| bit | meaning | read at |
|---|---|---|
| 0 | j, the intervening cell | `kind&1`, `kind&3` |
| 1 | water | `kind&2`, `kind&3` |
| 2 | no intermediate node on this floor | `kind&4` |
| 3-4 | step cost | `(kind>>3)&3` |
| 5 | the wide/custom cost family | `kind&32` |
| 6-7 | the direction, for j links | written `k<<6` |

`kind = j | (hw[next]&2)` puts j in bit 0, so j=2 would set the water bit. A gap
of two or three therefore needs a wider field, and `kind` is read by
`tactical_search.inc`, by the AI through the same links, and across the module
ABI -- a format change, not a loop bound.

**What it would take**, in order: widen the link record from two bytes to three
(the rows have the space, the stride does not change, but every reader and the
ABI version move with it), carry j in its own nibble, and check every
intervening cell rather than the first -- `long_jump_masks()` in
`long_jump_geometry.h` already does that part, and its clamp matches retail.

Via is not a problem beyond j=1: a true gap cannot hold a unit, which is what
`kind&4` ("presence is explicit") already says.

**Worth it?** Only on maps with ravines or separated platforms, and it widens
every unit's reach, so the AI has to understand it too. The formula and the
geometry are done; what is left is an ABI change, which belongs with the next
one that happens for another reason rather than on its own.

---

## Already implemented — do not re-ask

The same audit confirmed these, with where to look:

| mechanic | where |
|---|---|
| Remove all equipment at once | `remove_all_equipment()`, management menu row 2 |
| Equip to the maximum (auto-equip) | `best_fit_unit()`, and the shop's Best fit — see §4 for how it differs |
| Bare-handed damage from Brave | `fft.c`: `ya = w ? w->wp : (pa * brave) / 100` |
| Charging takes 1.5x physical damage | `fft.c`, the charge status adds `xa += xa >> 1` on the target |
| Elemental absorb / nullify / half / **weak** / strengthen | `State.affinity[unit][5]` |
| Stealing equipment, and breaking it | `AB_STEAL_EQUIP`, `AB_EQUIP_BREAK` |
| Reflect | `AB_REFLECT` |
| Chicken and Frog, including Frog's damage floor | `UNIT_FROG(ua) -> xa = 1` |
| Crystals and chests from a death counter | `fft.c`, the KO path — the **abilities** on a crystal are §2 |

## Not audited

The rumour file also mentions, and nobody has checked: a knife's damage scaling
with Speed, the Lancer's with Brave, the +40 CT for waiting or doing nothing,
whether a sleeping unit wakes when struck, and items only women can equip. Each
is a formula or a flag rather than a feature, so each is a quick check against
`tools/oracle.py` rather than an entry in this file. Add them here only if a
check shows the behaviour is both wrong and visible.
