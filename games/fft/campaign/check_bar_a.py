#!/usr/bin/env python3
"""Verify selected Bar reader A on real long prose; preserve prior review/save evidence."""
import hashlib
import json
import os
import re

from PIL import Image, ImageChops

import review_town_services as review
import extract

HERE, ROOT = review.HERE, review.ROOT


def main():
    out = HERE / 'x/bar-reader-a'; out.mkdir(parents=True, exist_ok=True)
    accepted = HERE / 'x/town-services-accepted'
    protected = set((HERE / 'data').glob('*.bin')) | set((HERE / 'data').glob('*.89y')) | set((HERE / 'data').glob('*.sav'))
    protected |= {HERE / 'x/check/ffsave_won.sav'}
    for folder in ('town-services-review', 'town-portrait-contrast', 'town-services-accepted'):
        protected |= {p for p in (HERE / 'x' / folder).iterdir()
                      if p.suffix in ('.bin', '.89y', '.png', '.gif', '.json', '.txt')}
    before = {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    for name in ('ffsk.bin', 'bar-face.bin', 'bar-planes.bin', 'rumor.txt'):
        (out / name).write_bytes((accepted / name).read_bytes())
    cue = ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    disc, files = extract.disc_files(str(cue)); raw = disc.read(*files['WORLD/WLDMES.BIN'])
    at = 0
    for _ in range(2): at = raw.index(254, at) + 1
    long_text = review.article(raw[at:])
    (out / 'long-rumor.txt').write_text(long_text)
    common = [HERE / 'preview_town_services.c', ROOT / 'runtime/core/rt_core.c', ROOT / 'runtime/platform-sw/rt_sw.c']
    normal = None
    for sanitize in (False, True):
        exe = out / ('preview-san' if sanitize else 'preview')
        flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie'] if sanitize else []
        review.run(['cc', '-std=gnu99', '-O1', '-g', *flags, '-o', exe, *common])
        log = review.run([exe, out, HERE / 'x/check/ffsave_won.sav', '--bar-a'], accepted / 'data', os.environ.copy())
        assert 'Save unchanged' in log
        match = re.search(r'Bar A: (\d+) pages, (\d+) wrapped lines, (\d+) source bytes preserved', log)
        assert match and int(match[1]) > 1 and int(match[3]) == len(long_text)
        pictures = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.png')}
        if sanitize: assert pictures == normal
        else: normal = pictures
        (out / ('sanitizers.log' if sanitize else 'pc.log')).write_text(log)
    assert (out / 'selected-short.png').read_bytes() == (accepted / 'a-read.png').read_bytes()
    for prefix in ('a-first', 'a-last'):
        a = Image.open(out / (prefix + '-0.png')).convert('RGB')
        b = Image.open(out / (prefix + '-4.png')).convert('RGB')
        assert ImageChops.difference(a, b).getbbox() == (151, 91, 154, 93)
        assert a.getpixel((151, 91)) != b.getpixel((151, 91))
        assert (out / (prefix + '-0.png')).read_bytes() == (out / (prefix + '-8.png')).read_bytes()
    pages = int(match[1])
    review.sheet(out, 'bar-a-selected.png', 2, [
        [('selected-home', 'Selected Bar / portrait D'), ('selected-short', 'Selected reader A - grey background')],
        [('long-page-0', 'Real long text - first page'), ('long-page-%u' % (pages - 1), 'Real long text - final page')]])
    frames = [Image.open(out / ('a-first-%u.png' % i)).resize((640, 400), Image.Resampling.NEAREST) for i in range(8)]
    frames[0].save(out / 'bar-a-indicator.gif', save_all=True, append_images=frames[1:], duration=133, loop=0)
    assert before == {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    result = dict(layout='A', background='shared command light-grey', approval='accepted',
                  pages=pages, wrapped_lines=int(match[2]), source_bytes=len(long_text),
                  source_sha256=hashlib.sha256(raw).hexdigest(), source_entry=2, source_offset=at,
                  short_frame_exact=True, pc_sanitizers_equal=True, pc_screens=len(normal),
                  indicator_bounds=[151,91,154,93], protected_files=len(before),
                  save_unchanged=True, native_run=False, shipping_changed=False,
                  scope='Selected visual/pagination proof only; actual Bar/rumor service remains to implement.',
                  screen_sha256=normal,
                  picture=review.fingerprint(out / 'bar-a-selected.png'))
    (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    (out / 'preserved.json').write_text(json.dumps(before, indent=2) + '\n')
    print(log.strip())
    print('%d PC/sanitizer screens agree; %d existing inputs preserved.' % (len(normal), len(before)))


if __name__ == '__main__':
    main()
