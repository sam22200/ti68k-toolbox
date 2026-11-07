#!/usr/bin/env python3
"""Boundary/truncation tests for the census, independent of commercial data."""
import struct
import tempfile
import unittest
from pathlib import Path

import core_audit as audit
import core_fixtures
import copy


def mesh(width,height):
    raw=bytearray(0x70+4098)
    struct.pack_into('<I',raw,0x68,0x70);raw[0x70:0x72]=bytes((width,height))
    for i in range(512):raw[0x72+8*i+6]=1
    return raw


def group(payload,name=b'test'):
    start=82
    raw=bytearray(b'**TI89**'+b'\x01\x00'+b'main\0\0\0\0'+bytes(40)+struct.pack('<H',1))
    raw+=struct.pack('<I8sBBH',start,name,0x1C,3,0)
    block=struct.pack('>H',len(payload))+payload
    raw+=struct.pack('<I',start+len(block)+6)+b'\xA5\x5A'
    return raw+bytes(4)+block+struct.pack('<H',sum(block)&65535)


class Census(unittest.TestCase):
    def test_upper_stride_is256_not_map_area(self):
        raw=mesh(4,18)
        at=0x72+256*8;raw[at+2]=31;raw[at+6]=0
        result=audit.terrain(raw,'fixture')
        self.assertEqual(result['cells_per_layer'],72)
        self.assertEqual(result['layers'][0]['unblocked'],0)
        self.assertEqual(result['layers'][1]['max_base_height'],31)

    def test_256th_tile_not_padding(self):
        raw=mesh(16,16);at=0x72+255*8
        raw[at+2]=17;raw[at+6]=0
        self.assertEqual(audit.terrain(raw,'fixture')['layers'][0]['max_base_height'],17)

    def test_padding_does_not_inflate_height(self):
        raw=mesh(4,18);at=0x72+72*8
        raw[at+2]=255;raw[at+6]=0
        self.assertEqual(audit.terrain(raw,'fixture')['layers'][0]['max_base_height'],0)

    def test_traversable_and_stoppable_differ(self):
        raw=mesh(1,1);raw[0x72+6]=2
        result=audit.terrain(raw,'fixture')['layers'][0]
        self.assertEqual((result['unblocked'],result['not_blocked_or_untargetable']),(1,0))

    def test_depth_and_slope_are_distinct(self):
        raw=mesh(1,1);raw[0x72+2:0x72+7]=bytes((4,3*32+2,0x85,0,0))
        result=audit.terrain(raw,'fixture')['layers'][0]
        self.assertEqual((result['max_corner_height'],result['max_depth']),(6,3))

    def test_mesh_truncation_and_bad_offsets(self):
        for raw in (b'',mesh(1,1)[:-1]):
            with self.assertRaises(ValueError):audit.terrain(raw,'fixture')
        raw=mesh(1,1);struct.pack_into('<I',raw,0x68,3)
        with self.assertRaises(ValueError):audit.terrain(raw,'fixture')

    def test_dimensions_reject_overflow_and_empty(self):
        for width,height in ((0,10),(19,19),(255,2)):
            with self.assertRaises(ValueError):audit.terrain(mesh(width,height),'fixture')

    def test_terrainless_resource_is_not_a_zero_size_map(self):
        raw=mesh(1,1);struct.pack_into('<I',raw,0x68,0)
        self.assertIsNone(audit.terrain(raw,'fixture'))

    def test_group_transport_is_distinct_from_payload(self):
        with tempfile.TemporaryDirectory()as name:
            path=Path(name)/'test.89g';path.write_bytes(group(b'own fixture'))
            result=audit.package(path)
            self.assertEqual(result['payload_bytes'],11)
            self.assertEqual(result['bytes'],11+90)
            self.assertEqual(result['variables'],1)

    def test_both_module_shards_are_code(self):
        with tempfile.TemporaryDirectory()as name:
            path=Path(name)/'test.89g'
            for label in (b'ffbm',b'ffbn'):
                path.write_bytes(group(b'own module fixture',label))
                self.assertEqual(audit.package(path)['entries'][0]['category'],'code_and_embedded_constants')

    def test_group_rejects_bad_checksum_span_and_trailing_data(self):
        raw=group(b'own fixture')
        bad=bytearray(raw);bad[-1]^=1
        invalid_offset=bytearray(raw);struct.pack_into('<I',invalid_offset,0x3C,10)
        with tempfile.TemporaryDirectory()as name:
            path=Path(name)/'test.89g'
            for data in (bad,invalid_offset,raw[:-1],raw+b'x'):
                path.write_bytes(data)
                with self.assertRaises(ValueError):audit.package(path)

    def test_packed_code_reports_input_limit_separately(self):
        with tempfile.TemporaryDirectory()as name:
            path=Path(name)/'test.89g'
            for unpacked,headroom in ((100,64836),(64831,105),(65000,-64)):
                path.write_bytes(group(struct.pack('<H',unpacked)+b'TP'+bytes(12),b'ffbcore'))
                entry=audit.package(path)['entries'][0]
                self.assertEqual(entry['payload_bytes'],16)
                self.assertEqual(entry['ttpack_input_bytes'],unpacked)
                self.assertEqual(entry['ttpack_headroom_bytes'],headroom)
            path.write_bytes(group(b'\x3f\xfdTP'+bytes(12),b'fftitle'))
            self.assertNotIn('ttpack_input_bytes',audit.package(path)['entries'][0])

    def test_scenario_uses_source_u16_ids(self):
        raw=bytearray(audit.SCENARIO_START+48)
        at=audit.SCENARIO_START
        struct.pack_into('<H',raw,at,445);raw[at+2]=50
        struct.pack_into('<3H',raw,at+7,0x185,257,258)
        result=audit.scenarios(raw)
        self.assertEqual((result[0]['entd'],result[0]['squads']),(0x185,[257,258]))

    def test_scenario_requires_terminator(self):
        raw=bytearray(audit.SCENARIO_START+24)
        struct.pack_into('<H',raw,audit.SCENARIO_START,9)
        with self.assertRaises(ValueError):audit.scenarios(raw)

    def test_synthetic_inputs_preserve_layers_and_high_ids(self):
        cases=core_fixtures.build()
        self.assertEqual(len(cases['actors13']['units']),13)
        self.assertEqual(cases['actors13']['units'][-1]['id'],12)
        self.assertEqual(cases['compact256']['pending'][0]['target'],255)
        for value in cases.values():self.assertTrue(core_fixtures.validate(value))
        a,b=cases['two_floors_4x18']['units']
        self.assertEqual(a['position'][:2],b['position'][:2])
        self.assertNotEqual(a['position'][2],b['position'][2])

    def test_synthetic_invalid_references_fail(self):
        original=core_fixtures.snapshot(13)
        for mutation in ('duplicate','missing_target','overflow','same_floor','bad_tile'):
            value=copy.deepcopy(original)
            if mutation=='duplicate':value['units'][1]['id']=0
            elif mutation=='missing_target':value['pending'][0].update(target_kind='unit',target=200)
            elif mutation=='overflow':value['width']=17;value['height']=17
            elif mutation=='same_floor':value['units'][1]['position']=value['units'][0]['position'][:]
            else:value['pending'][0]['target']=512
            with self.assertRaises(ValueError):core_fixtures.validate(value)

    def test_runtime_indices_are_compact_across_layers(self):
        value=core_fixtures.snapshot(2,4,18,2,((0,0),))
        value['pending'][0]['target']=72 # upper(0,0), compacted after72 lower cells
        self.assertTrue(core_fixtures.validate(value))
        value['pending'][0]['target']=256 # source address is not a runtime node
        with self.assertRaises(ValueError):core_fixtures.validate(value)

    def test_reject_fourteenth_actor_and_257th_tile(self):
        with self.assertRaises(ValueError):core_fixtures.validate(core_fixtures.snapshot(14))
        with self.assertRaises(ValueError):core_fixtures.validate(core_fixtures.snapshot(13,16,16,2,((0,0),)))


if __name__=='__main__':unittest.main()
