#!/usr/bin/env python3
"""Private local TI bundle, with all data banks and exact file hashes."""
import hashlib,json,zipfile
from reference import GAME
def main():
    names=('mmx.89z','mmxmap.89y','mmxart.89y')
    manifest={n:dict(bytes=(GAME/n).stat().st_size,sha256=hashlib.sha256((GAME/n).read_bytes()).hexdigest()) for n in names}
    readme=('Mega Man X - opening Highway section\n\n'
        'Send all three TI files. Archive mmxmap and mmxart, then run mmx().\n'
        'Arrows: run. 2nd: jump/wall jump. Shift: shoot/hold to charge/release.\n'
        'Enter: retry. Esc: quit.\n\n'
        'Short opening section, initial equipment and one roller.\n'
        'Headless PC/TI and original reference tests pass; hardware validation pending.\n')
    (GAME/'x').mkdir(exist_ok=True);output=GAME/'x/mmx-ti.zip'
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
        for n in names:z.write(GAME/n,n)
        z.writestr('README.txt',readme);z.writestr('manifest.json',json.dumps(manifest,indent=2)+'\n')
    with zipfile.ZipFile(output) as z:assert z.testzip() is None
    print('Packaged',output,output.stat().st_size,'bytes')
if __name__=='__main__':main()
