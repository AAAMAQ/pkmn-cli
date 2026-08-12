import struct
from dataclasses import dataclass


FLASH_SIZE = 0x20000
SECTOR_SIZE = 0x1000
SIGNATURE = 0x08012025
DATA_SIZES = [0xF24, 0xF80, 0xF80, 0xF80, 0xEE8] + [0xF80] * 8 + [0x7D0]


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def put_u16(data, offset, value):
    struct.pack_into("<H", data, offset, int(value) & 0xFFFF)


def put_u32(data, offset, value):
    struct.pack_into("<I", data, offset, int(value) & 0xFFFFFFFF)


def section_checksum(data):
    if len(data) % 4:
        raise ValueError("section checksum data must be divisible by four")
    total = sum(struct.unpack_from("<I", data, offset)[0] for offset in range(0, len(data), 4)) & 0xFFFFFFFF
    return ((total >> 16) + (total & 0xFFFF)) & 0xFFFF


@dataclass
class Slot:
    index: int
    counter: int
    sections: dict


def analyze_slots(image):
    if len(image) < FLASH_SIZE:
        raise ValueError("FireRed template must contain at least 128 KiB")
    valid = []
    for slot_index in range(2):
        sections = {}
        counters = set()
        okay = True
        for local in range(14):
            offset = (slot_index * 14 + local) * SECTOR_SIZE
            sector = image[offset:offset + SECTOR_SIZE]
            section_id = u16(sector, 0xFF4)
            counter = u32(sector, 0xFFC)
            if section_id >= 14 or u32(sector, 0xFF8) != SIGNATURE or section_id in sections:
                okay = False
                continue
            if u16(sector, 0xFF6) != section_checksum(sector[:DATA_SIZES[section_id]]):
                okay = False
            sections[section_id] = offset
            counters.add(counter)
        if okay and len(sections) == 14 and len(counters) == 1:
            valid.append(Slot(slot_index, counters.pop(), sections))
    if not valid:
        raise ValueError("template has no complete checksum-valid FireRed save slot")
    if len(valid) == 2 and valid[0].counter == valid[1].counter:
        raise ValueError("template active slot is ambiguous")
    return max(valid, key=lambda slot: slot.counter)


def assemble_logical(image, slot):
    def collect(first, last):
        output = bytearray()
        for section_id in range(first, last + 1):
            offset = slot.sections[section_id]
            output.extend(image[offset:offset + DATA_SIZES[section_id]])
        return output
    return collect(0, 0), collect(1, 4), collect(5, 13)


def scatter_logical(image, slot, blocks):
    for (first, last), logical in zip(((0, 0), (1, 4), (5, 13)), blocks):
        cursor = 0
        for section_id in range(first, last + 1):
            size = DATA_SIZES[section_id]
            offset = slot.sections[section_id]
            image[offset:offset + size] = logical[cursor:cursor + size]
            put_u16(image, offset + 0xFF6, section_checksum(image[offset:offset + size]))
            cursor += size
        if cursor != len(logical):
            raise ValueError("logical block size does not match FireRed layout")


def validate_generated_image(image, expected_slot):
    slot = analyze_slots(image)
    if slot.index != expected_slot.index:
        raise ValueError("generated output changed the active slot unexpectedly")
    return slot
