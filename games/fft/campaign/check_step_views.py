#!/usr/bin/env python3
"""Only the two affected movement-range views; preserve existing gallery files."""
import argparse,json,re,tempfile,subprocess
from pathlib import Path
from check import HERE,ROOT,run
from check_flow import data_links


def main():
    from PIL import Image,ImageDraw
    def pixels(path):
        # Host captures use the reviewed24..216 palette at160x100; ti-cycles
        # exports the full240x128 LCD at0..255. Compare viewport shade indices.
        im=Image.open(path).convert('L');native=im.width==240
        raw=im.crop((0,0,160,100)).tobytes()
        palette={v:i for i,v in enumerate((0,85,170,255)if native else(24,88,152,216))}
        return bytes(palette[v]for v in raw)
    out=HERE/'x/core-steps/views';out.mkdir(parents=True,exist_ok=True)
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--verify-accepted',action='store_true')
    args=ap.parse_args();accepted={};promoted=[]
    if args.verify_accepted:
        record=json.loads((out/'results.json').read_text());assert not record['review_pending']
        review=Image.open(out/'movement-before-after.png').convert('L');index=0
        for case in record['cases']:
            if not case['previous_image_equal']:
                im=review.crop((666,28+446*index,1306,428+446*index)).resize((160,100),Image.Resampling.NEAREST)
                accepted[case['map']]=im.tobytes();index+=1
    core=ROOT/'runtime/core/rt_core.c';sw=ROOT/'runtime/platform-sw/rt_sw.c'
    files={p.stem:p for folder in (HERE.parent/'opening/data',HERE/'data')for p in folder.glob('*.89y')}
    cases=[('gariland',HERE/'check_native.c',['-DCHECK_GAR'],61,HERE/'x/check/settings.keys',HERE/'x/check/gariland-reach-contour-pc.png'),
           ('orbonne',HERE.parent/'opening/check_native.c',[],1,HERE.parent/'opening/x/check/settings.keys',HERE.parent/'opening/x/check/reach-contour-pc.png')]
    results=[];changed=[]
    with tempfile.TemporaryDirectory(prefix='fft-step-views-')as name:
        temp=Path(name);data_links(temp)
        for label,source,flags,scenario,keys,previous in cases:
            run(['cc','-std=gnu99','-O1','-w',*flags,'-o',out/(label+'pc'),source,core,sw])
            run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES',*flags,'-o',out/(label[:3]+'v'),source,core,
                 ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
            pc=temp/(label+'-pc.png');native=temp/(label+'-native.png')
            text=run([out/(label+'pc'),scenario,70,keys,pc],temp)
            cmd=[ROOT/'tools/bin/ti-cycles','--arg',scenario,'--frames',70,'--max',3000,'--keys',keys,'--png',native]
            for p in files.values():cmd+=['--file',p]
            result=run([*cmd,out/(label[:3]+'v.89z')],temp)
            a=re.search(r'checksum (\w+)',text)[1].lower();b=re.search(r'checksum (\w+)',result)[1].lower()
            assert a==b,(label,a,b)
            assert pixels(pc)==pixels(native),label
            same=pc.read_bytes()==previous.read_bytes();results.append(dict(map=label,checksum=a,pc_native_equal=True,previous_image_equal=same))
            if args.verify_accepted:
                expected=accepted.get(label,Image.open(previous).convert('L').tobytes())
                assert Image.open(pc).convert('L').tobytes()==expected,(label,'accepted pixels')
                if not same:
                    # Promote only explicitly accepted captures. Keep the old
                    # reference pair once, without refreshing other gallery dates.
                    import zipfile
                    backup=out/(label+'-previous-reference.zip')
                    target=previous.with_name(previous.name.replace('-pc.png','-ti.png'))
                    if not backup.exists():
                        with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED)as archive:
                            archive.write(previous,previous.name)
                            if target.exists():archive.write(target,target.name)
                    import sys
                    sys.path.insert(0,str(HERE.parent/'tools'))
                    from review_files import write_changed
                    write_changed(previous,pc.read_bytes());write_changed(target,native.read_bytes())
                    promoted.append(label)
            elif not same:changed.append((label,previous.read_bytes(),pc.read_bytes()))
        if changed:
            import io
            scale=4;canvas=Image.new('RGB',(1316,446*len(changed)),'#eeeeee');draw=ImageDraw.Draw(canvas)
            for i,(label,before,after)in enumerate(changed):
                y=i*446+28;draw.text((8,y-18),label+' - previous movement rules',fill='black');draw.text((666,y-18),'Source endpoint rules',fill='black')
                for x,raw in [(8,before),(666,after)]:canvas.paste(Image.open(io.BytesIO(raw)).convert('RGB').resize((640,400),Image.Resampling.NEAREST),(x,y))
            review=out/'movement-before-after.png';canvas.save(review)
            # This PNG is an input; the capture runner rewrites PNG arguments
            # as temporary outputs and must not wrap the gallery marker call.
            subprocess.run([ROOT/'tools/bin/ti-view','--decision',review],check=True)
            print('Decision:',review,flush=True)
    report=out/('verification-accepted.json'if args.verify_accepted else'results.json')
    report.write_text(json.dumps(dict(cases=results,review_pending=bool(changed),accepted_pixels_checked=args.verify_accepted,promoted=promoted),indent=2)+'\n')
    print('Two affected movement views agree PC/native; changed references:',len(changed),flush=True)


if __name__=='__main__':main()
