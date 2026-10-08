#!/usr/bin/env python3
"""Frontend checks without BIOS, game emulation or a window."""
import ctypes as C
from pathlib import Path
import tempfile
import unittest

from neogeorun import NeoGeo, PAD, Variable, events, script, swap_words


class RunnerTests(unittest.TestCase):
    def test_cpu_order_and_signed_access(self):
        neo = NeoGeo.__new__(NeoGeo)
        neo.ram = (C.c_uint8 * 65536)()
        # CPU bytes 12 34 56 78 are stored as 34 12 78 56 by the reference core.
        neo.ram[:4] = (0x34, 0x12, 0x78, 0x56)
        self.assertEqual(neo.read(0x100000, 4), 0x12345678)
        self.assertEqual(neo.read(0x100001, 2), 0x3456)
        neo.write(0x10fffc, -123456, 4)
        self.assertEqual(neo.read(0x10fffc, 4, True), -123456)
        neo.write(0x100001, 0xabcd, 2)
        self.assertEqual(neo.dump()[:4], bytes.fromhex('12abcd78'))
        neo.write(0x10ffff, -1)
        self.assertEqual(neo.ram[65534], 255)
        self.assertEqual(neo.read(0x10ffff, 1, True), -1)

    def test_ram_bounds_reject_mirrors_and_crossing_access(self):
        for address, size in ((0xfffff, 1), (0x110000, 1), (0x10ffff, 2),
                              (0x10fffe, 4), (0x100000, 3)):
            with self.assertRaises(ValueError):
                NeoGeo.address(address, size)
        self.assertEqual(NeoGeo.address(0x10fffc, 4), 65532)

    def test_word_swap_is_reversible(self):
        data = bytes(range(256))
        self.assertEqual(swap_words(data)[:4], b'\x01\x00\x03\x02')
        self.assertEqual(swap_words(swap_words(data)), data)
        with self.assertRaises(ValueError):
            swap_words(b'123')

    def test_two_controllers_and_explicit_release(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'keys.txt'
            path.write_text('0 RIGHT A P2:LEFT P2:B\n8 P2:START\n10 # release\n')
            self.assertEqual(script(path), {
                0: [(1 << PAD['RIGHT']) | (1 << PAD['A']),
                    (1 << PAD['LEFT']) | (1 << PAD['B'])],
                8: [0, 1 << PAD['START']], 10: [0, 0]})
            for raw in ('-1 A', '0 A\n0 B', '0 P3:A', '0 X'):
                path.write_text(raw)
                with self.assertRaises(ValueError):
                    script(path)

    def test_input_callback_keeps_players_independent(self):
        neo = NeoGeo.__new__(NeoGeo)
        neo.pressed = [1 << PAD['A'], (1 << PAD['LEFT']) | (1 << PAD['B'])]
        self.assertEqual(neo.input_state(0, 1, 0, PAD['A']), 1)
        self.assertEqual(neo.input_state(1, 1, 0, PAD['A']), 0)
        self.assertEqual(neo.input_state(1, 1, 0, 256), neo.pressed[1])
        self.assertEqual(neo.input_state(0, 5, 0, PAD['A']), 0)
        self.assertEqual(neo.input_state(2, 1, 0, PAD['A']), 0)

    def test_core_options_retain_overrides_on_dynamic_registration(self):
        neo = NeoGeo.__new__(NeoGeo)
        neo.options, neo.allowed_options = {}, {}
        neo.directories = {}
        neo.overrides = {b'test-clock': b'100%'}
        table = (Variable * 2)(Variable(b'test-clock', b'CPU; 200%|100%'), Variable())
        self.assertTrue(neo.environment(16, C.addressof(table)))
        query = Variable(b'test-clock', None)
        self.assertTrue(neo.environment(15, C.addressof(query)))
        self.assertEqual(query.value, b'100%')
        update = Variable(b'test-clock', b'200%')
        self.assertTrue(neo.environment(70, C.addressof(update)))
        self.assertEqual(neo.options[b'test-clock'], b'100%')
        # Re-registering after game/DIP discovery must keep reference options.
        self.assertTrue(neo.environment(16, C.addressof(table)))
        self.assertEqual(neo.options[b'test-clock'], b'100%')

    def test_output_events_preserve_path_colons(self):
        self.assertEqual(events('2:/tmp/a:b.png'), {2: Path('/tmp/a:b.png')})
        for value in ('-1:x', '0:x,0:y'):
            with self.assertRaises(ValueError):
                events(value)

    def test_missing_bios_is_a_dependency_error(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, 'missing local Neo Geo BIOS'):
                NeoGeo(bios=Path(directory) / 'absent.zip')


if __name__ == '__main__':
    unittest.main()
