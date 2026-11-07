#!/usr/bin/env python3
"""Referenced retail maps: events plus active random-encounter routes.

Count every terrain variant conservatively. This is static reference evidence,
not a played proof that every branch/resource is reached.
"""
import argparse,collections,json,struct
from pathlib import Path
import core_audit,extract,surface_data


def profile(columns):
    flags=0
    for col in columns:
        if not col[7]&128:flags|=3
        if col[4]:flags|=4
        half=col[1]&31
        if 2*col[0]+3*half+2*(col[1]>>5)+6>127:flags|=1
    return flags


def build():
    disc,files=extract.disc_files(str(core_audit.DEFAULT_DISC))
    attack=disc.read(*files['EVENT/ATTACK.OUT']);events=core_audit.scenarios(attack)
    world=disc.read(*files['WORLD/WLDCORE.BIN']);base=0x80067000
    pointer=struct.unpack_from('<I',world,0x80096a40-base)[0];start=pointer-base
    if start<0 or start+57*24>len(world):raise ValueError('Random encounter table bounds')
    routes=[]
    for i in range(57):
        row=world[start+24*i:start+24*(i+1)]
        if any(row[3+3*j]and row[4+3*j]for j in range(4)):
            routes.append(dict(location=24+i//3,route=row[0],map=row[22]))
    event_maps={e['map']for e in events};random_maps={r['map']for r in routes}
    used=event_maps|random_maps
    resources,_=core_audit.maps(disc,files);by_map=collections.defaultdict(list);fixture=set()
    for r in resources:
        if r['map']not in used:continue
        mesh=disc.read(r['lba'],r['resource_bytes']);at=r['terrain_offset']
        raw=mesh[at+2:at+4098]
        columns=[surface_data.column(raw,i)for i in range(r['cells_per_layer'])]
        flags=profile(columns);fixture.update(bytes((flags,))+col+b'\0'for col in columns)
        by_map[r['map']].append(dict(record=r['record'],type=r['type'],width=r['width'],height=r['height'],
             movement_upper=bool(flags&2),positive_ray_upper=bool(flags&4),room_check=bool(flags&1),
             stoppable_upper=r['layers'][1]['not_blocked_or_untargetable'],sha256=r['resource_sha256']))
    if used-set(by_map):raise ValueError('Referenced map missing terrain')
    rows=[dict(map=m,event_reference=m in event_maps,random_reference=m in random_maps,
               no_upper_stop=all(not r['stoppable_upper']for r in by_map[m]),
               skip_upper_movement=all(not r['movement_upper']for r in by_map[m]),
               skip_upper_rays=all(not r['positive_ray_upper']for r in by_map[m]),
               variants=by_map[m])for m in sorted(used)]
    summary=dict(referenced_maps=len(used),event_maps=len(event_maps),random_maps=len(random_maps),
                 active_random_routes=len(routes),terrain_variants=sum(len(v)for v in by_map.values()),
                 no_upper_stop=sum(r['no_upper_stop']for r in rows),
                 skip_upper_movement=sum(r['skip_upper_movement']for r in rows),
                 skip_upper_rays=sum(r['skip_upper_rays']for r in rows))
    report=dict(schema='FFT_MAP_LAYERS_1',summary=summary,maps=rows,random_routes=routes,
                attack_sha256=core_audit.sha(attack),wldcore_sha256=core_audit.sha(world),
                random_table_address=hex(pointer),random_table_sha256=core_audit.sha(world[start:start+57*24]),
                scope='Static event and active random-route references, all terrain variants; includes scenes, not only deployment battles. No test/unreferenced map in the denominator; not a complete played conditional trace.')
    return report,sorted(fixture)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=core_audit.HERE/'campaign/x/core-single-floor')
    out=ap.parse_args().out;out.mkdir(parents=True,exist_ok=True)
    report,rows=build();data=out/'profiles';data.mkdir(exist_ok=True)
    for i in range(0,len(rows),6000):
        chunk=rows[i:i+6000];(data/f'fsp{i//6000:02}.bin').write_bytes(b'FSP1'+struct.pack('>H',len(chunk))+b''.join(chunk))
    report['profile_rows']=len(rows);(out/'maps.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report['summary']),flush=True)


if __name__=='__main__':main()
