import unittest
import rumor_rules as rules


class Bindings(unittest.TestCase):
    def test_pilot_does_not_enable_unbound_future_scenes(self):
        self.assertEqual(rules.gates([dict(event=12,flag=2)]),[(0,1,255),(1,1,255),(2,1,255)])

    def test_complete_source_replacements(self):
        rows=[dict(event=e,flag=f) for e,f in zip(rules.EVENTS,(2,33,34,35))]
        self.assertEqual(rules.gates(rows,complete=True),
                         [(0,1,33),(1,1,32),(2,1,34),(3,32,34),(4,33,34),(5,33,34)])

    def test_missing_ambiguous_and_unrepresentable_bindings_reject(self):
        for rows in ([dict(event=12,flag=256)],[dict(event=13,flag=2)],
                     [dict(event=12,flag=2),dict(event=45,flag=2)],
                     [dict(event=12,flag=2),dict(event=12,flag=3)],
                     [dict(event=12,flag=True)],[dict(event=12,flag=2,guess=True)]):
            with self.subTest(rows=rows),self.assertRaises(ValueError):rules.gates(rows)
        with self.assertRaises(ValueError):rules.gates([dict(event=12,flag=2)],complete=True)


if __name__=='__main__':unittest.main()
