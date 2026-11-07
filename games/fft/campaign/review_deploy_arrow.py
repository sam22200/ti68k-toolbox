#!/usr/bin/env python3
"""Isolated arrow A/B review; never changes the active deployment renderer."""
from pathlib import Path
import argparse,tempfile
from PIL import Image,ImageDraw,ImageChops
from check_flow import HERE,ROOT,run,data_links,fixture
from review_files import publish_images,save_image,write_changed
import check

def variant(source,style):
    start=source.index('static void deploy_orientation(void)')
    end=source.index('\nvoid game_render(void)',start)
    # Reconstruct the historical thin/sloped control after A was selected.
    body=source[start:end].replace('deploy_arrow_line(', 'deploy_line(')
    body=body.replace('y[3]={75,87,87}','y[3]={75,86,87}')
    if style=='A':
        # A 2x2 pixel brush doubles horizontal/vertical contour thickness.
        body=body.replace('deploy_line(', 'review_arrow_line(')
        helper='''static void review_arrow_line(s16 x,s16 y,s16 ex,s16 ey,u8 color)
{
    deploy_line(x,y,ex,ey,color);deploy_line(x+1,y,ex+1,ey,color);
    deploy_line(x,y+1,ex,ey+1,color);deploy_line(x+1,y+1,ex+1,ey+1,color);
}
'''
        body=helper+body
    elif style=='B':
        assert 'y[3]={75,86,87}' in body
        body=body.replace('y[3]={75,86,87}','y[3]={77,86,87}')
    return source[:start]+body+source[end:]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/qa/flow/review-10')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    active=(HERE/'deploy.c').read_bytes();source=active.decode()
    labels={'current':'Current:12px high / 1px contour','A':'A:12px high / 2px contour',
            'B':'B:10px high / 1px contour'}
    with tempfile.TemporaryDirectory(prefix='fft-arrow-ab-') as folder:
        temp=Path(folder);data=temp/'data';data.mkdir();data_links(data)
        initial=bytearray(fixture());initial[3114:3138]=bytes(24);initial[3114]=1
        prep=temp/'before.sav';prep.write_bytes(initial)
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','sav','before.sav','ffsave'],temp)
        for style in labels:
            stage=temp/style;stage.mkdir();shots=stage/'shots';shots.mkdir()
            deploy=stage/'deploy.c';code=variant(source,style)
            for name in ('manage.c','flow.h','deployment.inc'):
                code=code.replace(f'#include "{name}"',f'#include "{HERE/name}"')
            deploy.write_text(code)
            def harness(name):
                s=(HERE/name).read_text().replace('#include "deploy.c"',f'#include "{deploy}"')
                return s.replace('../../../runtime/',str(ROOT/'runtime')+'/')
            preview=stage/'preview.c';preview.write_text(harness('preview_deploy.c'))
            run(['cc','-std=gnu99','-O1','-w','-I',HERE,'-o',stage/'preview',preview,
                 ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c'])
            (data/'ffsave.sav').write_bytes(fixture());run([stage/'preview',shots],data)
            publish_images(shots,out/style)
            if style!='current':
                for name in ('deploy-initial.png','deploy-full.png','deploy-confirm.png'):
                    a=Image.open(out/'current'/name).convert('RGB');b=Image.open(out/style/name).convert('RGB')
                    box=ImageChops.difference(a,b).getbbox()
                    assert box and box[0]>=54 and box[1]>=72 and box[2]<=88 and box[3]<=91,(style,name,box)
            checker=stage/'check.c';checker.write_text(harness('check_deploy.c'))
            check.OUT=out/style/'native';check.OUT.mkdir(exist_ok=True)
            empty=check.keys(check.OUT/'none.keys',[])
            confirm=check.keys(check.OUT/'confirm.keys',[(2,'C')])
            # Same renderer, bank bytes, inputs and saved party on PC/actual68000.
            program='arrbase' if style=='current' else 'arr'+style.lower()
            results=check.program(program,str(checker),['ffmg','ffcp','ffcj','ffflow'],0x44504c31,
                    [('initial',200,4,empty),('confirm',200,8,confirm)],prep,temp/'ffsave.89y',None,workdir=data)
            write_changed(out/style/'native/results.txt',(str(results)+'\n').encode())
    sheet=Image.new('RGB',(1476,816),'white');d=ImageDraw.Draw(sheet)
    for col,(style,label) in enumerate(labels.items()):
        x=col*492
        for row,name in enumerate(('deploy-initial.png','deploy-full.png')):
            y=row*328;d.text((x+5,y+5),label,fill='black')
            shot=Image.open(out/style/name).convert('RGB')
            sheet.paste(shot.resize((480,300),Image.Resampling.NEAREST),(x,y+24))
        d.text((x+5,661),label+' / detail x6',fill='black')
        detail=Image.open(out/style/'deploy-full.png').convert('RGB').crop((52,70,89,91))
        sheet.paste(detail.resize((222,126),Image.Resampling.NEAREST),(x+114,684))
    save_image(sheet,out/'deployment-arrow-ab.png')
    assert (HERE/'deploy.c').read_bytes()==active
    # The baseline must also reproduce the last review without refreshing it.
    for name in ('deploy-initial.png','deploy-full.png','deploy-confirm.png'):
        assert ImageChops.difference(Image.open(out/'current'/name).convert('RGB'),
               Image.open(HERE/'x/qa/flow/review-9/new'/name).convert('RGB')).getbbox() is None,name
    print('Arrow A/B:6 PC/68000 cases; variants confined to arrow; active source and baseline unchanged')
    print(out/'deployment-arrow-ab.png')

if __name__=='__main__':main()
