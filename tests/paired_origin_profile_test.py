#!/usr/bin/env python3
import sys
from pathlib import Path

root = Path(sys.argv[1])
sys.path.insert(0, str(root / "runtime"))

from firered_generator.pokemon import encode_box_pokemon  # noqa: E402


def record(game):
    return {
        "personality": 0,
        "otId": 0,
        "nickname": "TEST",
        "otName": "MAQ",
        "species": {"internalId": 1},
        "experience": 0,
        "moves": [],
        "friendship": 70,
        "evs": {name: 0 for name in ("hp", "attack", "defense", "speed", "specialAttack", "specialDefense")},
        "ivs": {name: 0 for name in ("hp", "attack", "defense", "speed", "specialAttack", "specialDefense")},
        "abilitySlot": 0,
        "origins": {"metLocation": 255, "metLevel": 5, "metGame": game},
    }


def origin_game(encoded):
    # PID 0 selects G-A-E-M order and encryption key 0, so M begins at byte 68.
    origins = int.from_bytes(encoded[70:72], "little")
    return (origins >> 7) & 0xF


assert origin_game(encode_box_pokemon(record("FireRed"))) == 4
assert origin_game(encode_box_pokemon(record("LeafGreen"))) == 5
print("paired origin profile test passed")
