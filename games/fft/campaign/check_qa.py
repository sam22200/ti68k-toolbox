#!/usr/bin/env python3
"""Playtest reproducers on PC/actual68000; candidate pictures need user review."""
from pathlib import Path
import re
import subprocess
import tempfile
import sys

sys.path.insert(0,str(Path(__file__).resolve().parent.parent/"tools"))
from review_files import publish_images

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = HERE / 'x/qa'
BANKS = 'ffbm ffbn ffov0 ffov1 ffov2 ffov3 ffoa ffoi ffost ffod ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split()


def run(args, cwd):
    p = subprocess.run(list(map(str, args)), cwd=cwd, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        print(p.stdout)
        p.check_returncode()
    return p.stdout


def render(args, folder, cwd):
    with tempfile.TemporaryDirectory(prefix='fft-qa-images-') as temp:
        result=run([temp if Path(str(a))==folder else a for a in args],cwd)
        publish_images(temp,folder)
        return result


def main():
    data = OUT / 'data'
    data.mkdir(parents=True, exist_ok=True)
    (OUT / 'after').mkdir(exist_ok=True)
    for name in BANKS:
        for chapter in ('campaign', 'opening'):
            for suffix in ('.bin', '.89y'):
                src = HERE.parent / chapter / 'data' / (name + suffix)
                dst = data / src.name
                if src.exists() and not dst.exists():
                    dst.symlink_to(src)
    common = [HERE / 'check_qa.c', ROOT / 'runtime/core/rt_core.c']
    run(['cc', '-std=gnu99', '-O1', '-DQA_NO_VISUALS', '-o', OUT / 'qa', *common,
         ROOT / 'runtime/platform-sw/rt_sw.c'], data)
    pc = render([OUT / 'qa', OUT / 'after'], OUT/'after', data)
    run([ROOT / 'tools/bin/ti-cc', '-DRT_CYCLES', '-DQA_NO_VISUALS', '-o', OUT / 'qac', *common,
         ROOT / 'lib/unpack68k.s', ROOT / 'tools/extgraph/lib/tilemap.a'], data)
    command = [ROOT / 'tools/bin/ti-cycles', '--frames', 0, '--arg', 101, '--max', 10000]
    for bank in BANKS:
        command += ['--file', data / (bank + '.89y')]
    ti = run([*command, OUT / 'qac.89z'], data)
    (OUT / 'native.log').write_text(ti)
    (OUT / 'pc.log').write_text(pc)
    match = re.search(r'qa ([0-9A-F]+) failure 0 checks (\d+)', pc)
    assert match, pc
    values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', ti)]
    assert values[-3:] == [int(match[1], 16), 0, int(match[2])], ti[-6000:]
    print('QA: %s checks, candidate screens/cache/name checks match PC/68000; hash %s' %
          (match[2], match[1]))
    source = [HERE / 'check_qa_turns.c', ROOT / 'runtime/core/rt_core.c']
    run(['cc', '-std=gnu99', '-O1', '-o', OUT / 'turns', *source,
         ROOT / 'runtime/platform-sw/rt_sw.c', '-lm'], data)
    pc = render([OUT / 'turns', OUT / 'after'], OUT/'after', data)
    run([ROOT / 'tools/bin/ti-cc', '-DRT_CYCLES', '-o', OUT / 'turnsc', *source,
         ROOT / 'lib/unpack68k.s', ROOT / 'tools/extgraph/lib/tilemap.a'], data)
    ti = run([*command, OUT / 'turnsc.89z'], data)
    (OUT / 'turns-native.log').write_text(ti)
    values = [int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', ti)]
    match = re.search(r'turns ([0-9A-F]+) failures 0 checks (\d+)',pc)
    assert match and values[-3:] == [int(match[1],16),0,int(match[2])], ti[-4000:]
    assert len(re.findall(r'shot \d+ checksum',ti)) == 41  # runtime initialization +40 trial frames
    (OUT / 'half-turns').mkdir(exist_ok=True)
    run(['cc', '-std=gnu99', '-O1', '-DQA_HALF_TURN', '-o', OUT / 'half-turns-preview', *source,
         ROOT / 'runtime/platform-sw/rt_sw.c'], data)
    render([OUT / 'half-turns-preview', OUT / 'half-turns'], OUT/'half-turns', data)
    print('Rotation:40 source/moving/destination frames match PC/68000, both keys/all views; hash '+match[1])
    run(['cc', '-std=gnu99', '-O1', '-o', OUT / 'preview', *common,
         ROOT / 'runtime/platform-sw/rt_sw.c'], data)
    render([OUT / 'preview', OUT / 'after'], OUT/'after', data)
    source = [HERE / 'check_qa_motion.c', ROOT / 'runtime/core/rt_core.c']
    run(['cc', '-std=gnu99', '-O1', '-o', OUT / 'motion', *source,
         ROOT / 'runtime/platform-sw/rt_sw.c'], data)
    pc = run([OUT / 'motion'], data)
    run([ROOT / 'tools/bin/ti-cc', '-DRT_CYCLES', '-o', OUT / 'motionc', *source,
         ROOT / 'lib/unpack68k.s', ROOT / 'tools/extgraph/lib/tilemap.a'], data)
    ti = run([*command, OUT / 'motionc.89z'], data)
    (OUT / 'motion-native.log').write_text(ti)
    (OUT / 'motion-pc.log').write_text(pc)
    match = re.search(r'motion ([0-9A-F]+) failures 0 checks (\d+)', pc)
    values = [int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
    assert match and values[-3:] == [int(match[1],16),0,int(match[2])], (pc,ti[-4000:])
    print('Recoil: '+match[2]+' warmed-cache/first-frame/headroom checks match PC/68000; hash '+match[1])
    render([OUT / 'motion', OUT], OUT, data)
    source = [HERE / 'check_cursor_frames.c', ROOT / 'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-o',OUT/'cursor-frames',*source,ROOT/'runtime/platform-sw/rt_sw.c'],data)
    pc=run([OUT/'cursor-frames'],data)
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',OUT/'cursc',*source,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'],data)
    ti=run([*command,OUT/'cursc.89z'],data)
    (OUT/'cursor-native.log').write_text(ti);(OUT/'cursor-pc.log').write_text(pc)
    match=re.search(r'cursor ([0-9A-F]+) failures 0 checks (\d+)',pc)
    values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
    assert match and values[-3:]==[int(match[1],16),0,int(match[2])],(pc,values[-5:])
    print('Cursor:'+match[2]+' actual input/AI-glide first-frame mask checks match PC/68000; hash '+match[1])


if __name__ == '__main__':
    main()
