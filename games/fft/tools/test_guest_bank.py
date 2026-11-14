import struct
import unittest
import guest_bank


class Guests(unittest.TestCase):
    def test_shared_identity_moves_slots(self):
        raw = guest_bank.build([dict(route='g', slot=0, id=4), dict(route='h', slot=7, id=4)])
        self.assertEqual(len(raw), 16)
        self.assertEqual(set(guest_bank.validate(raw, {'g', 'h'})), {'g', 'h'})
        with self.assertRaises(ValueError): guest_bank.validate(raw, {'g'})

    def test_limits_and_duplicates(self):
        good = dict(route='g', slot=0, id=4)
        for entries in ([], [good, good], [good, dict(route='g', slot=1, id=4)],
                        [good, dict(route='g', slot=0, id=5)],
                        [dict(route='g', slot=i, id=i+1) for i in range(5)]):
            with self.subTest(entries=entries), self.assertRaises(ValueError): guest_bank.build(entries)
        for change in (dict(route='c'), dict(slot=13), dict(slot=True), dict(id=0), dict(id=65536)):
            with self.subTest(change=change), self.assertRaises(ValueError): guest_bank.build([dict(good, **change)])

    def test_corrupt(self):
        raw = guest_bank.build([dict(route='g', slot=0, id=4)])
        for size in (0, 7, 8, 11, 13):
            with self.assertRaises(ValueError): guest_bank.validate((raw+bytes(2))[:size])
        for at, value in ((0, 0), (5, 0), (7, 255), (8, ord('c')), (9, 13), (11, 0)):
            bad = bytearray(raw); bad[at] = value
            with self.subTest(at=at), self.assertRaises(ValueError): guest_bank.validate(bad)


if __name__ == '__main__': unittest.main()
