# compact view of mode7.s between two addresses: python tools/view.py 1bbc 2002
import re,sys
a0,a1=int(sys.argv[1],16),int(sys.argv[2],16)
on=False
for l in open('mode7.s'):
    m=re.search(r'\| ([0-9a-f]{4}) ?(.*)',l)
    if m:
        a=int(m.group(1),16); on = a0<=a<a1
        if on:
            ins=l.split('|')[0].strip().replace('\t',' ')
            print('%s %s%s'%(m.group(1),ins,('  ; '+m.group(2)) if m.group(2).strip() else ''))
    elif on and re.match(r'^[.\w]+:',l): print(l.strip())
