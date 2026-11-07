#!/usr/bin/env python3
"""Offline split-bank boundaries; no compiler, data regeneration or image writes."""
import struct,unittest,zlib
import module

def image(size):return b'FMD2'+struct.pack('>3H',0,size,2)+bytes(size-2)

class SplitBanks(unittest.TestCase):
    def test_partition_and_identity(self):
        mods=[image(n)for n in (100,220,40,80,90)]
        split,banks=module.pair_banks(mods)
        token=zlib.crc32(b''.join(mods));got=[]
        for first,bank in zip((0,split),banks):
            self.assertEqual(bank[:4],b'FMB3')
            at,count,total,key=struct.unpack_from('>3HI',bank,4)
            self.assertEqual((at,total,key),(first,len(mods),token))
            end=14+2*count
            for i in range(count):
                start=struct.unpack_from('>H',bank,14+2*i)[0];self.assertEqual(start,end)
                size=struct.unpack_from('>H',bank,start+6)[0];end=start+8+size;got.append(bank[start:end])
            self.assertEqual(end,len(bank))
        self.assertEqual(got,mods);self.assertEqual(module.pair_banks(mods),(split,banks))
        # Retain contiguous IDs, balancing actual headers and records.
        costs=[max(14+sum(len(m)+2 for m in mods[:i]),14+sum(len(m)+2 for m in mods[i:]))for i in range(1,len(mods))]
        self.assertEqual(max(map(len,banks)),min(costs))
    def test_three_shards(self):
        mods=[image(n)for n in (100,220,40,80,90,130,60)]
        splits,banks=module.triple_banks(mods);got=[]
        self.assertEqual(len(banks),3)
        for first,end,bank in zip((0,*splits),(*splits,len(mods)),banks):
            at,count,total,token=struct.unpack_from('>3HI',bank,4)
            self.assertEqual((at,count,total),(first,end-first,len(mods)))
            self.assertEqual(token,zlib.crc32(b''.join(mods)))
            offsets=list(struct.unpack_from('>'+count*'H',bank,14))+[len(bank)]
            got.extend(bank[a:b]for a,b in zip(offsets,offsets[1:]))
        self.assertEqual(got,mods)
        self.assertEqual(module.triple_banks(mods),(splits,banks))
        with self.assertRaises(ValueError):module.triple_banks([image(64000)]*3)

    def test_token_changes_when_code_changes(self):
        _,a=module.pair_banks([image(8),image(10)])
        _,b=module.pair_banks([image(8),image(12)])
        self.assertNotEqual(a[0][10:14],b[0][10:14]);self.assertEqual(b[0][10:14],b[1][10:14])
    def test_context_hole_reconstruction(self):
        original=b'prefix!!'+b'\x12\x34'+bytes(358)+b'nonzero-tail!!'
        record=module.compact_record(original,0,8,360)
        self.assertEqual(struct.unpack_from('>3H',record,4),(0,len(original),8))
        stored=record[10:]
        expanded=stored[:10]+bytes(358)+stored[10:]
        self.assertEqual(expanded,original)
        self.assertEqual(len(record),10+len(original)-358)
        bad=bytearray(original);bad[10]=1
        with self.assertRaises(AssertionError):module.compact_record(bad,0,8,360)
    def test_arithmetic_wrapper_discovery(self):
        asm="""__divsi3:
 move.l (%sp),%d0
 rts
__udivsi3:
 rts
__mulsi3:
 rts
worker:
 bsr __mulsi3
 rts
 .globl __divsi3
 .globl __udivsi3
 .globl __mulsi3
"""
        self.assertEqual(module.arithmetic_mask(asm),4)
        self.assertEqual(module.arithmetic_mask(asm.replace('bsr __mulsi3','bsr __udivsi3')),2)
        self.assertEqual(module.arithmetic_mask(asm.replace('bsr __mulsi3','nop')),0)
        self.assertEqual(module.arithmetic_mask(asm+'\nnext:\n bsr __divsi3\n'),5)

    def test_invalid_capacity(self):
        for mods in ([],[image(8)],[image(8)]*256,[image(63980)]*2):
            with self.assertRaises(ValueError):module.pair_banks(mods)

if __name__=='__main__':unittest.main()
