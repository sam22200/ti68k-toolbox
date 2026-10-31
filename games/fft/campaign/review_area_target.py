#!/usr/bin/env python3
"""Private actual-renderer area UI review; no active assets/build or emulator writes."""
import argparse,hashlib,json,re,struct,tempfile
from pathlib import Path
from PIL import Image,ImageDraw
from check import HERE,ROOT,run
from check_flow import data_links
from review_files import publish_images,save_image,write_changed


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/core-area-ui-review')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    # Accepted prototypes are historical evidence, not regeneration inputs for
    # the integrated engine. Keep their bytes and dates fixed after approval.
    if (out/'approval.json').exists():
        approval=json.loads((out/'approval.json').read_text())
        for name,digest in approval['artifacts'].items():
            assert sha(out/name)==digest,name
        print('Accepted review (read only):',out/'area-target-review.png',out/'area-target-glove.gif')
        return
    engine=HERE.parent/'fft.c';catalogue=HERE/'data/ffcj.bin'
    protected=[engine,HERE.parent/'fft.h',catalogue,HERE/'data/ffbm.bin',HERE/'data/ffbn.bin',HERE/'x/fftcampaign.89g']
    before={str(p):dict(sha256=sha(p),mtime_ns=p.stat().st_mtime_ns)for p in protected}
    text=engine.read_text();needle='else if (!st.morph_mask) draw_hud();'
    assert text.count(needle)==1
    write_changed(HERE/'x/core-area-ui-review/engine.inc',text.replace(needle,'else if (!st.morph_mask) area_review_hud();').encode())
    source=[HERE/'preview_area_target.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
    run(['cc','-std=gnu99','-O1','-g','-w','-I'+str(HERE.parent),'-o',out/'preview',*source])
    run(['cc','-std=gnu99','-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=shift',
         '-I'+str(HERE.parent),'-o',out/'preview-san',*source])
    with tempfile.TemporaryDirectory(prefix='fft-area-review-')as name:
        temp=Path(name);data_links(temp);shots=temp/'shots';shots.mkdir();san=temp/'san';san.mkdir()
        raw=bytearray(catalogue.read_bytes());offset=struct.unpack_from('>H',raw,30)[0]
        raw[offset+16*12:offset+17*12]=struct.pack('>8BH2B',0,14,4,255,128|32|1|2,255,100,0,50,8,128)
        (temp/'ffcj.bin').unlink();(temp/'ffcj.bin').write_bytes(raw)
        pc=run([out/'preview',shots],temp);(out/'pc.log').write_text(pc)
        sanitized=run(['env','ASAN_OPTIONS=detect_leaks=0',out/'preview-san',san],temp)
        assert sanitized==pc
        for path in shots.glob('*.png'):assert path.read_bytes()==(san/path.name).read_bytes()
        publish_images(shots,out/'images')
    for path,info in before.items():
        p=Path(path);assert sha(p)==info['sha256']and p.stat().st_mtime_ns==info['mtime_ns'],path
    m=re.search(r'area_review (\w+) failures0 checks(\d+)',pc);assert m,pc
    images=out/'images'
    entries=[('before-empty.png','Before: empty centre / existing battle HUD'),
             ('range-empty.png','Proposed: select a panel'),
             ('area-empty.png','Proposed: two targets around an empty centre'),
             ('confirm-empty.png','Confirm: fixed panel / Cancel'),
             ('area-ally.png','Friendly fire: ally counted and inspectable'),
             ('confirm-unit.png','Occupied centre: Unit / Panel / Cancel'),
             ('area-absorb.png','Absorption: signed healing forecast'),
             ('area-none.png','Empty footprint: explicit zero targets')]
    sheet=Image.new('RGB',(984,4*328),'white');draw=ImageDraw.Draw(sheet)
    for index,(filename,label)in enumerate(entries):
        x=(index%2)*492;y=(index//2)*328;draw.text((x+5,y+5),label,fill='black')
        sheet.paste(Image.open(images/filename).convert('RGB').resize((480,300),Image.Resampling.NEAREST),(x,y+24))
    save_image(sheet,out/'area-target-review.png')
    frames=[]
    for i in range(32):
        frame=Image.new('RGB',(984,328),'white');d=ImageDraw.Draw(frame)
        d.text((5,5),'Before: existing empty-centre aiming',fill='black')
        d.text((497,5),'Proposed: common animated confirmation menu',fill='black')
        frame.paste(Image.open(images/'before-empty.png').convert('RGB').resize((480,300),Image.Resampling.NEAREST),(0,24))
        frame.paste(Image.open(images/f'glove-{i:02}.png').convert('RGB').resize((480,300),Image.Resampling.NEAREST),(492,24))
        frames.append(frame)
    save_image(frames[0],out/'area-target-glove.gif',save_all=True,append_images=frames[1:],duration=32,loop=0,disposal=2)
    report=dict(status='private visual candidate; approval pending',checks=int(m[2]),hash=m[1],
        sanitizers_equal=True,rendered_pc_frames=len(list(images.glob('*.png'))),engine_hud_hook='one generated-copy call site',
        private_normalized_record=16,retail_casts_enabled=False,native_benchmarked=False,
        live_tiemu_touched=False,protected=before,source_sha256=sha(HERE/'preview_area_target.c'),
        scope='PC controller, source-derived radial footprint, shared pure forecasts/cards/menu, tile/unit pending binding and cancellation; not integrated impact presentation')
    names=['battle_state_handle_targeting_range_state.c','battle_target_select_tile.c',
           'battle_action_confirm.c','battle_state_handle_confirm_action_state.c',
           'battle_target_set_panels_for_action.c']
    report['static_source_sha256']={name:sha(ROOT/'sources/fft_decomp/src/battle'/name)for name in names}
    (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
    print(pc[-700:],flush=True);print('Review:',out/'area-target-review.png',out/'area-target-glove.gif',flush=True)


if __name__=='__main__':main()
