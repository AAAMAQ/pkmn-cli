from .errors import InputValidationError
from .util import value_at


def source_dex_number(mon):
    """Return the Kanto Pokédex number from either supported Red JSON shape."""
    value = mon.get("pokedexNumber")
    if value is None:
        value = mon.get("species", {}).get("nationalDexNumber")
    return value


def iter_source_pokemon(document):
    decoded = document["decoded"]
    party = decoded.get("party", {}).get("pokemon", [])
    for index, mon in enumerate(party):
        yield f"party/{index}", f"party/{index}", mon
    target_box = 0
    target_slot = 0
    for box_index, box in enumerate(decoded.get("pcStorage", {}).get("boxes", [])):
        for slot_index, mon in enumerate(box.get("pokemon", [])):
            yield f"pcStorage/boxes/{box_index}/pokemon/{slot_index}", f"storage/boxes/{target_box}/slots/{target_slot}", mon
            target_slot += 1
            if target_slot == 30:
                target_box += 1
                target_slot = 0
    daycare = decoded.get("daycare", {})
    if daycare.get("inUse") and isinstance(daycare.get("pokemon"), dict):
        yield "daycare/pokemon", "daycare/route5/0", daycare["pokemon"]


def validate_red_document(document):
    errors = []
    warnings = []
    if not isinstance(document, dict):
        raise InputValidationError(["top level must be an object"])
    schema = document.get("schema")
    if not isinstance(schema, dict):
        errors.append("schema object is required")
    else:
        fmt = schema.get("format")
        if fmt not in {"pkmn-red-master-save", "pkmn-red-json"}:
            errors.append(f"unsupported schema.format {fmt!r}")
        if not schema.get("schemaVersion"):
            errors.append("schema.schemaVersion is required")
    decoded = document.get("decoded")
    if not isinstance(decoded, dict):
        errors.append("decoded object is required")
        raise InputValidationError(errors)
    player_name = value_at(decoded, "trainer", "name")
    trainer_id = value_at(decoded, "trainer", "trainerId")
    if not isinstance(player_name, str) or not player_name:
        errors.append("decoded.trainer.name.value is required")
    if not isinstance(trainer_id, int) or not 0 <= trainer_id <= 65535:
        errors.append("decoded.trainer.trainerId.value must be a 16-bit integer")
    party = decoded.get("party", {}).get("pokemon", [])
    if len(party) > 6:
        errors.append("party contains more than six Pokémon")
    box_mons = sum(len(box.get("pokemon", [])) for box in decoded.get("pcStorage", {}).get("boxes", []))
    if box_mons > 420:
        errors.append("PC Pokémon exceed FireRed's 420-slot target capacity")
    for locator, _, mon in iter_source_pokemon(document):
        if not isinstance(mon, dict):
            errors.append(f"{locator} is not an object")
            continue
        dex = source_dex_number(mon)
        if not isinstance(dex, int) or not 1 <= dex <= 151:
            errors.append(f"{locator}.species.nationalDexNumber must be 1-151")
        level = mon.get("level")
        if not isinstance(level, int) or not 1 <= level <= 100:
            errors.append(f"{locator}.level must be 1-100")
        experience = mon.get("experience")
        if not isinstance(experience, int) or experience < 0 or experience > 0xFFFFFF:
            errors.append(f"{locator}.experience must fit the Gen III 24-bit field")
        dvs = mon.get("dvs", {})
        for name in ("hp", "attack", "defense", "speed", "special"):
            if not isinstance(dvs.get(name), int) or not 0 <= dvs[name] <= 15:
                errors.append(f"{locator}.dvs.{name} must be 0-15")
        stat_exp = mon.get("statExperience", {})
        for name in ("hp", "attack", "defense", "speed", "special"):
            if not isinstance(stat_exp.get(name), int) or not 0 <= stat_exp[name] <= 65535:
                errors.append(f"{locator}.statExperience.{name} must be 0-65535")
        if len(mon.get("moves", [])) > 4:
            errors.append(f"{locator} has more than four moves")
        nickname = value_at(mon, "nickname", default="")
        if isinstance(nickname, str) and len(nickname) > 10:
            warnings.append(f"{locator} nickname exceeds FireRed's ten-character field and will be truncated")
        ot_name = value_at(mon, "otName", default=None)
        if ot_name is None:
            ot_name = mon.get("originalTrainer", {}).get("name", "")
        if isinstance(ot_name, str) and len(ot_name) > 7:
            warnings.append(f"{locator} OT name exceeds FireRed's seven-character field and will be truncated")
    if errors:
        raise InputValidationError(errors)
    return warnings
