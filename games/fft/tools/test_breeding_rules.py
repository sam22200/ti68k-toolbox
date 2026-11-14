"""Cold FWB1 rejection boundaries on local-disc data; no shipping writes."""
from pathlib import Path
import unittest
import breeding_rules


class BreedingRulesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[3]
        cls.bank, _ = breeding_rules.build(root/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue')

    def reject(self, offset, value):
        raw = bytearray(self.bank); raw[offset] = value
        with self.assertRaises(ValueError): breeding_rules.validate(raw)

    def test_exact_length_and_header(self):
        self.assertEqual(breeding_rules.validate(self.bank), self.bank)
        for raw in (self.bank[:-1], self.bank+b'\0', b'BAD!'+self.bank[4:]):
            with self.assertRaises(ValueError): breeding_rules.validate(raw)

    def test_calendar(self):
        self.reject(9, 29)
        self.reject(20, 0)
        self.reject(21, 32)

    def test_breed_bounds(self):
        for days in (0, 16): self.reject(59, days)
        for job in (0, 93, 142, 255): self.reject(60, job)

    def test_order_and_zodiac_identity(self):
        self.reject(23, 1)
        self.reject(22, 12)
        self.reject(58, 0)

    def test_generator_calendar_agreement(self):
        # A still-sorted one-day main boundary shift must fail, not merely an
        # out-of-range/duplicate birthday limit.
        self.reject(262, self.bank[262]+1)


if __name__ == '__main__': unittest.main()
