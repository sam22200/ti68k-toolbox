#!/usr/bin/env python3
"""Game Boy ROM facts and graphics, for the port: header, then the video memory of a running
state (tiles are often compressed in the ROM; VRAM holds them decoded).

usage: gbextract.py ROM OUTDIR [--state S.state] [--frames N] [--tag NAME]

OUTDIR/info.json      header (title, cartridge type / MBC, ROM and RAM size, CGB flag, checksums
                      verified), ROM bank count, and the census of the code that the port needs
                      (interrupt vectors used, LCDC writes, DMA routine, uses of the window, of
                      STAT/LY interrupts: raster effects)
With --state (or --frames N from power-on), the video state at that point, files named by --tag:
OUTDIR/<tag>_tiles.png  the 384 VRAM tiles (16 per row, 8x8, GB shades through BGP)
OUTDIR/<tag>_bg.png     the 256x256 background map, the 160x144 viewport (SCX/SCY) outlined
OUTDIR/<tag>_win.png    the window map, if the window is on (HUDs often live there)
OUTDIR/<tag>_oam.txt    the 40 sprites: y x tile flags (palette, flip, priority)
OUTDIR/<tag>_vram.bin   0x8000-0x9fff raw, OUTDIR/<tag>_regs.txt LCDC SCX SCY WX WY BGP OBP0 OBP1
"""
import argparse, json, os, warnings

warnings.filterwarnings('ignore')
from PIL import Image, ImageDraw  # noqa: E402

SHADES = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]
CART = {0x00: 'ROM ONLY', 0x01: 'MBC1', 0x02: 'MBC1+RAM', 0x03: 'MBC1+RAM+BATTERY',
        0x05: 'MBC2', 0x06: 'MBC2+BATTERY', 0x0f: 'MBC3+TIMER+BATTERY',
        0x10: 'MBC3+TIMER+RAM+BATTERY', 0x11: 'MBC3', 0x12: 'MBC3+RAM',
        0x13: 'MBC3+RAM+BATTERY', 0x19: 'MBC5', 0x1a: 'MBC5+RAM', 0x1b: 'MBC5+RAM+BATTERY',
        0x1c: 'MBC5+RUMBLE', 0x1e: 'MBC5+RUMBLE+RAM+BATTERY'}


def header(rom):
    chk = 0
    for b in rom[0x134:0x14d]:
        chk = (chk - b - 1) & 0xff
    gsum = (sum(rom) - rom[0x14e] - rom[0x14f]) & 0xffff
    return {
        'title': rom[0x134:0x144].split(b'\0')[0].decode('ascii', 'replace').strip(),
        'cgb': {0x80: 'CGB compatible', 0xc0: 'CGB only'}.get(rom[0x143], 'DMG'),
        'sgb': rom[0x146] == 3,
        'cartridge': CART.get(rom[0x147], hex(rom[0x147])),
        'rom_kb': 32 << rom[0x148], 'rom_banks': (32 << rom[0x148]) // 16,
        'ram_kb': {0: 0, 2: 8, 3: 32, 4: 128, 5: 64}.get(rom[0x149], '?'),
        'header_checksum_ok': chk == rom[0x14d],
        'global_checksum_ok': gsum == (rom[0x14e] << 8 | rom[0x14f]),
        'file_bytes': len(rom),
    }


def census(rom):
    """Byte-pattern census (a hint, not a proof: data can match): what the port must care for."""
    def count(pat):
        return rom.count(bytes(pat))
    vecs = (('vblank', 0x40), ('stat', 0x48), ('timer', 0x50), ('serial', 0x58), ('joypad', 0x60))
    return {
        'interrupt_vectors_used': [k for k, a in vecs if rom[a] not in (0xd9, 0xff, 0x00)],
        'ldh_lcdc_writes': count([0xe0, 0x40]), 'ldh_stat_writes': count([0xe0, 0x41]),
        'ldh_scx_writes': count([0xe0, 0x43]), 'ldh_scy_writes': count([0xe0, 0x42]),
        'ldh_wy_wx_writes': count([0xe0, 0x4a]) + count([0xe0, 0x4b]),
        'ldh_lyc_writes': count([0xe0, 0x45]), 'ldh_ie_writes': count([0xe0, 0xff]),
        'oam_dma_writes': count([0xe0, 0x46]), 'mbc_bank_writes_2000': count([0xea, 0x00, 0x20]),
        'sound_writes_ff1x_ff2x': sum(count([0xe0, r]) for r in range(0x10, 0x27)),
    }


def tile(vram, n, pal):
    base = n * 16
    img = Image.new('RGB', (8, 8))
    for y in range(8):
        lo, hi = vram[base + 2 * y], vram[base + 2 * y + 1]
        for x in range(8):
            c = ((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1)
            img.putpixel((x, y), SHADES[(pal >> (2 * c)) & 3])
    return img


def render_map(vram, lcdc, base, bgp):
    img = Image.new('RGB', (256, 256))
    for my in range(32):
        for mx in range(32):
            t = vram[base - 0x8000 + my * 32 + mx]
            n = t if lcdc & 0x10 else (256 + t if t < 128 else t)
            img.paste(tile(vram, n, bgp), (mx * 8, my * 8))
    return img


def video(gb, out, tag):
    mem = gb.memory
    vram = bytes(mem[0x8000:0xa000])
    lcdc, scy, scx, wy, wx = (mem[a] for a in (0xff40, 0xff42, 0xff43, 0xff4a, 0xff4b))
    bgp, obp0, obp1 = mem[0xff47], mem[0xff48], mem[0xff49]
    open(os.path.join(out, tag + '_vram.bin'), 'wb').write(vram)
    open(os.path.join(out, tag + '_regs.txt'), 'w').write(
        'LCDC=%02x SCX=%d SCY=%d WX=%d WY=%d BGP=%02x OBP0=%02x OBP1=%02x\n'
        % (lcdc, scx, scy, wx, wy, bgp, obp0, obp1))
    sheet = Image.new('RGB', (128, 192))
    for n in range(384):
        sheet.paste(tile(vram, n, 0xe4), ((n % 16) * 8, (n // 16) * 8))
    sheet.save(os.path.join(out, tag + '_tiles.png'))
    bg = render_map(vram, lcdc, 0x9c00 if lcdc & 0x08 else 0x9800, bgp)
    d = ImageDraw.Draw(bg)
    d.rectangle([scx, scy, scx + 159, scy + 143], outline=(255, 0, 0))
    bg.save(os.path.join(out, tag + '_bg.png'))
    if lcdc & 0x20:
        render_map(vram, lcdc, 0x9c00 if lcdc & 0x40 else 0x9800, bgp).save(
            os.path.join(out, tag + '_win.png'))
    with open(os.path.join(out, tag + '_oam.txt'), 'w') as f:
        f.write('# n y x tile flags (screen y = y-16, x = x-8; flags: 7 prio 6 yflip 5 xflip 4 pal)\n')
        for n in range(40):
            y, x, t, fl = (mem[0xfe00 + 4 * n + i] for i in range(4))
            if 0 < y < 160 and 0 < x < 168:
                f.write('%2d %3d %3d %02x %02x\n' % (n, y, x, t, fl))
    gb.screen.image.convert('RGB').save(os.path.join(out, tag + '_screen.png'))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('rom')
    ap.add_argument('out')
    ap.add_argument('--state')
    ap.add_argument('--frames', type=int)
    ap.add_argument('--tag', default='video')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rom = open(a.rom, 'rb').read()
    info = header(rom)
    info['census'] = census(rom)
    json.dump(info, open(os.path.join(a.out, 'info.json'), 'w'), indent=1)
    print(json.dumps(info))
    if a.state or a.frames:
        from pyboy import PyBoy
        gb = PyBoy(a.rom, window='null', sound_emulated=False)
        if a.state:
            with open(a.state, 'rb') as f:
                gb.load_state(f)
        for _ in range(a.frames or 0):
            gb.tick()
        video(gb, a.out, a.tag)
        gb.stop(save=False)


if __name__ == '__main__':
    main()
