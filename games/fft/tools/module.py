#!/usr/bin/env python3
"""The battle engine's modules bank (module.h, campaign/ENGINE.md): a chapter's source built
with FFT_MODULE as a position-independent image.

usage: module.py OUT.bin SOURCE.c COUNT --second OTHER.bin
Module k is SOURCE built with FFT_MODULE = k+1. Active campaign packaging emits
an FMB3 pair: magic, first ID/count/total (u16), shared export token (u32), local
u16 offsets; each FMD2 stores entry/logical size/context offset then the image,
with only the signature retained from the zero-filled context (restored on load).
The token identifies a matching export, not a runtime integrity checksum.
A contiguous balanced partition is deterministic; stable module IDs do not move.
Both shards are read in place; the core retains one execution zone (MOD_ZONE).

GCC4TI -mpcrel emits separate sections. The established one_section adapter below
collects them into one .text; relocations/undefined symbols are rejected. No new
assembly kernel is authored here. Omitting --second emits FMB2 only for historical
controls; active runtime rejects it. modules.h records MOD_ZONE/MOD_BANK0_COUNT.
"""
from pathlib import Path
import argparse
import re
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[3]
T = ROOT / 'tools'
ENV = {'TIGCC': str(T / 'gcc4ti-bin'), 'PATH': '%s:/usr/bin:/bin' % (T / 'gcc4ti-bin/bin')}
CFLAGS = ['-Os', '-Wall', '-W', '-mpcrel', '-mregparm=5', '-fomit-frame-pointer', '-fwhole-program',
          '-I%s' % (T / 'extgraph/lib')]


def run(*args, cwd=None):
    p = subprocess.run([str(a) for a in args], env=ENV, cwd=cwd, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        sys.exit(p.stdout)
    return p.stdout


def one_section(asm):
    out = []
    for line in asm.splitlines():
        s = line.strip()
        if re.search(r'__ref_all_|_tigcc_native', s):    # tigcclib.h's program-format marks
            continue
        m = re.match(r'\.(l?comm)\s+([\w.]+),\s*(\d+)', s)
        if m:
            if m[1] == 'comm':
                out.append('\t.globl %s' % m[2])
            out += ['\t.text', '\t.even', '%s:' % m[2], '\t.skip %s' % m[3]]
        elif s.startswith('.section') or s in ('.data', '.bss'):
            out.append('\t.text')
        else:
            out.append(line)
    return '\n'.join(out) + '\n'


def module(source, k, d):
    src = Path(source).resolve()
    asm, obj, img = d / 'm.s', d / 'm.o', d / 'm.img'
    run('tigcc', '-S', *CFLAGS, '-DFFT_MODULE=%d' % (k + 1), '-o', asm, src, cwd=src.parent)
    mask=arithmetic_mask(asm.read_text())
    # Compiler libcalls appear after whole-program elimination. Compile once
    # to discover them, then retain exactly their C forwarding wrappers.
    run('tigcc','-S',*CFLAGS,'-DFFT_MODULE=%d'%(k+1),
        '-DFFT_MODULE_ARITH_MASK=%d'%mask,'-o',asm,src,cwd=src.parent)
    asm.write_text(one_section(asm.read_text()))
    run('tigcc', '-c', '-o', obj, asm)
    heads = run('m68k-coff-tigcc-objdump', '-h', '-r', obj)
    sizes = {n: int(v, 16) for n, v in re.findall(r'^\s*\d+\s+(\S+)\s+([0-9a-f]+)', heads, re.M)}
    if any(v for n, v in sizes.items() if n != '.text') or 'RELOCATION RECORDS' in heads:
        sys.exit('%s: more than one section or relocations\n%s' % (source, heads))
    syms = run('m68k-coff-tigcc-objdump', '-t', obj)
    undefined = re.findall(r'\(sec\s+0\).* (\S+)$', syms, re.M)
    if undefined:
        sys.exit('%s: undefined symbols %s' % (source, ' '.join(undefined)))
    addr = {n: int(v, 16) for v, n in re.findall(r'\(sec\s+1\).* 0x([0-9a-f]+) (\S+)$', syms, re.M)}
    run('m68k-coff-tigcc-objcopy', '-O', 'binary', obj, img)
    image = img.read_bytes()
    if len(image) & 1:
        image += b'\0'
    context_bytes=struct.unpack_from('>H',image,addr['mod_context_size'])[0]
    return compact_record(image,addr['mod_entry'],addr['M'],context_bytes)


def arithmetic_mask(assembly):
    helpers=('__divsi3','__udivsi3','__mulsi3')
    text=assembly
    for name in helpers:
        text=re.sub(r'^'+name+r':.*?(?=^\w+:|\Z)','',text,flags=re.M|re.S)
        text=re.sub(r'^\s*\.globl\s+'+name+r'\s*$','',text,flags=re.M)
    return sum(1<<i for i,name in enumerate(helpers)if re.search(r'\b'+name+r'\b',text))


def compact_record(image,entry,context,context_bytes):
    """Omit only the context initialized by the resident, preserving code offsets."""
    assert context_bytes>=2 and context+context_bytes<=len(image)
    assert image[context+2:context+context_bytes]==bytes(context_bytes-2)
    stored=image[:context+2]+image[context+context_bytes:]
    return b'FMD2'+struct.pack('>3H',entry,len(image),context)+stored


def pair_banks(mods):
    """Two contiguous ID ranges, balanced by archived bytes, deterministic tie break."""
    if not 2 <= len(mods) <= 255:
        raise ValueError('split modules require2..255 images')
    def size(part):return 14+sum(len(m)+2 for m in part)
    split=min(range(1,len(mods)),key=lambda i:max(size(mods[:i]),size(mods[i:])))
    token=zlib.crc32(b''.join(mods))
    banks=[]
    for first,part in ((0,mods[:split]),(split,mods[split:])):
        if size(part)>64000:raise ValueError("module shard exceeds64000-byte production budget")
        bank=bytearray(b'FMB3')+struct.pack('>3HI',first,len(part),len(mods),token)+bytes(2*len(part))
        for i,m in enumerate(part):
            struct.pack_into('>H',bank,14+2*i,len(bank));bank+=m
        if len(bank)>64000:
            raise ValueError('module shard exceeds64000-byte production budget')
        banks.append(bytes(bank))
    return split,banks


def triple_banks(mods):
    def size(part):return 14+sum(len(m)+2 for m in part)
    n=len(mods)
    x,y=min(((x,y)for x in range(1,n-1)for y in range(x+1,n)),key=lambda xy:max(size(mods[:xy[0]]),size(mods[xy[0]:xy[1]]),size(mods[xy[1]:])))
    token=zlib.crc32(b''.join(mods));banks=[]
    for first,end in ((0,x),(x,y),(y,n)):
        part=mods[first:end]
        if size(part)>64000:raise ValueError("module shard exceeds64000 bytes")
        bank=bytearray(b'FMB3')+struct.pack('>3HI',first,len(part),n,token)+bytes(2*len(part))
        for i,m in enumerate(part):struct.pack_into('>H',bank,14+2*i,len(bank));bank+=m
        banks.append(bytes(bank))
    return (x,y),banks

def write_changed(path,data):
    if not path.exists() or path.read_bytes()!=data:path.write_bytes(data)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('out',type=Path);ap.add_argument('source');ap.add_argument('count',type=int)
    ap.add_argument('--second',type=Path,help='active FMB3 pair; omit only for historical FMB2 controls')
    ap.add_argument('--third',type=Path,help='third matching FMB3 archive shard')
    ap.add_argument('--companion',type=int,help='image id kept out of the zone (its own heap block)')
    args=ap.parse_args();out,source,count=args.out,args.source,args.count
    with tempfile.TemporaryDirectory() as d:
        mods = [module(source, k, Path(d)) for k in range(count)]
    if args.third:
        if not args.second:ap.error('--third requires --second')
        splits,banks=triple_banks(mods)
        for path,bank in zip((out,args.second,args.third),banks):write_changed(path,bank)
        extra='#define MOD_BANK0_COUNT %d\n#define MOD_BANK1_END %d\n'%splits
    elif args.second:
        split,banks=pair_banks(mods)
        for path,bank in zip((out,args.second),banks):write_changed(path,bank)
        extra='#define MOD_BANK0_COUNT %d\n'%split
    else:
        # Historical control only. Active runtime rejects FMB2.
        bank=bytearray(b'FMB2')+struct.pack('>H',len(mods))+bytes(2*len(mods))
        for i,m in enumerate(mods):struct.pack_into('>H',bank,6+2*i,len(bank));bank+=m
        write_changed(out,bytes(bank));banks=[bank];extra=''
    zone = max(struct.unpack_from('>H',m,6)[0] for k,m in enumerate(mods) if k!=args.companion)  # the core's zone (fft.c): the largest image
    head = out.parent / 'modules.h'
    text = '// Generated by tools/module.py: execution zone and archived ID split.\n#define MOD_ZONE %d\n'%zone+extra
    if args.companion is not None:
        size=struct.unpack_from('>H',mods[args.companion],6)[0]
        limit=int(re.search(r'#define FFT_CORE_COMPANION_MAX (\d+)',(Path(__file__).resolve().parents[1]/'core_limits.h').read_text())[1])
        if size>limit:sys.exit('companion image %d bytes exceeds FFT_CORE_COMPANION_MAX %d (core_limits.h)'%(size,limit))
        text += '#define MOD_COMPANION_BYTES %d\n'%size
    if not head.exists() or head.read_text() != text:
        head.write_text(text)
    print('%s: banks %s bytes (images %s)' % (out.name, '/'.join(str(len(b))for b in banks), ', '.join(str(struct.unpack_from('>H',m,6)[0]) for m in mods)))


if __name__ == '__main__':
    main()
