#!/usr/bin/env python3
"""Generate the measured comparison and engineering figures from raw benchmark CSVs."""
import csv,json,subprocess,struct,hashlib
from pathlib import Path
from PIL import Image,ImageDraw
HERE=Path(__file__).resolve().parents[1]
NAMES=['Height DDA','Filled polygons','Portal polygons','Plane floor hybrid','Hidden line']
def load(label):return list(csv.DictReader((HERE/f'BENCHMARK_{label}.csv').open()))
def pick(rows,scene='Tenchu B',res='80x50',sprites=5):
    return next(r for r in rows if r['scene']==scene and r['resolution']==res and int(r['sprites'])==sprites)
def f(row,key):return float(row[key])
def image_for(scenario,keys='keys/sweep.txt',frames=48):
    out=HERE/'bench-output/report';out.mkdir(exist_ok=True)
    shot=out/f'{scenario}_{Path(keys).stem}.png'
    subprocess.run([str(HERE/'tenchu_pc'),'--headless','--scenario',str(scenario),
                    '--frames',str(frames),'--keys',keys,'--shot',str(shot)],cwd=HERE,check=True,capture_output=True)
    return Image.open(shot).convert('RGB').resize((160,100),Image.Resampling.NEAREST)
def main():
    datasets=[load(f'final_phase{i}') for i in range(1,6)]
    p=HERE/'tenchu.89z';b=p.read_bytes();offset=struct.unpack_from('<I',b,60)[0]
    executable=int.from_bytes(b[offset+4:offset+6],'big')
    text='# Measured renderer comparison\n\n'
    text+='MC68000 datasheet-cycle emulation; AMS costs estimated. These are CPU estimates, not physical FPS. '
    text+='The live application is capped at 12.8 FPS; add the verified approximately 17,500-cycle LCD copy and interrupt costs. '
    text+='All moving runs use the same 48-frame yaw/pitch sweep, five/ten actors and three resolutions.\n\n'
    text+='## Tenchu common fixture, 80x50, five actors\n\n'
    text+='| Method | Mean cycles incl. update | Mean ms at 12 MHz | CPU FPS ceiling | Conservative minimum CPU FPS | Worst render ms |\n|---|---:|---:|---:|---:|---:|\n'
    for i,rows in enumerate(datasets):
        r=pick(rows)
        text+=f'| {NAMES[i]} | {int(r["average_cycles"]):,} | {r["average_ms_12mhz"]} | {r["cpu_fps_12mhz"]} | {r["minimum_cpu_fps"]} | {r["worst_render_ms"]} |\n'
    text+='\nMinimum CPU FPS and worst-frame estimates use worst render plus the reported steady-frame worst update, '
    text+='which need not occur on the same frame; initialization is outside these frame zones. '
    text+='Per-frame minimum/maximum render samples and stage totals are in each CSV; nested zones overlap. '
    text+='Worst render includes the first frame; ti-cycles zone maxima alone deliberately exclude that call.\n\n'
    text+='## All common scenes at 80x50\n\n| Scene / actors | Height | Filled | Portal | Floor | Lines |\n|---|---:|---:|---:|---:|---:|\n'
    for scene in ['Tenchu B','FFT A','MGS C','Vagrant D']:
        for actors in (5,10):
            text+=f'| {scene} / {actors} | '+' | '.join(pick(d,scene,sprites=actors)['cpu_fps_12mhz'] for d in datasets)+' |\n'
    text+='\nGeometry differences: Height DDA flattens the Tenchu roof, thickens the bridge into a single column, '
    text+='and cannot represent the Vagrant bridge underside. The other four share identical source faces. '
    text+='Portal cells are authored for the MGS fixture only; the other three are single cells. '
    text+='These are synthetic fixtures, not extracted FFT/MGS/Vagrant game levels.\n\n'
    text+='## Original training\n\n| Mode / resolution | Mean cycles | CPU FPS ceiling | Worst render ms |\n|---|---:|---:|---:|\n'
    for label,description in [('native_ray','Lossy heightfield'),('native_poly','Polygon camera motion'),('native_portal','Portal path / single cell'),('native_floor','Base plane floor hybrid'),('native_lines','Hidden-line camera motion'),('native_static','Polygon static inspection')]:
        for r in load(label):text+=f'| {description} / {r["resolution"]} | {int(r["average_cycles"]):,} | {r["cpu_fps_12mhz"]} | {r["worst_render_ms"]} |\n'
    text+='\nStatic averages include one expensive first render and 47 cached frames. They do not imply camera motion at that rate. '
    text+='The unmodified mesh renderer is too slow for a 10–15 FPS moving inspection mode. '
    text+='The ray adapter is faster but cannot be treated as geometrically equivalent: walls are thickened and slopes/layers are lost. '
    text+='29,638 bank bytes, 221 vertices, 156 planar-merged faces, 100 ledges; no stored room screens.\n\n'
    text+='The native portal path has one cell, so its overhead does not buy visibility savings. '
    text+='Native floor mapping textures only the base plane; elevated native faces remain flat shaded. '
    text+='Separate elevated mappings are implemented and tested in the common box fixtures.\n\n'
    text+='## Sprite scaling (all five/ten visible; no background reuse)\n\n| Method / resolution / actors | Sprite stage cycles | Scale stage cycles (included) | Cache hits / misses |\n|---|---:|---:|---:|\n'
    for label,name in [('sprites_precomputed','Baked A'),('sprites_runtime','Runtime NN B'),('sprites_cache','LRU C')]:
        for r in load(label):
            text+=f'| {name} / {r["resolution"]} / {r["sprites"]} | {r["sprites_cycles"]} | {r["scale_cycles"]} | {r["cache_hits"]} / {r["cache_misses"]} |\n'
    text+='\nAll methods deliberately use the same eight scale bins and two animation poses; their screens match. '
    text+='The cache test is a warm, repetitive workload, not a promise about animated production actors. '
    text+='Mixed moving workloads can be rerun with --sprite-mode 2 to expose misses/thrashing. '
    text+='Baked pixels cost 2,208 Flash bytes; the shared popcount table costs 256; metadata/raw source costs 137. '
    text+='Raw ink+mask costs 32 bytes/pose versus 1,104 baked bytes/pose. Four cache slots allocate 2,064 bytes. '
    text+='The all-method harness allocates that cache in A/B too so methods can be switched; this is actual harness RAM, '
    text+='not a claimed per-method saving. A specialized A/B build could omit it, but that saving is not measured here.\n\n'
    moving=load('sprites_cache_moving')
    m5=pick(moving,scene='Sprite fixture',sprites=5)
    m10=pick(moving,scene='Sprite fixture',sprites=10)
    rate=lambda r:100*int(r['cache_hits'])/(int(r['cache_hits'])+int(r['cache_misses']))
    text+='The additional moving-camera cache fixture has four misses at each resolution: '
    text+=f'{m5["cache_hits"]} hits / {m5["cache_misses"]} misses for five actors and '
    text+=f'{m10["cache_hits"]} / {m10["cache_misses"]} for ten at 80x50 '
    text+=f'({rate(m5):.1f}% / {rate(m10):.1f}% hit rates). Some actors leave the view during that sweep; '
    text+='the static fixture above is the all-visible comparison. Neither workload exhausts '
    text+='all eight sizes and many animation poses simultaneously.\n\n'
    text+='## Floor and presentation variants\n\n| Variant / Tenchu B / 80x50 / five actors | Mean cycles | CPU FPS ceiling |\n|---|---:|---:|\n'
    for label,name in [('floor_half','Two-pixel floor sampling'),('floor_flat','Flat floor'),('wireframe','Full wireframe'),('silhouette','Black silhouette'),('sparse_dither','Sparse dither + outlines')]:
        r=pick(load(label));text+=f'| {name} | {r["average_cycles"]} | {r["cpu_fps_12mhz"]} |\n'
    text+='\nFlat ground wins this implementation. Outline/hidden-line work is not inherently cheaper than filled spans. '
    text+='Full wireframe shows hidden structures and is unsuitable for depth-valid aiming. Silhouettes lose interior ledge cues. '
    text+='Readability observations are qualitative engineering review, not a user preference study.\n\n'
    text+='## Memory, executable and tables\n\n'
    text+=f'All-method transfer file: {len(b):,} bytes; executable variable data: {executable:,}; stock AMS limit: 24,576. '
    text+=f'Current cycle binary SHA-256: `{hashlib.sha256((HERE/"tenchuc.89z").read_bytes()).hexdigest()}`.\n\n'
    text+='Tracked heap workspaces: world 1,028; framebuffer/cache/column spans 8,042; polygon/portal 8,644; sprite cache 2,064 = **19,778 bytes**. '
    text+='Live mono runtime adds 3,840 bytes: **23,618**, excluding program, AMS allocations, globals and stack. '
    text+='Reserve roughly 2 KB for nested rendering stack as a conservative allowance, not a measured stack high-water mark. '
    text+='ti-cycles allocates two planes for its generic screenshot path, so its runtime allocation is larger than live mono. '
    text+='Archive banks consume no copied data RAM; unarchived banks additionally occupy their full payloads. '
    text+='The 4,808-byte sine/reciprocal/row tables live inside the executable and are included in its size.\n\n'
    standalone=HERE/'teray.89z'
    if standalone.exists():
        text+=f'Standalone height transfer file: {standalone.stat().st_size:,} bytes. Polygon workspace is zero; '
        text+='live tracked workspace including runtime plane is **14,974 bytes**. Remaining polygon methods share the 8,644-byte workspace '
        text+='in this implementation, so measured RAM efficiency ranks height first and the other four tied.\n\n'
    text+='## Ranking and production recommendation\n\n'
    average=[]
    for i,rows in enumerate(datasets):
        subset=[f(r,'average_cycles') for r in rows if r['resolution']=='80x50' and r['sprites']=='5']
        average.append((sum(subset)/len(subset),NAMES[i]))
    text+='Measured performance, average of the four common half-resolution five-actor scenes: '+', '.join(n for _,n in sorted(average))+'. '
    text+='Original training ordering is different: lossy heightfield, filled polygons, single-cell portal path, base-floor hybrid, hidden line.\n\n'
    text+='| Criterion | Ranking / qualification |\n|---|---|\n'
    text+='| Geometry | Filled / portal / floor / hidden-line tied for expressible polygons; heightfield last. All painter paths share sorting limitations. |\n'
    text+='| Readability | Filled+clear ledge markers preferred in review; hidden lines clarify ledges but cost more; silhouette and wireframe sacrifice cues; busy checker floors add noise. |\n'
    text+='| Complexity | Height, filled, hidden-line, floor hybrid, portals (graph/general preprocessing increases complexity). |\n'
    text+='| Reuse | Filled geometry best baseline; portal optimization where real room cells exist; height useful for restricted MGS modes; floor/lines remain optional styles. |\n'
    text+='\nUse compact archived geometry, monochrome flat spans, early world-plane rejection, quantized baked sprites and last-frame reuse. '
    text+='For Tenchu, retain true geometry for ledge/roof inspection and implement native-map visibility before production; use the cheaper '
    text+='height adapter only where its losses are acceptable. For MGS-like interiors, portals can reduce work; the native training has no such '
    text+='measured optimization yet. Prefer simple flat floors over software Mode 7. Hidden line is an optional presentation, not the speed winner. '
    text+='No recommendation assumes full Z buffering, pre-rendered room orientations, modern caches or SIMD. Physical timing and a complete '
    text+='training route remain prerequisites for a production claim.\n'
    (HERE/'BENCHMARK.md').write_text(text)
    sheet=Image.new('RGB',(800,4*130),'white');draw=ImageDraw.Draw(sheet)
    for scene in range(4):
        for phase in range(5):
            scenario=scene+4+(phase<<4);x=phase*160;y=scene*130
            sheet.paste(image_for(scenario),(x,y+28))
            draw.text((x+3,y+2),NAMES[phase],fill='black')
            draw.text((x+3,y+14),['Tenchu B','FFT A','MGS C','Vagrant D'][scene],fill='black')
    (HERE/'bench-output/report').mkdir(exist_ok=True)
    sheet.save(HERE/'bench-output/report/common_scenes.png')
    image_for(1044,'keys/static.txt',8).save(HERE/'local/training_mono.png')
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        fig,ax=plt.subplots(figsize=(9,4))
        ax.bar(NAMES,[f(pick(d),'average_cycles')/1e6 for d in datasets],color='#555555')
        ax.set_ylabel('Million MC68000 cycles/frame');ax.set_title('Tenchu fixture, 80x50, 5 actors, moving camera')
        ax.tick_params(axis='x',labelsize=8);fig.tight_layout();fig.savefig(HERE/'bench-output/report/cycles.png',dpi=160)
    except ImportError:pass
    print(HERE/'BENCHMARK.md')
if __name__=='__main__':main()
