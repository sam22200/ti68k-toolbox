#!/usr/bin/env python3
"""Write the measured actor/wardrobe study and accepted eight-pack planning report.

Requires the existing read-only pack/text/map caches and actor_storage_study
evidence. Does not compile, extract art, write saves or operate an emulator.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def number(n):
    return f'{n:,}'


def main():
    ap = argparse.ArgumentParser(__doc__)
    ap.add_argument('--study', type=Path, default=HERE/'campaign/x/actor-storage')
    ap.add_argument('--baseline', type=Path, default=HERE/'campaign/x/pack-refinement')
    ap.add_argument('--output', type=Path, default=HERE/'campaign/CAMPAIGN_PACK_REPORT.md')
    args = ap.parse_args()
    study, baseline = args.study.resolve(), args.baseline.resolve()
    actors = json.loads((study/'results.json').read_text())
    costumes = json.loads((study/'costume-choices.json').read_text())
    prior = json.loads((baseline/'plan.json').read_text())
    text = json.loads((baseline/'text-cast.json').read_text())
    scene = json.loads((baseline/'scenes.json').read_text())
    map_names = json.loads((HERE/'campaign/x/core-audit/map-viewer-names.json').read_text())
    packages = json.loads((HERE/'campaign/x/core-abi99-release/packages/packages.json').read_text())
    pc = json.loads((study/'pc.json').read_text())
    native_log = (study/'native.log').read_text()
    values = [int(x) for x in re.findall(r'^value: (\d+)', native_log, re.M)]
    assert values == [pc['fixtures'], pc['failures'], pc['signature']]
    assert pc['failures'] == 0 and pc['cases'] == 270
    cycles = {}
    for mode, calls, total, mean in re.findall(r'^(\d)\s+.+?\s+(\d+)\s+(\d+)\s+(\d+)\s*$', native_log, re.M):
        cycles[int(mode)] = dict(calls=int(calls), total=int(total), mean=int(mean))
    for mode, peak in re.findall(r'^max (\d)\s+.+?\s+(\d+) at call', native_log, re.M):
        cycles[int(mode)]['peak'] = int(peak)
    assert set(cycles) == {1, 2, 3}

    #50KB is interpreted as a WHOLE-CAMPAIGN optional-wardrobe cap, not50KB
    # for each character. Count four optional portrait records and references.
    wardrobe_body = costumes['profiles']['extended_regular']['ramza_delita']['whole']['increment_bytes']
    wardrobe_portraits = 4*348
    wardrobe_refs = 4*8
    wardrobe_measured = wardrobe_body + wardrobe_portraits + wardrobe_refs
    wardrobe_cap = 50000
    assert wardrobe_measured < wardrobe_cap
    all_outfits = actors['increments']['extended_regular']['whole']['six_story_outfits']
    all_outfits_with_faces = all_outfits + 6*(348+8)
    assert all_outfits_with_faces > wardrobe_cap

    phases = ['I', 'II-a', 'II-b', 'III-a', 'III-b', 'IV-1', 'IV-2', 'IV-3']
    labels = ['I', 'II-A', 'II-B', 'III-A', 'III-B', 'IV-A', 'IV-B', 'IV-C']
    endings = ['Zeakden complete', 'Goug rescue complete, before Warjilis',
               'Lionel complete', 'Orbonne sequence complete, before Grog Hill',
               'Riovanes complete', 'Bethla sluice/rescue/aftermath complete',
               'Igros complete', 'St. Murond through the ending']
    projection = {}
    for key, label, end in zip(phases, labels, endings):
        old = prior['packs'][key]
        count = old['scene_locations']
        # Eight-pack boundaries do not change. Extra-field choices were
        # illustrative; final pack retains one instead of two for a firm margin.
        fields = list(old['extra_random_examples'])
        removal = [0, 0]
        if key == 'IV-3':
            fields.remove(86)  # Doguola320x200:64000 raw colors +8..18KB depth
            removal = [76000, 89000]  # include its4..7KB metadata allowance
        conservative = [old['with_reserve'][i] + (wardrobe_measured if i == 0 else wardrobe_cap) - removal[i]
                        for i in range(2)]
        # A single direction still may have several camera crops. Keep30KB
        # per-pack upper allowance for additional scene pictures/framing.
        one_view = [5000*count, 10000*count+30000]
        updated = [conservative[i] - old['breakdown']['scene_only_pictures'][i] + one_view[i]
                   for i in range(2)]
        projection[key] = dict(label=label, ending=end, story_maps=old['story_maps'],
            extra_random=fields, scene_maps=old['scene_only_maps'],
            full_map_count=len(old['story_maps'])+len(fields), scene_locations=count,
            wardrobe_addition=[wardrobe_measured, wardrobe_cap],
            conservative_with_reserve=conservative, one_orientation_with_reserve=updated,
            reserve_bytes=prior['reserve'], one_orientation_picture_allowance=one_view,
            optional_field_removal=removal)
    peak = max(p['one_orientation_with_reserve'][1] for p in projection.values())
    conservative_peak = max(p['conservative_with_reserve'][1] for p in projection.values())
    assert conservative_peak < 2700000 and peak < conservative_peak
    summary = dict(schema='FFT_MEASURED_PACK_REPORT_1', date='2026-11-24',
                   accepted_main_packs=8, threshold=wardrobe_cap,
                   wardrobe=dict(body_increment=wardrobe_body, portrait_allowance=wardrobe_portraits,
                                 reference_allowance=wardrobe_refs, total=wardrobe_measured,
                                 cap=wardrobe_cap, all_six_outfits=all_outfits_with_faces),
                   pc=pc, native_values=values, cycles=cycles, packs=projection,
                   conservative_peak=conservative_peak, one_orientation_peak=peak,
                   inputs={str(p.relative_to(ROOT)):sha(p) for p in
                           [study/'results.json', study/'costume-choices.json', study/'pc.json',
                            study/'native.log', baseline/'plan.json', baseline/'text-cast.json', baseline/'scenes.json']})
    (study/'pack-projection.json').write_text(json.dumps(summary, indent=2)+'\n')

    failures = Counter(error for m in actors['models'] for _, error in m['failures'])
    empty = sum(n for error, n in failures.items() if 'index 0 is out of bounds' in error)
    actual_failures = sum(failures.values())-empty
    lines = [
        '# FFT campaign: measured storage and accepted pack plan', '',
        'Date:2026-11-24. Target: TI-89 Titanium, retail PS1 US campaign.', '',
        '## Decisions', '',
        '- Keep the user-approved **eight main packs**: I whole; II split after Goug; III after Orbonne; IV after Bethla and after Igros.',
        '- Plan separate optional quest and Deep Dungeon packs. Content-pack phase never changes the original story chapter or town progression.',
        '- Use one orientation per scripted scene by default. Keep extra source-required directions, camera crops, pans and gestures.',
        '- Keep complete unique appearances for recruits and major story actors. Pool identical sprites, poses and mirrored frames physically.',
        '- Keep complete sprites in the combat renderer. Naive lossless two/three-part preparation is slower; it does not save space in the native30 corpus.',
        f'- Provisionally retain **Ramza and Delita wardrobe changes**: {number(wardrobe_measured)} bytes including body probes and portrait/reference allowances, under the user\'s50,000-byte whole-campaign cap. Budget the full50,000. If final required poses exceed that cap, return to one normal body per person.',
        f'- Do not retain the whole six-outfit set by default: {number(all_outfits_with_faces)} bytes with the same allowances exceeds the cap. Same-name wounded/guest/undead variants are not all ordinary clothes.',
        '- Reduce scene-only actors to their actual scripted poses when authoring. No generic head/body replacement has been visually approved or integrated.',
        '',
        'This report measures archive encodings and a bounded pose-preparation prototype. '
        'It does not install new banks, rip the complete campaign, change the shipping renderer, '
        'or certify final Flash/RAM fit. ABI99 and all reviewed art remain unchanged.',
        '', '## 1. Available storage and measured release', '',
        'TI specifies about2.7MB of user Flash shared by archived variables and apps. '
        'This is a maximum capacity, not measured free space on the user\'s calculator. '
        'Installed apps/files reduce it. '
        '[TI capacity table](https://education.ti.com/en/customer-support/knowledge-base/ti-83-84-plus-family/general-information/12174).', '',
        '| Item | Bytes | Status |', '|---|---:|---|',
        f'| Promoted ABI99 core, stored variables/size words | {number(packages["core"]["stored_variable_bytes"])} | Measured |',
        f'| Same core, transport file | {number(packages["core"]["bytes"])} | Measured; excluded from archive sums |',
        '| Planning ceiling | 2,700,000 | Configurable assumption, not live free Flash |',
        '| Flash/VAT/save reserve | 131,072 | Retained;128KiB |',
        f'| Wardrobe body increment, Ramza+Delita | {number(wardrobe_body)} | Extended converted-pose corpus |',
        f'| Optional four portrait records + references | {number(wardrobe_portraits+wardrobe_refs)} | Conservative allowance |',
        '',
        'MB/KB in this report are decimal. KiB means1,024 bytes. Actor-study stored '
        'counts include compressed streams, u16 offsets, mirrored logical mappings, '
        'recipe/part headers and bounded variable DAT/size overhead. AMS sector '
        'fragmentation and other installed software are not measured.',
        '', '## 2. Human bodies, parts and costume measurements', '',
        'The corpus reads52 named-human identity records (38 logical people) and38 generic '
        'job bodies from the local disc. Each pose uses the existing16x26,208-byte '
        'four-plane converter. Preserve white outlines, transparency and the internal hit mask. '
        'The study changes storage only and reconstructs every retained input pose byte for byte.', '',
        'Two coverage controls are reported:', '',
        '- **native30:** the previous thirty-pose proxy, using the same existing source-frame list.',
        '- **extended_regular:** all regular-body SHP frames plus native30 for named bodies; generic jobs remain native30. '
        'This includes many unused gestures as a wardrobe stress case. It is not a complete source-SEQ/attack/SP2 animation rip.', '',
        f'There are{sum(m["converted"] for m in actors["models"]):,} converted model/pose pairs. '
        f'{empty:,} empty source-shape attempts are explicit;{actual_failures} other attempts fail the existing converter '
        '(Rudvich pose24 and male Summoner pose86, both eye-position bounds). '
        'They are not repaired or silently treated as certified animations. '
        'The source-sized model profiles are storage controls, not approved final sprite sheets.', '',
        '### 2.1 Lossless whole sprites versus row parts', '',
        'The two-part control splits rows0..7 /8..25. The three-part control splits '
        'rows0..7 /8..17 /18..25. These fixed bands are **not anatomical heads, torsos '
        'and feet for arbitrary attacks or KO poses**. Parts keep all four planes and '
        'follow the canonical whole-frame orientation. Each unique complete pose has '
        'an explicit2/3-ID recipe; no free descriptors are assumed.', '',
        '| Corpus:38 canonical named +38 generic bodies | Whole | Two parts | Three parts |',
        '|---|---:|---:|---:|']
    for profile in ('native30', 'extended_regular'):
        row = actors['cases'][profile]['canonical_plus_jobs']
        lines.append('| '+profile+' | '+' | '.join(number(row[k]['stored_bytes']) for k in ('whole','two','three'))+' |')
    lines += ['',
        'For native30, splitting adds20,376 /28,168 bytes. For extended_regular, '
        'two parts add12,794 bytes and three save9,133 bytes. The larger named-only '
        'library has more repeated rows, but that does not make all combat actors '
        'cheaper or faster. Important characters retain exactly their source-converted '
        'appearance in every lossless case.', '',
        '### 2.2 Hybrid scopes', '',
        '| Profile | Actors split; everyone else whole | Two-part saving | Three-part saving |',
        '|---|---|---:|---:|']
    for profile, scopes in costumes.get('hybrid_parts', {}).items():
        for scope, result in scopes.items():
            lines.append(f'| {profile} | {scope} | {number(result["two"]["saving_vs_all_whole"])} | {number(result["three"]["saving_vs_all_whole"])} |')
    lines += ['',
        'Negative savings mean a larger bank. Hybrid counts include separate whole/part '
        'dictionaries. Do not add their savings together: these are alternative encodings.', '',
        '### 2.3 Head substitution: archive potential, not approved art', '',
        '| Corpus | Full canonical bodies + jobs | Count-only shared-body option | Potential difference |',
        '|---|---:|---:|---:|']
    for profile, result in actors['head_substitution_count_only'].items():
        full = actors['cases'][profile]['canonical_plus_jobs']['whole']['stored_bytes']
        lines.append(f'| {profile} | {number(full)} | {number(result["hypothetical_total"])} | {number(result["saving_vs_full"])} |')
    lines += ['',
        'This counterfactual keeps18 important/recruitable people fully unique and '
        'counts unique upper bands for20 secondary people against an already required '
        'generic body catalogue. It does not select compatible body templates, rig head '
        'anchors, repair outline seams or approve altered silhouettes. No changed '
        'character picture was integrated or used in the CPU benchmark. '
        '**No head-substitution saving is credited to the pack budgets.**', '',
        '### 2.4 Wardrobe cap', '',
        'Interpret the user threshold as50,000 additional stored bytes across the '
        'whole campaign, not50KB per person. Compare complete sprites; do not rely '
        'on a new part format simply to cross the threshold.', '',
        '| Optional set | native30 body increment | Extended body increment | Extended + portrait/reference allowance |',
        '|---|---:|---:|---:|']
    for key, count in [('ramza_only',2), ('delita_only',2), ('ramza_delita',4), ('wiegraf_only',1)]:
        lo = costumes['profiles']['native30'][key]['whole']['increment_bytes']
        hi = costumes['profiles']['extended_regular'][key]['whole']['increment_bytes']
        lines.append(f'| {key} | {number(lo)} | {number(hi)} | {number(hi+count*(348+8))} |')
    lines += [f'| All six story-outfit candidates | {number(actors["increments"]["native30"]["whole"]["six_story_outfits"])} | {number(all_outfits)} | {number(all_outfits_with_faces)} |',
        f'| All fourteen same-name variants | {number(actors["increments"]["native30"]["whole"]["all_fourteen_same_name_variants"])} | {number(actors["increments"]["extended_regular"]["whole"]["all_fourteen_same_name_variants"])} | Not all are clothing variants |', '',
        'The optional source identity17 control adds only324 bytes in extended_regular '
        '(mapping records; retained pictures deduplicate). This does **not** price '
        'an independently authored hooded executioner: special scene-only shapes and '
        'attack/SP2 pages still require their actual source extraction.', '',
        'Ramza2/3 and Delita2/3 fit provisionally. Reserve the full50KB, including '
        'four optional portraits and lookup records. The final source-required pose '
        'inventory must still stay under the cap. Keep Wiegraf and the other secondary '
        'same-name variants canonical by default; do not accumulate every wounded or '
        'guest identity as another complete wardrobe.', '',
        '## 3. Native preparation cost and RAM', '',
        'The standalone C prototype reconstructs45 diverse poses, ordinary and '
        'mirrored, through all three encodings:270 cases. PC ASan/UBSan and the '
        'actual68000 binary under ti-cycles report zero byte differences and signature '
        f'0x{pc["signature"]:04X}. The native path calls the existing verified ZX0 and '
        'ExtGraph mirror routines. No new assembly, I/O driver or live TiEmu test.', '',
        '| Preparation | Calls | Mean cycles | Maximum cycles | Mean vs whole |',
        '|---|---:|---:|---:|---:|']
    for mode, label in [(1,'Whole'),(2,'Two parts'),(3,'Three parts')]:
        row = cycles[mode]
        delta = (row['mean']/cycles[1]['mean']-1)*100
        lines.append(f'| {label} | {row["calls"]} | {number(row["mean"])} | {number(row["peak"])} | {delta:+.1f}% |')
    lines += ['',
        'These are pose-cache-miss microbenchmarks, not complete game frames or '
        'hardware FPS. Per-mode maxima can occur on different poses. The warm '
        'complete-sprite blit would remain the same, but extra preparation still '
        'consumes the frame budget. Current selected Zeklaus13 rendering has only '
        '3,168 cycles of tightest measured margin, so the average3.5..3.9K extra '
        'preparation cannot be assumed to fit.', '',
        'The simple prototype needs an additional144-byte part workspace for '
        'two parts, or80 bytes if compiled specifically for three. Both bench modes '
        'share144 bytes. A208-byte assembly temporary can use the role of the current '
        'mirror temporary; this is not an installed RAM optimization. The actual game '
        'has not been linked with the new path, its stack/lifetimes are not certified, '
        'and the42-entry cache is not reduced. Installed ABI99 Zeklaus13 previously '
        'had436 bytes free in the settled diagnostic. Keep the shipping whole-pose path.', '',
        '## 4. Scene-only actors and character lifetime', '',
        'Appearance uniqueness and storage ownership are separate. A major enemy can '
        'keep a unique body without remaining in every later pack. A recruited person '
        'must keep their body available after their last scripted line. Names and '
        'speaker portraits remain in shared identity data.', '',
        'The eleven scene-only human candidates are Ovelia, Larg, Goltana, Funeral, '
        'Simon, Teta, Barinten, Balmafula, Rudvich, human Draclau and Ajora. Human '
        'Draclau is separate from the Queklain boss body. These candidates still '
        'need an authored source-action inventory; no required injury/death/gesture '
        'may be removed merely because the person does not fight.', '',
        '| Same eleven people, whole sprites | Stored bytes | Meaning |',
        '|---|---:|---|']
    for scope, meaning in [('standing_walking10','Minimum motion probe; does not cover all scenes'),
                           ('with_hurt_kneel_ko16','Adds hit/kneel/KO controls; gestures remain'),
                           ('native30','Previous compact pose proxy'),
                           ('extended_regular','Broad gesture/body library control')]:
        lines.append(f'| {scope} | {number(actors["scene_pose_scopes"][scope]["stored_bytes"])} | {meaning} |')
    lines += ['',
        'The16-pose scope is17,138 bytes smaller than native30, and66,209 smaller '
        'than the broad library. Those are alternative corpus measurements, '
        '**not savings already earned in the campaign**. Extract exactly the poses '
        'used by each scene, including rare deaths and gestures.', '',
        '### 4.1 All38 named-human lifetime candidates', '',
        'Chapters below come from the bounded static ENTD/message census. Silent '
        'actors, script graphics replacements and later party use can extend them. '
        'They are not proof of an actor\'s last possible use.', '',
        '| Person | Source narrative chapters | Appearance plan | Lifetime policy |',
        '|---|---|---|---|']
    recruits = {'Ramza','Agrias','Mustadio','Rafa','Malak','Orlandu','Meliadoul','Reis','Beowulf','Cloud'}
    unique = set(actors['important_unique'])
    for name, record in sorted(text['cast'].items()):
        appearance = 'Unique' if name in unique else 'Canonical; shared-body candidate only'
        lifetime = 'Retain after joining' if name in recruits else 'Active narrative packs; core only if useful across packs'
        lines.append(f'| {name} | {", ".join(record["chapters"])} | {appearance} | {lifetime} |')
    lines += ['',
        'Clear chapter-confined non-recruits include Algus/Teta(I), '
        'Draclau/Rudvich(II), Barinten/Izlude(III). Named generic-job enemies such '
        'as Gustav/Miluda/Golagros are separate from this dedicated-human corpus; '
        'a proper name does not imply a dedicated physical sprite bank.', '',
        '### 4.2 Human-bank controls per active pack', '',
        'The following counts include38 generic job controls, all13 final stable '
        'human banks, the active named NPCs and the optional four hero outfits. '
        'They are exact counts of these converted profiles, excluding portraits, '
        'nonhuman bosses and missing poses. Early packs need not front-load future '
        'recruits; doing so here is conservative.', '',
        '| Pack | native30 | Extended named bodies / native30 jobs |',
        '|---|---:|---:|']
    for key, row in costumes.get('pack_whole_body_controls', {}).items():
        lines.append(f'| {key} | {number(row["native30"])} | {number(row["extended_regular"])} |')
    lines += ['',
        'The current actor reader has six human-bank slots. A complete broad '
        'profile can exceed that directory, even while the archive budget fits. '
        'Keep pose pruning and explicit bank-directory extension as implementation '
        'gates; file size alone does not make these banks loadable by ABI99.', '',
        '## 5. Scene and text compression', '',
        'Scenes already decompress ZX0 pictures once at entry. Their low processing '
        'load permits cold decompression; it does not create a new entropy saving '
        'by itself. Keep the existing codec and all six picture planes.', '',
        '| Picture width x100 | Raw bytes | Compressed minimum | Mean | Maximum |',
        '|---|---:|---:|---:|---:|']
    for width, row in scene['summary'].items():
        lines.append(f'| {width} | {number(row["raw_bytes"])} | {number(row["zx0_min"])} | {number(row["zx0_mean"])} | {number(row["zx0_max"])} |')
    lines += ['',
        'These are six representative rooms, two tested directions per room and '
        'three widths. Testing two directions did not require shipping both. '
        'They are source-texture compression samples, not approved new C2 scene art '
        'or an inventory of original cameras. Row-XOR preprocessing saves only '
        'about4% at320px; do not add its reader just for this small difference.', '',
        'Default to one source-appropriate direction per scene. Same-direction '
        'camera crops/pans may share a panorama; different scenes in one room '
        'may still need different compositions. The one-direction planning case '
        'allows5..10KB per scene-only location plus up to30KB per pack for extra '
        'framing/pictures. Keep the older8..20KB/location allowance as a separate '
        'conservative control until the source camera list exists.', '',
        '| Pack | Referenced event text, raw |4KiB ZX0 chunks + directory |',
        '|---|---:|---:|']
    for key in phases:
        row = text['packs'][key]['text']
        lines.append(f'| {projection[key]["label"]} | {number(row["raw_bytes"])} | {number(row["zx0_4k_chunks"])} |')
    lines += ['',
        'Whole chapters I/II/III are30,995/31,833/34,852 raw bytes and '
        '18,432/18,535/20,567 with4KiB chunks, directories included. The full '
        'linear source scan finds2,230 entries,167,838 raw bytes and96,972 chunked '
        'bytes, including material outside the selected main route. It has zero '
        'cursor/instruction failures; unmapped glyphs and dynamic placeholders '
        'remain explicit in text-cast.json. This is not a final faithful translation.', '',
        'Decode only a bounded text block at a scene/dialogue boundary; do not '
        'expand the whole chapter or decompress the same line every frame. '
        'The native cold text format/reader is not implemented. Item/help/rumor '
        'catalogue text is separate, and already shared core catalogues must not '
        'be added twice.', '',
        '## 6. Accepted eight-pack breakdown and budgets', '',
        'All amounts below include the shared core, one installed active pack, '
        'characters/boss allowances, ten optional adapted effects, a50KB global '
        'hero-wardrobe cap and the131,072-byte reserve. They are forecasts, '
        'not ripped/installed pack sizes. No hypothetical head-substitution or '
        'unproven scene-pose pruning is credited.', '',
        'The eight boundaries stay accepted. One small terrain-list refinement '
        'is proposed: IV-C keeps Bed Desert as its sole additional random field '
        'instead of also shipping Doguola. After the final Orbonne entry there '
        'is no world travel. This preserves more margin for wardrobes/late bosses. '
        'Earlier extra-field choices remain illustrative until formations are authored.', '',
        '| Pack | End boundary | Story + extra full maps | Scene-only places | One-direction upper incl reserve | Conservative upper incl reserve |',
        '|---|---|---:|---:|---:|---:|']
    for key in phases:
        p = projection[key]
        lines.append(f'| {p["label"]} | {p["ending"]} | {len(p["story_maps"])} +{len(p["extra_random"])} | {p["scene_locations"]} | {p["one_orientation_with_reserve"][1]/1e6:.3f}MB | {p["conservative_with_reserve"][1]/1e6:.3f}MB |')
    lines += ['',
        f'The one-direction forecast peaks at **{peak/1e6:.3f}MB including reserve**, '
        f'or{(peak-prior["reserve"])/1e6:.3f}MB of FFT data. The retained conservative '
        f'scene allowance peaks at **{conservative_peak/1e6:.3f}MB including reserve**, '
        f'leaving{number(2700000-conservative_peak)} additional planning bytes. '
        'This remaining margin is not a claim about actual free Flash.', '',
        '### 6.1 Core and content ownership', '',
        '- Current measured core:700,273 bytes. The broader shared core projection, '
        'including generic/stable humans, ten optional effects and the wardrobe cap, '
        'is roughly1.09..1.24MB; late optional nonhuman recruits bring it to1.12..1.30MB.',
        '- Core: rules/catalogues, shared UI/portraits/names, ordinary monster families, '
        'generic job art, retained recruits/common actors and selected reusable effects.',
        '- Pack: battle maps/depth, scene backdrops, narrative text/choreography, '
        'encounters/objectives/deployment, local NPC body poses, local large boss forms '
        'and source progression bindings. Pool every resource across that pack\'s consumers.',
        '- Moving an active bank between core and pack saves zero combined installed bytes. '
        'Dropping an expired NPC bank from later packs is a real installed-content reduction.',
        '- Character appearance is independent of job/team/identity. Costume/body sharing '
        'must not change saved stats, recruit ownership or known-speaker portraits.', '',
        '### 6.2 Map lists', '',
        'Source IDs distinguish physical terrains, not battle count. Both Bethla '
        'approaches are retained even though one playthrough chooses only one. '
        'Repeated fights on the same map do not add another bank. Scene-only lists '
        'are working static census groups, not a completed event reachability proof.', '']
    for key in phases:
        p = projection[key]
        def named(ids):
            return '; '.join(f'{i:03} {map_names[str(i)]}' for i in ids)
        lines += [f'#### {p["label"]}', '',
                  f'- Story battle terrains: {named(p["story_maps"])}.',
                  f'- Additional random examples: {named(p["extra_random"])}.',
                  f'- Scene-only locations: {named(p["scene_maps"])}.', '']
    lines += ['### 6.3 Optional packs and revisits', '',
        '- **Quest pack:** Colliery/Goug/Nelveska/Zarghidas, with its own scenes and '
        'source unlock conditions. The working census has six quest battle terrains; '
        'Cloud\'s street/slum map distinction still needs the authored source route audit.',
        '- **Deep Dungeon:** ten floors105..114 in a separate pack, with its own '
        'optional boss/scene allowance. No world-random field bank is required while inside.',
        '- These are separately proposed packs, not two fully extracted size-certified '
        'deliveries. Do not force all optional content into IV-C.',
        '- Returning for an old encounter or Brave Story scene replay requires the '
        'corresponding content pack. Do not silently remove old travel nodes. Source '
        'random destinations can route to selected active terrains, but formations '
        'need legal authored placements; changing only a bank name is insufficient.', '',
        '### 6.4 Pack replacement and saves', '',
        'Save after the boundary\'s battle chain, rewards and following scenes settle. '
        'Return to HOME before replacing content variables from the previous pack\'s '
        'generated list. Preserve ffsave/ffsave1/ffsave2/ffsave3/ffslot/ffcfg. '
        'Use matching core/package keys and dependency lengths/checksums. '
        'A missing next pack must be detected before partial progression/rewards are '
        'committed. This report does not implement automatic pack switching.', '',
        'Content phase must be distinct from original chapter: II-A/II-B share '
        'ChapterII shop/rumor/proposition rules; all IV packs keep ChapterIV rules. '
        'Preserve inventory, XP/JP/levels, recruitment, deaths, breeding, poach stock '
        'and durable source flags across swaps. Current FSV9 PC/TI save compatibility '
        'is existing evidence; future pack-boundary/Continue flows still need their '
        'actual authored PC replay and selected native checks.', '',
        '## 7. Remaining gates and next work', '',
        '1. Extract actual scene camera/pose references, defaulting to one orientation. '
        'Keep every required story gesture and source staging; do not use this report '
        'to certify unreviewed cropping or a generic head-swap appearance.',
        '2. Keep hero wardrobe data under50KB after actual required poses/portraits '
        'are packed. Retain canonical fallback; other outfits remain omitted by default.',
        '3. Keep complete battle sprites. A future secondary-body or scene-only parts '
        'prototype needs a visual verdict if it changes pixels, and whole-frame/stack '
        'gates if integrated. Microbenchmarks are not those gates.',
        '4. Author remaining battle/scene maps using approved C2 treatment, full '
        'geometry and source handedness. Current320/352px/row/depth ceilings are '
        'planning constraints, not proof of future full-map fit. Finath and BookStorage5 '
        'still need envelope work; the old Sweegy depth excess remains a ChapterI gate.',
        '5. Extend/validate human-bank directories and cold text ownership before '
        'shipping larger banks. Keep42 actor entries,18488 depth bytes, existing '
        'allocation lifetimes and ordinary360k cycle gates.16KiB free remains deferred.',
        '6. Build the actual eight data-only packs, pool resources, measure final '
        'stored bytes and available Flash, replay saves/Continue on PC, then use '
        'selected native/hardware checks only where changed content requires them.', '',
        '## 8. Evidence and reproduction', '',
        'Tracked measurement tools:', '',
        '- tools/actor_storage_study.py: local-disc conversion, exact split/join, '
        'optimal ZX0 roundtrips, bank accounting and native fixtures.',
        '- tools/actor_costume_increment_study.py: wardrobe subsets, hybrid scopes '
        'and per-pack whole-human controls, reusing the measured corpus.',
        '- tools/actor_parts_bench.c: PC/native byte reconstruction and existing '
        'ZX0/mirror preparation costs; no game changes.',
        '- tools/campaign_pack_report.py: this report and pack-projection.json.', '',
        'Private local evidence (art/fixtures stay ignored): '
        'x/actor-storage/results.json, costume-choices.json, pc.json, native.log, '
        'pack-projection.json and verification.json. Existing pack/text/scene '
        'baseline: x/pack-refinement/{plan.json,text-cast.json,scenes.json}. '
        'The report contains the scope/IDs/assumptions needed to review these '
        'estimates even when the private extracted art is not distributed.', '',
        'Run from the repository root after preparing the existing read-only baseline caches:', '',
        '```sh',
        'rtk proxy tools/pyenv/bin/python games/fft/tools/actor_storage_study.py',
        'rtk proxy tools/pyenv/bin/python games/fft/tools/actor_costume_increment_study.py',
        'rtk proxy cc -std=c99 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \\',
        '  -DSTUDY_FIXTURES_FILE=\'"/home/samuelcoz/workspace/ti/games/fft/campaign/x/actor-storage/fixtures.h"\' \\',
        '  games/fft/tools/actor_parts_bench.c -o games/fft/campaign/x/actor-storage/parts-pc',
        'rtk proxy games/fft/campaign/x/actor-storage/parts-pc > games/fft/campaign/x/actor-storage/pc.json',
        'rtk proxy tools/bin/ti-cc \\',
        '  -DSTUDY_FIXTURES_FILE=\'"/home/samuelcoz/workspace/ti/games/fft/campaign/x/actor-storage/fixtures.h"\' \\',
        '  -o games/fft/campaign/x/actor-storage/apartb games/fft/tools/actor_parts_bench.c lib/unpack68k.s',
        'rtk proxy tools/bin/ti-cycles --max 80 games/fft/campaign/x/actor-storage/apartb.89z \\',
        '  > games/fft/campaign/x/actor-storage/native.log 2>&1',
        'rtk proxy tools/pyenv/bin/python games/fft/tools/campaign_pack_report.py',
        '```', '',
        'The fixture include path is workspace-specific; adjust it if the checkout '
        'moves. No emulator restart, input, screenshot, installed-group replacement '
        'or save write is part of these commands. Preserve all historical protected '
        'proof/save bytes and mtimes. Current release remains ABI99, core key '
        '`10965478d3a2c348c22d2dea8e6ac51d`. The user subsequently requested '
        'ti-commit for these measurement tools and documentation; extracted art '
        'and private proof files are excluded from Git.', '']
    args.output.write_text('\n'.join(lines))
    print(json.dumps(dict(report=str(args.output), wardrobe_total=wardrobe_measured,
                         conservative_peak=conservative_peak, one_orientation_peak=peak,
                         native=cycles), indent=2))


if __name__ == '__main__':
    main()
