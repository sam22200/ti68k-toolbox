#!/usr/bin/env python3
"""Apply the approved C/D/C/D records; verify the shared campaign continuation cue.

Creates new private evidence, then updates only active ffsk.bin/.89y. Existing
review files, packaged groups, sessions and player saves retain bytes and mtimes.
"""
import hashlib
import json
import os
import shutil
import struct

from PIL import Image, ImageChops

import review_town_services as review
import town_service_assets as assets
import shop_scene
import extract

HERE, ROOT = review.HERE, review.ROOT


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    out = HERE / 'x/town-services-accepted'
    out.mkdir(parents=True, exist_ok=True)
    previous = HERE / 'x/town-services-review'
    contrasts = HERE / 'x/town-portrait-contrast'
    protected = set((HERE / 'data').glob('*.bin')) | set((HERE / 'data').glob('*.89y')) | set((HERE / 'data').glob('*.sav'))
    protected |= {HERE / 'x/check/ffsave_won.sav'}
    for directory in (previous, contrasts):
        protected |= {p for p in directory.iterdir() if p.suffix in ('.png', '.gif', '.json', '.bin', '.txt')}
    protected -= {HERE / 'data/ffsk.bin', HERE / 'data/ffsk.89y'}
    before = {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    for name in ('ffsk.bin', 'ffsk.89y'):
        saved = out / ('before-' + name)
        if not saved.exists(): shutil.copy2(HERE / 'data' / name, saved)
    old = (out / 'before-ffsk.bin').read_bytes()
    cue = ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    disc, files = extract.disc_files(str(cue))
    faces = disc.read(*files['EVENT/WLDFACE.BIN'])
    approved = (contrasts / 'portraits.bin').read_bytes()
    # Pin every reviewed record, including controls and unused alternatives, through factoring.
    for variant in range(6):
        for speaker, index in enumerate(assets.SPEAKERS.values()):
            at = (variant * 4 + speaker) * 348
            assert assets.portrait(faces, index, variant) == approved[at:at + 348]
    selected = assets.corrected_rooms(old, faces, assets.SELECTED)
    rebuilt, info = shop_scene.build(str(cue))
    assert rebuilt == selected, 'production extractor differs from the selected exact records/rooms'
    room_at = struct.unpack_from('>H', old, 12)[0]
    assert selected[room_at:] == old[room_at:] and len(selected) == len(old)
    (out / 'ffsk.bin').write_bytes(selected)
    bar = assets.portrait(faces, assets.SPEAKERS['bar'], assets.SELECTED['bar'])
    (out / 'bar-face.bin').write_bytes(bar)
    for name in ('bar-planes.bin', 'rumor.txt'):
        (out / name).write_bytes((previous / name).read_bytes())
    data = out / 'data'; data.mkdir(exist_ok=True)
    for bank in (HERE / 'data').glob('*.bin'):
        p = data / bank.name
        if p.exists() or p.is_symlink(): p.unlink()
        if bank.name == 'ffsk.bin': p.write_bytes(old)
        else: p.symlink_to(bank)
    (data / 'ffsave.sav').write_bytes((HERE / 'x/check/ffsave_won.sav').read_bytes())
    common = [HERE / 'preview_town_services.c', ROOT / 'runtime/core/rt_core.c', ROOT / 'runtime/platform-sw/rt_sw.c']
    hashes = None
    for sanitize in (False, True):
        exe = out / ('preview-san' if sanitize else 'preview')
        flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie'] if sanitize else []
        review.run(['cc', '-std=gnu99', '-O1', '-g', *flags, '-o', exe, *common])
        log = review.run([exe, out, HERE / 'x/check/ffsave_won.sav', '--selected'], data, os.environ.copy())
        current = {p.name: sha(p.read_bytes()) for p in out.glob('*.png') if p.name.startswith(
            ('store-', 'office-', 'fur-', 'bar-home', 'bar-hand-', 'a-', 'b-', 'continue-', 'final-'))}
        if sanitize: assert hashes == current
        else: hashes = current
        (out / ('sanitizers.log' if sanitize else 'pc.log')).write_text(log)
    for speaker, (name, variant) in enumerate(zip(('store-after', 'office-after', 'fur-after', 'bar-home'), (3, 4, 3, 4))):
        assert (out / (name + '.png')).read_bytes() == (contrasts / ('portrait-%u-%u.png' % (variant, speaker))).read_bytes()
    first_off = Image.open(out / 'continue-0.png').convert('RGB')
    first_on = Image.open(out / 'continue-4.png').convert('RGB')
    assert ImageChops.difference(first_off, first_on).getbbox() == (152, 87, 155, 89)
    assert (out / 'continue-0.png').read_bytes() == (out / 'continue-8.png').read_bytes()
    final_off = Image.open(out / 'final-0.png').convert('RGB')
    final_on = Image.open(out / 'final-4.png').convert('RGB')
    assert ImageChops.difference(final_off, final_on).getbbox() == (152, 87, 155, 89)
    assert (out / 'final-0.png').read_bytes() == (out / 'final-8.png').read_bytes()
    review.sheet(out, 'selected-portraits.png', 2, [
        [('store-after', 'Shopkeeper - C'), ('office-after', 'Guildmaster - D')],
        [('fur-after', 'Fur Merchant - C'), ('bar-home', 'Bar owner - D')]])
    review.sheet(out, 'bar-continuation.png', 2, [
        [('a-read', 'Wide reader - no numeric counter'), ('continue-4', 'Dialogue - shared campaign cue')],
        [('continue-0', 'First page - blink off'), ('final-4', 'Final page - same campaign prompt')]])
    frames = [Image.open(out / ('continue-%u.png' % i)).resize((640, 400), Image.Resampling.NEAREST) for i in range(8)]
    frames[0].save(out / 'bar-continuation.gif', save_all=True, append_images=frames[1:], duration=133, loop=0)
    review.run([ROOT / 'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', 'ffsk.bin', 'ffsk'], out)
    transport = bytearray((out / 'ffsk.89y').read_bytes())
    transport[0x49] = 3  # Same archived-variable attribute as campaign/Makefile's data rule.
    (out / 'ffsk.89y').write_bytes(transport)
    assert before == {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    # The explicit user-approved source bank is now active; installed groups are untouched.
    for name in ('ffsk.bin', 'ffsk.89y'):
        target = HERE / 'data' / name
        raw = (out / name).read_bytes()
        if target.read_bytes() != raw: target.write_bytes(raw)
    result = dict(portrait_approval='accepted', selections=dict(zip(assets.SPEAKERS, ('C','D','C','D'))),
                  source_speakers=assets.SPEAKERS, ffsk_bytes=len(selected), rooms_exact=True,
                  reviewed_records_exact=24, selected_screens_exact=4, pc_sanitizers_equal=True,
                  pc_screens=len(hashes), protected_files=len(before), indicator_bounds=[152,87,155,89],
                  final_page_indicator=True, package_groups_changed=False, native_run=False,
                  ffsk_archived=True,
                  source_findings='Original Bar initializes graphical page/scroll windows; no numeric counter.',
                  ffsk_sha256=sha(selected), bar_face_sha256=sha(bar),
                  source_sha256=sha(faces), screen_sha256=hashes,
                  pictures={name: review.fingerprint(out / name) for name in (
                      'selected-portraits.png', 'bar-continuation.png', 'bar-continuation.gif')})
    original = ROOT / 'sources/fft_decomp/src/wldcore'
    result['original_source_sha256'] = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in (
        original / 'wldcore_text_init_scrollable_window.c',
        original / 'wldcore_text_render_scrollable_window_page.c',
        original / 'wldcore_text_update_scroll_indicators.c',
        original / 'wldcore_text_is_window_finished.c')}
    (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    (out / 'preserved.json').write_text(json.dumps(before, indent=2) + '\n')
    print('C/D/C/D exact to approval;24 records and4 selected screens retained; active FSK4 %d bytes.' % len(selected))
    print('PC/sanitizer screens agree; shared3x2 blink on first/final page and32-frame return pass.')
    print('%d existing bank/save/review files preserved; installed groups untouched.' % len(before))


if __name__ == '__main__':
    main()
