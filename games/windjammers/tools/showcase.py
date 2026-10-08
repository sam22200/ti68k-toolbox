#!/usr/bin/env python3
"""Capture the native engine; compose a named, labelled GIF outside the LCD."""
import csv
import json
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw, ImageFont

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / 'x/showcase'
JOBS = [
    (4, 'keys/idle.txt', 100, 'ÉCHANGES', 'Déplacements, réceptions et relances'),
    (27, 'keys/dash.txt', 60, 'DASH · H. MITA', 'Sans disque : direction + A / AltGr'),
    (28, 'keys/dash_yoo.txt', 60, 'DASH · B. YOO', 'Les diagonales fonctionnent aussi'),
    (13, 'keys/timing_return.txt', 85, 'RETOUR PUISSANT', 'A à la réception : relance immédiate'),
    (15, 'keys/lob.txt', 100, 'LOB', 'B / Maj · Le viseur fixe indique le point de chute'),
    (17, 'keys/special.txt', 135, 'SPÉCIALE · H. MITA', ''),
    (18, 'keys/special.txt', 135, 'SPÉCIALE · B. YOO', ''),
    (21, 'keys/superlob.txt', 160, 'SUPER LOB · REBOND', ''),
    (23, 'keys/curve23.txt', 100, 'LANCER COURBE', 'Haut → haut-droite → droite + A'),
    (24, 'keys/curve24.txt', 100, 'COURBE INVERSE', 'Bas → bas-droite → droite + A'),
    (2, 'keys/idle.txt', 90, 'REBONDS SUR LES BORDS', 'La trajectoire continue après le rebond'),
    (6, 'keys/idle.txt', 60, 'POINTS & SERVICE', 'Zones 3 / 5 / 3 · Le joueur qui encaisse reprend le service'),
]

def caption(scenario, row, fallback):
    if scenario not in (17, 18, 21): return fallback
    if row['charged']:
        return 'Étoile pleine : récupérer le disque, puis B pour le super lob' if scenario == 21 else 'Étoile pleine : récupérer le disque, puis A pour la spéciale'
    if row['charging']: return 'Charge automatique · Attendre la barre pleine'
    if row['mode'] == 14: return 'Réception précise : le frisbee est levé'
    if row['mode'] in (42, 44): return 'A après la charge · Spéciale en vol'
    if row['mode'] == 38: return 'B après la charge · Super lob'
    if row['mode'] == 40: return 'Lob manqué : rebond et accélération au sol'
    if row['mode'] == 18: return 'Point marqué · Service rendu au joueur qui a encaissé'
    return 'Sans disque : relâcher les flèches, taper A juste avant la réception'

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-DRT_FRAME_TICKS2=17', '-o', 'x/capture_showcase',
        'tools/capture_showcase.c', 'windjam.c', 'render.c', '../../runtime/core/rt_core.c',
        '../../runtime/platform-sw/rt_sw.c'], cwd=GAME, check=True)
    fonts = Path('/usr/share/fonts/truetype/dejavu')
    title = ImageFont.truetype(str(fonts / 'DejaVuSans-Bold.ttf'), 30)
    label = ImageFont.truetype(str(fonts / 'DejaVuSans-Bold.ttf'), 23)
    body = ImageFont.truetype(str(fonts / 'DejaVuSans.ttf'), 16)
    shades = [232, 158, 86, 20]
    colors = [(value, value, value) for value in shades]
    colors += [(20, 23, 28), (244, 245, 248), (250, 210, 75)]
    for end in ((244, 245, 248), (250, 210, 75)):
        colors += [tuple((start * (16 - n) + finish * n) >> 4 for start, finish in zip((20, 23, 28), end))
                   for n in range(1, 16)]
    palette = Image.new('P', (1, 1))
    palette.putpalette([component for color in colors for component in color] + [0] * (768 - len(colors) * 3))
    frames, durations, reports = [], [], []
    ticks = elapsed_ms = 0
    def append(lcd, heading, text, duration):
        canvas = Image.new('RGB', (640, 560), (20, 23, 28))
        draw = ImageDraw.Draw(canvas)
        draw.text((22, 14), 'WINDJAMMERS TI-89', font=title, fill=(244, 245, 248))
        canvas.paste(lcd, (0, 64))
        draw.text((22, 476), heading, font=label, fill=(250, 210, 75))
        # Wrap captions outside the unchanged full-screen court.
        lines, current = [], ''
        for word in text.split():
            candidate = (current + ' ' + word).strip()
            if current and draw.textlength(candidate, font=body) > 596:
                lines.append(current); current = word
            else: current = candidate
        lines.append(current)
        assert len(lines) <= 2, text
        for line, content in enumerate(lines):
            draw.text((22, 510 + line * 22), content, font=body, fill=(244, 245, 248))
        frames.append(canvas.quantize(palette=palette, dither=Image.Dither.NONE)); durations.append(duration)
    for scenario, keys, count, heading, fallback in JOBS:
        raw = OUT / f'{scenario}.frames'
        state = OUT / f'{scenario}.csv'
        subprocess.run(['./x/capture_showcase', str(scenario), keys, str(count), str(raw), str(state)],
                       cwd=GAME, check=True)
        rows = [{k: int(v) for k, v in row.items()} for row in csv.DictReader(state.open())]
        modes = sorted({row['mode'] for row in rows})
        if scenario in (27, 28): assert any(row['dash'] for row in rows)
        if scenario in (17, 18, 21):
            assert any(row['charging'] for row in rows) and any(row['charged'] for row in rows)
            assert (38 if scenario == 21 else 44 if scenario == 17 else 42) in modes
        if scenario == 21: assert 40 in modes and 18 in modes
        if scenario in (23, 24): assert 6 in modes or 8 in modes
        if scenario == 13: assert any(row['throw_catch'] for row in rows)
        data = raw.read_bytes(); assert len(data) == count * 16000
        for frame, row in enumerate(rows):
            lcd = Image.frombytes('L', (160, 100), data[frame * 16000:(frame + 1) * 16000])
            lcd = lcd.point(shades + [0] * 252).convert('RGB').resize((640, 400), Image.Resampling.NEAREST)
            ticks += 8 if not frame & 1 else 9
            end_ms = round(ticks * 100 / 256) * 10
            append(lcd, heading, caption(scenario, row, fallback), end_ms - elapsed_ms)
            elapsed_ms = end_ms
        # A short readable pause at each clip's first frame, preserving game cadence.
        durations[-count] += 400
        reports.append(dict(scenario=scenario, frames=count, modes=modes, checksum=rows[-1]['checksum']))
        print(f'Showcase {scenario}: {count} native frames, modes {modes}', flush=True)
    gif = GAME / 'x/Windjammers-TI89-showcase.gif'
    frames[0].save(gif, save_all=True, append_images=frames[1:], duration=durations,
                   loop=0, optimize=False, disposal=1)
    (OUT / 'manifest.json').write_text(json.dumps(dict(name='Windjammers TI-89', clips=reports,
        native_frames=len(frames), duration_ms=sum(durations), gif=str(gif)), indent=2) + '\n')
    # Contact sheet for an art review of the actual animation's key moments.
    review = Image.new('RGB', (640 * 3, 560 * 2))
    indices = [125, 405, 545, 595, 690, 790]
    for i, index in enumerate(indices): review.paste(frames[index], ((i % 3) * 640, (i // 3) * 560))
    review.save(OUT / 'review.png')
    print(f'{gif}: {sum(durations)/1000:.2f}s, {gif.stat().st_size} bytes', flush=True)
if __name__ == '__main__': main()
