#!/usr/bin/env python3
"""The town shops: original prices, stock tiers and per-shop categories (FSH1).

usage: shop_assets.py DISC.cue OUT.bin

OBSERVED on the disc (battle.py ITEMS, 12 bytes an item): the price is a
little-endian u16 at offset 8 (the PlayStation's order; the bank stores it
big-endian like every other record we ship), the shop stock tier a byte at offset 10 (0: never
sold, 20: the rare items no shop carries), and the item's category the byte at
offset 5. The location record's flag byte (world_assets.py DOTS) is the kind of
place: 0 a castle, 1 a town, 2 a battle site -- only the first two have shops.

INTERPRETATION (Shop Guide, S. Maxson 2004): which categories each shop type
carries, and which type each town is. The game derives both from data we have
not located yet; the tables below reproduce its shop lists.
"""
from pathlib import Path
import struct
import sys
import battle
import extract
import management_data
import world_assets

ITEMS = 256
# The item category byte (offset 5), named as the guide names them.
DAGGER, NINJA_SWORD, SWORD, KNIGHT_SWORD, KATANA, AXE = 1, 2, 3, 4, 5, 6
ROD, STAFF, FLAIL, GUN, CROSSBOW, LONGBOW, HARP, BOOK, SPEAR, STICK = range(7, 17)
BAG, TEXTILE, SHIELD, HELMET, HAT, HAIR, ARMOR, CLOTHES, ROBE = range(17, 26)
SHOE, ARMGUARD, RING, ARMLET, MANTLE, PERFUME = range(26, 32)
SHURIKEN, BOMB, BASIC = 32, 33, 34
# "All shops have all accessories and basic items available."
EVERY = (HAIR, SHOE, ARMGUARD, RING, ARMLET, MANTLE, PERFUME, BASIC)
TYPES = {
    1: (BAG, BOMB, CLOTHES, DAGGER, HAT, ROBE, ROD, SHURIKEN, STAFF, STICK),      # A Gariland
    2: (ARMOR, AXE, CROSSBOW, DAGGER, FLAIL, HELMET, LONGBOW, SHIELD, SPEAR, SWORD),  # B castles
    3: (BAG, BOMB, BOOK, CLOTHES, FLAIL, HARP, HAT, KATANA, NINJA_SWORD, ROBE,    # C trade cities
        ROD, SHIELD, SHURIKEN, STAFF, STICK, TEXTILE),
    4: (BAG, BOMB, CLOTHES, HAT, ROBE, ROD, SHURIKEN, STAFF, STICK),              # D other cities
    5: (BOMB, CROSSBOW, GUN),                                                     # E Goug
    6: (ARMOR, AXE, DAGGER, HELMET, LONGBOW, SHIELD, SPEAR, SWORD),               # F Lesalia
}
# Location index (world_assets order) -> shop type.
SHOPS = {6: 1, 2: 2, 3: 2, 1: 2, 5: 2, 4: 2, 9: 3, 12: 3, 14: 3,
         10: 4, 8: 4, 7: 4, 13: 4, 11: 5, 0: 6}
# Gariland alone carries one sword, and no other.
LONE = {1: ('Broad Sword',)}
TIERS = 16
# Story flag that opens each stock tier: 0 always, 255 not in this build.
# Chapter I advances it as its battles are authored; the pilot has two.
TIER_FLAG = {1: 0}


def build(cue):
    disc, files = extract.disc_files(cue)
    d = battle.Disc(cue)
    core = disc.read(*files['WORLD/WLDCORE.BIN'])
    world = disc.read(*files['WORLD/WORLD.BIN'])
    names = management_data.names(world, world_assets.NAMES, 43)
    item_names = management_data.names(d.bat, management_data.ITEM_NAMES, ITEMS)
    price = bytearray(2 * ITEMS)
    tier = bytearray(ITEMS)
    kind = bytearray(ITEMS)
    for i in range(ITEMS):
        r = d.at(battle.ITEMS + 12 * i, 12)
        struct.pack_into('>H', price, 2 * i, r[8] | r[9] << 8)
        tier[i], kind[i] = r[10], r[5]
    cats = bytearray(5 * (len(TYPES) + 1))
    for t, group in TYPES.items():
        for c in tuple(group) + EVERY:
            cats[5 * t + (c >> 3)] |= 1 << (c & 7)
    lone = bytearray(len(TYPES) + 1)
    for t, items in LONE.items():
        lone[t] = item_names.index(items[0])
    loc = bytearray(43)
    for i in range(43):
        a, _ = struct.unpack_from('<II', core, world_assets.DOTS + 8 * i)
        if (a >> 16) & 255 > 1 and i in SHOPS:
            raise ValueError('%s is a battle site, not a town' % names[i])
        loc[i] = SHOPS.get(i, 0)
    flags = bytearray(255 for _ in range(TIERS))
    for t, f in TIER_FLAG.items():
        flags[t] = f
    out = (b'FSH1' + bytes((ITEMS - 1, len(TYPES))) + bytes(price) + bytes(tier)
           + bytes(kind) + bytes(cats) + bytes(lone) + bytes(loc) + bytes(flags))
    sold = sum(1 for t in tier if t and t < 20)
    return out, dict(bytes=len(out), sold=sold,
                     early=sum(1 for t in tier if 0 < t <= 4))


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    raw, info = build(cue)
    out.write_bytes(raw)
    print('%s: %d bytes, %d items ever sold, %d by stock tier 4' %
          (out.name, info['bytes'], info['sold'], info['early']))


if __name__ == '__main__':
    main()
