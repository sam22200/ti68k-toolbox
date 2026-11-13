"""FKO1 pool aliases and shipping rejection; all artwork below is synthetic."""
import struct
import unittest
import json
import sys

import monster_ko_bank as bank


class KoBank(unittest.TestCase):
    def pose(self):
        return bytes((1, 16, 1, 0))+struct.pack('>4I', 0x10000000, 0, 0xefffffff, 0x10000000)

    def test_alias_and_mirror_share_physical_pose(self):
        pose = self.pose()
        raw, info = bank.pack({94: [pose, bank.mirror(pose)], 133: [pose, bank.mirror(pose)]})
        self.assertTrue(bank.validate(raw))
        self.assertEqual((info['physical_frames'], info['pairs']), (1, 1))
        self.assertEqual(struct.unpack_from('>4H', raw, bank.HEADER), (0, 32768, 32768, 0))
        self.assertEqual(bank.mirror(bank.mirror(pose)), pose)

    def test_reject_invalid_offsets_and_transparent_body(self):
        raw, _ = bank.pack({94: [self.pose()]*2})
        for at, replacement in ((0, b'BAD!'), (4, b'\xff\xff'), (8, b'\x00\x01'),
                                (bank.HEADER, b'\x7f\xff'), (bank.HEADER+8, b'\x00\x00')):
            data = bytearray(raw)
            data[at:at+len(replacement)] = replacement
            with self.assertRaises(ValueError):
                bank.validate(data)
        for data in (raw[:-1], raw+b'\0\0'):
            with self.assertRaises(ValueError):
                bank.validate(data)
        data = bytearray(raw)
        data[-8:-4] = b'\xff'*4
        with self.assertRaises(ValueError):
            bank.validate(data)

    def test_optional_bank_requires_capable_core_and_bounded_keys(self):
        sys.path.insert(0, str(bank.review.HERE/'campaign'))
        from check_chapter_packages import synthetic
        import chapter_packages as packages
        here = bank.review.HERE/'campaign/packages'
        core = json.loads((here/'core.json').read_text())
        chapter = json.loads((here/'chapter-01.json').read_text())
        source = synthetic(core, chapter)
        for name in ('ffca', 'ffcb'):
            v = source[name]
            source[name] = packages.Variable(name, v.kind, v.attr, b'\0\0'+v.raw[2:]+b'\0dat\0\xf8')
        raw, _ = bank.pack({94: [self.pose()]*2})
        source['ffko'] = packages.Variable('ffko', 0x1c, 3, raw+b'\0dat\0\xf8')
        chapter['variables'].append('ffko')
        result = packages.chapter_package(source, chapter, core, bytes(16))
        self.assertEqual(sum(v.name == 'ffko' for v in result), 1)
        with self.assertRaisesRegex(ValueError, 'capable'):
            packages.chapter_package(source, chapter, {**core, 'ko_actor_format': ''}, bytes(16))
        source['ffca'] = packages.Variable('ffca', 0x1c, 3, b'\x7f\x00\0dat\0\xf8')
        with self.assertRaisesRegex(ValueError, 'reserved'):
            packages.chapter_package(source, chapter, core, bytes(16))


if __name__ == '__main__':
    unittest.main()
