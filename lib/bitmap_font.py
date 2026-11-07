"""Offline BMF1 builder. Pillow on the host; no vector parser/scaling on the TI."""
from pathlib import Path
import struct
from PIL import Image, ImageDraw, ImageFont

DEFAULT_FACE = Path('/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf')


def glyph(font, char):
    """Baseline-relative ink bounds, black shadow and four-grey column sprites."""
    im = Image.new('L', (80, 64), 128)
    draw = ImageDraw.Draw(im)
    draw.text((25, 33), char, font=font, fill=0, anchor='ls')
    draw.text((24, 32), char, font=font, fill=255, anchor='ls')
    ink = im.point(lambda v: 255 if v != 128 else 0).getbbox()
    advance = round(font.getlength(char) * 64)
    if ink is None:
        return struct.pack('>HbbBBBB', advance, 0, 0, 0, 0, 0, 0)
    x, y, right, bottom = ink
    width, height = right-x, bottom-y
    assert width <= 32 and height <= 24, (char, width, height)
    body = bytearray(struct.pack('>HbbBBBB', advance, x-24, y-32, width, height, 0, 0))
    for col in range(0, width, 8):
        rows = [bytearray(height) for _ in range(3)]
        for row in range(height):
            for bit in range(8):
                v = im.getpixel((x+col+bit, y+row)) if col+bit < width else 128
                grey = -1 if v == 128 else 0 if v > 212 else 1 if v > 127 else 2 if v > 42 else 3
                mask = 0x80 >> bit
                if grey < 0:
                    rows[2][row] |= mask
                else:
                    if grey & 1: rows[0][row] |= mask
                    if grey & 2: rows[1][row] |= mask
        body += b''.join(rows)
    return bytes(body)


def build(faces, path=DEFAULT_FACE):
    """faces: iterable of (point_size, character_repertoire), stable face IDs."""
    faces = list(faces)
    bank = bytearray(b'BMF1' + bytes((len(faces), 0, 0, 0)) + bytes(8*len(faces)))
    report = []
    for face_id, (size, chars) in enumerate(faces):
        chars = ''.join(sorted(set(chars) | {' ', '?'}))
        assert all(32 <= ord(c) <= 126 for c in chars)
        font = ImageFont.truetype(str(path), size)
        directory = len(bank)
        bank += bytes(190)
        for char in chars:
            struct.pack_into('>H', bank, directory+2*(ord(char)-32), len(bank))
            bank += glyph(font, char)
        kern_at = len(bank)
        for a in chars:
            for b in chars:
                delta = round((font.getlength(a+b)-font.getlength(a)-font.getlength(b))*64)
                if delta:
                    assert -256 <= delta <= 256
                    bank += struct.pack('>Hh', ord(a)<<8 | ord(b), delta)
        kerns = (len(bank)-kern_at)//4
        struct.pack_into('>HHHBB', bank, 8+8*face_id, directory, kern_at, kerns, size, 0)
        report.append(dict(face=face_id, point_size=size, glyphs=len(chars), kern_pairs=kerns,
                           bytes=len(bank)-directory, repertoire=chars))
    assert len(bank) < 65536
    struct.pack_into('>H', bank, 6, len(bank))
    return bytes(bank), report
