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


_JAPANESE_CHARACTERS = {}
for _glyphs, _start in (
    ("あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをん", 0x01),
    ("ぁぃぅぇぉゃゅょ", 0x2F),
    ("がぎぐげござじずぜぞだぢづでど", 0x37),
    ("ばびぶべぼ", 0x46),
    ("ぱぴぷぺぽ", 0x4B),
    ("アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン", 0x51),
    ("ァィゥェォャュョ", 0x7F),
    ("ガギグゲゴザジズゼゾダヂヅデド", 0x87),
    ("バビブベボ", 0x96),
    ("パピプペポ", 0x9B),
):
    _JAPANESE_CHARACTERS.update(
        {glyph: _start + index for index, glyph in enumerate(_glyphs)}
    )
_JAPANESE_CHARACTERS.update({
    "っ": 0x50, "ッ": 0xA0, "　": 0x00,
    "！": 0xAB, "？": 0xAC, "。": 0xAD, "ー": 0xAE,
    ":": 0xF0, "·": 0xAF, "×": 0xB9, "▶": 0xEF,
})
_JAPANESE_CHARACTERS.update({
    glyph: 0xBB + index for index, glyph in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
})
_JAPANESE_CHARACTERS.update({
    glyph: 0xD5 + index for index, glyph in enumerate("abcdefghijklmnopqrstuvwxyz")
})
_JAPANESE_CHARACTERS.update({
    glyph: 0xA1 + index for index, glyph in enumerate("0123456789")
})
_JAPANESE_CHARACTERS.update({
    glyph: 0xA1 + index for index, glyph in enumerate("０１２３４５６７８９")
})
_JAPANESE_CHARACTERS.update({"♂": 0xB5, "♀": 0xB6})
_JAPANESE_CHARACTERS.update({
    glyph: _JAPANESE_CHARACTERS[glyph.translate(str.maketrans(
        "ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"))]
    for glyph in "ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
})


def encode_japanese_name(text, field_length):
    # FireRed's Japanese nickname display reads no more than six bytes.
    # Japanese Red's ordinary five-glyph name plus EOS fits this limit.
    if not isinstance(text, str) or not 1 <= len(text) <= 5:
        raise ValueError("Japanese name must contain 1-5 glyphs")
    if field_length < len(text) + 1:
        raise ValueError("Japanese name field cannot hold the terminator")
    result = bytearray([0xFF] * field_length)
    for index, glyph in enumerate(text):
        if glyph not in _JAPANESE_CHARACTERS:
            raise ValueError(f"unsupported FireRed Japanese glyph {glyph!r}")
        result[index] = _JAPANESE_CHARACTERS[glyph]
    return result


def encode_name(text, field_length, allow_full=False, language="English"):
    if language == "Japanese":
        return encode_japanese_name(text, field_length)
    if language != "English":
        raise ValueError(f"unsupported FireRed name language {language!r}")
    maximum = field_length if allow_full else field_length - 1
    if not isinstance(text, str) or not text or len(text) > maximum:
        raise ValueError(f"name must contain 1-{maximum} characters")
    result = bytearray([0xFF] * field_length)
    for index, character in enumerate(text):
        result[index] = _encode_character(character)
    return result
