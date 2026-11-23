#!/usr/bin/env python3
"""Source reward pipeline and persisted one-shot receipts, PC first."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile

import review_town_services as review
import proposition_rules
from check_proposition_rules import bytes_c

HERE, ROOT = review.HERE, review.ROOT


def original_adapter(out):
    source = ROOT / 'sources/fft_decomp'
    header = (source / 'include/fft/wldcore.h').read_text()
    def declaration(first, last):
        a = header.index(first); b = header.index(last, a) + len(last)
        return header[a:b] + '\n'
    shapes = declaration('typedef union wldcore_proposition_fields {', '} wldcore_proposition_fields_t;')
    shapes += declaration('typedef struct wldcore_job_selection {', '} wldcore_job_selection_t;')
    (out / 'fft').mkdir(exist_ok=True); (out / 'psx').mkdir(exist_ok=True)
    (out / 'fft/wldcore.h').write_text('''#ifndef ORIGINAL_REWARD_SHIM
#define ORIGINAL_REWARD_SHIM
#include <stdint.h>
typedef int32_t s32;typedef uint32_t u32;typedef uint8_t u8;typedef uint16_t u16;
''' + shapes + '''
enum {WLDCORE_PROPOSITION_DATA_CATEGORY_JOB_SCORES=3,WLDCORE_PROPOSITION_DATA_JOB_LIST_JOB_SCORES=4,
WLDCORE_PROPOSITION_DATA_BRAVERY_LEVEL_SCORES=5,WLDCORE_PROPOSITION_DATA_FAITH_SCORES=6,
WLDCORE_PROPOSITION_DATA_JP_AMOUNTS=8,WLDCORE_PROPOSITION_DATA_GIL_AMOUNTS=9,
WLDCORE_PROPOSITION_DATA_DISCOVERY_REWARDS=10,WLDCORE_PROPOSITION_DATA_RANDOM_REWARDS=11,
WLDCORE_PROPOSITION_DATA_EXTRA_DAY_JP_SCALES=12,WLDCORE_PROPOSITION_DATA_EXTRA_DAY_GIL_SCALES=13,
WLDCORE_PROPOSITION_PROGRESS_RECALLED=2,WLDCORE_PROPOSITION_RESULT_SUCCESS=0,
WLDCORE_PROPOSITION_RESULT_FAILURE=1,WLDCORE_PROPOSITION_RESULT_BAD_FAILURE=2,
WLDCORE_PROPOSITION_REWARD_NONE=0,WLDCORE_PROPOSITION_REWARD_TREASURE=1,
WLDCORE_PROPOSITION_REWARD_UNEXPLORED_LAND=2,WLDCORE_PROPOSITION_REWARD_RANDOM_BONUS=3,
EVENT_SCRIPT_VAR_TREASURE_FIRST=0x321,EVENT_SCRIPT_VAR_UNEXPLORED_LAND_FIRST=0x350,
JOB_ID_SQUIRE=74,JOB_ID_MIME=93,
WLDCORE_PROPOSITION_SPEAKER_HIGHEST_BRAVERY=1,WLDCORE_PROPOSITION_SPEAKER_LOWEST_BRAVERY=2,
WLDCORE_PROPOSITION_SPEAKER_HIGHEST_FAITH=3,WLDCORE_PROPOSITION_SPEAKER_LOWEST_FAITH=4};
typedef struct {u8 flags,proposition_id,elapsed_days,assigned_days,location,participant_count,participant_indices[3];} wldcore_proposition_progress_t;
typedef struct {u8 job_id,bravery,faith,level;} party_data_t;
static wldcore_proposition_progress_t g_main_active_propositions[8];
static wldcore_job_selection_t g_wldcore_job_selection;
static wldcore_proposition_fields_t g_wldcore_selected_proposition_row[1];
static party_data_t original_units[16];static s32 original_vars[1024];
static u16 original_words[4][10],original_rand_value,original_draws;
static s32 world_script_get_variable(s32 id){return original_vars[id];}
static party_data_t *wldcore_get_party_data_pointer(s32 slot){return &original_units[slot];}
static void *wldcore_proposition_get_data_pointer(s32 id){
if(id>=3&&id<=6)return (void *)prop_reward_table(rewards,(u8)(id-3));
if(id==8||id==9)return original_words[id-8];
if(id==10||id==11)return (void *)prop_reward_table(rewards,(u8)(id-4));
return original_words[id-10];}
static s32 original_rand(void){original_draws++;return original_rand_value;}
#endif
''')
    (out / 'psx/types.h').write_text('/* Narrow host source environment. */\n')
    (out / 'psx/libc.h').write_text('#define rand original_rand\n')
    names = ('wldcore_map_job_id_to_category_index', 'wldcore_proposition_select_report_speaker',
             'wldcore_proposition_determine_success', 'wldcore_proposition_calculate_base_jp_and_gil',
             'wldcore_proposition_roll_bonus_reward', 'wldcore_proposition_apply_extra_days_multiplier')
    includes = ''.join('#include "%s"\n' % (source / 'src/wldcore' / (name + '.c')) for name in names)
    (out / 'original_adapter.h').write_text(includes + '''
#undef rand
static u8 original_result(const PropositionState *s,const Save *save,u16 roll){
u8 i,k;u32 gil;const PropositionJob *j=&s->dispatches.job[0];const PropositionReport *r=&s->report;
memset(&g_wldcore_job_selection,0,sizeof g_wldcore_job_selection);
memset(original_vars,0,sizeof original_vars);memcpy(&g_main_active_propositions[0],j,9);
for(i=0;i<23;i++)g_wldcore_selected_proposition_row[0].values[i]=prop_row(rules,j->id)[i];
g_wldcore_selected_proposition_row[0].fields.preferred_job=0; /* Undefined source fallback excluded. */
for(i=0;i<save->n;i++){original_units[i].job_id=save->unit[i].job;original_units[i].bravery=save->unit[i].brave;
original_units[i].faith=save->unit[i].faith;original_units[i].level=save->unit[i].level;}
for(i=0;i<4;i++)for(k=0;k<(i<2?8:10);k++)
original_words[i][k]=prop_word(prop_reward_table(rewards,(u8)(i<2?4+i:6+i))+2*k);
for(i=0;i<31;i++)original_vars[0x321+i]=prop_date(s->treasure_date,i)!=0;
for(i=0;i<16;i++)original_vars[0x350+i]=prop_date(s->land_date,i)!=0;
original_rand_value=roll;original_draws=0;
if(wldcore_proposition_select_report_speaker(0)!=r->speaker)return 0;
wldcore_proposition_determine_success();wldcore_proposition_calculate_base_jp_and_gil();
wldcore_proposition_roll_bonus_reward();wldcore_proposition_apply_extra_days_multiplier();
if(g_wldcore_job_selection.result!=r->result||g_wldcore_job_selection.reward_type!=r->kind)return 0;
if(r->kind&&g_wldcore_job_selection.reward_index!=r->index)return 0;
gil=g_wldcore_job_selection.reward_value;
for(i=0;i<j->n;i++){if(prop_word(r->jp[i])!=g_wldcore_job_selection.rows[0][i])return 0;
gil+=g_wldcore_job_selection.rows[2][i];}
return gil==prop_long(r->gil);}
''')
    return {name: hashlib.sha256((source / 'src/wldcore' / (name + '.c')).read_bytes()).hexdigest() for name in names}


def main():
    ap = argparse.ArgumentParser(description=__doc__); ap.add_argument('--native', action='store_true'); args = ap.parse_args()
    out = HERE / 'x/town-complete/propositions-rewards'; out.mkdir(parents=True, exist_ok=True)
    protected = {p for p in (HERE / 'data').iterdir() if p.is_file() and p.suffix in ('.bin', '.89y', '.sav')}
    protected.add(HERE / 'x/check/ffsave_won.sav')
    for name in ('town-services-review', 'town-portrait-contrast', 'town-services-accepted', 'bar-reader-a',
                 'bar-service', 'bar-rumor-rules', 'propositions-review', 'propositions-rules'):
        protected |= {p for p in (HERE / 'x' / name).rglob('*') if p.is_file() and not p.is_symlink()}
    before = {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    cue = ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    rules, info = proposition_rules.build(str(cue)); rewards, reward_info = proposition_rules.build_rewards(str(cue))
    assert rules == (HERE / 'x/propositions-rules/ffpr.bin').read_bytes()
    # Verify the bounded /100 reciprocal before allowing it in native rewards.
    assert all((n * 5243) >> 19 == n // 100 for n in range(32768))
    (out / 'ffpe.bin').write_bytes(rewards)
    fixture = bytes_c('rules', rules) + bytes_c('rewards', rewards)
    (out / 'fixture.h').write_text(fixture); original = original_adapter(out)
    logs = {}; reference = None
    for sanitize in (False, True):
        label = 'sanitizers' if sanitize else 'pc'; exe = out / ('rewards-' + label)
        flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if sanitize else []
        review.run(['cc', '-std=gnu99', '-O1', '-g', '-no-pie', '-I', out, *flags, '-o', exe, HERE / 'test_proposition_rewards.c'])
        with tempfile.TemporaryDirectory(prefix='fft-proposition-receipt-') as temp:
            snapshot = Path(temp) / 'private-receipt.bin'; logs[label] = review.run([exe, snapshot], env=os.environ.copy())
            assert snapshot.stat().st_size == 4684 + 402
        (out / (label + '.log')).write_text(logs[label])
        if reference is None: reference = logs[label]
        else: assert reference == logs[label]
    runtime = {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest() for name in
               ('proposition_rules.h', 'proposition_rewards.h', 'proposition_transactions.h', 'test_proposition_rewards.c',
                'save_types.h', 'world_types.h', '../base.inc')}
    runtime['fixture'] = hashlib.sha256(fixture.encode()).hexdigest()
    native_path = out / 'native-results.json'; native = None
    if not args.native and native_path.exists():
        previous = json.loads(native_path.read_text())
        if previous['runtime_sha256'] == runtime: native = previous['case']
    if args.native:
        source = out / 'test_proposition_rewards.c'; source.write_bytes((HERE / 'test_proposition_rewards.c').read_bytes())
        exe = out / 'rewardck'
        (out / 'native-build.log').write_text(review.run([ROOT / 'tools/bin/ti-cc', '-I' + str(HERE), '-I' + str(out), '-o', exe, source]))
        log = review.run([ROOT / 'tools/bin/ti-cycles', '--max', '30', exe.with_suffix('.89z')])
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        match = re.search(r'bounds (\d+) (\d+) ([0-9A-F]+) (\d+)', reference)
        expected = [int(match[1]), int(match[2]), int(match[3], 16), int(match[4])]
        assert values[-4:] == expected and not values[-3], (values, expected, log)
        native = dict(values=expected, scope='One private reward arithmetic/receipt/settlement executable; no emulator or shipping modules')
        (out / 'native.log').write_text(log)
        native_path.write_text(json.dumps(dict(case=native, runtime_sha256=runtime), indent=2) + '\n')
    assert before == {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    (out / 'source.json').write_text(json.dumps(dict(catalogue=info['source_sha256'], rewards=reward_info,
                                                    original_routines_sha256=original), indent=2) + '\n')
    result = dict(pc_sanitizers_equal=True, rewards_bank_bytes=len(rewards), state_bytes=402, frozen_receipt_bytes=24,
                  campaign_save_bytes=4684, native=native, protected_files=len(before), shipping_changed=False,
                  scope='Reward/settlement regression with current FSV9 layout; playable flows in check_town_complete.py',
                  adaptation='Preferred-job scan checks assigned units instead of reading beyond the original participant list; original <61 threshold retained')
    (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    (out / 'preserved.json').write_text(json.dumps(before, indent=2) + '\n')
    print(reference.strip()); print('%u-byte reward bank, %u protected files unchanged.' % (len(rewards), len(before)))


if __name__ == '__main__': main()
