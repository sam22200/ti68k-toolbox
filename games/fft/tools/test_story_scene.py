import copy
from pathlib import Path
import struct
import sys
import unittest
import chapter_packages as packages
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'campaign'))
from check_story_scene import fixture


class StorySceneFormat(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source={}
        base=Path(__file__).resolve().parents[1]/'campaign/x/guests/release'
        for group in ('fft-core.89g','chapter-01.89g'):cls.source.update(packages.read_group(base/group))
        cls.banks=fixture(cls.source)

    def source_with(self,banks):
        source=dict(self.source)
        for name,raw in banks.items():source[name]=packages.Variable(name,0x1c,3,raw+packages.resources.TRAILER)
        return source

    def test_pooled_choice_and_scalar_branch(self):
        source=self.source_with(self.banks)
        _,parts=packages.resources.scene_parts(source,'ffs3',True)
        self.assertEqual(parts[0],(7,1,[3,7,1]))
        self.assertEqual(parts[2][0],6)
        self.assertEqual(parts[2][2][0][:4],b'FCX1')
        a,b=(self.banks[n] for n in ('ffs3','ffs5'))
        self.assertEqual(struct.unpack_from('>H',a,20+8*2+2),struct.unpack_from('>H',b,20+8*2+2))

    def test_bad_flow_rejected_before_packaging(self):
        source=self.source_with(self.banks)
        with self.assertRaises(ValueError):packages.resources.scene_parts(source,'ffs4',True)
        raw=bytearray(self.banks['ffs3']);raw[20+8]=3 # prior question must remain a STORY
        source=self.source_with(dict(self.banks,ffs3=bytes(raw)))
        with self.assertRaises(ValueError):packages.resources.scene_parts(source,'ffs3',True)


if __name__=='__main__':unittest.main()
