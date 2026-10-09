"""Offline decoder for the GBA BIOS type-0x10 LZ77 format."""


def decompress(data, max_output=0x100000):
    if len(data) < 4 or data[0] != 0x10:
        raise ValueError('expected GBA type-0x10 LZ77 header')
    size = int.from_bytes(data[1:4], 'little')
    if not 0 < size <= max_output:
        raise ValueError('unexpected decompressed size')
    output = bytearray()
    pos = 4
    while len(output) < size:
        if pos >= len(data):
            raise ValueError('truncated LZ77 flags')
        flags = data[pos]
        pos += 1
        for bit in range(7, -1, -1):
            if len(output) == size:
                break
            if flags & (1 << bit):
                if pos + 2 > len(data):
                    raise ValueError('truncated LZ77 back reference')
                code = (data[pos] << 8) | data[pos + 1]
                pos += 2
                length, distance = (code >> 12) + 3, (code & 0xfff) + 1
                if distance > len(output) or len(output) + length > size:
                    raise ValueError('invalid LZ77 back reference')
                for _ in range(length):
                    output.append(output[-distance])
            else:
                if pos >= len(data):
                    raise ValueError('truncated LZ77 literal')
                output.append(data[pos])
                pos += 1
    return bytes(output)
