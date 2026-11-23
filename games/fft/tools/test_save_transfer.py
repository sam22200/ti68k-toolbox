"""Codec boundaries and safe destinations, without a calculator or live saves."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import chapter_packages as packages
import save_transfer as transfer
import save_dump


class Exchange(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def seed(self, version=b'FSV9'):
        # Opaque codec fixture, not a claim of gameplay-valid proposition/egg data.
        raw = bytearray((i * 37 + 13) & 255 for i in range(transfer.LENGTHS[version]))
        raw[:4] = version
        raw[36] = 16
        if version != b'FSV2':
            raw[3115] = 5
        if version == b'FSV9':
            raw[4024] = 0
        return bytes(raw)

    def variable(self, name, raw):
        return packages.Variable(name, 0x1c, 3, raw + transfer.TRAILER)

    def test_all_slots_and_session_preserve_every_byte(self):
        for name in transfer.NAMES:
            with self.subTest(name=name):
                raw = self.seed()
                pc = self.root / (name + '.sav')
                pc.write_bytes(raw)
                ti = self.root / (name + '.89y')
                transfer.to_ti(pc, ti)
                var = packages.read_group(ti)[name]
                self.assertEqual((var.kind, var.attr, var.folder), (0x1c, 3, b'main\0\0\0\0'))
                self.assertEqual(var.payload, raw + transfer.TRAILER)
                _, actual, out = transfer.to_pc(ti, self.root/'received')
                self.assertEqual(out.name, pc.name)
                self.assertEqual(actual, raw)
                self.assertEqual(out.read_bytes(), raw)
                self.assertEqual(save_dump.payload(ti), raw)

    def test_legacy_versions_are_preserved_without_python_migration(self):
        for version in transfer.LENGTHS:
            with self.subTest(version=version):
                pc = self.root/'old.sav'
                pc.write_bytes(self.seed(version))
                ti = self.root/'old.89y'
                transfer.to_ti(pc, ti, slot='3', replace=True)
                _, raw, out = transfer.to_pc(ti, self.root/'legacy', replace=True)
                self.assertEqual(raw, pc.read_bytes())
                self.assertEqual(out.name, 'ffsave3.sav')

    def test_ambiguous_group_requires_selection_and_can_remap_destination(self):
        group = self.root/'backup.89g'
        raw = self.seed()
        group.write_bytes(packages.group_bytes([self.variable(n, raw) for n in transfer.NAMES]))
        with self.assertRaisesRegex(ValueError, '--name'):
            transfer.to_pc(group, self.root/'received')
        transfer.to_pc(group, self.root/'received', name='ffsave3', slot='1')
        self.assertEqual((self.root/'received/ffsave1.sav').read_bytes(), raw)
        self.assertFalse((self.root/'received/ffsave.sav').exists())
        # The older diagnostic helper also serves title selection/config files.
        # Keep that API broad while campaign interchange accepts FSV records only.
        selection = self.root/'ffslot.89y'
        selection.write_bytes(packages.group_bytes([self.variable('ffslot', b'FSL1\x02\0')], single=True))
        self.assertEqual(save_dump.payload(selection), b'FSL1\x02\0')
        with self.assertRaises(ValueError):
            transfer.to_pc(selection, self.root/'received')

    def test_destination_requires_explicit_replace(self):
        source = self.root/'ffsave1.sav'
        raw = self.seed()
        source.write_bytes(raw)
        target = self.root/'transfer.89y'
        target.write_bytes(b'keep existing')
        with self.assertRaises(FileExistsError):
            transfer.to_ti(source, target)
        self.assertEqual(target.read_bytes(), b'keep existing')
        transfer.to_ti(source, target, replace=True)
        dest = self.root/'received'
        dest.mkdir()
        (dest/'ffsave1.sav').write_bytes(b'keep slot')
        with self.assertRaises(FileExistsError):
            transfer.to_pc(target, dest)
        self.assertEqual((dest/'ffsave1.sav').read_bytes(), b'keep slot')
        transfer.to_pc(target, dest, replace=True)
        self.assertEqual((dest/'ffsave1.sav').read_bytes(), raw)
        self.assertFalse(list(self.root.rglob('.fft-save-*')))

    def test_bad_envelopes_and_bounds_are_rejected(self):
        raw = self.seed()
        valid = packages.group_bytes([self.variable('ffsave1', raw)], single=True)
        cases = [valid[:-1], valid+b'x', valid[:100]+bytes([valid[100]^1])+valid[101:],
                 packages.group_bytes([packages.Variable('ffsave1', 0x1c, 3, raw+b'\0dat\0\xf8')], single=True),
                 packages.group_bytes([packages.Variable('ffsave1', 0x12, 3, raw+transfer.TRAILER)], single=True)]
        for n, data in enumerate(cases):
            with self.subTest(envelope=n):
                path = self.root/'bad.89y'
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    transfer.to_pc(path, self.root/'received')
        cases = [raw[:-1], raw+b'x', b'FSV0'+raw[4:], bytes(2332)]
        for offset, value in ((36,17), (3115,6), (4024,9)):
            bad = bytearray(raw); bad[offset] = value; cases.append(bytes(bad))
        bad = bytearray(raw); bad[4024] = 1; bad[4030] = 4; cases.append(bytes(bad))
        for n, data in enumerate(cases):
            with self.subTest(raw=n):
                with self.assertRaises(ValueError):
                    transfer.validate(data)
        self.assertFalse((self.root/'received').exists())

    def test_cli_roundtrip_and_clear_state_error(self):
        source = self.root/'checkpoint.sav'
        source.write_bytes(self.seed())
        ti = self.root/'slot.89y'
        tool = Path(transfer.__file__)
        for args in (['to-ti',source,ti,'--slot','2'], ['to-pc',ti,self.root/'received']):
            result = subprocess.run([sys.executable,str(tool),*map(str,args)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('FSV9, 4684 bytes', result.stdout)
        self.assertEqual((self.root/'received/ffsave2.sav').read_bytes(), source.read_bytes())
        source.write_bytes(bytes(2332))
        result = subprocess.run([sys.executable,str(tool),'to-ti',str(source),str(ti),'--slot','1'], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('platform-specific', result.stderr)
        self.assertNotIn('Traceback', result.stderr)


if __name__ == '__main__':
    unittest.main()
