#!/usr/bin/env python3
"""extract.mesh_blob(): a mesh spread over several GNS records, on synthetic data.

FFT's state records fill only the header pointers they override. Map 53 (Entrance to
Death City) is the one retail map whose primary leaves the geometry pointer at 0, and
reading that record alone drew an empty map until mesh_blob() assembled the chunks.
These fixtures carry no commercial data.
"""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import extract   # noqa: E402

GEOM, PAL, TERR = 0x40, 0x44, 0x68
HEAD = 0xC0                       # past every header pointer this reader uses


def record(geometry=None, palette=None, terrain=None):
    """One mesh record: a pointer header, then only the chunks it carries."""
    body, head = bytearray(), bytearray(HEAD)
    for off, payload in ((GEOM, geometry), (PAL, palette), (TERR, terrain)):
        if payload is None:
            continue
        struct.pack_into('<I', head, off, HEAD + len(body))
        body += payload
    return bytes(head + body)


class Disc:
    """gns_records()/mesh_blob() only ever call read(sector, size)."""

    def __init__(self, records):
        self.blobs, gns = {}, bytearray()
        for i, (kind, payload) in enumerate(records, 1):
            self.blobs[i] = payload
            gns += (b'\0' * 4 + struct.pack('<H', kind) + b'\0' * 2
                    + struct.pack('<II', i, len(payload)) + b'\0' * 4)   # 20 bytes a record
        gns += b'\0' * 4 + struct.pack('<H', 0x3101) + b'\0' * 34       # the end record, then slack
        self.blobs[0] = bytes(gns)

    def read(self, sector, size):
        return self.blobs[sector][:size]


def blob(records):
    disc = Disc(records)
    return extract.mesh_blob(disc, {'MAP/MAP%03d.GNS' % extract.MAP: (0, len(disc.blobs[0]))})


class MeshBlob(unittest.TestCase):
    def setUp(self):
        self.addCleanup(setattr, extract, 'MAP', extract.MAP)

    def test_complete_primary_is_returned_untouched(self):
        """The 120 maps whose primary carries all three chunks must not change by a byte."""
        primary = record(b'G' * 8, b'P' * 8, b'T' * 8)
        out = blob([(0x2E01, primary), (0x3001, record(b'g' * 8, b'p' * 8, b't' * 8))])
        self.assertEqual(out, primary)

    def test_geometry_missing_from_the_primary_comes_from_the_next_record(self):
        """Map 53's shape: palette and terrain on the primary, polygons on the 0x2F01."""
        donor = record(b'GEOMETRY')
        out = blob([(0x2E01, record(None, b'PALETTE!', b'TERRAIN!')), (0x2F01, donor)])
        p = struct.unpack_from('<I', out, GEOM)[0]
        self.assertEqual(out[p:p + 8], b'GEOMETRY')
        # The chunks the primary did carry stay where they were.
        self.assertEqual(out[struct.unpack_from('<I', out, PAL)[0]:][:8], b'PALETTE!')
        self.assertEqual(out[struct.unpack_from('<I', out, TERR)[0]:][:8], b'TERRAIN!')

    def test_a_map_without_a_primary_record_uses_its_first_0x2F01(self):
        """16 retail maps carry no 0x2E01 at all; the day/clear state is the first 0x2F01."""
        first = record(b'FIRST!!!', b'P' * 8, b'T' * 8)
        out = blob([(0x2F01, first), (0x3001, record(b'LATER!!!', b'p' * 8, b't' * 8))])
        self.assertEqual(out, first)

    def test_the_primary_wins_even_when_a_state_record_comes_first(self):
        """Besrodio's House lists two 0x3001 before its 0x2F01: order alone must not decide."""
        primary = record(b'PRIMARY!', b'P' * 8, b'T' * 8)
        out = blob([(0x3001, record(b'OTHER!!!', b'p' * 8, b't' * 8)), (0x2E01, primary)])
        self.assertEqual(out, primary)

    def test_a_chunk_no_record_carries_stays_empty(self):
        """A missing chunk must leave the pointer at 0, not point into another chunk."""
        out = blob([(0x2E01, record(b'G' * 8, None, b'T' * 8)), (0x3001, record(b'g' * 8))])
        self.assertEqual(struct.unpack_from('<I', out, PAL)[0], 0)


if __name__ == '__main__':
    unittest.main()
