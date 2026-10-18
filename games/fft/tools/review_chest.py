#!/usr/bin/env python3
"""Original OTHER.SPR chest versus the actual packed TI sprite and PC battlefield.

Run after make pc ti. Local-disc artwork and resulting captures remain ignored in x/.
"""
from pathlib import Path
import json
import re
import struct
import subprocess
import numpy as np
from PIL import Image, ImageDraw
import extract
import units

OUT = Path('x/chest')
DISC = '../../roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'


def frame_image(frame):
    rows = np.array(struct.unpack('>104H', frame)).reshape(4, units.H)
    bits = 0x8000 >> np.arange(16)
    pixels = ((rows[0, :, None] & bits) != 0).astype(int)
    pixels |= ((rows[1, :, None] & bits) != 0).astype(int) * 2
    return Image.fromarray(np.where(rows[2, :, None] & bits, 170,
                                   np.array([255, 170, 85, 0])[pixels]).astype(np.uint8))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(DISC)
    sprite = disc.read(*files['BATTLE/OTHER.SPR'])
    pal = np.frombuffer(sprite[:32], '<u2').astype(int)
    rgb = np.stack([pal & 31, pal >> 5 & 31, pal >> 10 & 31], axis=1) * 8
    packed = np.frombuffer(sprite[512:], np.uint8).reshape(-1, 128)
    sheet = np.empty((len(packed), 256), np.uint8)
    sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
    x, y, w, h = units.CHEST
    indices = sheet[y:y+h, x:x+w]
    source = rgb[indices].astype(np.uint8); source[indices == 0] = 170
    bank = Path('fftu.bin').read_bytes()
    unit_frames = int(re.search(r'UNIT_FRAMES (\d+)', Path('units.h').read_text())[1])
    offset = (len(units.CHARS) * 2 * unit_frames + 1) * 8 * units.H
    size = 8 * units.H
    current = frame_image(bank[offset:offset+size])
    panels = [('Original FFT / OTHER.SPR', Image.fromarray(source)), ('TI adaptation / packed frame', current)]
    before = OUT / 'before-fftu.bin'
    unchanged = None
    if before.exists():
        old = before.read_bytes()
        unchanged = (old[:offset] == bank[:offset]
                     and old[offset+size:] == bank[offset+size:len(old)])
        assert unchanged and len(bank) - len(old) in (0, 2 * size)
        panels.insert(1, ('Previous TI adaptation', frame_image(old[offset:offset+size])))
    review = Image.new('RGB', (320 * len(panels), 450), 'white'); draw = ImageDraw.Draw(review)
    for i, (label, image) in enumerate(panels):
        draw.text((i*320+8, 8), label, fill='black')
        image = image.resize((image.width * 12, image.height * 12), Image.Resampling.NEAREST)
        review.paste(image, (i*320+(320-image.width)//2, 40))
    subprocess.run(['./fft_pc', '--headless', '--scenario', '70', '--frames', '40',
                    '--scale', '3', '--shot', str(OUT/'scene.png')], check=True, stdout=subprocess.DEVNULL)
    review.save(OUT/'comparison.png')
    animation = []
    for frame in range(2, 161, 2):
        shot = OUT/('frame-%03u.png' % frame)
        subprocess.run(['./fft_pc', '--headless', '--scenario', '70', '--frames', str(frame),
                        '--scale', '3', '--shot', str(shot)], check=True, stdout=subprocess.DEVNULL)
        animation.append(Image.open(shot).copy())
    animation[0].save(OUT/'chest-pc.gif', save_all=True, append_images=animation[1:],
                      duration=[60, 60, 60, 70] * (len(animation)//4), loop=0, disposal=2)
    (OUT/'art-check.json').write_text(json.dumps(dict(
        source='Local disc BATTLE/OTHER.SPR, palette 0', source_box=units.CHEST,
        opaque_target_box=[14, 14], stored_frame_bytes=size, asset_bank_bytes=len(bank),
        previous_non_chest_slots_unchanged=unchanged, runtime_sprite_blitter_changed=False,
        chest_float_pixels=2, chest_float_period_ticks=64, additional_float_frames_bytes=2*size,
        preview='PC framebuffer at intended 32 ticks/s, not a TI FPS measurement'), indent=2)+'\n')
    print(OUT/'comparison.png'); print(OUT/'scene.png'); print(OUT/'chest-pc.gif')


if __name__ == '__main__':
    main()
