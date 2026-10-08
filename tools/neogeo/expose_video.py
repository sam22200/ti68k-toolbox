#!/usr/bin/env python3
"""Add reference snapshots and explicit initial-RTC setting to local FBNeo."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CORE_REV = 'a49cfac4b97cc62d0196c1d0cde8f5b14fde662c'
MARKER = '// TI Neo Geo reference exports v1'
# Appended in neo_run.cpp to access its private RAM/latched video variables.
# retro_* names are exported by the core's existing linker version script.
EXPORTS = r'''
// TI Neo Geo reference exports v1
extern "C" void *retro_ti_neogeo_region(UINT32 id)
{
    if (!AllRAM || !(nNeoSystemType & NEO_SYS_CART)) return NULL;
    switch (id) {
        case 0: return Neo68KRAM;
        case 1: return NeoGraphicsRAM;
        case 2: return NeoPalSrc[0];
        case 3: return NeoPalSrc[1];
        case 4: return NeoNVRAM;
        case 5: return NeoMemoryCard;
        case 6: return NeoInput;
        case 7: return NeoZ80RAM;
    }
    return NULL;
}

extern "C" UINT32 retro_ti_neogeo_size(UINT32 id)
{
    if (!retro_ti_neogeo_region(id)) return 0;
    switch (id) {
        case 0: return nNeo68KRAMLen;
        case 1: return 0x20000;
        case 2: case 3: return 0x2000;
        case 4: return 0x10000;
        case 5: return 0x20000;
        case 6: return sizeof(NeoInput);
        case 7: return 0x800;
    }
    return 0;
}

extern "C" INT32 retro_ti_neogeo_info(UINT32 id)
{
    if (id == 104) return 1; // Deterministic RTC initialization export
    if (id == 100) return 1; // Reference export ABI version
    if (id == 101) {
#ifdef LSB_FIRST
        return 1; // Word-swapped 68000 memory on this host
#else
        return 0;
#endif
    }
    if (id == 102) {
#ifdef USE_SPEEDHACKS
        return 1;
#else
        return 0;
#endif
    }
    if (id == 103) {
#ifdef __FAST_MATH__
        return 1;
#else
        return 0;
#endif
    }
    if (!AllRAM || !(nNeoSystemType & NEO_SYS_CART)) return -1;
    switch (id) {
        case 0: return nBIOS;
        case 1: return nNeoSystemType;
        case 2: return NeoSystem;
        case 3: return nNeoPaletteBank;
        case 4: return nNeoSpriteFrame;
        case 5: return nSpriteFrameSpeed;
        case 6: return nSpriteFrameTimer;
        case 7: return bBIOSTextROMEnabled;
        case 8: return bNeoDarkenPalette;
        case 9: return bNeoEnableGraphics;
        case 10: return bNeoEnableSprites;
        case 11: return bNeoEnableText;
        case 12: return NeoGraphicsRAMPointer;
        case 13: return nNeoGraphicsModulo;
        case 14: {
            struct BurnRomInfo ri;
            if (nBIOS < 0 || BurnDrvGetRomInfo(&ri, 0x80 + nBIOS)) return -1;
            return ri.nCrc;
        }
    }
    return -1;
}

extern "C" const char *retro_ti_neogeo_bios_name()
{
    char *name = NULL;
    if (!AllRAM || nBIOS < 0) return NULL;
    if (BurnDrvGetRomName(&name, 0x80 + nBIOS, 0)) return NULL;
    return name;
}

extern "C" UINT32 retro_ti_neogeo_bus_read(UINT32 address, UINT32 size)
{
    if (!AllRAM || address < 0x100000 || size < 1 || size > 4 ||
        address > 0x110000 - size) return 0;
    UINT32 value = 0;
    SekOpen(0);
    for (UINT32 i = 0; i < size; i++) value = (value << 8) | SekReadByte(address + i);
    SekClose();
    return value;
}
'''

CLOCK_EXPORTS = r'''
// TI Neo Geo reference initial RTC v1
extern "C" INT32 retro_ti_neogeo_clock_init()
{
    // Call immediately after driver init, before any emulated CPU frame.
    // Leave registers, tick counters and the ordinary RTC update path intact.
    if (!totcyc_cb || !nOneSecond || uPD4990A.nCount || uPD4990A.nTPCount ||
        uPD4990A.nCommand || uPD4990A.nMode) return 0;
    uPD4990A.nSeconds = uPD4990A.nMinutes = uPD4990A.nHours = 0;
    uPD4990A.nDay = uPD4990A.nMonth = 1;
    uPD4990A.nYear = 0;
    uPD4990A.nWeekDay = 6; // 2000-01-01 was Saturday.
    return 1;
}
'''


def main():
    path = ROOT / 'sources/neogeo_core/src/burn/drv/neogeo/neo_run.cpp'
    raw = path.read_bytes()
    marker = MARKER.encode()
    if marker in raw:
        if not raw.endswith(EXPORTS.encode()):
            legacy = EXPORTS.replace('    if (id == 104) return 1; // Deterministic RTC initialization export\n', '').encode()
            if not raw.endswith(legacy):
                raise ValueError('existing reference exports differ; inspect the local patch')
            path.write_bytes(raw[:-len(legacy)] + EXPORTS.encode())
            print('Reference RTC feature flag added; rebuild required')
    else:
        path.write_bytes(raw + EXPORTS.encode())
        print('Neo Geo RAM/video reference exports added; rebuild required')
    clock_path = path.with_name('neo_upd4990a.cpp')
    raw = clock_path.read_bytes()
    if b'// TI Neo Geo reference initial RTC v1' in raw:
        if not raw.endswith(CLOCK_EXPORTS.encode()):
            raise ValueError('existing reference RTC export differs; inspect the local patch')
    else:
        clock_path.write_bytes(raw + CLOCK_EXPORTS.encode())
        print('Initial RTC export added; rebuild required')


if __name__ == '__main__':
    main()
