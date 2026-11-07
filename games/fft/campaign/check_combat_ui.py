#!/usr/bin/env python3
"""Block C existing battle presentation, PC/sanitizer/68000 pixels."""
import json
from check import HERE
from check_buffs import trial
if __name__=='__main__':
 out=HERE/'x/core-combat/ui'
 result=trial('combat_ui','check_combat_ui.c',out,False)
 result['pictures']=4
 (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
