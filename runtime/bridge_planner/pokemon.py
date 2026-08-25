import math

from .errors import ConversionInvariantError
from .util import deterministic_bytes, deterministic_u16, deterministic_u32, sha256_json
from .validation import source_dex_number


NATURES = [
    "HARDY", "LONELY", "BRAVE", "ADAMANT", "NAUGHTY",
    "BOLD", "DOCILE", "RELAXED", "IMPISH", "LAX",
    "TIMID", "HASTY", "SERIOUS", "JOLLY", "NAIVE",
    "MODEST", "MILD", "QUIET", "BASHFUL", "RASH",
    "CALM", "GENTLE", "SASSY", "CAREFUL", "QUIRKY",
]
EV_STATS = ["hp", "attack", "defense", "speed", "specialAttack", "specialDefense"]


def source_is_shiny(mon):
    for key in ("isShiny", "shiny"):
        value = mon.get(key)
        if isinstance(value, bool):
            return value, "explicit-source-field"
        if isinstance(value, dict) and isinstance(value.get("isShiny"), bool):
            return value["isShiny"], "explicit-source-field"
    dvs = mon["dvs"]
    attack = int(dvs["attack"])
    shiny = (
        int(dvs["defense"]) == 10
        and int(dvs["speed"]) == 10
        and int(dvs["special"]) == 10
        and attack in {2, 3, 6, 7, 10, 11, 14, 15}
    )
    return shiny, "gen2-dv-compatibility-calculation"


def infer_gender(gender_ratio, attack_dv):
    if gender_ratio == 255:
        return "genderless"
    if gender_ratio == 0:
        return "male"
    if gender_ratio == 254:
        return "female"
    return "female" if int(attack_dv) * 17 < gender_ratio else "male"


def pid_gender(gender_ratio, pid):
    if gender_ratio == 255:
        return "genderless"
    if gender_ratio == 0:
        return "male"
    if gender_ratio == 254:
        return "female"
    return "female" if (pid & 0xFF) < gender_ratio else "male"


def is_target_shiny(pid, tid, sid):
    return ((pid & 0xFFFF) ^ (pid >> 16) ^ tid ^ sid) < 8


def scaled_evs(stat_exp):
    def ceil_sqrt(value):
        value = max(0, min(65535, int(value)))
        root = math.isqrt(value)
        return root if root * root == value else root + 1

    raw = {
        "hp": ceil_sqrt(stat_exp.get("hp", 0)),
        "attack": ceil_sqrt(stat_exp.get("attack", 0)),
        "defense": ceil_sqrt(stat_exp.get("defense", 0)),
        "speed": ceil_sqrt(stat_exp.get("speed", 0)),
        "specialAttack": ceil_sqrt(stat_exp.get("special", 0)),
        "specialDefense": ceil_sqrt(stat_exp.get("special", 0)),
    }
    total = sum(raw.values())
    if total <= 510:
        return raw, False, raw.copy()
    scaled = {name: raw[name] * 510 // total for name in EV_STATS}
    remaining = 510 - sum(scaled.values())
    order = sorted(EV_STATS, key=lambda name: (-(raw[name] * 510 % total), EV_STATS.index(name)))
    for name in order[:remaining]:
        scaled[name] += 1
    return scaled, True, raw


class PokemonConverter:
    def __init__(self, metadata, policy, source_fingerprint, player_name, player_tid, player_sid,
                 target_game="firered"):
        self.metadata = metadata
        self.policy = policy
        self.source_fingerprint = source_fingerprint
        self.player_name = player_name
        self.player_tid = int(player_tid)
        self.player_sid = int(player_sid)
        self.target_game = target_game
        self.target_display_name = "LeafGreen" if target_game == "leafgreen" else "FireRed"
        self.species = metadata["species"]
        self.moves_by_id = {row["redId"]: row for row in metadata["moves"]}
        self.moves_by_name = {row["redName"]: row for row in metadata["moves"]}
        self.ot_sid_cache = {(player_name.upper(), self.player_tid): self.player_sid}

    def ot_sid(self, name, tid):
        key = (str(name).upper(), int(tid))
        if key not in self.ot_sid_cache:
            self.ot_sid_cache[key] = deterministic_u16(
                self.policy.salt, self.source_fingerprint, "ot-sid", key[0], key[1]
            )
        return self.ot_sid_cache[key]

    def _pid_matches(self, pid, nature, gender, ratio, ability_slot, shiny, tid, sid):
        return (
            pid % 25 == nature
            and pid_gender(ratio, pid) == gender
            and (pid & 1) == ability_slot
            and is_target_shiny(pid, tid, sid) == shiny
        )

    def generate_pid(self, context, nature, gender, ratio, ability_slot, shiny, tid, sid):
        if shiny:
            for counter in range(1_000_000):
                material = deterministic_bytes(self.policy.salt, context, "shiny-pid", counter, size=4)
                low = int.from_bytes(material[:2], "little")
                shiny_value = material[2] & 7
                high = low ^ tid ^ sid ^ shiny_value
                pid = low | (high << 16)
                if self._pid_matches(pid, nature, gender, ratio, ability_slot, True, tid, sid):
                    return pid, counter + 1
        else:
            for counter in range(100_000):
                pid = deterministic_u32(self.policy.salt, context, "nonshiny-pid", counter)
                if self._pid_matches(pid, nature, gender, ratio, ability_slot, False, tid, sid):
                    return pid, counter + 1
        raise ConversionInvariantError(f"Could not generate a policy-compatible PID for {context}")

    def convert_moves(self, mon):
        output = []
        changes = []
        warnings = []
        for slot_index in range(4):
            source = mon.get("moves", [])[slot_index] if slot_index < len(mon.get("moves", [])) else None
            if not source:
                source = {"slot": slot_index + 1, "move": {"id": 0, "name": "NO MOVE"}, "pp": {"current": 0, "ppUps": 0}}
            move = source.get("move", {})
            source_id = int(move.get("id", source.get("moveId", 0)) or 0)
            source_name = str(move.get("name", source.get("moveName", "NO MOVE"))).upper().replace(" ", "_")
            row = self.moves_by_id.get(source_id) or self.moves_by_name.get(source_name)
            if not row or row["classification"] != "DIRECT_MOVE_EQUIVALENT":
                warnings.append(f"Move slot {slot_index + 1} ({source_name}/{source_id}) has no safe FireRed mapping and was cleared.")
                row = self.moves_by_id[0]
            pp_value = source.get("pp", 0)
            pp_ups = max(0, min(3, int(
                pp_value.get("ppUps", 0) if isinstance(pp_value, dict) else source.get("ppUps", 0)
            ) or 0))
            base_pp = int(row["fireRedBasePp"] or 0)
            maximum = base_pp * (5 + pp_ups) // 5 if base_pp else 0
            current_value = pp_value.get("current", 0) if isinstance(pp_value, dict) else pp_value
            current = max(0, min(maximum, int(current_value or 0)))
            output.append({
                "slot": slot_index + 1,
                "id": row["fireRedId"],
                "name": row["fireRedName"],
                "pp": current,
                "ppUps": pp_ups,
                "maximumPp": maximum,
            })
            changes.append({
                "field": f"moves[{slot_index}]",
                "source": {"id": source_id, "name": source_name},
                "target": {"id": row["fireRedId"], "name": row["fireRedName"]},
                "action": "preserve-by-semantic-move-name",
            })
        return output, changes, warnings

    def convert(self, mon, locator, target_locator):
        dex = int(source_dex_number(mon) or 0)
        if str(dex) not in self.species:
            raise ConversionInvariantError(f"{locator}: species {dex} is not a supported Kanto species")
        species = self.species[str(dex)]
        level = int(mon.get("level", 0) or 0)
        experience = int(mon.get("experience", 0) or 0)
        dvs = mon.get("dvs", {})
        stat_exp = mon.get("statExperience", {})
        ot = mon.get("originalTrainer", {})
        canonical_ot_name = mon.get("otName", {})
        if isinstance(canonical_ot_name, dict):
            canonical_ot_name = canonical_ot_name.get("value")
        ot_name = str(canonical_ot_name or ot.get("name") or self.player_name)
        raw_tid = mon.get("trainerId", ot.get("idNo", self.player_tid))
        tid = self.player_tid if raw_tid is None else int(raw_tid)
        sid = self.ot_sid(ot_name, tid)
        shiny, shiny_source = source_is_shiny(mon)
        nature = experience % 25
        gender = infer_gender(species["genderRatio"], dvs["attack"])
        second_ability = species["abilities"][1]["id"] != 0
        preferred_slot = deterministic_u32(self.policy.salt, self.source_fingerprint, locator, "ability") & 1
        ability_slot = preferred_slot if second_ability else 0
        context = sha256_json({"locator": locator, "pokemon": mon, "policy": self.policy.profile_label})
        pid, attempts = self.generate_pid(context, nature, gender, species["genderRatio"], ability_slot, shiny, tid, sid)

        iv_names = {"hp": "hp", "attack": "attack", "defense": "defense", "speed": "speed",
                    "specialAttack": "special", "specialDefense": "special"}
        ivs = {}
        for index, (target_name, source_name) in enumerate(iv_names.items()):
            bit = deterministic_bytes(self.policy.salt, context, "iv", target_name, size=1)[0] & 1
            ivs[target_name] = int(dvs[source_name]) * 2 + bit
        evs, ev_scaled, raw_evs = scaled_evs(stat_exp)
        moves, move_changes, warnings = self.convert_moves(mon)
        nickname_value = mon.get("nickname", {})
        if isinstance(nickname_value, dict):
            nickname_value = nickname_value.get("value")
        nickname = str(nickname_value or species["speciesName"])[:10]
        status = mon.get("status", {})
        ability = species["abilities"][ability_slot]
        target = {
            "occupied": True,
            "species": {"internalId": dex, "nationalDex": dex, "name": species["speciesName"]},
            "nickname": nickname,
            "otName": ot_name[:7],
            "publicTrainerId": tid,
            "secretTrainerId": sid,
            "otId": tid | (sid << 16),
            "personality": pid,
            "personalityHex": f"0x{pid:08X}",
            "isShiny": shiny,
            "shinyValue": (pid & 0xFFFF) ^ (pid >> 16) ^ tid ^ sid,
            "level": level,
            "experience": experience,
            "nature": {"id": nature, "name": NATURES[nature]},
            "gender": gender,
            "abilitySlot": ability_slot,
            "ability": {"id": ability["id"], "name": ability["symbol"].removeprefix("ABILITY_")},
            "ivs": ivs,
            "evs": evs,
            "moves": moves,
            "friendship": 70,
            "heldItem": {"id": 0, "name": "NONE"},
            "language": "English",
            "pokerus": {"raw": 0, "strain": 0, "daysRemaining": 0},
            "origins": {
                "metLocation": 255,
                "metLevel": level,
                "metGame": self.target_display_name,
                "pokeball": "Poke Ball",
                "otFemale": False,
                "fatefulEncounter": False,
            },
            "ribbons": {"raw": 0, "names": []},
            "contest": {"cool": 0, "beauty": 0, "cute": 0, "smart": 0, "tough": 0, "sheen": 0},
            "sourceBattleState": {
                "status": status.get("name", "unknown") if isinstance(status, dict) else status,
                "currentHp": mon.get("currentHp", mon.get("stats", {}).get("hpCurrent")),
                "policy": "generator-recalculates-stats-and-clamps-current-hp",
            },
        }
        if dex == 151:
            warnings.append("Mew was preserved with a legality warning; ORIGINAL_V1 does not silently sanitize event identity.")
        if ev_scaled:
            warnings.append("Gen I stat-experience-derived EVs exceeded 510 and were proportionally scaled.")

        field_changes = [
            {"field": "species", "source": dex, "target": dex, "action": "preserve-national-dex-species"},
            {"field": "experience", "source": experience, "target": experience, "action": "ORIGINAL_V1-override-preserve-exact"},
            {"field": "pid", "source": None, "target": pid, "action": "deterministically-generate", "attempts": attempts},
            {"field": "nature", "source": None, "target": nature, "action": "experience-modulo-25"},
            {"field": "gender", "source": None, "target": gender, "action": "infer-from-attack-dv-and-constrain-pid"},
            {"field": "shiny", "source": shiny, "target": shiny, "action": "preserve-status", "sourceMethod": shiny_source},
            {"field": "ivs", "source": dvs, "target": ivs, "action": "ORIGINAL_V1-override-dv-times-two-plus-deterministic-bit"},
            {"field": "evs", "source": stat_exp, "intermediate": raw_evs, "target": evs, "action": "ORIGINAL_V1-override-stat-experience-conversion"},
            {"field": "secretTrainerId", "source": None, "target": sid, "action": "deterministically-generate-per-ot"},
        ] + move_changes
        audit = {
            "sourceLocator": locator,
            "targetLocator": target_locator,
            "sourceGeneration": 1,
            "targetGeneration": 3,
            "policy": self.policy.profile_label,
            "standard": "PCCS ORIGINAL foundation with recorded ORIGINAL_V1 overrides",
            "standardCommit": self.policy.raw["baseStandard"]["commit"],
            "sourcePokemonFingerprint": sha256_json(mon),
            "fieldChanges": field_changes,
            "warnings": warnings,
        }
        self.validate_target(target, audit)
        return target, audit

    def validate_target(self, target, audit):
        errors = []
        pid = target["personality"]
        if pid % 25 != target["nature"]["id"]:
            errors.append("PID/nature mismatch")
        species = self.species[str(target["species"]["nationalDex"])]
        if pid_gender(species["genderRatio"], pid) != target["gender"]:
            errors.append("PID/gender mismatch")
        if (pid & 1) != target["abilitySlot"]:
            errors.append("PID/ability mismatch")
        if is_target_shiny(pid, target["publicTrainerId"], target["secretTrainerId"]) != target["isShiny"]:
            errors.append("PID/TID/SID shiny mismatch")
        if any(not 0 <= value <= 31 for value in target["ivs"].values()):
            errors.append("IV outside 0-31")
        if any(not 0 <= value <= 255 for value in target["evs"].values()) or sum(target["evs"].values()) > 510:
            errors.append("EV spread violates Gen III caps")
        if errors:
            raise ConversionInvariantError(f"{audit['sourceLocator']}: " + "; ".join(errors))
