#!/usr/bin/env python3
"""Execute genuine 68000 binaries; never substitute host timings. CSV + raw logs.
Clock conversions are estimates at 12 MHz HW2 (also report 10 MHz sensitivity).
"""
import argparse,csv,json,re,subprocess,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[1]

def run(cmd):
    p=subprocess.run([str(x) for x in cmd],cwd=HERE,text=True,capture_output=True,check=True)
    return p.stdout

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--phase',type=int,default=1)
    ap.add_argument('--frames',type=int,default=48)
    ap.add_argument('--label',default='current')
    ap.add_argument('--sprite-mode',type=int,choices=(0,1,2),default=0)
    ap.add_argument('--binary',default='tenchuc.89z')
    ap.add_argument('--pc',default='tenchu_pc')
    ap.add_argument('--keys',default='keys/sweep.txt')
    ap.add_argument('--native',action='store_true')
    ap.add_argument('--sprites-only',action='store_true')
    ap.add_argument('--style',type=int,choices=range(4),default=0)
    a=ap.parse_args()
    out=HERE/'bench-output'/a.label
    out.mkdir(parents=True,exist_ok=True)
    rows=[]
    for scene in (range(1) if a.native or a.sprites_only else range(4)):
        for res in range(3):
            for sprites in ((0,) if a.native else (5,10)):
                scenario=scene+(res<<2)+((a.phase-1)<<4)+(512 if sprites==10 else 0)+(a.sprite_mode<<12)+(1024 if a.native else 0)+(2048 if a.sprites_only else 0)+(a.style<<14)
                args=[ROOT/'tools/bin/ti-cycles','--arg',scenario,'--frames',a.frames,
                      '--max',1000,'--keys',a.keys,'--file','tnspr.89y','--png',out/f'{scenario}.png',a.binary]
                if a.native:args[-1:-1]=['--file','tnmap.89y']
                log=run(args)
                (out/f'{scenario}.log').write_text(log)
                if '_main returned' not in log:raise RuntimeError(f'incomplete run {scenario}')
                zones={int(n):int(total)/a.frames for n,total in re.findall(r'^(\d+)\s+.+?\s+\d+\s+(\d+)\s+\d+\s*$',log,re.M)}
                maxima={int(n):int(v) for n,v in re.findall(r'^max\s+(\d+)\s+.+?\s+(\d+) at call',log,re.M)}
                ti=re.search(r'shot 0 checksum (\w+)',log).group(1)
                pc=run([HERE/a.pc,'--headless','--scenario',scenario,'--frames',a.frames,'--keys',a.keys,'--shot',out/f'{scenario}_pc.png'])
                host=re.search(r'checksum (\w+)',pc).group(1)
                if ti!=host:raise RuntimeError(f'TI/PC mismatch {scenario}: {ti}/{host}')
                tagged=[int(h,16) for h in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',log)]
                samples=[v&0xffffff for v in tagged if v>>24==0x5f]
                if len(samples)!=a.frames:raise RuntimeError('missing per-frame cycle samples')
                overflow=[v&0xffffff for v in tagged if v>>24==0x43]
                if any(overflow):raise RuntimeError(f'capacity overflow scenario {scenario}')
                counts=lambda tag:[v&0xffffff for v in tagged if v>>24==tag]
                avg=zones[2]+zones[1]
                # ti-cycles intentionally excludes call zero from zone maxima.
                # Include our first-frame sample, especially for cached views.
                worst_render=max(maxima.get(2,0),max(samples))
                row=dict(phase=a.phase,scene='Original training' if a.native else 'Sprite fixture' if a.sprites_only else ('Tenchu B','FFT A','MGS C','Vagrant D')[scene],
                         sprite_mode=a.sprite_mode,
                         resolution=('160x100','80x50','80x100')[res],sprites=sprites,
                         scenario=scenario,frames=a.frames,average_cycles=round(avg),
                         worst_render_cycles=worst_render,average_ms_12mhz=round(avg/12000,3),
                         cpu_fps_12mhz=round(12000000/avg,2),worst_render_ms=round(worst_render/12000,3),
                         minimum_render_cycles=min(samples),maximum_render_cycles=max(samples),
                         worst_update_cycles=maxima[1],
                         worst_frame_estimate_cycles=worst_render+maxima[1],
                         worst_frame_estimate_ms=round((worst_render+maxima[1])/12000,3),
                         minimum_cpu_fps=round(12000000/(worst_render+maxima[1]),2),
                         average_visible_walls=round(sum(counts(0x41))/a.frames,2),
                         average_visible_polygons=round(sum(counts(0x44))/a.frames,2),
                         maximum_visible_polygons=max(counts(0x44)),
                         cache_hits=sum(counts(0x48)),cache_misses=sum(counts(0x49)),
                         cpu_fps_10mhz=round(10000000/avg,2),checksum=ti)
                for n,nam in [(3,'walls'),(4,'sprites'),(5,'present'),(6,'clear'),(7,'aim'),(8,'project'),(9,'visibility'),(10,'fill'),(11,'floor'),(12,'background'),(13,'occlusion'),(14,'scale')]:
                    row[nam+'_cycles']=round(zones.get(n,0))
                rows.append(row)
                print(f'{a.phase}: {row["scene"]} {row["resolution"]} {sprites} sprites: {row["average_cycles"]} cyc, {row["cpu_fps_12mhz"]} estimated fps; TI=PC')
    target=HERE/f'BENCHMARK_{a.label}.csv'
    with target.open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=rows[0],lineterminator='\n');w.writeheader();w.writerows(rows)
    (out/'manifest.json').write_text(json.dumps(dict(binary_bytes=(HERE/a.binary).stat().st_size,binary_sha256=hashlib.sha256((HERE/a.binary).read_bytes()).hexdigest(),phase=a.phase,frames=a.frames,keys=a.keys,sprite_mode=a.sprite_mode,style=a.style,clock='MC68000 datasheet, ROM costs estimated; no interrupts or LCD measured'),indent=2)+'\n')
    print(target)
if __name__=='__main__':main()
