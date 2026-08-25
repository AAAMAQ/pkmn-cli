import hashlib
import json
from pathlib import Path


def canonical_json_bytes(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def sha256_json(value):
    return hashlib.sha256(canonical_json_bytes(value)).hexdigest()


def sha256_bytes(value):
    return hashlib.sha256(value).hexdigest()


def deterministic_bytes(*parts, size=32):
    material = "\x1f".join(str(part) for part in parts).encode("utf-8")
    output = bytearray()
    counter = 0
    while len(output) < size:
        output.extend(hashlib.sha256(material + counter.to_bytes(4, "big")).digest())
        counter += 1
    return bytes(output[:size])


def deterministic_u16(*parts):
    return int.from_bytes(deterministic_bytes(*parts, size=2), "little")


def deterministic_u32(*parts):
    return int.from_bytes(deterministic_bytes(*parts, size=4), "little")


def load_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def value_at(record, *path, default=None):
    current = record
    for key in path:
        if not isinstance(current, dict) or key not in current:
            return default
        current = current[key]
    if isinstance(current, dict) and "value" in current:
        return current["value"]
    return current


def clamp(value, low, high):
    return max(low, min(high, int(value)))
