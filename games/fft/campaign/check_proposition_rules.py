#!/usr/bin/env python3
"""PC-first original-rule/dispatch/day proof; optional one bounded native run."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import tempfile

import review_town_services as review
import proposition_rules

HERE, ROOT = review.HERE, review.ROOT


def bytes_c(name, raw):
    return 'static const unsigned char %s[]={%s};\n' % (name, ','.join(str(c) for c in raw))


def original_adapter(out):
    """Compile the actual narrow original routines, with only their environment stubbed."""
    source = ROOT / 'sources/fft_decomp'
    header = (source / 'include/fft/wldcore.h').read_text()
    begin = header.index('typedef union wldcore_proposition_fields {')
    end = header.index('} wldcore_proposition_fields_t;', begin) + len('} wldcore_proposition_fields_t;')
    shim = out / 'fft'; shim.mkdir(exist_ok=True)
    (shim / 'wldcore.h').write_text('''#ifndef PROP_ORIGINAL_SHIM
#define PROP_ORIGINAL_SHIM
#include <stdint.h>
typedef int32_t s32;typedef uint32_t u32;typedef uint8_t u8;typedef uint16_t u16;
''' + header[begin:end] + '''
enum {WLDCORE_PROPOSITION_DATA_ROWS=0,WLDCORE_PROPOSITION_DATA_FEES=1,
EVENT_SCRIPT_VAR_SHOP_ITEM_AVAILABILITY=0x6f,EVENT_SCRIPT_VAR_MONTH=0x2d,
EVENT_SCRIPT_VAR_DAY=0x2e,EVENT_SCRIPT_VAR_PROPOSITION_FIRST=0x360,
WLDCORE_PROPOSITION_CONDITION_SCRIPT_VAR_6F=1,WLDCORE_PROPOSITION_CONDITION_MONTH=2,
WLDCORE_PROPOSITION_CONDITION_STATUS=4,WLDCORE_PROPOSITION_VARIABLE_COMPLETED=4,
WLDCORE_PROPOSITION_PROGRESS_RETURNED=4,WLDCORE_PROPOSITION_PROGRESS_RECALLED=2,
WLDCORE_PROPOSITION_STATE_DAYS_MASK=63,WLDCORE_PROPOSITION_STATE_FAILURE=64,
WLDCORE_PROPOSITION_STATE_BAD_FAILURE=128,MAIN_SFX_INVALID=5};
static s32 original_vars[1024];static u8 original_rows[96*23];static u16 original_fees[10];
static struct {u8 flags,id,elapsed_days,assigned_days,location,n,unit[3];} g_main_active_propositions[8];
static u8 g_main_proposition_states[96];
static s32 g_main_save_proposition_count;static u8 g_wldcore_zodiac_start_dates[12][2];
static u8 g_wldcore_days_per_month[12];
static s32 world_script_get_variable(s32 n){return original_vars[n];}
static void *wldcore_proposition_get_data_pointer(s32 n){return n?(void *)original_fees:(void *)original_rows;}
static void wldcore_sound_play_effect(s32 n){(void)n;}
#endif
''')
    (out / 'psx').mkdir(exist_ok=True); (out / 'psx/types.h').write_text('/* Types supplied by the narrow shim. */\n')
    names = ('wldcore_unpack_proposition_row', 'wldcore_convert_date_to_zodiac_date',
             'wldcore_proposition_set_based_on_location', 'wldcore_proposition_advance_daily_counters')
    includes = ''.join('#include "%s"\n' % (source / 'src/wldcore' / (n + '.c')) for n in names)
    (out / 'original_adapter.h').write_text(includes + '''
static void original_init(const u8 *p){u8 i;for(i=0;i<96;i++)memcpy(original_rows+23*(u16)i,prop_row(p,i),23);
for(i=0;i<10;i++)original_fees[i]=prop_word(p+8+2*i);
memcpy(g_wldcore_zodiac_start_dates,source_zodiac,24);memcpy(g_wldcore_days_per_month,world_month_days,12);}
static u16 original_fee(const u8 *p,u8 id){wldcore_proposition_fields_t row;original_init(p);
wldcore_unpack_proposition_row(&row,id);return (u16)(row.values[4]+row.values[5]);}
static u8 original_zodiac(u8 month,u8 day){s32 m=month,d=day;original_init(rules);
wldcore_convert_date_to_zodiac_date(&m,&d);return (u8)m;}
static u8 original_available(const u8 *p,const PropositionJobs *s,u8 id,const PropositionContext *c){
u8 i;original_init(p);memset(original_vars,0,sizeof original_vars);
for(i=0;i<96;i++)original_vars[0x360+i]=prop_status(s,i);
original_vars[0x6f]=c->tier;
original_vars[EVENT_SCRIPT_VAR_MONTH]=source_zodiac[2*(c->zodiac-1)];
original_vars[EVENT_SCRIPT_VAR_DAY]=source_zodiac[2*(c->zodiac-1)+1];
return !(prop_status(s,id)&6)&&!(s->retry[id]&63)&&wldcore_proposition_set_based_on_location(id,c->location+1);}
static void original_load(const PropositionJobs *s){g_main_save_proposition_count=s->n;
memcpy(g_main_active_propositions,s->job,sizeof s->job);memcpy(g_main_proposition_states,s->retry,96);}
static void original_day(void){wldcore_proposition_advance_daily_counters();}
static u8 original_equal(const PropositionJobs *s){return !memcmp(s->job,g_main_active_propositions,sizeof s->job)&&
!memcmp(s->retry,g_main_proposition_states,96);}
''')
    return {n: hashlib.sha256((source / 'src/wldcore' / (n + '.c')).read_bytes()).hexdigest() for n in names}


def main():
    ap = argparse.ArgumentParser(description=__doc__); ap.add_argument('--native', action='store_true'); args = ap.parse_args()
    out = HERE / 'x/town-complete/propositions-rules'; out.mkdir(parents=True, exist_ok=True)
    protected = {p for p in (HERE / 'data').iterdir() if p.is_file() and p.suffix in ('.bin', '.89y', '.sav')}
    protected.add(HERE / 'x/check/ffsave_won.sav')
    for name in ('town-services-review', 'town-portrait-contrast', 'town-services-accepted',
                 'bar-reader-a', 'bar-service', 'bar-rumor-rules', 'propositions-review'):
        protected |= {p for p in (HERE / 'x' / name).rglob('*') if p.is_file() and not p.is_symlink()}
    before = {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    cue = ROOT / 'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    raw, info = proposition_rules.build(str(cue)); (out / 'ffpr.bin').write_bytes(raw)
    # Refuse a catalogue inconsistent with the accepted original Gariland sample.
    assert info['titles'][30] == 'Testimony of Ex-miner' and info['rows'][30][6:8] == [8, 11]
    disc, files = proposition_rules.extract.disc_files(str(cue)); core = disc.read(*files['WORLD/WLDCORE.BIN'])
    zodiac = core[0x8009e678 - 0x80067000:0x8009e678 - 0x80067000 + 24]
    fixture = bytes_c('rules', raw) + bytes_c('source_zodiac', zodiac) + bytes_c('breed_rules', (HERE / 'data/ffwb.bin').read_bytes())
    (out / 'fixture.h').write_text(fixture)
    source_hashes = original_adapter(out)
    logs = {}; bounds = None
    for sanitize in (False, True):
        label = 'sanitizers' if sanitize else 'pc'
        flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie'] if sanitize else []
        exe = out / ('rules-' + label)
        review.run(['cc', '-std=gnu99', '-O1', '-g', '-no-pie', '-I', out, *flags, '-o', exe, HERE / 'test_proposition_rules.c'])
        with tempfile.TemporaryDirectory(prefix='fft-proposition-jobs-') as temp:
            path = Path(temp) / 'private-jobs.bin'
            logs[label] = review.run([exe, path], env=os.environ.copy())
            assert path.stat().st_size == 4684 + 217
        (out / (label + '.log')).write_text(logs[label])
        if bounds is None: bounds = logs[label]
        else: assert logs[label] == bounds
    runtime_hashes = {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest() for name in
                      ('proposition_rules.h', 'test_proposition_rules.c', 'save_types.h', 'world_types.h', 'breeding.h', '../base.inc')}
    runtime_hashes['fixture'] = hashlib.sha256(fixture.encode()).hexdigest()
    native = None; native_path = out / 'native-results.json'
    if not args.native and native_path.exists():
        previous = json.loads(native_path.read_text())
        if previous['runtime_sha256'] == runtime_hashes:
            native = previous['case']
    if args.native:
        # Build from a private source path so GCC4TI never creates intermediates
        # beside tracked files or the user's unrelated assembly files.
        source = out / 'test_proposition_rules.c'; source.write_bytes((HERE / 'test_proposition_rules.c').read_bytes())
        exe = out / 'propchk'
        (out / 'native-build.log').write_text(review.run([ROOT / 'tools/bin/ti-cc', '-I' + str(HERE), '-I' + str(out), '-o', exe, source]))
        log = review.run([ROOT / 'tools/bin/ti-cycles', '--max', '20', exe.with_suffix('.89z')])
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        match = re.search(r'bounds (\d+) (\d+) ([0-9A-F]+) (\d+)', bounds)
        expected = [int(match[1]), int(match[2]), int(match[3], 16), int(match[4])]
        assert values[-4:] == expected and values[-3] == 0, (values, expected, log)
        native = dict(values=expected, scope='One private cold-bank/dispatch/day boundary executable; no emulator or shipping modules')
        (out / 'native.log').write_text(log)
        native_path.write_text(json.dumps(dict(case=native, runtime_sha256=runtime_hashes), indent=2) + '\n')
    assert before == {str(p.relative_to(ROOT)): review.fingerprint(p) for p in sorted(protected)}
    (out / 'source.json').write_text(json.dumps(dict(**info, original_routines_sha256=source_hashes), indent=2) + '\n')
    result = dict(pc_sanitizers_equal=True, bank_bytes=len(raw), job_state_bytes=217, campaign_save_bytes=4684,
                  native=native, protected_files=len(before), shipping_changed=False,
                  scope='Rules/dispatch/day regression with current FSV9 layout; playable flows in check_town_complete.py')
    (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    (out / 'preserved.json').write_text(json.dumps(before, indent=2) + '\n')
    print(logs['pc'].strip()); print('%u-byte private catalogue; %u protected files unchanged.' % (len(raw), len(before)))


if __name__ == '__main__': main()
