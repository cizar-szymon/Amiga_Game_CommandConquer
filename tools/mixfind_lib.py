import sys

def tdhash(name):
    """Westwood TD id: CRC engine, 4-byte little-endian groups, rotl1 + group."""
    b = name.upper().encode("latin-1")
    crc = 0
    for i in range(0, len(b), 4):
        chunk = b[i:i + 4]
        grp = int.from_bytes(chunk + bytes(4 - len(chunk)), "little")
        crc = (((crc << 1) | (crc >> 31)) & 0xFFFFFFFF) + grp
        crc &= 0xFFFFFFFF
    return crc

