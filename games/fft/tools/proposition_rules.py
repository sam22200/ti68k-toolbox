"""Source-backed Propositions catalogue for the portable service backend.

FPR1: magic,total BE u16,count,version; ten BE fee words;96 records of
25 bytes (23 original fields, BE title offset); pooled NUL ASCII titles.
Byte reads only. Original descriptions stay outside this rules-only bank.
"""
import hashlib
import struct

import extract
import item_help
import management_data as md
import management_research as mr

COUNT = 96
FIRST = 8 + 20 + COUNT * 25
REWARD_TABLES = (3, 4, 5, 6, 8, 9, 10, 11, 12, 13)
REWARD_SIZES = (176, 66, 31, 15, 16, 16, 16, 48, 20, 20)


def validate(raw):
    if len(raw) < FIRST or raw[:4] != b'FPR1' or struct.unpack_from('>H', raw, 4)[0] != len(raw):
        raise ValueError('Propositions header/length')
    if raw[6:8] != bytes((COUNT, 1)):
        raise ValueError('Propositions count/version')
    if max(struct.unpack_from('>10H', raw, 8)) > 10000:
        raise ValueError('Propositions fee lookup')
    for i in range(COUNT):
        at = 28 + 25 * i
        row = raw[at:at + 23]
        title = struct.unpack_from('>H', raw, at + 23)[0]
        if row[2] != i + 1 or max(row[4:6]) >= 10 or not 1 <= row[6] <= row[7] <= 32:
            raise ValueError('Propositions id/fee/duration')
        if row[16] > 43 or row[17] & ~7 or (row[17] & 2 and not 1 <= row[22] <= 12):
            raise ValueError('Propositions location/season')
        if row[17] & 4 and row[22] >= COUNT:
            raise ValueError('Propositions prerequisite')
        if not FIRST <= title < len(raw):
            raise ValueError('Propositions title offset')
        end = raw.find(b'\0', title)
        if end <= title or end - title > 80 or any(c < 32 or c > 126 for c in raw[title:end]):
            raise ValueError('Propositions title')


def build(cue):
    disc, files = extract.disc_files(cue)
    names = ('WORLD/WLDCORE.BIN', 'EVENT/WORLD.LZW')
    core, world = data = [disc.read(*files[name]) for name in names]
    base = struct.unpack_from('<I', core, 0x8009d340 - 0x80067000)[0] - 0x80067000
    tables = struct.unpack_from('<15I', core, base)
    fees = struct.unpack_from('<10H', core, base + tables[1])
    sections = struct.unpack_from('<32I', world)
    starts = item_help.packed_entries(world, 128 + sections[26], 128 + sections[27])
    if len(starts) != COUNT:
        raise ValueError('Original proposition title count')
    raw = bytearray(b'FPR1\0\0' + bytes((COUNT, 1)) + struct.pack('>10H', *fees))
    raw.extend(bytes(COUNT * 25))
    rows = []; titles = []; pool = {}
    for i in range(COUNT):
        row = core[base + tables[0] + 23 * i:base + tables[0] + 23 * (i + 1)]
        title = md.decode(bytes(mr.expanded(world, starts[i]))[:-1])
        text = title.encode('ascii') + b'\0'
        if text not in pool:
            pool[text] = len(raw); raw.extend(text)
        at = 28 + 25 * i
        raw[at:at + 23] = row
        struct.pack_into('>H', raw, at + 23, pool[text])
        rows.append(list(row)); titles.append(title)
    struct.pack_into('>H', raw, 4, len(raw)); validate(raw)
    info = dict(source_sha256={name: hashlib.sha256(b).hexdigest() for name, b in zip(names, data)},
                fees=list(fees), rows=rows, titles=titles,
                scope='All96 original rule rows/titles; private catalogue, no Chapter I unlock or shipping write.')
    return bytes(raw), info


def build_rewards(cue):
    """FPE1: ten BE offsets, original scoring/reward data; shared subspans.

    The original level lookup intentionally reaches beyond its16-byte declared
    region into the adjacent Faith bytes. Preserve31 source bytes and alias
    the15-byte Faith lookup at+16, rather than substituting an invented table.
    """
    disc, files = extract.disc_files(cue)
    core = disc.read(*files['WORLD/WLDCORE.BIN'])
    base = struct.unpack_from('<I', core, 0x8009d340 - 0x80067000)[0] - 0x80067000
    offsets = struct.unpack_from('<15I', core, base)
    raw = bytearray(b'FPE1\0\0\x0a\x01' + bytes(20))
    tables = []
    for slot, (index, size) in enumerate(zip(REWARD_TABLES, REWARD_SIZES)):
        data = core[base + offsets[index]:base + offsets[index] + size]
        if index in (8, 9, 12, 13):
            data = struct.pack('>' + str(size // 2) + 'H', *struct.unpack('<' + str(size // 2) + 'H', data))
        start = raw.find(data, 28)
        if start < 0:
            start = len(raw); raw.extend(data)
        struct.pack_into('>H', raw, 8 + 2 * slot, start)
        tables.append(list(data))
    struct.pack_into('>H', raw, 4, len(raw))
    if struct.unpack_from('>H', raw, 14)[0] != struct.unpack_from('>H', raw, 12)[0] + 16:
        raise ValueError('Original scoring/Faith overlap lost')
    return bytes(raw), dict(source_sha256=hashlib.sha256(core).hexdigest(), tables=tables,
                           original_records=list(REWARD_TABLES), sizes=list(REWARD_SIZES),
                           scope='Source scoring/reward tables, retaining original overlapping lookups; no shipping writes.')
