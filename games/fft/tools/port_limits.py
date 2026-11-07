#!/usr/bin/env python3
"""Recount the reduced TI envelope from retail data and optional community JSON.

Excluding test maps is policy, not an inference that every other map is a story
battle. Stoppable-cell counts are not a complete movement/targeting graph.
"""
import argparse
import json
from pathlib import Path
import re

import core_audit
import extract

TEST_MAPS = (0, 117, 118, 119, 125)
LIMIT_HEADER = core_audit.HERE/'core_limits.h'
def limit(name):
    match=re.search(r'^#define '+name+r' (\d+)U?\s*$',LIMIT_HEADER.read_text(),re.M)
    if not match:raise ValueError('Missing core envelope define '+name)
    return int(match[1])
UNIT_MAX=limit('FFT_CORE_UNIT_MAX')
TILE_MAX=limit('FFT_CORE_TILE_MAX')
AXIS_MAX=limit('FFT_CORE_AXIS_MAX')
COMMUNITY_COMMIT = 'c4f8252d1ca8446006e6cbd751c2f02b776e5d66'
NAMES = {29:'Colliery Underground Second Floor',53:'Entrance to Death City',
         76:'Zeklaus Desert',83:'Zirekile Falls',88:'Finath River',
         89:'Poeskas Lake',100:'Public Cemetery'}


def row(record):
    lower,upper=[l['not_blocked_or_untargetable'] for l in record['layers']]
    return dict(map=record['map'],name=NAMES.get(record['map']),
                resource_record=record['record'],type=record['type'],
                width=record['width'],height=record['height'],
                lower=lower,upper=upper,total=lower+upper,
                # Preserve ALL lower coordinates, including transit/target
                # obstacles; upper records whose source bit0 is clear are a
                # conservative additional geometry set, not just stop cells.
                lower_rectangle_plus_unblocked_upper=record['cells_per_layer']+
                    record['layers'][1]['unblocked'],
                resource_sha256=record['resource_sha256'])


def rank(rows,key='total'):
    return sorted(rows,key=lambda r:(-r[key],r['map'],r['resource_record']))


def census(disc):
    disk,files=extract.disc_files(str(disc));resources,_=core_audit.maps(disk,files)
    retained=[row(r) for r in resources if r['map'] not in TEST_MAPS]
    upper=[r for r in retained if r['upper']]
    if any(r['lower_rectangle_plus_unblocked_upper']>TILE_MAX or
           max(r['width'],r['height'])>AXIS_MAX for r in retained):
        raise ValueError('Retail resource exceeds the accepted tile envelope')
    return dict(test_map_exclusions=list(TEST_MAPS),resources=retained,
                distinct_maps=len({r['map']for r in retained}),
                most_stoppable=rank(retained)[0],
                most_stoppable_with_upper=rank(upper)[0],
                most_upper=rank(upper,'upper')[0],
                largest_conservative_node_set=rank(retained,'lower_rectangle_plus_unblocked_upper')[0],
                max_axis=max(max(r['width'],r['height'])for r in retained),
                confidence='Raw retail-US flag census over every terrain resource/variant; '
                           'not an original movement oracle or proof that every retained map is a battle.')


def community(folder,source):
    rows=[]
    for path in sorted(folder.glob('MAP*.json')):
        data=json.loads(path.read_text());counts=[]
        for level in ('lower','upper'):
            counts.append(sum(not cell['no_walk'] and not cell['no_cursor']
                              for line in data[level] for cell in line))
        number=int(path.stem[3:]);gns=data['gns']
        rows.append(dict(map=number,gns=gns,width=data['width'],height=data['height'],
                         lower=counts[0],upper=counts[1],total=sum(counts),
                         source_sha256=core_audit.sha(path.read_bytes()),
                         synthetic_gns=gns!=f'MAP{number:03}.GNS'))
    retained=[r for r in rows if r['map']not in TEST_MAPS]
    primary={r['map']:r for r in source['resources']if r['type']=='2E01'}
    mismatches=[]
    for r in retained:
        actual=primary.get(r['map'])
        if actual is None or any(r[k]!=actual[k]for k in ('width','height','lower','upper')):
            mismatches.append(dict(map=r['map'],community={k:r[k]for k in ('gns','width','height','lower','upper')},
                                   retail={k:actual[k]for k in ('width','height','lower','upper')}if actual else None))
    return dict(files=len(rows),commit=COMMUNITY_COMMIT,
                url='https://github.com/rainbowbismuth/fft-map-json/tree/'+COMMUNITY_COMMIT,
                counted_filter='not no_walk and not no_cursor',rows=rows,
                synthetic_gns=[r for r in rows if r['synthetic_gns']],
                retained_top=sorted(retained,key=lambda r:(-r['total'],r['map']))[:10],
                mismatches_with_retail_primary=mismatches,
                caveat='README mentions MAP000/MAP062 problems. This repository revision includes '
                       'both files plus a synthetic MAP120, and does not establish their retail identity.')


def build(disc,folder=None):
    import battle
    source=census(disc)
    retail=battle.Disc(str(disc))
    zirekile=[retail.entd(0x195,i)for i in range(16)]
    zirekile=[dict(slot=i,identity=b[0],presence=b[24]&0xC0,team_flags=b[24])
              for i,b in enumerate(zirekile)if b[0]]
    return dict(schema='FFT_PORT_ENVELOPE_1',
                limits_header_sha256=core_audit.sha(LIMIT_HEADER.read_bytes()),
                policy=dict(units=UNIT_MAX,unit_index_bits=8,unit_mask_bits=16,
                            compact_tiles=TILE_MAX,tile_index_bits=8,tile_count_bits=16,
                            reserved_invalid_tile=None,max_axis=AXIS_MAX,layers=2,
                            excluded_encounter='The eleven-Monk random encounter; other encounters above13 '
                                               'are rejected by content validation, never truncated.'),
                retail=source,community=community(folder,source)if folder else None,
                zirekile=dict(map=83,entd=0x195,source_units=zirekile,
                               always_present=sum(u['presence']==0x80 for u in zirekile),
                               deployment_squad=270,deployment_capacity=4,
                               deployment_evidence='Existing original-overlay observation in '
                                  'x/core-audit/source-observed.json, squad270; '
                                  '9 always-present ENTD plus4 declared deployed =13. '
                                  'This is not a complete played Zirekile trace.'),
                runtime_status='Descriptor-driven13-actor capacity is implemented; existing encounters use11. '
                               'The compact256-node/two-floor identity/search kernel passes native fixtures '
                               '(CORE_GEOMETRY.md); battle unit/cursor/render adapters still use one floor.')


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('disc',nargs='?',type=Path,default=core_audit.DEFAULT_DISC)
    ap.add_argument('--community',type=Path,help='local pinned fft-map-json data directory')
    ap.add_argument('--out',type=Path,default=core_audit.HERE/'campaign/x/core-audit/port-limits.json')
    args=ap.parse_args();report=build(args.disc,args.community)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    text=json.dumps(report,indent=2,sort_keys=True)+'\n'
    if not args.out.exists()or args.out.read_text()!=text:args.out.write_text(text)
    print('Target:13 units,256 compact u8 tile IDs, u16 count, separate coordinates/layer; no invalid-tile ID')
    for key in ('most_stoppable','most_stoppable_with_upper','most_upper','largest_conservative_node_set'):
        print(key,json.dumps(report['retail'][key],sort_keys=True))
    if report['community']:print('Community files:',report['community']['files'],
                                 'retail-primary differences:',len(report['community']['mismatches_with_retail_primary']))
    print('Report:',args.out)


if __name__=='__main__':main()
