#!/usr/bin/env python3
"""Private PC area-impact candidate: shared source engine, no native build writes."""
import argparse,hashlib,json,re,struct,tempfile
from pathlib import Path
from PIL import Image,ImageDraw
from check import HERE,ROOT,run
from check_flow import data_links
from review_files import save_image,write_changed


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def pair(shots,kind,frame,label):
    picture=Image.new('RGB',(984,328),'white');d=ImageDraw.Draw(picture)
    d.text((5,5),'Before: current impact / common review camera',fill='black')
    d.text((497,5),'Candidate: '+label,fill='black')
    for baseline,x in ((1,0),(0,492)):
        with Image.open(shots/f'{kind}-{baseline}-{frame:03}.png')as source:
            picture.paste(source.convert('RGB').resize((480,300),Image.Resampling.NEAREST),(x,24))
    return picture


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=HERE/'x/core-area-impact-review')
    args=ap.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    if (out/'approval.json').exists():
        for name,digest in json.loads((out/'approval.json').read_text())['artifacts'].items():
            assert sha(out/name)==digest
        print('Accepted impact review (read only):',out);return
    protected=[HERE.parent/p for p in ('fft.c','fft.h','module.h','batch_prepare.inc','batch_commit.inc')]
    protected+=[HERE/'data'/p for p in ('ffbm.bin','ffbn.bin','ffcj.bin','modules.h')]
    protected+=[HERE/'x/fftcampaign.89g',HERE/'x/core-area-ui/fftcampaign.89g']
    accepted=HERE/'x/core-area-ui-review'
    protected += [accepted/p for p in ('area-target-review.png','area-target-glove.gif')]
    before={str(p):dict(sha256=sha(p),mtime_ns=p.stat().st_mtime_ns)for p in protected}
    text=(HERE.parent/'fft.c').read_text()
    hooks={
        'static u8 hit_flash(u8 i)':'static u8 original_hit_flash(u8 i)',
        'static u8 unit_pose(u8 i)':'static u8 original_unit_pose(u8 i)',
        'static void draw_number(void)':'static void original_draw_number(void)',
        'return st.mode == M_ACT && i == st.tgt ? hit_flash(i) : 0;':
            'return st.mode == M_ACT ? hit_flash(i) : 0;',
        'if (st.mode == M_ACT && i == st.tgt && hit_flash(i)) {':
            'if (st.mode == M_ACT && hit_flash(i)) {',
        'if (st.mode == M_ACT && st.act_t >= ACT_HIT) draw_number();':
            'impact_effect();\n    if (st.mode == M_ACT && st.act_t >= ACT_HIT) draw_number();'}
    for old,new in hooks.items():assert text.count(old)==1,old;text=text.replace(old,new)
    # The source include path is fixed so both fixtures and diagnostics remain reproducible.
    write_changed(HERE/'x/core-area-impact-review/engine.inc',text.encode())
    source=[HERE/'preview_area_impact.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']
    for name,flags in [('preview',[]),('preview-san',['-fsanitize=address,undefined','-fno-sanitize=shift'])]:
        run(['cc','-std=gnu99','-O1','-g','-w','-I'+str(HERE.parent),'-o',out/name,*flags,*source])
    labels=['Empty centre, two victims','Hit and Miss','Friendly fire',
            'Absorbed healing','No victims: ground pulse only','KO and Oil removed',
            'Victory waits for settlement','Auto-Potion; no repeated root effect']
    with tempfile.TemporaryDirectory(prefix='fft-impact-review-')as folder:
        data=Path(folder);data_links(data);shots=data/'shots';shots.mkdir();san=data/'san';san.mkdir()
        raw=bytearray((HERE/'data/ffcj.bin').read_bytes());norm=struct.unpack_from('>H',raw,30)[0]
        raw[norm+16*12:norm+17*12]=struct.pack('>8BH2B',0,14,4,255,128|32|1|2,255,100,0,50,8,128)
        (data/'ffcj.bin').unlink();(data/'ffcj.bin').write_bytes(raw)
        pc=run([out/'preview',shots],data);(out/'pc.log').write_text(pc)
        sanitized=run(['env','ASAN_OPTIONS=detect_leaks=0',out/'preview-san',san],data)
        assert sanitized==pc
        frames=list(shots.glob('*.png'))
        for p in frames:assert p.read_bytes()==(san/p.name).read_bytes(),p.name
        sheet=Image.new('RGB',(984,4*328),'white')
        for kind,label in enumerate(labels):
            tile=pair(shots,kind,30,label)
            # Two columns of comparisons would make the160x100 details too small.
            # Keep four representative comparisons here; all eight run in the GIFs.
            if kind in (0,1,3,4):sheet.paste(tile,(0,(0,1,3,4).index(kind)*328))
        save_image(sheet,out/'area-impact-review.png')
        for name,kinds in [('area-impact-targets.gif',range(4)),('area-impact-empty-status.gif',range(4,8))]:
            gif=[]
            for kind in kinds:
                for frame in range(0,160 if kind==7 else 96,2):gif.append(pair(shots,kind,frame,labels[kind]))
            save_image(gif[0],out/name,save_all=True,append_images=gif[1:],duration=66,loop=0,disposal=2)
    for name,info in before.items():
        p=Path(name);assert sha(p)==info['sha256']and p.stat().st_mtime_ns==info['mtime_ns'],name
    m=re.search(r'area_impact_review (\w+) failures0 checks(\d+)',pc);assert m,pc
    names=['battle_effect_store_targets_and_display_types.c','battle_effect_run_keyframe_actions_all_targets.c',
           'battle_action_build_targets_post_action_message.c']
    report=dict(status='private visual candidate; approval pending',checks=int(m[2]),hash=m[1],
        sanitizers_equal=True,compared_pc_frames=len(frames),generated_engine_hooks=len(hooks),
        private_normalized_record=16,retail_casts_enabled=False,native_benchmarked=False,
        live_tiemu_touched=False,protected=before,source_sha256=sha(HERE/'preview_area_impact.c'),
        scope='Existing compound preparation/commit/response/reward rules, common review camera; per-victim labels/poses/flash, depth-masked centre pulse, Oil message. Not original spell-art/timing fidelity or a native scratch budget.',
        static_source_sha256={name:sha(ROOT/'sources/fft_decomp/src/battle'/name)for name in names})
    (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
    print(pc[-800:]);print('Review:',out/'area-impact-review.png',out/'area-impact-targets.gif',out/'area-impact-empty-status.gif')


if __name__=='__main__':main()
