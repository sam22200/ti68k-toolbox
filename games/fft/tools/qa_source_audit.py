#!/usr/bin/env python3
"""Read original deployment/reward inputs; never infer final loot from an initial ENTD.

Usage: qa_source_audit.py DISC.cue GARILAND_DEPLOYMENT.state
The input must be the ATTACK overlay state produced by mandalia_preflight.py.
No input is sent and no state is written back to the original game.
"""
import hashlib
import json
from pathlib import Path
import sys

import battle
import extract
import oracle

HERE = Path(__file__).resolve().parents[1]


def main():
    cue, state = sys.argv[1], Path(sys.argv[2])
    disk, files = extract.disc_files(cue)
    d = battle.Disc(cue)
    attack = disk.read(*files['EVENT/ATTACK.OUT'])
    scenarios = []
    for at in range(0x801CF938-0x801BF000, len(attack)-23, 24):
        raw = attack[at:at+24]
        event = int.from_bytes(raw[:2], 'little')
        if not event:
            break
        if event == 9:
            scenarios.append(dict(event=event, map=raw[2],
                entd=int.from_bytes(raw[7:9], 'little'),
                squad=int.from_bytes(raw[9:11], 'little')))
    assert scenarios == [dict(event=9, map=22, entd=0x184, squad=256)], scenarios
    p = oracle.PSX(cue)
    p.load(str(state))
    assert oracle.var(p, oracle.EVENT) == 9, 'Requires the live deployment overlay'
    pointer = p.read(0x801CD270, 4)
    assert 0x80000000 <= pointer <= 0x80200000-258*12
    raw = bytes(p.read(pointer+256*12+i) for i in range(12))
    assert raw[8:10] == bytes((5,22)), raw.hex()
    bitmap = int.from_bytes(raw[:4], 'little')
    roster = []
    for i in range(16):
        rec = bytes(p.read(0x80057F74+256*i+j) for j in range(256))
        if rec[1] != 255:
            roster.append(dict(slot=i, party_id=rec[1], job=rec[2],
                               level=rec[0x16], record_sha256=hashlib.sha256(rec).hexdigest()))
    assert len(roster) == 7 and [r['job'] for r in roster] == [1,74,74,75,74,74,75], roster
    rewards = []
    for i in range(6):
        entd = d.entd(0x184, i)
        rewards.append(dict(entd_slot=i, trophy=entd[0x1E], money_modifier=entd[0x1F],
                            initial_level=entd[3]))
    report = dict(
        scope='Read-only original deployment state and disc ENTD; no native deployment/reward implementation.',
        state_sha256=hashlib.sha256(state.read_bytes()).hexdigest(),
        scenario=scenarios[0],
        deployment=dict(record_hex=raw.hex(), bitmap=bitmap,
            center=list(raw[4:6]), orientation_flags=raw[7], limit=raw[8]),
        initial_roster=roster, initial_reward_inputs=rewards,
        original_rules={
            'gray_jobs':'world_job_build_unit_list(mode2): unlocked by another roster member, unavailable to this unit.',
            'bonus_money':'battle_unit_find_war_trophies_and_bonus_money: enemy-level sum times100; also computes conditional level bonus.',
            'war_trophies':'Collect present units\' current trophy and money fields; the initial ENTD is not a final-result oracle.',
        },
        remaining=['Keep all seven recruits before deployment, preserve unselected reserve units on victory.',
                   'Persist an explicit legal five-slot placement and consume it in party_load.',
                   'Add money persistence and source-observed final rewards, with no results screens after Orbonne.',
                   'Restore dark narration, chapter/location titles, deployment and Gariland pre/post-battle dialogues.'])
    out = HERE/'campaign/x/qa/source-audit.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2)+'\n')
    print('Source QA: seven Gariland recruits, five deployment slots; reward inputs exported, final rewards not inferred.')


if __name__ == '__main__':
    main()
