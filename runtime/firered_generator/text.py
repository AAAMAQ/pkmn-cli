def _encode_character(value):
    if value == " ": return 0x00
    if "0" <= value <= "9": return 0xA1 + ord(value) - ord("0")
    if "A" <= value <= "Z": return 0xBB + ord(value) - ord("A")
    if "a" <= value <= "z": return 0xD5 + ord(value) - ord("a")
    table = {"é": 0x1B, "&": 0x2D, "+": 0x2E, "=": 0x35, ";": 0x36, "%": 0x5B,
             "(": 0x5C, ")": 0x5D, "!": 0xAB, "?": 0xAC, ".": 0xAD,
             "-": 0xAE, "'": 0xB4, "♂": 0xB5, "♀": 0xB6,
             ",": 0xB8, "/": 0xBA, ":": 0xF0}
    if value not in table:
        raise ValueError(f"unsupported FireRed English character {value!r}")
    return table[value]


def encode_name(text, field_length, allow_full=False):
    maximum = field_length if allow_full else field_length - 1
    if not isinstance(text, str) or not text or len(text) > maximum:
        raise ValueError(f"name must contain 1-{maximum} characters")
    result = bytearray([0xFF] * field_length)
    for index, character in enumerate(text):
        result[index] = _encode_character(character)
    return result
