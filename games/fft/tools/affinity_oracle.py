#!/usr/bin/env python3
"""Original job/equipment elemental aggregation; isolate unrelated stat arithmetic."""
import json,struct,subprocess
import surface_data as s
from status_state_oracle import bind_function


def main():
    import unicorn
    out=s.HERE/'campaign/x/core-affinity/oracle';out.mkdir(parents=True,exist_ok=True)
    disc=s.battle.Disc(str(s.core_audit.DEFAULT_DISC))
    names=dict(job='main_unit_copy_job_data',gear='main_unit_set_equipment_attributes',
               actual='main_unit_calculate_actual_stats')
    o=s.Original(disc);o.uc.mem_write(0x10000,disc.exe[0x800:]);o.instruction_limit=200000
    hashes={k:bind_function(k,n,'main',disc)for k,n in names.items()}
    o.write(0x80066194,s.battle.JOBS)
    def actual(uc,address,size,data):
        uc.reg_write(o.reg.UC_MIPS_REG_V0,0)
        uc.reg_write(o.reg.UC_MIPS_REG_PC,uc.reg_read(o.reg.UC_MIPS_REG_RA))
    at=s.FUNCTIONS['actual'][0];o.uc.hook_add(unicorn.UC_HOOK_CODE,actual,begin=at,end=at)
    jobs=[disc.at(s.battle.JOBS+48*j+0x29,4)+b'\0'for j in range(160)]
    items=[disc.at(s.battle.ATTRS+25*disc.at(s.battle.ITEMS+12*i+7,1)[0]+20,5)for i in range(256)]
    representatives={}
    for i,row in enumerate(items[:255]):
        if any(row):representatives.setdefault(row,i)
    profiles=[(j,[255]*5)for j in range(160)]
    profiles += [(0x4a,[i,255,255,255,255])for i in representatives.values()]
    chosen=list(representatives.values())
    profiles += [(0x4a,chosen[k:k+5]+[255]*max(0,5-len(chosen[k:k+5])))for k in range(0,len(chosen),5)]
    if chosen:profiles += [(0x5e,[chosen[0]]*5),(0x4a,[chosen[0]]*5)]
    rows=[];ptr=0x801d4000
    for job,eq in profiles:
        o.uc.mem_write(ptr&0x1fffff,bytes(0x1c0));o.uc.mem_write((ptr&0x1fffff),bytes((0x80,0,0,job,0,0,0x80)))
        o.uc.mem_write((ptr&0x1fffff)+0x1a,bytes((*eq,255,255)))
        # Poison the old cache: job copy must replace it before equipment merges.
        o.uc.mem_write((ptr&0x1fffff)+0x6d,b'\xff'*5)
        o.call('job',ptr);o.call('gear',ptr,0)
        result=bytes(o.uc.mem_read((ptr&0x1fffff)+0x6d,5));expected=bytearray(jobs[job])
        for item in eq:
            if item!=255:
                for k in range(5):expected[k]|=items[item][k]
        assert result==bytes(expected),(job,eq,result,expected)
        rows.append([job,*eq,*result])
    lines=['/* Original job/equipment affinity cache; equipment legality is separate. */',
           'static const u8 source_affinity[][11]={']
    lines+=[' {'+','.join(map(str,row))+'},'for row in rows];lines.append('};')
    (out/'source.h').write_text('\n'.join(lines)+'\n')
    report=dict(cases=rows,jobs=[list(r)for r in jobs],items=[list(r)for r in items],
        distinct_item_masks=len(representatives),functions=hashes,exe_sha256=s.core_audit.sha(disc.exe),
        decomp_commit=subprocess.check_output(['git','-C',s.ROOT/'sources/fft_decomp','rev-parse','HEAD'],text=True).strip(),
        scope='Unchanged original job-copy and equipment attribute loops, ordinary-human flags; actual-stat arithmetic stubbed. All160 job bases, distinct item masks, overlapping/duplicate gear OR. Arbitrary fixture gear is not legal-equipment certification. Monster equipment policy, status traits, elemental formula/application and battle lifecycle are separate.')
    (out/'source.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Original affinity:',len(rows),'job/equipment aggregations;',len(representatives),'distinct nonzero item masks')


if __name__=='__main__':main()
