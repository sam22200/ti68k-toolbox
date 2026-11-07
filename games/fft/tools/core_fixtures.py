#!/usr/bin/env python3
"""Generate synthetic audit inputs. These do NOT execute a future battle engine.

Keep input shape/identity separate from behavioral assertions. Unsupported cases
must remain pending in core_fixtures.json, never silently marked passing.
"""
import argparse
import json
from pathlib import Path
from port_limits import UNIT_MAX, TILE_MAX, AXIS_MAX

HERE=Path(__file__).resolve().parents[1]


def validate(snapshot):
    if snapshot.get('schema')!='FFT_CORE_SNAPSHOT_DRAFT_2':raise ValueError('snapshot schema')
    width,height,levels=(snapshot[k] for k in ('width','height','levels'))
    if not 1<=width<=AXIS_MAX or not 1<=height<=AXIS_MAX or width*height>256 or levels not in (1,2):
        raise ValueError('geometry outside audit envelope')
    tiles=snapshot['tiles'];count=len(tiles)
    if not 1<=count<=TILE_MAX:raise ValueError('compact tile count')
    tile_ids={tile['id'] for tile in tiles}
    if tile_ids!=set(range(count)):raise ValueError('compact tile identities')
    positions=set()
    for tile in tiles:
        x,y,level=tile['position']
        if not 0<=x<width or not 0<=y<height or not 0<=level<levels:raise ValueError('tile coordinate')
        if (x,y,level)in positions:raise ValueError('duplicate tile coordinate')
        positions.add((x,y,level))
    units=snapshot['units']
    if len(units)>UNIT_MAX:raise ValueError('13-unit port envelope')
    ids={u['id'] for u in units}
    if len(ids)!=len(units)or any(not 0<=i<UNIT_MAX for i in ids):raise ValueError('duplicate/invalid unit identity')
    occupied=set()
    for u in units:
        if u['presence'] not in ('field','off_field','removed'):raise ValueError('presence')
        if u['presence']!='field':continue
        x,y,level=u['position']
        if not 0<=x<width or not 0<=y<height or not 0<=level<levels:raise ValueError('unit position')
        if (x,y,level)not in positions:raise ValueError('unit has no tile')
        if (x,y,level)in occupied:raise ValueError('duplicate occupied cell')
        occupied.add((x,y,level))
    for pending in snapshot['pending']:
        if pending['actor']not in ids:raise ValueError('unknown pending actor')
        if pending['kind']not in ('charge','cast','jump','performance'):raise ValueError('execution mechanism')
        if pending['target_kind']=='unit':
            if pending['target']not in ids:raise ValueError('unknown target identity')
        elif pending['target_kind']=='tile':
            if pending['target']not in tile_ids:raise ValueError('unknown target tile')
        else:raise ValueError('target kind')
    return True


def snapshot(count,width=16,height=16,levels=1,upper_positions=()):
    units=[]
    for i in range(count):
        units.append(dict(id=i,team='player'if i<4 else'guest'if i<6 else'enemy',
                          presence='field',hp=0 if i in (10,11) else 100,
                          position=[i%width,i//width,0],ct=99))
    tiles=[dict(id=cell,position=[cell%width,cell//width,0],height=cell%17,blocked=False)
           for cell in range(width*height)]
    for x,y in upper_positions:
        tiles.append(dict(id=len(tiles),position=[x,y,1],height=18,blocked=False))
    pending=[dict(actor=i,kind='charge',due_tick=1,target_kind='tile',target=len(tiles)-1)
             for i in range(count)if units[i]['hp']]
    return dict(schema='FFT_CORE_SNAPSHOT_DRAFT_2',width=width,height=height,levels=levels,
                tiles=tiles,
                units=units,pending=pending,
                expectation='Input contract only. Separate native13-actor and compact identity/search '
                            'and actual battle-cell fixtures pass; 13-actor/256-node adapters pass; source upper-floor '
                            'movement/LoS/cover and all-effects oracle traces remain pending '
                            '(CORE_FULL_GRID.md).')


def build():
    thirteen=snapshot(13,15,12,2,tuple((i,0)for i in range(15)))
    full=snapshot(13)
    layers=snapshot(2,4,18,2,((0,0),(3,17)));layers['units'][1]['position']=[0,0,1]
    layers['pending']=[]
    return {'actors13':thirteen,'compact256':full,'two_floors_4x18':layers}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'campaign/x/core-audit/fixtures')
    args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    # Retire only our two known superseded generated inputs. Keep the old
    # envelope as historical research instead of mixing it with active fixtures.
    for filename in ('actors16.json','slots21.json'):
        old=args.out/filename
        if old.exists()and json.loads(old.read_text()).get('schema')=='FFT_CORE_SNAPSHOT_DRAFT_1':
            history=args.out/'historical-envelope';history.mkdir(exist_ok=True)
            old.replace(history/filename)
    for name,value in build().items():
        validate(value);text=json.dumps(value,indent=2,sort_keys=True)+'\n';path=args.out/(name+'.json')
        if not path.exists()or path.read_text()!=text:path.write_text(text)
    print('3 validated synthetic INPUT fixtures; future-engine mechanics remain pending')


if __name__=='__main__':main()
