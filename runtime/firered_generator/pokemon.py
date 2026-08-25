import math

from .binary import put_u16, put_u32
from .text import encode_name


LOGICAL_TO_PHYSICAL = [
    [0,1,2,3], [0,1,3,2], [0,2,1,3], [0,3,1,2], [0,2,3,1], [0,3,2,1],
    [1,0,2,3], [1,0,3,2], [2,0,1,3], [3,0,1,2], [2,0,3,1], [3,0,2,1],
    [1,2,0,3], [1,3,0,2], [2,1,0,3], [3,1,0,2], [2,3,0,1], [3,2,0,1],
    [1,2,3,0], [1,3,2,0], [2,1,3,0], [3,1,2,0], [2,3,1,0], [3,2,1,0],
]
NATURE_RAISE = [-1,1,1,1,1, 2,-1,2,2,2, 3,3,-1,3,3, 4,4,4,-1,4, 5,5,5,5,-1]
NATURE_LOWER = [-1,2,3,4,5, 1,-1,3,4,5, 1,2,-1,4,5, 1,2,3,-1,5, 1,2,3,4,-1]


def _pokemon_checksum(decrypted):
    return sum(int.from_bytes(decrypted[i:i + 2], "little") for i in range(0, 48, 2)) & 0xFFFF


def _stat(base, iv, ev, level, nature_id, stat_index):
    ev_term = math.isqrt(int(ev)) // 4
    value = ((2 * int(base) + int(iv) + ev_term) * int(level)) // 100 + 5
    if NATURE_RAISE[nature_id] == stat_index:
        value = value * 110 // 100
    elif NATURE_LOWER[nature_id] == stat_index:
        value = value * 90 // 100
    return min(65535, value)


def party_stats(mon, species):
    level = int(mon["level"])
    ivs, evs, base = mon["ivs"], mon["evs"], species["baseStats"]
    hp = ((2 * base["hp"] + ivs["hp"] + math.isqrt(evs["hp"]) // 4) * level) // 100 + level + 10
    order = ["attack", "defense", "speed", "specialAttack", "specialDefense"]
    return hp, [_stat(base[name], ivs[name], evs[name], level, mon["nature"]["id"], index) for index, name in enumerate(order, 1)]


def encode_box_pokemon(mon):
    result = bytearray(80)
    pid = int(mon["personality"])
    ot_id = int(mon["otId"])
    put_u32(result, 0, pid)
    put_u32(result, 4, ot_id)
    result[8:18] = encode_name(mon["nickname"], 10, allow_full=True)
    result[18] = 2  # English
    result[19] = 0x02  # hasSpecies
    result[20:27] = encode_name(mon["otName"], 7, allow_full=True)
    result[27] = 0

    logical = [bytearray(12) for _ in range(4)]
    growth, attacks, effort, misc = logical
    put_u16(growth, 0, mon["species"]["internalId"])
    put_u16(growth, 2, mon.get("heldItem", {}).get("id", 0))
    put_u32(growth, 4, mon["experience"])
    pp_bonuses = 0
    for slot, move in enumerate(mon["moves"][:4]):
        put_u16(attacks, slot * 2, move["id"])
        attacks[8 + slot] = int(move["pp"]) & 0xFF
        pp_bonuses |= (int(move["ppUps"]) & 3) << (slot * 2)
    growth[8] = pp_bonuses
    growth[9] = int(mon["friendship"]) & 0xFF

    ev_order = ["hp", "attack", "defense", "speed", "specialAttack", "specialDefense"]
    for index, name in enumerate(ev_order): effort[index] = int(mon["evs"][name]) & 0xFF
    contest = mon.get("contest", {})
    for index, name in enumerate(("cool", "beauty", "cute", "smart", "tough")):
        effort[6 + index] = int(contest.get(name, 0)) & 0xFF
    effort[11] = int(contest.get("sheen", 0)) & 0xFF

    misc[0] = int(mon.get("pokerus", {}).get("raw", 0)) & 0xFF
    misc[1] = int(mon["origins"]["metLocation"]) & 0xFF
    game_codes = {"FireRed": 4, "LeafGreen": 5}
    met_game = mon["origins"].get("metGame", "FireRed")
    if met_game not in game_codes:
        raise ValueError(f"unsupported Gen III origin game: {met_game}")
    origins = ((int(mon["origins"]["metLevel"]) & 0x7F)
               | (game_codes[met_game] << 7) | (4 << 11))
    if mon["origins"].get("otFemale"): origins |= 1 << 15
    put_u16(misc, 2, origins)
    iv_word = 0
    for index, name in enumerate(ev_order): iv_word |= (int(mon["ivs"][name]) & 31) << (index * 5)
    iv_word |= (int(mon["abilitySlot"]) & 1) << 31
    put_u32(misc, 4, iv_word)
    ribbon_names = {str(name).lower().replace("_", " ") for name in mon.get("ribbons", {}).get("names", [])}
    ribbon_word = 1 << 15 if "champion ribbon" in ribbon_names or "champion" in ribbon_names else 0
    if mon["origins"].get("fatefulEncounter"):
        ribbon_word |= 1 << 31
    put_u32(misc, 8, ribbon_word)

    decrypted = bytearray(48)
    for logical_type, block in enumerate(logical):
        physical = LOGICAL_TO_PHYSICAL[pid % 24][logical_type]
        decrypted[physical * 12:(physical + 1) * 12] = block
    put_u16(result, 28, _pokemon_checksum(decrypted))
    key = pid ^ ot_id
    for offset in range(0, 48, 4):
        value = int.from_bytes(decrypted[offset:offset + 4], "little") ^ key
        put_u32(result, 32 + offset, value)
    return result


def encode_party_pokemon(mon, species):
    result = encode_box_pokemon(mon) + bytearray(20)
    hp, stats = party_stats(mon, species)
    put_u32(result, 80, 0)
    result[84] = int(mon["level"]) & 0xFF
    result[85] = 0xFF
    put_u16(result, 86, hp)
    put_u16(result, 88, hp)
    for index, value in enumerate(stats): put_u16(result, 90 + index * 2, value)
    return result
