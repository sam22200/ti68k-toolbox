"""Metadata-only migration preserves all descriptor-owned payloads and igfx offsets."""
import struct
import unittest
import chapter_desc as cd


class SourceMap(unittest.TestCase):
    def fixture(self):
        raw=bytearray(b'FCD4'+bytes(100)+bytes(range(64))+b'SGE1'+struct.pack('>HH',152,16))
        for name in ['UNIT_DEFS','UNIT_REWARDS','WATER','MAP_SCRIPT']:
            struct.pack_into('>H',raw,4+2*cd.FIELDS.index(name),104)
        struct.pack_into('>H',raw,4+2*cd.FIELDS.index('UNIT_RAW'),222)
        return bytes(raw)

    def test_payload_and_offsets(self):
        raw=self.fixture();result=cd.upgrade_descriptor(raw,74)
        self.assertEqual(result[:4],b'FCD5');self.assertEqual(result[106:-8],raw[104:-8])
        self.assertEqual(struct.unpack_from('>H',result,104)[0],74)
        for name in ['UNIT_DEFS','UNIT_REWARDS','WATER','MAP_SCRIPT']:
            self.assertEqual(struct.unpack_from('>H',result,4+2*cd.FIELDS.index(name))[0],106)
        self.assertEqual(struct.unpack_from('>H',result,4+2*cd.FIELDS.index('UNIT_RAW'))[0],222)
        self.assertEqual(struct.unpack_from('>HH',result,len(result)-4),(154,16))

    def test_map_is_per_encounter(self):
        first=cd.upgrade_descriptor(self.fixture(),22)
        second=cd.upgrade_descriptor(first,74)
        self.assertEqual(first[:104]+first[106:],second[:104]+second[106:])
        self.assertEqual(struct.unpack_from('>H',second,104)[0],74)
        self.assertEqual(cd.upgrade_descriptor(second,74),second)

    def test_bad_metadata(self):
        for map_id in [-1,128,256,True]:
            with self.assertRaises(ValueError):cd.upgrade_descriptor(self.fixture(),map_id)
        for data in [b'FCD4',self.fixture()[:-1],b'FCD3'+self.fixture()[4:]]:
            with self.assertRaises(ValueError):cd.upgrade_descriptor(data,22)
        self.assertEqual(struct.unpack_from('>H',cd.upgrade_descriptor(self.fixture(),255),104)[0],255)


if __name__=='__main__':unittest.main()
