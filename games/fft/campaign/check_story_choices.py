#!/usr/bin/env python3
"""FCX1 backend carrier; real scene flows are checked separately, no implicit TI work."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from check_flow import HERE, ROOT, run
import story_choice


def specs():
    recruit = dict(done=40, guard=dict(flag=3, value=True), options=[
        dict(flag=41, label='Do not recruit', effects=[dict(op='remove_guest', id=4), dict(op='clear_flag', id=70)]),
        dict(flag=42, label='Recruit', effects=[dict(op='set_flag', id=71), dict(op='recruit_guest', id=4), dict(op='clear_flag', id=70)])])
    multi = dict(done=100, guard=dict(flag=50, value=False), options=[
        dict(flag=101, label='Not now'), dict(flag=102, label='Join us', effects=[dict(op='set_flag', id=256),
            dict(op='recruit_guest', id=4), dict(op='remove_guest', id=11), dict(op='recruit_guest', id=12), dict(op='clear_flag', id=70)])])
    return dict(recruit_choice=recruit, multi_choice=multi)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--native', action='store_true')
    args = ap.parse_args(); out = HERE/'x/story-choices'; out.mkdir(exist_ok=True)
    banks = {name: story_choice.build(spec) for name, spec in specs().items()}
    (out/'fixture.h').write_text('// Private, generated FCX1 fixtures.\n'+''.join(
        'static const u8 '+name+'[]={'+','.join(map(str, raw))+'};\n' for name, raw in banks.items()))
    logs = []
    for mode, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-pie', '-no-pie'])]:
        (out/f'{mode}-build.log').write_text(run(['cc', '-std=gnu99', '-O1', '-g', '-Wall', '-Wextra', *flags,
            '-o', out/mode, HERE/'test_story_choice.c']))
        log = run(['env', 'ASAN_OPTIONS=detect_leaks=1', 'UBSAN_OPTIONS=halt_on_error=1', out/mode])
        (out/f'{mode}.log').write_text(log); logs.append(log)
    assert logs[0] == logs[1]
    if args.native:
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'choiceck', HERE/'test_story_choice.c']))
        native = run([ROOT/'tools/bin/ti-cycles', '--frames', 0, '--max', 2000, out/'choiceck.89z'])
        (out/'native.log').write_text(native)
        match = re.search(r'story-choice (\w+) failures 0 checks (\d+)', logs[0]); assert match
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', native)]
        assert values[-3:] == [int(match[1], 16), 0, int(match[2])], values
    result = dict(result=logs[0].strip(), native=args.native, scene_flows_checked_separately=True,
        bank_bytes={name: len(raw) for name, raw in banks.items()}, source_sha256={str(p.relative_to(ROOT)):
            hashlib.sha256(p.read_bytes()).hexdigest() for p in [HERE/'story_choice.h', HERE/'test_story_choice.c', HERE.parent/'tools/story_choice.py']})
    (out/'results.json').write_text(json.dumps(result, indent=2)+'\n'); print(json.dumps(result, indent=2))


if __name__ == '__main__': main()
