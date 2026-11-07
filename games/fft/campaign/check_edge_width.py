#!/usr/bin/env python3
"""Both selectable Gariland widths: source colours, depth and PC/68000 parity."""
from pathlib import Path
import argparse,json,re,subprocess,sys,tempfile
import numpy as np
from PIL import Image,ImageDraw

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'x/edge-width'
sys.path.insert(0,str(HERE.parent/'tools'))
from review_files import publish_images,save_image

BANKS='ffbm ffbn ffbo ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split()

def run(args,cwd=HERE/'data'):
    p=subprocess.run(list(map(str,args)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode:print(p.stdout);p.check_returncode()
    return p.stdout

def decode(raw):
    with tempfile.TemporaryDirectory() as name:
        p=Path(name);(p/'i').write_bytes(raw)
        run([ROOT/'tools/bin/dzx0','-f',p/'i',p/'o'])
        return (p/'o').read_bytes()

def digest(raw):
    h=17
    for byte in raw:h=(((h<<1)|(h>>31))^byte)&0xffffffff
    return h

def expected():
    import campaign_scenery
    colours=[];depths=[];report=[]
    for rot in range(4):
        # Independent before/after fixture: former reviewed source raw plus
        # the prior isolated2px V2 bank, not the new runtime apply algorithm.
        one=campaign_scenery.chapter_planes((HERE.parent/f'x/isometric-style/variant-{rot}.raw').read_bytes(),rot)
        old=(HERE/f'x/qa/followup/edges/two-pixel/ffgv{rot}.bin').read_bytes()
        size=len(one)//6;count=int.from_bytes(old[2:4],'big');two=decode(old[4:4+count])
        bank=(HERE/f'data/ffgv{rot}.bin').read_bytes();assert bank[:2]==b'V3'
        if bank[-10:-6] in (b'FC01',b'FC02'):bank=bank[:int.from_bytes(bank[-6:-4],'big')]
        a,b=[int.from_bytes(bank[i:i+2],'big')for i in (2,4)]
        assert decode(bank[6:6+a])==one[:2*size]
        mask=decode(bank[6+a:6+a+b]);assert len(mask)==size
        low=np.frombuffer(one[:size],np.uint8);dark=np.frombuffer(one[size:2*size],np.uint8);m=np.frombuffer(mask,np.uint8)
        assert ((low&~m)|(~dark&m)).tobytes()+(dark^m).tobytes()==two
        depth=np.frombuffer(decode(bank[6+a+b:]),np.uint8).reshape(4,-1,40).copy()
        for k in (1,3):depth[k]=np.bitwise_xor.accumulate(depth[k],axis=0)
        assert depth.tobytes()==one[2*size:]
        colours.append([digest(one[:2*size]),digest(two)]);depths.append(digest(depth.tobytes()))
        report.append(dict(view=rot,mask_bytes=b,colour_bytes=a,decoded_mask_bytes=size))
    (OUT/'expected.h').write_text('// Generated local visual fixtures; do not edit.\nstatic const u32 edge_colours[4][2]={'+','.join('{'+','.join('0x%08XUL'%v for v in row)+'}'for row in colours)+'};\nstatic const u32 edge_depth[4]={'+','.join('0x%08XUL'%v for v in depths)+'};\n')
    return report

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--pc-only',action='store_true');args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True);rows=expected();common=[HERE/'check_edge_width.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-w','-o',OUT/'check',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    with tempfile.TemporaryDirectory(prefix='fft-edge-screens-')as folder:
        pc=run([OUT/'check',folder]);publish_images(folder,OUT/'shots')
    (OUT/'pc.log').write_text(pc);print(pc,end='')
    match=re.search(r'edges ([0-9A-F]+) failures 0 checks (\d+)',pc);assert match,pc
    sheet=Image.new('RGB',(984,4*328),'white');draw=ImageDraw.Draw(sheet)
    for rot in range(4):
        for width in range(2):
            x,y=492*width,328*rot
            draw.text((x+5,y+5),f'Gariland {rot} / {width+1}px / MODE > Edges',fill='black')
            sheet.paste(Image.open(OUT/f'shots/edge-{rot}-{width}.png').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(sheet,OUT/'edge-width-runtime.png')
    settings=Image.new('RGB',(984,328),'white');draw=ImageDraw.Draw(settings)
    for width in range(2):
        x=492*width;draw.text((x+5,5),f'MODE > Edges: {width+1}px / ENTER to toggle',fill='black')
        settings.paste(Image.open(OUT/f'shots/settings-{width+1}px.png').resize((480,300),Image.Resampling.NEAREST),(x,24))
    save_image(settings,OUT/'settings-review.png')
    if args.pc_only:return
    run([ROOT/'tools/bin/ti-cc','-o',OUT/'edgesch','-DRT_CYCLES',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    command=[ROOT/'tools/bin/ti-cycles','--max',10000,'--arg',61,'--frames',0]
    for bank in BANKS:command+=['--file',HERE/'data'/(bank+'.89y')]
    native=run([*command,OUT/'edgesch.89z']);(OUT/'native.log').write_text(native)
    values=[int(v,16)for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',native)]
    assert values[-3:]==[int(match[1],16),0,int(match[2])],native[-8000:]
    screens=re.findall(r'shot \d+ checksum (\w+)',native)[-8:]
    assert [s.lower()for s in screens]==[s.lower()for s in re.findall(r'screen \d \d (\w+)',pc)]
    report=dict(checks=int(match[2]),state_and_plane_digest=match[1],views=rows,
                screenshots=screens,scope='Selectable Gariland1px/2px, depth/cache and paused input; no emulator run.')
    report['colour_reload_zones']=[s for s in native.splitlines()if re.match(r'^(?:18|19)\s',s)]
    (OUT/'checked.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
