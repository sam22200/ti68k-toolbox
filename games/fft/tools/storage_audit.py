#!/usr/bin/env python3
"""Read-only FFT storage/RAM audit; generated data and probes stay under x/storage.

Compares the pre-cleanup snapshot with the current native camera manifest. Historical
compression candidates remain reproducible; no game assets are staged or changed.
Compression figures verify host round trips, not native decode latency or heap fit.
"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE / 'x/storage'
DATA = HERE / 'x/turn-comparison/textured80x25/data'
BEFORE = OUT / 'before-cleanup'
sys.path.insert(0, str(ROOT / 'lib'))
import zx0pack


def define(header, name):
    return int(re.search(r'^#define ' + name + r' (\d+)', header, re.M)[1])


def chunks(data, size):
    assert len(data) % size == 0
    return [data[i:i+size] for i in range(0, len(data), size)]


def dedup(data, size):
    parts = chunks(data, size)
    unique = list(dict.fromkeys(parts))
    ids = {v: i for i, v in enumerate(unique)}
    indices = [ids[p] for p in parts]
    assert b''.join(unique[i] for i in indices) == data
    # Exact representation candidate: u16 entry IDs, no decoder changes assumed.
    return dict(input_bytes=len(data), entries=len(parts), unique=len(unique),
                dictionary_bytes=len(unique)*size, index_bytes=len(parts)*2,
                candidate_bytes=len(unique)*size+len(parts)*2)


def ram_probe(compact):
    fields = ['cover', 'wcover', 'feet', 'feet_k', 'span_l', 'span_r', 'from', 'half',
              'dbl', 'order', 'count', 'glint', 'glints', 'key', 'gx', 'gy', 'hud',
              'win', 'forecast', 'status', 'cur_m', 'num', 'morph_g', 'dcg', 'sq',
              'blk', 'tx', 'tz', 'hs', 'hl', 'hw', 'eg', 'en', 'top', 'col', 'row',
              'ai_m', 'ai_prox', 'ai_dist', 'ai_gain', 'ai_key', 'ai_grid', 'ai_ord',
              'ai_rank', 'bg', 'pan', 'tag']
    expressions = {'work': 'sizeof(Work)', 'state_in_program': 'sizeof(State)',
                   'view': 'VIEW_BYTES', 'scene': '2UL*SC_PLANE'}
    expressions.update({f: 'sizeof(((Work *)0)->' + f + ')' for f in fields})
    if not compact:
        expressions.update(blade_cache='sizeof(((Work *)0)->blade)',
                           blade_cache_key='sizeof(((Work *)0)->blade_k)')
    flags = []
    if compact:
        flags = ['-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_CINEMA', '-DFFT_CONFIG',
                 '-DFFT_COMPACT_UI', '-DFFT_TURN_LOWPOLY_NATIVE']
        expressions.update(idle_struct='sizeof(PIdle)',
                           idle_allocations='sizeof(PIdle)+13600UL+3400+1700',
                           fixed_work_alias='sizeof(FWork)')
    name = 'compact' if compact else 'full_iso'
    source = OUT / (name + '.c')
    source.write_text('#include "../../fft.c"\n' + ''.join('#undef '+f+'\n' for f in fields)
                      + 'const unsigned long audit_sizes[] = {\n'
                      + ',\n'.join(expressions.values()) + '\n};\n')
    assembly = OUT / (name + '.s')
    subprocess.run([str(ROOT/'tools/bin/ti-cc'), '-S', '-DRT_CYCLES', *flags,
                    str(source), '-o', str(assembly)], cwd=HERE, check=True,
                   stdout=subprocess.PIPE)
    output = assembly.read_text().split('audit_sizes:', 1)[1].split('\t.section', 1)[0]
    values = [int(n) for n in re.findall(r'^\s*\.long\s+(\d+)\s*$', output, re.M)]
    assert len(values) == len(expressions), (name, values)
    result = dict(zip(expressions, values))
    result['allocated_game_buffers'] = result['view'] + result['scene'] + result['work']
    result['hud_caches'] = sum(result[f] for f in ['hud', 'win', 'forecast', 'status', 'pan', 'tag'])
    result['movement_topology'] = sum(result[f] for f in ['from', 'sq', 'blk', 'tx', 'tz', 'hs',
                                                         'hl', 'hw', 'eg', 'en', 'top', 'col', 'row'])
    result['ai_arrays'] = sum(result[f] for f in ['ai_m', 'ai_prox', 'ai_dist', 'ai_gain',
                                               'ai_key', 'ai_grid', 'ai_ord', 'ai_rank'])
    return result


def decode_bench(jobs):
    binary = OUT/'auddec'
    subprocess.run([str(ROOT/'tools/bin/ti-cc'), '-o', str(binary), '-DRT_CYCLES',
                    'tools/storage_decode_bench.c', '../../runtime/core/rt_core.c',
                    '../../runtime/platform-ti68k/rt_ti.c', '../../lib/unpack68k.s',
                    '../../tools/extgraph/lib/tilemap.a'], cwd=HERE, check=True,
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    results = {}
    for name, raw in jobs.items():
        if len(raw) > 65535:
            continue
        packed = (OUT/(name+'.zx0')).read_bytes()
        (OUT/'audpack.bin').write_bytes(struct.pack('>H',len(raw))+packed)
        subprocess.run([str(ROOT/'tools/gcc4ti-bin/bin/ttbin2oth'), '-89', 'dat',
                        'audpack.bin', 'audpack'], cwd=OUT, check=True, stdout=subprocess.PIPE)
        log = subprocess.check_output([str(ROOT/'tools/bin/ti-cycles'), '--max', '10000',
                    '--file', str(OUT/'audpack.89y'), str(binary)+'.89z'], cwd=HERE, text=True)
        (OUT/(name+'-decode.log')).write_text(log)
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        a = b = 0
        for v in raw:
            a = (a+v)%255; b = (b+a)%255
        assert values[:2] == [len(raw), (b<<8)|a] and '_main returned' in log, (name, values)
        # Zone 4 reports one invocation; keep its full measured line for reproducibility.
        zone = next(line for line in log.splitlines() if 'Audit ZX0 decode' in line)
        cycles = int(zone.split()[-1])
        results[name] = dict(cycles=cycles, cpu_ms_12mhz=cycles/12000,
                             raw_buffer_bytes=len(raw), native_checksum=(b<<8)|a)
    return results


def main():
    OUT.mkdir(exist_ok=True, parents=True)
    # Inventory only one native variant: historical exploration directories are not additive.
    banks = {n: (BEFORE / (n+'.bin')).read_bytes() for n in ['fftu', 'fftw', 'ffti']}
    banks['fftmesh']=(HERE/'fftmesh.bin').read_bytes()
    for n in ['fftat0', 'fftat1', 'fftat2', 'fftuv', 'fftsurf', 'fftlook', 'fftshot',
              'fftv0', 'fftv1', 'fftv2', 'fftv3']:
        banks[n] = (DATA / (n+'.bin')).read_bytes()
    units = (BEFORE/'units.h').read_text()
    fb, frames = define(units, 'UNIT_SH')*8, define(units, 'UNIT_FRAMES')
    unit_bytes = 6*2*frames*fb
    weapon_off = define((BEFORE/'weapons.h').read_text(), 'WEAPON_OFF')
    ui = (BEFORE/'interface.h').read_text()
    sections = [(n, int(o)) for n, o in re.findall(r'^#define UI_(\w+) (\d+)UL', ui, re.M)]
    interface = {n: banks['ffti'][start:end] for (n, start), end in
                 zip(sections, [o for _, o in sections[1:]]+[len(banks['ffti'])])}
    surface = banks['fftsurf']; count = struct.unpack_from('>H', surface, 8)[0]
    record_end = 144 + count*22
    atlas = b''.join(banks[n] for n in ['fftat0', 'fftat1', 'fftat2'])
    directory = [struct.unpack_from('>2H', surface, 16+4*i) for i in range(32)]
    lists = [surface[off:off+2*n] for off, n in directory]
    unique_lists = list(dict.fromkeys(lists))
    aliased = bytearray(surface[:record_end]); offsets = {}
    for slot, sequence in enumerate(lists):
        if sequence not in offsets:
            offsets[sequence] = len(aliased); aliased.extend(sequence)
        struct.pack_into('>2H', aliased, 16+4*slot, offsets[sequence], len(sequence)//2)
    for slot, expected in enumerate(lists):
        start, count_list = struct.unpack_from('>2H', aliased, 16+4*slot)
        assert aliased[start:start+count_list*2] == expected
    (OUT/'fftsurf-aliased.bin').write_bytes(aliased)
    categories = {
        'rotation_texture_atlases': len(atlas), 'ordinary_character_frames': unit_bytes,
        'iso_views_zx0_contrast_edges': sum(len(banks['fftv'+str(i)]) for i in range(4)),
        'preshifted_font': len(interface['FONT']), 'weapon_blades': len(banks['fftw'])-weapon_off,
        'armed_character_frames': weapon_off, 'rotation_visibility_lists': len(surface)-record_end,
        'shared_mesh_and_tables': len(banks['fftmesh']), 'inspection_fixture_backgrounds': len(banks['fftlook']),
        'merged_surface_records_and_directory': record_end, 'rotation_uvs': len(banks['fftuv']),
        'legacy_disabled_attack_backgrounds': len(banks['fftshot']),
        'loot_frames': len(banks['fftu'])-unit_bytes,
    }
    for n, data in interface.items():
        if n != 'FONT': categories['interface_'+n.lower()] = len(data)
    total = sum(map(len, banks.values()))
    assert sum(categories.values()) == total
    # Atlas tiles retain every texel (including transparency), even unused map regions.
    tiles = []
    for page in chunks(atlas, 8192):
        for ty in range(16):
            for tx in range(16):
                tiles.append(b''.join(page[(ty*8+y)*64+tx*4:(ty*8+y)*64+tx*4+4] for y in range(8)))
    tile_audit = dedup(b''.join(tiles), 32)
    tile_audit['with_3bit_tile_dictionary_bytes'] = tile_audit['unique']*24+tile_audit['index_bytes']
    tile_audit['with_3bit_saved_bytes'] = len(atlas)-tile_audit['with_3bit_tile_dictionary_bytes']
    # Exact same atlas, alternative representation: 2-bit shade plus 1-bit transparency.
    packed_pages = []
    for page in chunks(atlas, 8192):
        pixels = [v for byte in page for v in (byte >> 4, byte & 15)]
        assert max(pixels) <= 4
        colours = bytes(sum((pixels[i+j]&3) << (6-2*j) for j in range(4)) for i in range(0,len(pixels),4))
        alpha = bytes(sum((pixels[i+j]==4) << (7-j) for j in range(8)) for i in range(0,len(pixels),8))
        restored = [4 if alpha[i//8] & (128 >> (i&7)) else (colours[i//4] >> (6-2*(i&3)))&3 for i in range(len(pixels))]
        assert restored == pixels
        packed_pages.append(colours+alpha)
    packed_atlas = b''.join(packed_pages)
    # Existing mirror halves include masks and inside/cover planes, not just colour.
    mirror_bytes = unit_bytes//2 + weapon_off//2 + len(interface['PORTRAITS'])//2
    mirrors = dict(ordinary=unit_bytes//2, armed=weapon_off//2,
                   portraits=len(interface['PORTRAITS'])//2,
                   total_flash_saved_if_generated=mirror_bytes,
                   full_mirror_ram_bytes=mirror_bytes,
                   one_class_mirror_ram_bytes=frames*fb + 8*fb + len(interface['PORTRAITS'])//12)
    duplicate_frames = dedup(banks['fftu']+banks['fftw'][:weapon_off], fb)
    blade_dedup = dedup(banks['fftw'][weapon_off:], 432)
    # Preshifted text contains no new artwork: recover all 16 shifts from 475 bytes.
    font = interface['FONT']; canonical = bytearray(); restored_font = bytearray()
    for glyph in chunks(font, 320):
        rows = [v >> 24 for v in struct.unpack_from('>5I', glyph)]
        canonical.extend(rows)
        for shift in range(16):
            restored_font.extend(struct.pack('>5I', *(v << (24-shift) for v in rows)))
    assert restored_font == font
    # Alias identical facing lists through the existing offset/count directories.
    list_candidates = dict(current_bytes=sum(map(len,lists)), unique_lists=len(unique_lists),
                           unique_bytes=sum(map(len,unique_lists)),
                           saved_bytes=sum(map(len,lists))-sum(map(len,unique_lists)),
                           half_y_only_list_bytes=sum(map(len, lists[16:])),
                           half_y_unique_list_bytes=sum(map(len,dict.fromkeys(lists[16:]))))
    # Samples of useful compression granularity; no giant all-assets RAM expansion.
    jobs = {'units_whole': banks['fftu'], 'weapons_whole': banks['fftw'],
            'interface_whole': banks['ffti'], 'font': interface['FONT'],
            'mesh': banks['fftmesh'], 'surface': surface, 'look': banks['fftlook'],
            'atlas_page0': atlas[:8192], 'atlas_3bit': packed_atlas,
            'unit_class0': banks['fftu'][:2*frames*fb],
            'unit_frame0': banks['fftu'][:fb]}
    def compress(item):
        name, raw = item; cached = OUT/(name+'.zx0'); packed = None
        if cached.exists():
            with tempfile.TemporaryDirectory() as tmp:
                back = Path(tmp)/'back'
                subprocess.run([zx0pack.DZX0, '-f', str(cached), str(back)], check=True,
                               stdout=subprocess.PIPE)
                if back.read_bytes() == raw:
                    packed = cached.read_bytes()
        if packed is None:
            packed = zx0pack.pack(raw); cached.write_bytes(packed)
        return name, dict(raw_bytes=len(raw), zx0_bytes=len(packed), saved_bytes=len(raw)-len(packed))
    with ThreadPoolExecutor(max_workers=3) as workers:
        compressed = dict(workers.map(compress, jobs.items()))
    decoded = decode_bench(jobs)
    ram = {n: ram_probe(n=='compact') for n in ['full_iso', 'compact']}
    executable = {}
    for p in [HERE/'fft.89z', HERE/'x/config/fftcfg.89z']:
        raw = p.read_bytes(); offset = struct.unpack_from('<I', raw, 0x3c)[0]
        executable[str(p.relative_to(HERE))] = dict(container_bytes=len(raw),
            payload_bytes=struct.unpack_from('>H', raw, offset+4)[0], sha256=hashlib.sha256(raw).hexdigest(),
            freshness='Existing binaries from the cleanup validation; this audit only reads them.')
    current=json.loads((HERE/'x/config/checked.json').read_text())
    report = dict(scope='Pre-cleanup Gariland candidates versus current cleaned native manifest. Raw payload bytes; excludes variable/archive overhead.',
        banks={n:dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest()) for n,b in banks.items()},
        total_data_bytes=total, categories=dict(sorted(categories.items(), key=lambda p:-p[1])),
        top10_banks=dict(sorted({
            'rotation_texture_atlases':len(atlas), 'unit_sprites':len(banks['fftu']),
            'interface':len(banks['ffti']), 'iso_views':categories['iso_views_zx0_contrast_edges'],
            'armed_bodies_and_blades':len(banks['fftw']), 'surface_and_visibility':len(surface),
            'shared_mesh':len(banks['fftmesh']), 'legacy_attack_fixtures':len(banks['fftshot']),
            'inspection_fixtures':len(banks['fftlook']), 'uv_coordinates':len(banks['fftuv'])
        }.items(), key=lambda p:-p[1])),
        atlas_8x8_dedup=tile_audit, atlas_3bit_bytes=len(packed_atlas), sprite_mirrors=mirrors,
        sprite_exact_dedup=duplicate_frames, blade_exact_dedup=blade_dedup,
        surface_lists=list_candidates,
        canonical_font=dict(current_bytes=len(font), unshifted_art_bytes=len(canonical),
                            saved_bytes=len(font)-len(canonical), exact_regeneration=True),
        compression=compressed, native_decode=decoded, ram_native_abi=ram, executables=executable,
        current_native_banks=current['native_banks'], current_data_bytes=current['data_bytes'],
        data_bytes_saved=total-current['data_bytes'],
        limits='Native decode counts exclude allocation, checksum, grayscale and presentation. No AMS peak/free RAM or TiEmu tests. Historical candidates are distinguished from current manifest cleanup. Savings overlap; do not add all of them.')
    (OUT/'audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
