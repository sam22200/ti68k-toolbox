#!/usr/bin/env python3
"""Package growth report; --native also measures real68000 layouts/ZX0 streams.

Every compiler invocation is serialized. This never drives TiEmu or writes saves.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

import core_audit

HERE, ROOT = core_audit.HERE, core_audit.ROOT
FIELDS = ('unit_capacity unit_bytes state_bytes work_bytes cover_bytes feet_bytes '
          'save_unit_bytes save_bytes map_capacity scene_rows depth_buffer_bytes '
          'scene_and_work_block_bytes actor_cache_bytes state_bytes_per_extra_unit '
          'work_bytes_per_extra_unit management_cache_bytes battle_state_allocation_bytes '
          'nav_bytes_per_node nav_max_bytes nav_single_bytes_per_node '
          'nav_envelope_bytes module_zone_bytes companion_bytes hidden_render_bytes').split()


def run(args, cwd=ROOT):
    proc = subprocess.run(list(map(str,args)), cwd=cwd, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if proc.returncode:
        raise RuntimeError(' '.join(map(str,args))+'\n'+proc.stdout)
    return proc.stdout


def native_layout(out):
    source=HERE/'campaign/probe_core_layout.c'
    run(['cc','-std=gnu99','-O1','-w','-ffunction-sections','-fdata-sections',
         '-Wl,--gc-sections','-o',out/'layout-pc',source])
    pc=list(map(int,run([out/'layout-pc']).split()))
    build=run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'layout',source,
               ROOT/'runtime/core/rt_core.c',ROOT/'lib/unpack68k.s',
               ROOT/'tools/extgraph/lib/tilemap.a'])
    (out/'layout-build.log').write_text(build)
    log=run([ROOT/'tools/bin/ti-cycles','--frames',0,'--max',100,out/'layout.89z'])
    (out/'layout-native.log').write_text(log)
    native=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
    if len(pc)!=len(FIELDS) or len(native)!=len(FIELDS):
        raise ValueError('Missing layout values')
    # Host pointer/alignment differences are expected; serialized bytes agree.
    for key in ('save_unit_bytes','save_bytes','unit_capacity','map_capacity'):
        i=FIELDS.index(key)
        if pc[i]!=native[i]:raise ValueError(f'Unexpected serialized layout difference: {key}')
    layouts={label:dict(zip(FIELDS,values))for label,values in (('native',native),('host',pc))}
    for layout in layouts.values():
        # State and deferred-save tail share a longer-lived allocation. Neither
        # may be treated as free embedded storage or an already released Work cache.
        layout['battle_heap_block_bytes']=[layout[key]for key in
            ('battle_state_allocation_bytes','depth_buffer_bytes','scene_and_work_block_bytes','actor_cache_bytes')]
        # The production bootstrap reserves the map envelope once, independent of
        # the dense-map NAV_* helpers. Executable zone and companion are separate
        # live heap blocks; neither is embedded in the resident anymore.
        descriptor_nodes={}
        for name,path in (('orbonne',HERE/'opening/data/ffod.bin'),
                          ('gariland',HERE/'campaign/data/ffgd.bin')):
            words=struct.unpack('>52H',path.read_bytes()[:104])
            descriptor_nodes[name]=words[9]*words[10]
        base_blocks=layout['battle_heap_block_bytes']+[layout[key]for key in
            ('nav_envelope_bytes','module_zone_bytes','companion_bytes')]
        layout['battle_heap_by_map']={}
        for name,nodes in {**descriptor_nodes,'maximum':layout['map_capacity']}.items():
            blocks=list(base_blocks)
            layout['battle_heap_by_map'][name]=dict(nodes=nodes,blocks=blocks,requested_bytes=sum(blocks))
        layout['battle_heap_block_bytes']=layout['battle_heap_by_map']['gariland']['blocks']
        layout['battle_heap_requested_bytes']=sum(layout['battle_heap_block_bytes'])
        layout['maximum_battle_heap_requested_bytes']=layout['battle_heap_by_map']['maximum']['requested_bytes']
        layout['nav_growth_transient_bytes']=0
    return dict(**layouts,
                caveat='sizeof and allocation formulas only; not measured heap high-water, '
                       'stack, fragmentation, allocator overhead, full program residency or '
                       'combined supervisor/program/grayscale residency. Battle State plus deferred-save '
                       'tail, fixed navigation envelope, module zone and companion are included; '
                       'all seven game blocks must be simultaneously available. hidden_render_bytes '
                       'is a separate runtime request; GrayOn also allocates its visible planes.')


def unpack(packed):
    with tempfile.TemporaryDirectory(prefix='fft-audit-zx0-') as folder:
        a,b=Path(folder)/'in',Path(folder)/'out';a.write_bytes(packed)
        run([ROOT/'tools/bin/dzx0','-f',a,b]);return b.read_bytes()


def checksum(raw):
    a=b=0
    for value in raw:
        a=(a+value)%255;b=(b+a)%255
    return (b<<8)|a


def decode_views(out,repeats):
    build=run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'auddec',
               HERE/'tools/storage_decode_bench.c',ROOT/'runtime/core/rt_core.c',
               ROOT/'runtime/platform-ti68k/rt_ti.c',ROOT/'lib/unpack68k.s',
               ROOT/'tools/extgraph/lib/tilemap.a'])
    (out/'decode-build.log').write_text(build)
    results=[]
    for folder,prefix,rows in ((HERE/'opening/data','ffov',192),
                               (HERE/'campaign/data','ffgv',202)):
        for rot in range(4):
            path=folder/f'{prefix}{rot}.bin';bank=path.read_bytes()
            if bank[:2] not in (b'R2',b'R3',b'R4'):raise ValueError(f'{path}: expected ragged view')
            tail_bytes=0
            if bank[-10:-6]in (b'FC01',b'FC02'):
                original=int.from_bytes(bank[-6:-4],'big')
                tail_bytes=len(bank)-original;bank=bank[:original]
            n=int.from_bytes(bank[2:4],'big')
            mask=int.from_bytes(bank[4:6],'big')
            for label,packed,expected in (('colours',bank[8:8+n],2*40*rows),
                                           ('depth_masks',bank[8+n+mask:],int.from_bytes(bank[6:8],'big'))):
                copy=bank[:2]==b'R4' and label=='colours'
                raw=packed if copy else unpack(packed)
                if len(raw)!=expected:raise ValueError(f'{path}/{label}: decoded span mismatch')
                (out/'audpack.bin').write_bytes(struct.pack('>H',len(raw)|(32768 if copy else 0))+packed)
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','audpack.bin','audpack'],out)
                timings=[]
                for repeat in range(repeats):
                    log=run([ROOT/'tools/bin/ti-cycles','--max',10000,
                             '--file',out/'audpack.89y',out/'auddec.89z'])
                    (out/f'{prefix}{rot}-{label}-{repeat}.log').write_text(log)
                    vals=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
                    if vals[:2]!=[len(raw),checksum(raw)] or '_main returned' not in log:
                        raise ValueError(f'{path}: native decode differs')
                    zone=next(line for line in log.splitlines()if re.match(r'^4\s+Audit view transfer',line))
                    timings.append(int(zone.split()[-1]))
                results.append(dict(bank=path.stem,stream=label,raw_bytes=len(raw),
                                    stored_bytes=len(packed),compressed_bytes=0 if copy else len(packed),
                                    method='raw copy'if copy else 'ZX0',
                                    bank_contour_tail_bytes=tail_bytes,
                                    raw_sha256=core_audit.sha(raw),native_checksum=checksum(raw),
                                    cycles=timings,cpu_ms_12mhz=max(timings)/12000))
    return dict(streams=results,repeats=repeats,
                caveat='Archive copy/verified ZX0 decoder only, datasheet CPU; ragged depth rows remain XOR encoded. '
                       'Contour tail is excluded from compressed stream lengths. '
                       'Not complete loading/rotation/LCD latency; no secondary recompression applied.')


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--group',type=Path,default=HERE/'campaign/x/fftcampaign.89g')
    ap.add_argument('--out',type=Path,default=HERE/'campaign/x/core-audit/budget.json')
    ap.add_argument('--native',action='store_true')
    ap.add_argument('--repeats',type=int,default=3)
    ap.add_argument('--archive-warning',type=int,default=2*1024*1024,
                    help='provisional package planning alert, NOT available hardware memory')
    args=ap.parse_args()
    args.out=args.out.resolve();args.group=args.group.resolve()
    if not 1<=args.repeats<=10:ap.error('repeats must be1..10')
    old=json.loads(args.out.read_text()) if args.out.exists() else None
    package=core_audit.package(args.group)
    report=dict(schema='FFT_CORE_BUDGET_1',package=package,alerts=[],
                provisional_archive_alert_bytes=args.archive_warning)
    if old and old['package']['sha256']!=package['sha256']:
        report['previous_package']=dict(sha256=old['package']['sha256'],
                                        bytes=old['package']['bytes'],
                                        delta_bytes=package['bytes']-old['package']['bytes'])
    elif old and 'previous_package'in old:report['previous_package']=old['previous_package']
    if package['bytes']>args.archive_warning:report['alerts'].append('Provisional archive planning alert exceeded')
    for entry in package['entries']:
        if entry['payload_bytes']>64000:report['alerts'].append(entry['name']+': variable exceeds64000-byte planning guard')
        if entry.get('ttpack_headroom_bytes',512)<512:
            report['alerts'].append(entry['name']+': TTPack input headroom '+str(entry['ttpack_headroom_bytes'])+' bytes')
    args.out.parent.mkdir(parents=True,exist_ok=True)
    if args.native:
        report['layout']=native_layout(args.out.parent)
        report['decode']=decode_views(args.out.parent,args.repeats)
        sources=[HERE/'fft.h',HERE/'fft.c',HERE/'core_limits.h',HERE/'module.h',
                 HERE/'module_bank.h',HERE/'tools/module.py',
                 HERE/'chapter.h',HERE/'unit_setup.inc',HERE/'actors.h',HERE/'chapters.h',
                 HERE/'campaign/probe_core_layout.c',HERE/'campaign/save.h',HERE/'campaign/save_types.h',
                 HERE/'tactical_grid.h',HERE/'tactical_search.inc',HERE/'surface_geometry.h',
                 HERE/'tools/surface_data.py',HERE/'tools/chapter_desc.py',HERE/'save.inc',
                 HERE/'tools/storage_decode_bench.c',ROOT/'lib/unpack68k.s',
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-ti68k/rt_ti.c']
        report['benchmark_sources']={str(p.relative_to(ROOT)):core_audit.sha(p.read_bytes())for p in sources}
        if old and 'layout'in old and 'decode'in old:
            report['native_change']=dict(
                layout_bytes={k:v-old['layout']['native'][k]for k,v in report['layout']['native'].items()
                              if k.endswith('_bytes')and isinstance(v,int)and k in old['layout']['native']},
                stream_cycle_deltas={r['bank']+'/'+r['stream']:
                    max(r['cycles'])-max(previous['cycles'])
                    for r in report['decode']['streams']for previous in old['decode']['streams']
                    if (r['bank'],r['stream'])==(previous['bank'],previous['stream'])})
    else:
        report['native_measurements']='Not run; use --native after source changes. Old measurements are not copied.'
    text=json.dumps(report,indent=2,sort_keys=True)+'\n'
    if not args.out.exists()or args.out.read_text()!=text:args.out.write_text(text)
    print(f"FFT package: {package['bytes']} bytes; {package['variables']} variables; "
          f"payload {package['payload_bytes']}; transport {package['transport_overhead']}")
    for name,size in package['categories'].items():print(f'  {name}: {size}')
    if 'previous_package'in report:print('Change:',report['previous_package']['delta_bytes'],'bytes')
    for alert in report['alerts']:print('ALERT:',alert)
    print('Report:',args.out)


if __name__=='__main__':main()
