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

    # The user dropped optional costume changes, Ramza and Delita included:
    # one canonical body per person. These increments stay as the measurement
    # that priced the choice; no wardrobe byte is credited to any budget.
    wardrobe_body = costumes['profiles']['extended_regular']['ramza_delita']['whole']['increment_bytes']
    wardrobe_portraits = 4*348
    wardrobe_refs = 4*8
    wardrobe_measured = wardrobe_body + wardrobe_portraits + wardrobe_refs
    all_outfits = actors['increments']['extended_regular']['whole']['six_story_outfits']
    all_outfits_with_faces = all_outfits + 6*(348+8)

    # Measured on the project's clean Titanium .sav through EM_survey
    # (experiments/archfree, ti68k-c-patterns.md). inUse+free is constant; TI's
    # "2.7 MB" is allExceptBaseCode and is NOT archivable. Replaces the former
    # 2,700,000 planning assumption.
    ARCHIVE_CEILING = 2555826
    ARCHIVE_ALL_EXCEPT_BASE = 2686976
    ARCHIVE_PER_VARIABLE = 23          # measured 22.39 / 22.34, rounded up
    MEASURED_CORE_INSTALL = 695696     # 36 archived variables
    MEASURED_CORE_PLUS_I = 990766      # 62 archived variables
    # Each bank is one archive variable capped at 65,000 bytes (tools/common_actors.py
    # split(); actors.h ACTOR_BANKS). Six slots: ffca..ffcf.
    BANK_BYTES, BANK_SLOTS = 65000, 6
    BANK_CEILING = BANK_BYTES*BANK_SLOTS
    bank_profiles = {k: actors['cases'][k]['canonical_plus_jobs']['whole']['stream_banks']['banks']
                     for k in ('native30', 'extended_regular')}
    pack_banks = {k: max(v[k] for v in costumes['pack_whole_body_controls'].values())
                  for k in ('native30', 'extended_regular')}

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
        conservative = [old['with_reserve'][i] - removal[i] for i in range(2)]
        # A single direction still may have several camera crops. Keep30KB
        # per-pack upper allowance for additional scene pictures/framing.
        one_view = [5000*count, 10000*count+30000]
        updated = [conservative[i] - old['breakdown']['scene_only_pictures'][i] + one_view[i]
                   for i in range(2)]
        projection[key] = dict(label=label, ending=end, story_maps=old['story_maps'],
            extra_random=fields, scene_maps=old['scene_only_maps'],
            full_map_count=len(old['story_maps'])+len(fields), scene_locations=count,
            conservative_with_reserve=conservative, one_orientation_with_reserve=updated,
            reserve_bytes=prior['reserve'], one_orientation_picture_allowance=one_view,
            optional_field_removal=removal)
    # PackI's human-body estimate in the earlier plan, against which 4.3's measured
    # native30 cast is compared: core generic + core stable + this pack's temporary
    # named bodies + its civilian/unseen pose allowance.
    human_lo, human_hi = [prior['core_breakdown']['generic_combat_bodies'][i]
                          + prior['core_breakdown']['stable_named_bodies'][i]
                          + prior['packs']['I']['breakdown']['temporary_named_bodies'][i]
                          + prior['packs']['I']['breakdown']['additional_civilian_and_unseen_pose_allowance'][i]
                          for i in range(2)]
    for key, pr in projection.items():
        pr['one_orientation_margin'] = [ARCHIVE_CEILING-v for v in pr['one_orientation_with_reserve']]
        pr['conservative_margin'] = [ARCHIVE_CEILING-v for v in pr['conservative_with_reserve']]
    peak = max(p['one_orientation_with_reserve'][1] for p in projection.values())
    conservative_peak = max(p['conservative_with_reserve'][1] for p in projection.values())
    low_peak = max(p['one_orientation_with_reserve'][0] for p in projection.values())
    # Every pack's LOW forecast fits the measured ceiling; the unconstrained HIGH
    # forecast does not, which is what choosing a pose profile has to settle.
    assert low_peak < ARCHIVE_CEILING and peak < conservative_peak
    over = {projection[k]['label']: ARCHIVE_CEILING-projection[k]['one_orientation_with_reserve'][1]
            for k in phases if projection[k]['one_orientation_with_reserve'][1] > ARCHIVE_CEILING}
    summary = dict(schema='FFT_MEASURED_PACK_REPORT_1', date='2026-11-24',
                   accepted_main_packs=8,
                   wardrobe_dropped=dict(credited_bytes=0, measured_body_increment=wardrobe_body,
                                 portrait_allowance=wardrobe_portraits, reference_allowance=wardrobe_refs,
                                 measured_total=wardrobe_measured, all_six_outfits=all_outfits_with_faces),
                   archive=dict(measured_ceiling=ARCHIVE_CEILING, all_except_base_code=ARCHIVE_ALL_EXCEPT_BASE,
                                per_variable_overhead=ARCHIVE_PER_VARIABLE,
                                measured_core_install=MEASURED_CORE_INSTALL,
                                measured_core_plus_chapter1=MEASURED_CORE_PLUS_I),
                   actor_banks=dict(bytes_per_bank=BANK_BYTES, slots=BANK_SLOTS, ceiling=BANK_CEILING,
                                    corpus_banks=bank_profiles,
                                    worst_pack_native30=pack_banks['native30'],
                                    worst_pack_extended=pack_banks['extended_regular']),
                   budget_basis='native30', packs_over_ceiling_at_high_forecast=over,
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
        f'- **Budget against the measured archive ceiling of {number(ARCHIVE_CEILING)} bytes**, not TI\'s2.7MB. That figure is `allExceptBaseCode` ({number(ARCHIVE_ALL_EXCEPT_BASE)}) and is not archivable. Add {ARCHIVE_PER_VARIABLE} bytes per archived variable; the reserve absorbs it.',
        '- **native30 is the official pose profile for every budget here.** It is the list the shipping renderer already uses. extended_regular stays a stress control and is now ruled out: it breaks the archive ceiling and the actor-bank ceiling at once.',
        '- Plan separate optional quest and Deep Dungeon packs. Content-pack phase never changes the original story chapter or town progression.',
        '- Use one orientation per scripted scene by default. Keep extra source-required directions, camera crops, pans and gestures.',
        '- Keep complete unique appearances for recruits and major story actors. Pool identical sprites, poses and mirrored frames physically.',
        '- Keep complete sprites in the combat renderer. Naive lossless two/three-part preparation is slower; it does not save space in the native30 corpus.',
        f'- **One canonical body per person, Ramza and Delita included.** The user dropped optional costume changes. Their measured cost, {number(wardrobe_measured)} bytes for Ramza2/3 + Delita2/3 and {number(all_outfits_with_faces)} for the six-outfit set, is now free budget, not a reserved allowance.',
        '- Costume variants share no sprite band: the measured two/three-part increments are the same size as the whole-sprite ones, so a second outfit always costs a second complete body.',
        '- Reduce scene-only actors to their actual scripted poses when authoring. No generic head/body replacement has been visually approved or integrated.',
        '',
        'This report measures archive encodings and a bounded pose-preparation prototype. '
        'It does not install new banks, rip the complete campaign, change the shipping renderer, '
        'or certify final Flash/RAM fit. ABI99 and all reviewed art remain unchanged.',
        '', '## 1. Available storage and measured release', '',
        'The ceiling is now measured, not assumed. `experiments/archfree/` calls '
        '`EM_survey` and logs its six figures through tilog, so `ti-mem log` reads '
        'them without a screenshot. On the project\'s clean Titanium `.sav`, '
        '`inUse + free` is constant at '
        f'**{number(ARCHIVE_CEILING)} bytes**: that is what archived variables may '
        'actually occupy. The2.7MB TI quotes is `allExceptBaseCode` '
        f'({number(ARCHIVE_ALL_EXCEPT_BASE)}), which also counts one permanently '
        'unused65,536-byte sector and the archive\'s own bookkeeping. The former '
        f'2,700,000 planning ceiling overstated capacity by {number(2700000-ARCHIVE_CEILING)} '
        'bytes on an empty calculator, before any other app or file. '
        '[TI capacity table](https://education.ti.com/en/customer-support/knowledge-base/ti-83-84-plus-family/general-information/12174).', '',
        'Archive overhead was measured too, and it is small and linear: 36 archived '
        f'variables holding694,890 stored bytes occupied {number(MEASURED_CORE_INSTALL)} '
        f'(+806,22.39 each);62 variables holding989,381 occupied {number(MEASURED_CORE_PLUS_I)} '
        f'(+1,385,22.34 each). Budget {ARCHIVE_PER_VARIABLE} bytes per archived variable. '
        'There is no sector-fragmentation term to model: AMS packs archived blocks '
        'contiguously and only garbage collection works by sector. An installed set of '
        f'150 variables therefore costs about3,450 bytes, which the {number(prior["reserve"])}-byte '
        'reserve already absorbs; it is not budgeted again per pack. Variables sent '
        'unarchived cost RAM instead, like the core\'s5,381-byte `fftstory`.', '',
        '| Item | Bytes | Status |', '|---|---:|---|',
        f'| Promoted ABI99 core, stored variables/size words | {number(packages["core"]["stored_variable_bytes"])} | Measured |',
        f'| Same core, transport file | {number(packages["core"]["bytes"])} | Measured; excluded from archive sums |',
        f'| Archivable ceiling, empty Titanium | {number(ARCHIVE_CEILING)} | **Measured**, EM_survey |',
        f'| TI\'s quoted capacity (allExceptBaseCode) | {number(ARCHIVE_ALL_EXCEPT_BASE)} | Measured; NOT archivable |',
        f'| Archive overhead per archived variable | {ARCHIVE_PER_VARIABLE} | Measured22.39/22.34, rounded up |',
        f'| Core installed, 36 archived variables | {number(MEASURED_CORE_INSTALL)} | Measured in the archive |',
        f'| Core + Chapter I pilot,62 variables | {number(MEASURED_CORE_PLUS_I)} | Measured in the archive |',
        '| Flash/VAT/save reserve | 131,072 | Retained;128KiB |',
        f'| Dropped wardrobe increment, Ramza+Delita | {number(wardrobe_measured)} | Measured; not retained, not budgeted |',
        '',
        'MB/KB in this report are decimal. KiB means1,024 bytes. Actor-study stored '
        'counts include compressed streams, u16 offsets, mirrored logical mappings, '
        'recipe/part headers and bounded variable DAT/size overhead. Archive headers '
        'are now measured and budgeted; other installed software on the user\'s own '
        'calculator is still not, and comes off the same ceiling.',
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
        '### 2.4 Costume variants: measured, then dropped', '',
        'The user dropped optional costume changes. Every person keeps one canonical '
        'body. The table prices what was given up; none of it is budgeted anywhere '
        'else in this report.', '',
        '| Dropped set | native30 body increment | Extended body increment | Extended + portrait/reference allowance |',
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
        'A second outfit cannot reuse the first one\'s legs and torso. The same '
        'corpus measured with the row-band encodings gives the increments below: '
        'splitting the sprite recovers nothing, because an alternate chapter sheet '
        'redraws the whole figure, head band included.', '',
        '| Dropped set, native30 | Whole | Two parts | Three parts |',
        '|---|---:|---:|---:|']
    for key in ('ramza_only', 'delita_only', 'ramza_delita'):
        row = costumes['profiles']['native30'][key]
        lines.append(f'| {key} | {number(row["whole"]["increment_bytes"])} | '
                     f'{number(row["two"]["increment_bytes"])} | {number(row["three"]["increment_bytes"])} |')
    lines += ['',
        'Ramza2/3 costs8,761 bytes whole and8,729 split in two: the shared lower '
        'band saves32 bytes,0.4%. Delita2/3 is worse split (+209). Head-only '
        'substitution is measured in2.3 for *different people* sharing one generic '
        'body, not for one person\'s wardrobe. Keep every same-name wounded, guest '
        'or undead identity canonical as well.', '',
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
        'human banks and the active named NPCs, one body per person. '
        'They are exact counts of these converted profiles, excluding portraits, '
        'nonhuman bosses and missing poses. Early packs need not front-load future '
        'recruits; doing so here is conservative.', '',
        '| Pack | native30 | Extended named bodies / native30 jobs |',
        '|---|---:|---:|']
    for key, row in costumes.get('pack_whole_body_controls', {}).items():
        lines.append(f'| {key} | {number(row["native30"])} | {number(row["extended_regular"])} |')
    lines += ['',
        'Both columns are measured. The native30 column is the budget; the extended '
        'column is the stress control that4.3 rules out.', '',
        '### 4.3 Actor-bank ceiling, and why native30 is the budget', '',
        'Actor frames live in `ffca..ffcf`. Each is one archive variable that '
        f'`tools/common_actors.py` `split()` caps at {number(BANK_BYTES)} bytes '
        '(header + u16 directory + ZX0 streams; the in-bank offsets are u16, so the '
        f'cap is a format limit, not a preference). `actors.h` declares ACTOR_BANKS '
        f'{BANK_SLOTS}, giving a hard ceiling of **{number(BANK_CEILING)} bytes of '
        'actor frames resident at once** for any pack. The builder asserts rather '
        'than truncating, so the limit cannot be crossed silently.', '',
        '| Profile | Whole corpus | Banks needed | Worst pack | Fits 6 slots |',
        '|---|---:|---:|---:|---|',
        f'| native30 | {number(actors["cases"]["native30"]["canonical_plus_jobs"]["whole"]["stored_bytes"])} | '
        f'{bank_profiles["native30"]} | {number(pack_banks["native30"])} | yes, {number(BANK_CEILING-pack_banks["native30"])} spare |',
        f'| extended_regular | {number(actors["cases"]["extended_regular"]["canonical_plus_jobs"]["whole"]["stored_bytes"])} | '
        f'{bank_profiles["extended_regular"]} | {number(pack_banks["extended_regular"])} | '
        f'**no, {number(pack_banks["extended_regular"]-BANK_CEILING)} over** |',
        '',
        'Two independent limits therefore agree. extended_regular needs '
        f'{bank_profiles["extended_regular"]} banks for the whole corpus against {BANK_SLOTS} slots, '
        f'and its worst pack ({number(pack_banks["extended_regular"])}) exceeds the '
        f'{number(BANK_CEILING)}-byte ceiling outright, while PackI at '
        f'{number(costumes["pack_whole_body_controls"]["I"]["extended_regular"])} is also over. '
        'The same profile is what pushes the high archive forecast past the measured '
        'ceiling in6. **native30 is adopted as the budget basis.**', '',
        'That choice buys a measurable gesture budget rather than closing the door. '
        'A stored pose costs about141 bytes (303,628 stream bytes over2,216 frames, '
        'plus about4 bytes of mapping per logical pose). The worst pack leaves '
        f'{number(BANK_CEILING-pack_banks["native30"])} bank bytes free, roughly '
        f'{(BANK_CEILING-pack_banks["native30"])//141:,} extra poses spread over that '
        'pack\'s cast, before any bank-directory change is needed. Author the actual '
        'scripted gestures against that budget.', '',
        'Raising ACTOR_BANKS is cheap in itself: the banks are read in place from the '
        'archive, the extra pointers cost four bytes each, the name is built as '
        "`'a'+k` up to26 banks, and the miss-path bank walk is linear against a "
        '26,804-cycle decode. It is not free in practice: ACTOR_BANKS is part of the '
        'core/chapter contract (`contract_layout.c`), so changing it bumps the ABI and '
        'forces matching core and chapter packages. Do it when an authored pose '
        'inventory proves it necessary, not pre-emptively.', '',
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
        'characters/boss allowances, ten optional adapted effects and the '
        '131,072-byte reserve. No wardrobe allowance remains. They are forecasts, '
        'not ripped/installed pack sizes. No hypothetical head-substitution or '
        'unproven scene-pose pruning is credited.', '',
        'The eight boundaries stay accepted. One small terrain-list refinement '
        'is proposed: IV-C keeps Bed Desert as its sole additional random field '
        'instead of also shipping Doguola. After the final Orbonne entry there '
        'is no world travel. This preserves more margin for late bosses and '
        'scene framing. '
        'Earlier extra-field choices remain illustrative until formations are authored.', '',
        '| Pack | End boundary | Maps | Scenes | One-direction low..high | Margin at high | Conservative high | Margin |',
        '|---|---|---:|---:|---:|---:|---:|---:|']
    for key in phases:
        p = projection[key]
        lo, hi = p['one_orientation_with_reserve']
        lines.append(f'| {p["label"]} | {p["ending"]} | {len(p["story_maps"])} +{len(p["extra_random"])} | '
                     f'{p["scene_locations"]} | {lo/1e6:.3f}..{hi/1e6:.3f}MB | '
                     f'{number(p["one_orientation_margin"][1])} | '
                     f'{p["conservative_with_reserve"][1]/1e6:.3f}MB | '
                     f'{number(p["conservative_margin"][1])} |')
    lines += ['',
        f'Margins are against the measured {number(ARCHIVE_CEILING)}-byte ceiling; a '
        'negative margin is an overflow. **Every pack fits at its low forecast** '
        f'(worst low: {low_peak/1e6:.3f}MB, {number(ARCHIVE_CEILING-low_peak)} spare). '
        f'At the unconstrained high forecast the one-direction peak is {peak/1e6:.3f}MB '
        f'and PackI is {number(-over["I"])} bytes over; the conservative high peak is '
        f'{conservative_peak/1e6:.3f}MB, with PackI over by '
        f'{number(-projection["I"]["conservative_margin"][1])} and PackIV-C by '
        f'{number(-projection["IV-3"]["conservative_margin"][1])}.', '',
        'The high column is the sum of every range\'s upper bound at once, so it is a '
        'worst case, not a forecast. Adopting native30 (4.3) relieves it from below the '
        'low column, not merely toward it: PackI\'s human bodies were estimated at '
        f'{number(human_lo)}..{number(human_hi)} bytes, while the measured native30 cast '
        f'for that pack is {number(costumes["pack_whole_body_controls"]["I"]["native30"])}, '
        f'{number(human_lo-costumes["pack_whole_body_controls"]["I"]["native30"])} bytes '
        f'under the low estimate. The measured extended cast ({number(costumes["pack_whole_body_controls"]["I"]["extended_regular"])}) '
        f'sits inside the old range but over the {number(BANK_CEILING)}-byte actor-bank '
        'ceiling, so the profile that would justify the high column cannot ship. '
        '**PackI is the only pack with '
        'no slack under either reading, and it is the one the user asked to keep '
        'whole.** Author its actual map, scene and pose inventory before the others: if '
        'the authored figures land above the low forecast, splitting PackI after Dorter '
        'or the Sand Mouse cellar is the lever, since battle maps '
        f'({number(prior["packs"]["I"]["breakdown"]["full_four_view_maps"][0])}..'
        f'{number(prior["packs"]["I"]["breakdown"]["full_four_view_maps"][1])} bytes for11 '
        'four-view maps) dominate every other item in the pack.', '',
        'These margins assume nothing else is installed. The user\'s own calculator '
        'carries HW3Patch, AMS files and any other software, so measure it with '
        '`experiments/archfree/` before committing to a boundary.', '',
        '### 6.1 Core and content ownership', '',
        f'- Current measured core:700,273 stored bytes; installed, its36 archived '
        f'variables occupy {number(MEASURED_CORE_INSTALL)} archive bytes and `fftstory` '
        'stays in RAM. The broader shared core projection, including generic/stable '
        'humans and ten optional effects, is roughly1.04..1.19MB; late optional '
        'nonhuman recruits bring it to1.07..1.25MB.',
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
        '1. **Done for storage** ([PACK1_INVENTORY.md](PACK1_INVENTORY.md)): PackI\'s '
        'eleven maps and nine rooms measured from the disc total1,894,549 bytes with '
        'today\'s core,530,205 inside the budget, so PackI stays whole and needs no '
        'split. Two depth overruns remain (Sweegy19,304, Slums in Dorter18,776 against '
        '18,488). Still extract scene camera/pose references defaulting to one orientation, keeping '
        'every required story gesture and source staging; do not use this report to '
        'certify unreviewed cropping or a generic head-swap appearance.',
        '2. Keep one canonical body per person. Reopening any costume variant needs '
        'a new measurement and a user decision, not this report\'s freed margin.',
        '3. Keep complete battle sprites. A future secondary-body or scene-only parts '
        'prototype needs a visual verdict if it changes pixels, and whole-frame/stack '
        'gates if integrated. Microbenchmarks are not those gates.',
        '4. Author remaining battle/scene maps using approved C2 treatment, full '
        'geometry and source handedness. Current320/352px/row/depth ceilings are '
        'planning constraints, not proof of future full-map fit. Finath and BookStorage5 '
        'still need envelope work; the old Sweegy depth excess remains a ChapterI gate.',
        f'5. Author poses against the native30 budget and its {number(BANK_CEILING-pack_banks["native30"])} '
        'spare bank bytes (4.3). Raise ACTOR_BANKS only when an authored inventory '
        'needs it, and then bump the ABI and rebuild matching core/chapter packages. '
        'Cold text ownership still needs its own pass. Keep42 actor entries,18488 depth '
        'bytes, existing allocation lifetimes and ordinary360k cycle gates.16KiB free '
        'remains deferred.',
        '6. Build the actual eight data-only packs, pool resources, measure final '
        'stored bytes and available Flash, replay saves/Continue on PC, then use '
        'selected native/hardware checks only where changed content requires them.',
        '7. Run `experiments/archfree/` on the target calculator before fixing any '
        'boundary. Every margin here assumes an otherwise empty Titanium; HW3Patch, AMS '
        'files and other installed software all come off the same ceiling.', '',
        '## 8. Evidence and reproduction', '',
        'Tracked measurement tools:', '',
        '- tools/actor_storage_study.py: local-disc conversion, exact split/join, '
        'optimal ZX0 roundtrips, bank accounting and native fixtures.',
        '- tools/actor_costume_increment_study.py: wardrobe subsets, hybrid scopes '
        'and per-pack whole-human controls, reusing the measured corpus.',
        '- tools/actor_parts_bench.c: PC/native byte reconstruction and existing '
        'ZX0/mirror preparation costs; no game changes.',
        '- tools/campaign_pack_report.py: this report and pack-projection.json.',
        '- experiments/archfree/archfree.c: the EM_survey archive measurement, run in '
        'TiEmu on the clean Titanium `.sav` and read back with `ti-mem log`, no screenshot.', '',
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
    print(json.dumps(dict(report=str(args.output), dropped_wardrobe_bytes=wardrobe_measured,
                         conservative_peak=conservative_peak, one_orientation_peak=peak,
                         native=cycles), indent=2))


if __name__ == '__main__':
    main()
