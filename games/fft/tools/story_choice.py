"""FCX1 choices: two labeled answers with persistent markers and bounded effects.

The standalone scene player consumes these records through FSC choice/branch
operations. Exact record/label/effect spans remain poolable chapter resources.
"""
import struct

OPS = {'set_flag': 1, 'clear_flag': 2, 'recruit_guest': 3, 'remove_guest': 4}


def word(raw, at):
    return struct.unpack_from('>H', raw, at)[0]


def validate_prompt(assets, at, stride):
    if at % 2 or at+2 > len(assets): raise ValueError('Invalid question beat table')
    count=word(assets,at)
    if not 1<=count<=255 or at+2+2*count>len(assets): raise ValueError('Invalid question count')
    off=word(assets,at+2*count)
    if off%2 or off<2+2*count or at+off+4>len(assets): raise ValueError('Invalid question beat')
    at+=off
    actors=assets[at+1]
    if not 20<=stride<=40 or assets[at+3]>(stride-20)*8 or assets[at+2] or actors>13 or at+4+8*actors>len(assets):
        raise ValueError('Invalid question pan/hold/actors')
    if any(assets[at+4+8*i]>>4>=13 for i in range(actors)): raise ValueError('Question actor exceeds capacity')
    at+=4+8*actors
    for maximum,minimum in ((31,0),(127,1)):
        end=assets.index(0,at)
        if not minimum<=end-at<=maximum or any(c<32 or c>126 for c in assets[at:end]):
            raise ValueError('Invalid question speaker/text')
        at=end+1


def validate(raw):
    if len(raw) < 28 or raw[:4] != b'FCX1':
        raise ValueError('Invalid choice header')
    size, done, guard = struct.unpack_from('>HHH', raw, 4)
    if size < 28 or size > len(raw) or not 1 <= done <= 256 or guard > 256 or raw[10] > 1 or not guard and raw[10] or raw[11] != 2:
        raise ValueError('Invalid choice length, guard or completion marker')
    results = [word(raw, 12+8*i) for i in range(2)]
    markers = {done, *results}
    if len(markers) != 3 or any(not 1 <= n <= 256 for n in markers) or guard and guard in markers:
        raise ValueError('Choice markers must be distinct and independent of guard')
    for i in range(2):
        at = 12+8*i
        label, effects = word(raw, at+2), word(raw, at+4)
        count = raw[at+6]
        if raw[at+7] or count > 8 or label < 28 or label % 2 or label >= size:
            raise ValueError('Invalid choice answer')
        end = raw.find(b'\0', label, size)
        if end < 0 or not 1 <= end-label <= 31 or any(c < 32 or c > 126 for c in raw[label:end]):
            raise ValueError('Invalid choice label')
        if not count:
            if effects: raise ValueError('Empty effects must have no offset')
            continue
        if effects < 28 or effects % 2 or effects+4*count > size:
            raise ValueError('Invalid effect span')
        seen = set()
        for op, reserved, value in struct.iter_unpack('>BBH', raw[effects:effects+4*count]):
            key = (op <= 2, value)
            if op not in OPS.values() or reserved or not value or op <= 2 and (value > 256 or value in markers) or key in seen:
                raise ValueError('Invalid/conflicting choice effects')
            seen.add(key)
    return size


def build(spec):
    def number(n, limit):
        if type(n) is not int or not 1 <= n <= limit: raise ValueError('Invalid choice ID')
        return n
    options = spec['options']
    if len(options) != 2: raise ValueError('Exactly two answers are required')
    guard = spec.get('guard')
    if guard and type(guard['value']) is not bool: raise ValueError('Guard value must be boolean')
    raw = bytearray(b'FCX1'+bytes(2)+struct.pack('>HHBB', number(spec['done'], 256),
        number(guard['flag'], 256) if guard else 0, int(guard['value']) if guard else 0, 2)+bytes(16))
    blobs = {}
    def add(blob):
        if blob not in blobs:
            if len(raw) % 2: raw.append(0)
            blobs[blob] = len(raw); raw.extend(blob)
        return blobs[blob]
    for i, option in enumerate(options):
        effects = option.get('effects', [])
        if len(effects) > 8: raise ValueError('Too many choice effects')
        stream = b''.join(struct.pack('>BBH', OPS[e['op']], 0, number(e['id'], 256 if OPS[e['op']] <= 2 else 65535)) for e in effects)
        try: label = option['label'].encode('ascii')
        except (UnicodeEncodeError, AttributeError): raise ValueError('Choice labels must be ASCII') from None
        if not 1 <= len(label) <= 31 or any(c < 32 or c > 126 for c in label):
            raise ValueError('Choice labels must be printable,1..31 bytes')
        label_at = add(label+b'\0'); effects_at = add(stream) if stream else 0
        struct.pack_into('>HHHBB', raw, 12+8*i, number(option['flag'], 256), label_at, effects_at, len(effects), 0)
    struct.pack_into('>H', raw, 4, len(raw)); validate(raw)
    return bytes(raw)
