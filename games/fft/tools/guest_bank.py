"""FGP1: chapter-wide stable guest identities, independent of actor positions."""
import re
import struct


def build(entries):
    rows = bytearray()
    for e in entries:
        route, slot, identity = e['route'], e['slot'], e['id']
        if not isinstance(route, str) or not re.fullmatch('[abd-z0-9]', route):
            raise ValueError('Invalid guest route')
        if type(slot) is not int or not 0 <= slot < 13 or type(identity) is not int or not 1 <= identity <= 65535:
            raise ValueError('Invalid guest slot/identity')
        rows += struct.pack('>BBH', ord(route), slot, identity)
    raw = b'FGP1'+struct.pack('>HH', 8+len(rows), len(rows)//4)+rows
    validate(raw)
    return raw


def validate(raw, routes=None):
    if len(raw) < 8 or raw[:4] != b'FGP1':
        raise ValueError('Invalid guest header')
    size, count = struct.unpack_from('>HH', raw, 4)
    if size != len(raw) or not count or size > 65512 or size != 8+4*count:
        raise ValueError('Invalid guest directory span')
    seen = {}
    for route, slot, identity in struct.iter_unpack('>BBH', raw[8:]):
        route = chr(route)
        if not re.fullmatch('[abd-z0-9]', route) or slot >= 13 or not identity or routes is not None and route not in routes:
            raise ValueError('Invalid guest binding')
        slots, identities = seen.setdefault(route, (set(), set()))
        if slot in slots or identity in identities or len(slots) >= 4:
            raise ValueError('Duplicate or excessive guests in encounter')
        slots.add(slot); identities.add(identity)
    return seen
