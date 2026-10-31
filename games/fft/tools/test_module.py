#!/usr/bin/env python3
"""Offline split-bank boundaries; no compiler, data regeneration or image writes."""
import struct,unittest,zlib
import module

def image(size):return b'FMD1'+struct.pack('>3H',0,size,2)+bytes(size)

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
                size=struct.unpack_from('>H',bank,start+6)[0];end=start+10+size;got.append(bank[start:end])
            self.assertEqual(end,len(bank))
        self.assertEqual(got,mods);self.assertEqual(module.pair_banks(mods),(split,banks))
        # Retain contiguous IDs, balancing actual headers and records.
        costs=[max(14+sum(len(m)+2 for m in mods[:i]),14+sum(len(m)+2 for m in mods[i:]))for i in range(1,len(mods))]
        self.assertEqual(max(map(len,banks)),min(costs))
    def test_token_changes_when_code_changes(self):
        _,a=module.pair_banks([image(8),image(10)])
        _,b=module.pair_banks([image(8),image(12)])
        self.assertNotEqual(a[0][10:14],b[0][10:14]);self.assertEqual(b[0][10:14],b[1][10:14])
    def test_invalid_capacity(self):
        for mods in ([],[image(8)],[image(8)]*256,[image(63980)]*2):
            with self.assertRaises(ValueError):module.pair_banks(mods)

if __name__=='__main__':unittest.main()
