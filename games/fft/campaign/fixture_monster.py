"""Monster sets for check fixtures that write synthetic records into the monster ability
interval: set 0xB0 + k holds the records 0x100 + 4k .. 0x103 + 4k (fixture_monster.h)."""
import struct


def monster_sets(catalogue):
    """Rewrite the fixture catalogue's 28 first monster sets in place."""
    at=struct.unpack_from('>H',catalogue,4+2*9)[0]      # CJ_MONSTER: 5 bytes per set
    menu=struct.unpack_from('>H',catalogue,4+2*7)[0]+168   # CJ_SPECIAL: one menu type per set
    for k in range(28):
        catalogue[at+5*k:at+5*k+5]=bytes((0xF0,*(4*k+j for j in range(4))))   # four ids above 255
        catalogue[menu+0xB0+k]=0     # the default menu: these synthetic records pay their MP
