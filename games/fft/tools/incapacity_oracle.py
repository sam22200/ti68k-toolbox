#!/usr/bin/env python3
"""Original ordinary-team outcome and post-formula Petrify validity, not new producers."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function,mask


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-incapacity/oracle';out.mkdir(parents=True,exist_ok=True)
    d=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    for key,name in dict(outcome='battle_action_check_battle_outcome',final='battle_action_finalize_target_current_action',sort='battle_target_sort_list').items():bind_function(key,name,'battle',d)
    o=s.Original(d);o.uc.mem_write(0x10000,d.exe[0x800:]);o.instruction_limit=200000
    o.function_hashes['init']=bind_function('init','main_status_init_check_data','main',d);o.call('init')
    masks=[bytes(o.uc.mem_read(0x662d0+5*i,5))for i in range(4)]
    unit=0x801908cc;states=[(),(2,),(8,),(1,),(15,),(35,),(30,),(9,),(13,),(),(5,)]
    def reset():
        o.uc.mem_write(unit&0x1fffff,bytes(21*448))
        for i in range(21):o.uc.mem_write((unit&0x1fffff)+448*i+1,b'\xff')
    def actor(i,state,red=0,importance=0,disabled=0):
        at=(unit&0x1fffff)+448*i
        o.uc.mem_write(at+1,bytes((255 if state==9 else i,)))
        o.uc.mem_write(at+0x1ba,bytes(((0x10 if red else 0)|importance,)))
        o.uc.mem_write(at+0x183,bytes((0x80 if disabled else 1,)))
        o.uc.mem_write(at+0x58,mask(*states[state]));o.uc.mem_write(at+0x28,struct.pack('<H',0 if state in(1,3,4)else 100))
    profiles=[(p,0,9)for p in range(11)]+[(0,e,9)for e in range(11)]
    profiles += [(1,1,9),(2,2,9),(1,2,9),(2,1,9),(1,0,0),(2,0,0),(1,2,0),(2,1,0)]
    rows=[]
    for p,e,g in profiles:
        reset();actor(0,p);actor(12,e,1);actor(6,g)
        result=o.call('outcome');native=2 if result==0xffffffff else 1 if result==0 else 0
        rows.append([p,e,g,native])
    objectives=[]
    for red,state,disabled in ((0,2,0),(0,7,0),(1,7,0),(0,0,1),(1,0,1)):
        reset();actor(0,0);actor(12,0,1);actor(6,state,red,3,disabled)
        objectives.append(dict(red=red,state=state,disabled=disabled,result=o.call('outcome')))
    def sort_only(uc,address,size,data):uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    at=s.FUNCTIONS['sort'][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,sort_only,begin=at,end=at)
    finals=[]
    for petrify in(0,1):
        for cancel in(0,1):
            reset();actor(0,0);actor(12,2 if petrify else 0,1)
            target=unit+12*448;action=target+0x18c
            o.write(0x80192d94,unit);o.write(0x80192d98,target);o.write(0x80192d90,action)
            o.uc.mem_write(0x1938c0,bytes(0x60));o.uc.mem_write(0x193860,mask(*([8]if cancel else[])))
            o.uc.mem_write(0x18f5f4,bytes(16));o.write(0x8018f5f0,0)
            o.uc.mem_write(action&0x1fffff,b'\x01');o.uc.mem_write((action&0x1fffff)+4,struct.pack('<H',7))
            o.uc.mem_write((action&0x1fffff)+0x2a,struct.pack('<H',100));o.call('final')
            raw=bytes(o.uc.mem_read(action&0x1fffff,44));finals.append([petrify,cancel,raw[0],struct.unpack_from('<H',raw,4)[0]])
    lines=['/* Original ordinary outcome and post-formula validity; local-only. */']
    for name,data in [('source_outcomes',rows),('source_stone_final',finals)]:
        lines.append(f'static const u16 {name}[][{len(data[0])}]={{');lines.extend(' {'+','.join(map(str,p))+'},'for p in data);lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(outcomes=rows,finals=finals,source_only_importance3=objectives,status_masks=[b.hex()for b in masks],state_ids=states,
        functions=o.function_hashes,exe_sha256=s.core_audit.sha(d.exe),battle_sha256=s.core_audit.sha(d.bat),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Unchanged ordinary team-outcome and prepared target-finalizer instructions, graphical sort isolated. Source-only importance3 objectives and cancelling Petrify are recorded, not enabled natively. No Petrify infliction/cure/presentation, physical RNG or full action/HP/reward trace.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original incapacity:',len(rows),'ordinary outcomes,',len(finals),'finalizers; masks',[b.hex()for b in masks])


if __name__=='__main__':main()
