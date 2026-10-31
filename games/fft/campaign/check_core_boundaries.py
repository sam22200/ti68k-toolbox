#!/usr/bin/env python3
"""Current-capacity core stress baseline on PC and real68000, no emulator UI."""
import json
import argparse
import re
import struct
import tempfile
from pathlib import Path
from check_flow import HERE, ROOT, run, data_links


def descriptor13():
    from surface_data import detach_descriptor,attach_descriptor
    original,surface=detach_descriptor((HERE/'data/ffgd.bin').read_bytes())
    data=bytearray(original)
    assert data[:4]==b'FCD3'
    unit_field,count_field=17,50
    original=struct.unpack_from('>H',data,2*unit_field)[0]
    count=struct.unpack_from('>H',data,2*count_field)[0]
    assert count==11
    definitions=bytearray(data[original:original+count*28])
    occupied={(definitions[i*28+8],definitions[i*28+9])for i in range(count)}
    interface=(HERE/'data/ffgi.bin').read_bytes()
    tile_offset=struct.unpack_from('>H',data,2*21)[0]
    width,height=(struct.unpack_from('>H',data,2*field)[0]for field in (10,9))
    candidates=[(i%width,i//width)for i in range(width*height)
                if interface[tile_offset+i*12+9]==1 and (i%width,i//width)not in occupied]
    for x,z in candidates[:2]:
        clone=bytearray(definitions[28:56]);clone[8]=x;clone[9]=z;clone[11]=1
        definitions.extend(clone)
    assert len(definitions)==13*28
    if len(data)&1:data.append(0)
    struct.pack_into('>H',data,2*unit_field,len(data));data.extend(definitions)
    struct.pack_into('>H',data,2*count_field,13)
    struct.pack_into('>H',data,2*51,len(data));data.extend(bytes(22)+bytes((2,17,3,25)))
    return attach_descriptor(data,surface)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/core-audit/boundaries')
    out=ap.parse_args().out.resolve();out.mkdir(parents=True,exist_ok=True)
    source=HERE/'check_core_boundaries.c';core=ROOT/'runtime/core/rt_core.c'
    run(['cc','-std=gnu99','-O1','-w','-o',out/'check',source,core,
         ROOT/'runtime/platform-sw/rt_sw.c'])
    # Serialize GCC4TI; never build modules/checkers at the same time.
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',out/'checkc',source,core,
         ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')
           for p in folder.glob('*.89y')}
    results=[]
    thirteen=descriptor13();variants=[('orbonne',101,None),('gariland',201,None),('gariland13',201,thirteen)]
    for label,patch in (('count0',(100,0)),('count14',(100,14)),('truncated-defs',(34,len(thirteen)-10)),
                        ('width0',(20,0)),('area270',(20,18)),('height0',(18,0)),
                        ('scene-too-tall',(26,203)),('truncated-tiles',(42,65530))):
        bad=bytearray(thirteen);struct.pack_into('>H',bad,*patch);variants.append((label,299,bytes(bad)))
    variants.append(('old-version',299,b'FCD2'+thirteen[4:]))
    from surface_data import detach_descriptor
    plain,geometry=detach_descriptor(thirteen)
    variants.append(('missing-geometry',299,plain))
    variants.append(('truncated-geometry',299,thirteen[:-1]))
    bad=bytearray(thirteen);bad[-4:]=bytes((255,254,0,8))
    variants.append(('invalid-geometry-span',299,bytes(bad)))
    variants.append(('state-allocation-failure',298,None))
    for chapter,scenario,descriptor in variants:
        with tempfile.TemporaryDirectory(prefix='fft-core-boundaries-')as name:
            folder=Path(name);data_links(folder)
            case_files=dict(files)
            if descriptor is not None:
                (folder/'ffgd.bin').unlink();(folder/'ffgd.bin').write_bytes(descriptor)
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffgd.bin','ffgd'],folder)
                case_files['ffgd']=folder/'ffgd.89y'
            pc=run([out/'check',scenario],folder)
            command=[ROOT/'tools/bin/ti-cycles','--frames',0,'--arg',scenario,'--max',10000]
            for p in case_files.values():command+=['--file',p]
            native=run([*command,out/'checkc.89z'])
        (out/(chapter+'-pc.log')).write_text(pc);(out/(chapter+'-native.log')).write_text(native)
        match=re.search(r'boundaries ([0-9A-F]+) failures 0 checks (\d+)',pc)
        if not match:raise AssertionError(pc)
        values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
        assert values[-3:]==[int(match[1],16),0,int(match[2])],(pc,values)
        results.append(dict(chapter=chapter,checks=int(match[2]),hash=match[1],failures=0,
                            units=13 if chapter=='gariland13'else 0 if scenario in (298,299) else 11))
        print(chapter+': '+pc.strip(),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
