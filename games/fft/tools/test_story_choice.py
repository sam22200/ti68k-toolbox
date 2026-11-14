import copy
import struct
import unittest
import story_choice as choices


class StoryChoices(unittest.TestCase):
    def setUp(self):
        self.spec = dict(done=40, options=[dict(flag=41, label='No'),
            dict(flag=42, label='Yes', effects=[dict(op='recruit_guest', id=4)])])

    def test_record_and_pool_span(self):
        raw = choices.build(self.spec)
        self.assertEqual(choices.validate(raw), len(raw))
        self.assertEqual(choices.validate(raw+bytes(12)), len(raw))
        # Labels/effect lists may share exact physical bytes.
        self.spec['options'][1] = dict(self.spec['options'][0], flag=42)
        raw = choices.build(self.spec)
        self.assertEqual(choices.word(raw, 14), choices.word(raw, 22))

    def test_authoring_rejections(self):
        for edit in ('marker', 'guard', 'guest_duplicate', 'flag_duplicate', 'marker_effect', 'effects', 'label', 'nul', 'boolean'):
            s = copy.deepcopy(self.spec)
            if edit == 'marker': s['options'][0]['flag'] = 40
            elif edit == 'guard': s['guard'] = dict(flag=41, value=True)
            elif edit == 'guest_duplicate': s['options'][1]['effects'] += [dict(op='remove_guest', id=4)]
            elif edit == 'flag_duplicate': s['options'][1]['effects'] = [dict(op='set_flag', id=50), dict(op='clear_flag', id=50)]
            elif edit == 'marker_effect': s['options'][0]['effects'] = [dict(op='set_flag', id=42)]
            elif edit == 'effects': s['options'][0]['effects'] = [dict(op='set_flag', id=50+i) for i in range(9)]
            elif edit == 'label': s['options'][0]['label'] = 'é'
            elif edit == 'nul': s['options'][0]['label'] = 'No\0Yes'
            else: s['done'] = True
            with self.subTest(edit=edit), self.assertRaises(ValueError): choices.build(s)

    def test_bad_offsets_and_opcodes(self):
        raw = choices.build(self.spec)
        for length in (0, 27, len(raw)-1):
            with self.assertRaises(ValueError): choices.validate(raw[:length])
        for at, value in ((4, 65535), (14, 27), (16, 28), (24, 65534)):
            bad = bytearray(raw); struct.pack_into('>H', bad, at, value)
            with self.subTest(at=at), self.assertRaises(ValueError): choices.validate(bad)
        effect = choices.word(raw, 24)
        for pos, value in ((effect, 5), (effect+1, 1), (26, 9)):
            bad = bytearray(raw); bad[pos] = value
            with self.assertRaises(ValueError): choices.validate(bad)


if __name__ == '__main__': unittest.main()
