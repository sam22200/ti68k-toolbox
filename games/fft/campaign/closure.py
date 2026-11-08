#!/usr/bin/env python3
"""Core family closure: run the core-*-check suites in parallel, skipping the
suites whose inputs are unchanged since their last green run.

Inputs are recorded, not guessed: each suite runs under strace, and every file
under the workspace that make, the compilers, the Python checks or ti-cycles
opened or looked up (stat) is hashed, absent paths included (an include file
created later invalidates the cache). A suite is skipped only when all of them
are byte-identical, so its result cannot differ. Toolchains (tools/pyenv,
tools/gcc4ti-bin) are excluded from the key; pass --force after changing them.

  closure.py                 all suites, cached, -j 12
  closure.py guts talk-skill only these (still cached)
  closure.py --first guts    run guts first; --fail-fast stops scheduling at the first red
  closure.py --all           also the 26 older suites (66 core-*-check targets)
"""
import argparse,concurrent.futures as cf,hashlib,json,os,re,subprocess,sys,threading,time
from pathlib import Path

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
CACHE=HERE/'x/closure-cache'
SUITES=('guts punch-strikes punch-art talk-skill area-impact area-ui area-forecast targets batch knockback '
        'headroom protect control-status status-lifetime sleep terminal scheduler actions ai-pause reactions '
        'banks packages area-ai final-combat identity restoration cleanse control incapacity area-ct buffs charge-hands hands healing '
        'offensive-magic percent reflect status-state strikes surface steal-exp breaks equip-breaks mp-break steal-equip gil geomancy secret-fist line draw-out performance allegiance float throw math stat-save critical-reactions lancer-jump status-reactions brave-faith-reactions counter-reactions movement equipment combat discovery mime hamedo weapon-reach weapon-formulas weapons ai-interactions melee-collision arcs panel-victims direct-interception').split()
# Older family suites outside the usual closure: run with --all after a core-wide change.
# All 63 suites are green since 2026-11-01 (five stale expectations repaired, CORE_TESTING.md).
EXTRA=('affinity boundaries capacity-render casting cell compact context deferred elements full-grid grid '
       'haste-slow incantation jump jump-actors jump-cost long-jump magic magic-evasion profile profile-deep '
        'span-search status-rules step step-cache weather').split()
# Shared generated data that more than one suite would otherwise rebuild concurrently.
SHARED=['data/ffbm.89y','data/ffbn.89y','data/ffbo.89y','data/ffcj.89y','data/ffgd.89y','data/fftitle.89y']
EXCLUDE=[ROOT/'tools/pyenv',ROOT/'tools/gcc4ti-bin',CACHE]
BIG=8<<20   # larger files (the disc image) are keyed by size and mtime
LINE=re.compile(r'^(\d+)\s+(.*)$')
CALL=re.compile(r'^(\w+)\((.*)\)\s+=\s+(-?\d+)')
PATHS=('openat','open','newfstatat','stat','lstat','statx','access','faccessat','faccessat2','execve')


def file_key(p):
    try:st=p.stat()
    except FileNotFoundError:return 'absent'
    if not p.is_file():return 'dir'
    if st.st_size>BIG:return 'big:%d:%d'%(st.st_size,st.st_mtime_ns)
    return hashlib.sha1(p.read_bytes()).hexdigest()


def inputs(log,cwd):
    """Files read or looked up under the workspace. strace -y prints each call's working
    directory (AT_FDCWD</dir>); calls without a dirfd use the process's last known one."""
    read,written=set(),set();cwds={};pending={}
    for line in log.read_text(errors='replace').splitlines():
        m=LINE.match(line)
        if not m:continue
        pid,body=m.groups()
        if body.endswith('<unfinished ...>'):pending[pid]=body[:-len('<unfinished ...>')].rstrip();continue
        r=re.match(r'^<\.\.\. (\w+) resumed>(.*)$',body)
        if r:
            if pid not in pending:continue
            body=pending.pop(pid)+r.group(2)
        c=CALL.match(body)
        if not c or c.group(1) not in PATHS:continue
        call,argv=c.group(1),c.group(2)
        d=re.match(r'AT_FDCWD<([^>]*)>',argv)
        if d:cwds[pid]=d.group(1)
        q=re.search(r'"((?:[^"\\]|\\.)*)"',argv)
        if not q:continue
        path=q.group(1)
        base=d.group(1) if d else cwds.get(pid,str(cwd))
        p=Path(os.path.normpath(path if path.startswith('/') else os.path.join(base,path)))
        if ROOT not in p.parents or any(e==p or e in p.parents for e in EXCLUDE):continue
        if call in('openat','open')and re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC',argv):written.add(p);continue
        read.add(p)
    return {str(p):file_key(p) for p in sorted(read-written)}


def valid(suite):
    f=CACHE/(suite+'.json')
    if not f.exists():return None
    c=json.loads(f.read_text())
    return c if all(file_key(Path(p))==k for p,k in c['inputs'].items()) else None


def run(suite,force):
    if not force and (c:=valid(suite)):return suite,0,c['summary'],True
    CACHE.mkdir(parents=True,exist_ok=True)
    log=CACHE/(suite+'.strace');out=CACHE/(suite+'.log')
    with out.open('w')as o:
        rc=subprocess.run(['strace','-f','-qq','-y','-o',str(log),'-e','trace=openat,open,newfstatat,stat,lstat,statx,access,faccessat,faccessat2,execve',
                           'make','core-%s-check'%suite],cwd=HERE,stdout=o,stderr=subprocess.STDOUT).returncode
    text=out.read_text(errors='replace')
    traced=True
    if rc and 'LeakSanitizer has encountered a fatal error' in text:
        # LeakSanitizer cannot run under ptrace: rerun untraced; such a suite is never cached.
        with out.open('w')as o:rc=subprocess.run(['make','core-%s-check'%suite],cwd=HERE,stdout=o,stderr=subprocess.STDOUT).returncode
        text=out.read_text(errors='replace');traced=False
    summary=' '.join(re.findall(r'\S+ \w+ failures \d+ checks \d+',text))
    if traced and rc==0 and summary and 'failures 0' in summary and not re.search(r'failures [1-9]',summary):
        (CACHE/(suite+'.json')).write_text(json.dumps(dict(summary=summary,inputs=inputs(log,HERE)),indent=0))
    log.unlink(missing_ok=True)
    return suite,rc,summary,False


def main():
    ap=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('suites',nargs='*');ap.add_argument('-j',type=int,default=12)
    ap.add_argument('--first',default='',help='comma-separated suites scheduled first')
    ap.add_argument('--fail-fast',action='store_true');ap.add_argument('--all',action='store_true',help='also the older suites (EXTRA)');ap.add_argument('--force',action='store_true')
    args=ap.parse_args()
    suites=args.suites or list(SUITES)+(list(EXTRA) if args.all else [])
    first=[s for s in args.first.split(',') if s]
    suites=[s for s in first if s in suites]+[s for s in suites if s not in first]
    t0=time.time()
    subprocess.run(['make',*SHARED],cwd=HERE,stdout=subprocess.DEVNULL,check=True)   # serial, once
    stop=threading.Event();results={}
    def task(s):
        if stop.is_set():return s,None,'not run',False
        r=run(s,args.force)
        if args.fail_fast and r[1]:stop.set()
        return r
    with cf.ThreadPoolExecutor(args.j)as ex:
        for s,rc,summary,cached in ex.map(task,suites):
            results[s]=(rc,summary,cached)
            print('%-16s %s %s'%(s,'cached' if cached else 'rc=%s'%rc,summary),flush=True)
    bad=[s for s,(rc,_,_) in results.items() if rc!=0]
    ran=sum(1 for rc,_,c in results.values() if rc is not None and not c)
    print('closure: %d suites, %d run, %d cached, %d red %s, %.0fs'%(len(results),ran,
          sum(1 for _,_,c in results.values() if c),len(bad),' '.join(bad),time.time()-t0))
    (CACHE/'summary.txt').write_text(''.join('%s rc=%s %s\n'%(s,'cached' if c else rc,m) for s,(rc,m,c) in results.items()))
    return 1 if bad else 0

if __name__=='__main__':sys.exit(main())
