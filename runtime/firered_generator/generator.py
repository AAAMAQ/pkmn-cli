import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path

from .binary import (
    FLASH_SIZE, SECTOR_SIZE, SIGNATURE, analyze_slots, assemble_logical,
    put_u16, put_u32, scatter_logical, section_checksum,
    validate_generated_image,
)
from .pokemon import encode_box_pokemon, encode_party_pokemon
from .text import encode_name


POCKETS = {
    "pcItems": (0x298, 30, False),
    "items": (0x310, 42, True),
    "keyItems": (0x3B8, 30, True),
    "pokeBalls": (0x430, 13, True),
    "tmCase": (0x464, 58, True),
    "berries": (0x54C, 43, True),
}

GAME_STAT_IDS = {
    "SAVED_GAME": 0,
    "FIRST_HOF_PLAY_TIME": 1,
    "TRAINER_BATTLES": 9,
    "ENTERED_HOF": 10,
    "RECEIVED_RIBBONS": 42,
}

# pret/pokefirered@df4449a: FLAG_WORLD_MAP_* values consumed by
# GetMapsecType() and sMapFlyDestinations in src/region_map.c. These are the
# direct Red-equivalent city destinations. Route 4/10 Pokémon Centers and
# Sevii have no Red visited-town counterparts and are controlled separately.
RED_EQUIVALENT_FLY_DESTINATION_FLAGS = (
    0x890,  # Pallet Town
    0x891,  # Viridian City
    0x892,  # Pewter City
    0x893,  # Cerulean City
    0x894,  # Lavender Town
    0x895,  # Vermilion City
    0x896,  # Celadon City
    0x897,  # Fuchsia City
    0x898,  # Cinnabar Island
    0x899,  # Indigo Plateau exterior
    0x89A,  # Saffron City
)

FIRERED_ONLY_KANTO_FLY_DESTINATION_FLAGS = (
    0x8A2,  # Route 4 Pokemon Center 1F
    0x8A3,  # Route 10 Pokemon Center 1F
)

# Compatibility export for callers that mean every normal Kanto destination.
KANTO_FLY_DESTINATION_FLAGS = (
    *RED_EQUIVALENT_FLY_DESTINATION_FLAGS,
    *FIRERED_ONLY_KANTO_FLY_DESTINATION_FLAGS,
)


@dataclass
class GenerationResult:
    image: bytes
    report: dict


class FireRedTemplateGenerator:
    def __init__(self, metadata, event_authority, event_rules=None, template_profile=None, location_rules=None):
        self.metadata = metadata
        self.events = {row["semanticId"]: row for row in event_authority["events"]}
        self.event_rules = (event_rules or {}).get("events", {})
        self.event_implications = (event_rules or {}).get("implications", [])
        self.template_profile = template_profile
        self.location_rules = (location_rules or {}).get("policies", {})

    @classmethod
    def from_repository(cls, root):
        root = Path(root)
        rules_path = root / "data/firered_event_generator_rules.json"
        profile_path = root / "data/firered_v1_clean_template_profile.json"
        location_rules_path = root / "data/firered_location_generator_rules.json"
        return cls(
            json.loads((root / "data/original_v1_conversion_metadata.json").read_text()),
            json.loads((root / "data/event_bridge_red_to_firered.json").read_text()),
            json.loads(rules_path.read_text()) if rules_path.exists() else None,
            json.loads(profile_path.read_text()) if profile_path.exists() else None,
            json.loads(location_rules_path.read_text()) if location_rules_path.exists() else None,
        )

    def generate(self, proposed, template_bytes, template_name=None):
        self._validate_proposed(proposed)
        template_sha = hashlib.sha256(template_bytes).hexdigest()
        if self.template_profile:
            expected = self.template_profile["source"]["saveSha256"]
            if template_sha != expected:
                raise ValueError(
                    "template SHA-256 is not the approved FIRERED_V1 clean skeleton"
                )
        image = bytearray(template_bytes)
        slot = analyze_slots(image)
        sb2, sb1, storage = assemble_logical(image, slot)
        semantic = proposed["semantic"]
        warnings = []
        changes = []

        self._identity(sb2, sb1, semantic, changes)
        self._location(sb1, semantic["location"], changes)
        self._pokedex(sb2, sb1, semantic["pokedex"], changes)
        self._inventory(sb2, sb1, semantic["inventory"], changes)
        self._pokemon(sb1, storage, semantic, changes)
        self._reset_bridge_state(sb1)
        self._badges(sb1, semantic["badges"], changes)
        self._trainers(sb1, semantic["trainerBattles"], changes)
        self._events(sb1, semantic["progression"], warnings, changes)
        self._party_menu_unlock(sb1, semantic["party"], changes)
        self._fly_destinations(sb1, semantic.get("flyDestinations"), changes)
        self._whole_save_policies(sb1, semantic, changes)
        self._game_stats(sb2, sb1, semantic.get("gameStats"), changes)

        scatter_logical(image, slot, (sb2, sb1, storage))
        hall_of_fame_written = self._hall_of_fame(
            image, semantic.get("hallOfFame"), semantic["party"], changes
        )
        validate_generated_image(image, slot)
        report = {
            "format": "pkmn-firered-template-generation-report",
            "version": "1.0.0",
            "status": "CANDIDATE_REQUIRES_EMULATOR" if warnings else "STATIC_VALIDATION_PASS",
            "template": {
                "fileName": template_name,
                "sha256": template_sha,
                "profileId": self.template_profile["profileId"] if self.template_profile else None,
                "activeSlot": slot.index,
                "counter": slot.counter,
                "inactiveSlotPreserved": True,
                "specialSectorsPolicy": (
                    "generated-hall-of-fame" if hall_of_fame_written else "preserved"
                ),
            },
            "sourcePlanSha256": hashlib.sha256(
                json.dumps(proposed, sort_keys=True, separators=(",", ":")).encode()
            ).hexdigest(),
            "output": {
                "sha256": hashlib.sha256(image).hexdigest(),
                "size": len(image),
                "activeSlot": slot.index,
                "mainSectionChecksumsValid": True,
                "pokemonRecordsWritten": len(semantic["party"])
                    + sum(len(box["slots"]) for box in semantic["storage"]["boxes"])
                    + len(semantic["daycare"]["route5"]),
            },
            "changes": changes,
            "warnings": warnings,
            "verificationRequired": [
                "Boot generated save in a FireRed v1.0-compatible emulator.",
                "Inspect converted Pokémon, inventory, badges, mapped trainers and major story objects.",
                "Save in game, close emulator, reload, and reanalyze the result with Save Genie.",
            ],
        }
        return GenerationResult(bytes(image), report)

    def _validate_proposed(self, proposed):
        if proposed.get("format") != "pkmn-firered-planned-save" or proposed.get("schemaVersion") != "1.0.0":
            raise ValueError("generator requires pkmn-firered-planned-save@1.0.0")
        if proposed.get("planningStatus") == "REJECTED" or not proposed.get("generatorReady"):
            raise ValueError("refusing a rejected/non-generator-ready plan")
        if not proposed.get("doesNotContainPhysicalSaveBytes"):
            raise ValueError("planned input boundary marker is missing")

    def _identity(self, sb2, sb1, semantic, changes):
        trainer = semantic["trainer"]
        sb2[0:8] = encode_name(trainer["playerName"], 8)
        sb1[0x3A4C:0x3A54] = encode_name(trainer["rivalName"], 8)
        sb2[8] = 0
        put_u16(sb2, 0xA, trainer["publicTrainerId"])
        put_u16(sb2, 0xC, trainer["secretTrainerId"])
        play = semantic["playtime"]
        put_u16(sb2, 0xE, play["hours"])
        sb2[0x10], sb2[0x11], sb2[0x12] = play["minutes"], play["seconds"], play["vblanks"]
        options = semantic["options"]
        sb2[0x13] = int(options.get("buttonMode", 0)) & 0xFF
        raw_options = (int(options["textSpeed"]) & 7) | ((int(options.get("windowFrameType", 0)) & 31) << 3)
        if options.get("sound") == "stereo": raw_options |= 1 << 8
        if options.get("battleStyle") == "set": raw_options |= 1 << 9
        if options.get("battleSceneOff"): raw_options |= 1 << 10
        put_u16(sb2, 0x14, raw_options)
        key = int.from_bytes(sb2[0xF20:0xF24], "little")
        put_u32(sb1, 0x290, int(semantic["currency"]["money"]) ^ key)
        put_u16(sb1, 0x294, int(semantic["currency"]["coins"]) ^ (key & 0xFFFF))
        changes.extend(["identity", "playtime", "options", "currency"])

    @staticmethod
    def _write_warp(sb1, offset, warp):
        sb1[offset] = int(warp["mapGroup"]) & 0xFF
        sb1[offset + 1] = int(warp["mapNumber"]) & 0xFF
        sb1[offset + 2] = int(warp["warpId"]) & 0xFF
        sb1[offset + 3] = 0
        put_u16(sb1, offset + 4, int(warp["x"]) & 0xFFFF)
        put_u16(sb1, offset + 6, int(warp["y"]) & 0xFFFF)

    def _location(self, sb1, planned_location, changes):
        policy_name = planned_location.get("policy")
        policy = self.location_rules.get(policy_name)
        if not policy:
            raise ValueError(f"unsupported or missing location policy: {policy_name}")
        put_u16(sb1, 0, int(policy["x"]))
        put_u16(sb1, 2, int(policy["y"]))
        self._write_warp(sb1, 4, policy)
        self._write_warp(sb1, 0x1C, policy["lastHealLocation"])
        put_u16(sb1, 0x32, int(policy["mapLayoutId"]))
        changes.append(
            f"safe-location:{policy['mapName']}({policy['x']},{policy['y']}),layout={policy['mapLayoutId']}"
        )

    def _pokedex(self, sb2, sb1, pokedex, changes):
        owned = bytearray(52)
        seen = bytearray(52)
        for number in pokedex["ownedNationalDexNumbers"]:
            if 1 <= int(number) <= 386: owned[(int(number) - 1) // 8] |= 1 << ((int(number) - 1) % 8)
        for number in pokedex["seenNationalDexNumbers"]:
            if 1 <= int(number) <= 386: seen[(int(number) - 1) // 8] |= 1 << ((int(number) - 1) % 8)
        sb2[0x28:0x5C] = owned
        sb2[0x5C:0x90] = seen
        sb1[0x5F8:0x62C] = seen
        sb1[0x3A18:0x3A4C] = seen
        changes.append("pokedex-owned-and-three-seen-mirrors")

    def _inventory(self, sb2, sb1, inventory, changes):
        tmhm_ids = set(range(289, 347))
        for name in ("pcItems", "items", "keyItems", "pokeBalls", "berries"):
            misplaced = [row["id"] for row in inventory.get(name, []) if int(row["id"]) in tmhm_ids]
            if misplaced:
                raise ValueError(f"TM/HM item IDs must be stored in tmCase, not {name}: {misplaced}")
        invalid_tmhm = [row["id"] for row in inventory.get("tmCase", []) if int(row["id"]) not in tmhm_ids]
        if invalid_tmhm:
            raise ValueError(f"tmCase contains non-TM/HM item IDs: {invalid_tmhm}")
        key_item_ids = {int(row["id"]) for row in inventory.get("keyItems", [])}
        if inventory.get("tmCase") and 364 not in key_item_ids:
            raise ValueError("a non-empty tmCase requires ITEM_TM_CASE in keyItems")
        if inventory.get("berries") and 365 not in key_item_ids:
            raise ValueError("a non-empty berries pocket requires ITEM_BERRY_POUCH in keyItems")

        key = int.from_bytes(sb2[0xF20:0xF24], "little") & 0xFFFF
        for name, (offset, capacity, encrypted) in POCKETS.items():
            entries = list(inventory.get(name, []))
            ids = [int(row["id"]) for row in entries]
            if len(ids) != len(set(ids)):
                raise ValueError(f"{name} contains duplicate item IDs")
            if name == "tmCase":
                entries.sort(key=lambda row: (0 if int(row["id"]) >= 339 else 1, int(row["id"])))
            if len(entries) > capacity:
                raise ValueError(f"{name} exceeds FireRed capacity {capacity}")
            sb1[offset:offset + capacity * 4] = bytes(capacity * 4)
            for slot, entry in enumerate(entries):
                put_u16(sb1, offset + slot * 4, entry["id"])
                quantity = int(entry["quantity"])
                put_u16(sb1, offset + slot * 4 + 2, quantity ^ (key if encrypted else 0))
        put_u16(sb1, 0x296, 0)
        changes.append("all-inventory-pockets")

    def _pokemon(self, sb1, storage, semantic, changes):
        party = semantic["party"]
        if len(party) > 6: raise ValueError("party exceeds six Pokémon")
        sb1[0x34] = len(party)
        sb1[0x38:0x290] = bytes(600)
        for index, mon in enumerate(party):
            species = self.metadata["species"][str(mon["species"]["nationalDex"])]
            sb1[0x38 + index * 100:0x38 + (index + 1) * 100] = encode_party_pokemon(mon, species)

        storage[0] = max(0, min(13, int(semantic["storage"].get("currentBox", 1)) - 1))
        storage[4:4 + 420 * 80] = bytes(420 * 80)
        occupied = set()
        for box in semantic["storage"]["boxes"]:
            box_index = int(box["boxNumber"]) - 1
            for entry in box["slots"]:
                slot_index = int(entry["slot"])
                absolute = box_index * 30 + slot_index
                if not 0 <= absolute < 420 or absolute in occupied:
                    raise ValueError("invalid or duplicate PC slot")
                occupied.add(absolute)
                start = 4 + absolute * 80
                storage[start:start + 80] = encode_box_pokemon(entry["pokemon"])

        # Reset both FireRed-only daycare slots and Route 5; then write the planned Route 5 mon.
        sb1[0x2F80:0x3124] = bytes(0x1A4)
        sb1[0x3C98:0x3D24] = bytes(140)
        route5 = semantic["daycare"]["route5"]
        if len(route5) > 1: raise ValueError("Route 5 daycare accepts one Pokémon")
        if route5:
            sb1[0x3C98:0x3CE8] = encode_box_pokemon(route5[0])
        changes.append(f"pokemon:party={len(party)},pc={len(occupied)},daycare={len(route5)}")

    def _whole_save_policies(self, sb1, semantic, changes):
        hidden = semantic.get("hiddenItems")
        if hidden:
            if hidden.get("policy") != "safe-default-uncollected":
                raise ValueError("Phase 3 supports only safe-default-uncollected hidden items")
            changes.append("hidden-items:safe-default-uncollected-template-baseline")

        defaults = semantic.get("fireRedOnlyDefaults")
        if defaults:
            if defaults.get("policy") != "locked":
                raise ValueError("FireRed-only state requires the locked Phase 3 policy")
            for flag_id in (0x840, 0x844, 0x845, 0x846, 0x2A1, 0x2DC, 0x2DD, 0x292, 0x801, 0x29B):
                self._flag(sb1, flag_id, False)
            put_u16(sb1, 0x1000 + (0x404E - 0x4000) * 2, 0)
            changes.append("firered-only-defaults:national-dex-sevii-celio-vs-seeker-locked")

    def _game_stats(self, sb2, sb1, policy, changes):
        if not policy:
            return
        if policy.get("policy") != "explicit-zero-default":
            raise ValueError("unsupported game-stat policy")
        key = int.from_bytes(sb2[0xF20:0xF24], "little")
        values = [0] * 64
        for raw_name, raw_value in policy.get("values", {}).items():
            stat_id = GAME_STAT_IDS.get(raw_name)
            if stat_id is None:
                try:
                    stat_id = int(raw_name)
                except ValueError as exc:
                    raise ValueError(f"unknown game stat: {raw_name}") from exc
            if not 0 <= stat_id < 64:
                raise ValueError(f"game stat outside 0..63: {stat_id}")
            values[stat_id] = int(raw_value) & 0xFFFFFFFF
        for stat_id, value in enumerate(values):
            put_u32(sb1, 0x1200 + stat_id * 4, value ^ key)
        changes.append(f"game-stats:explicit={len(policy.get('values', {}))},unsupported-zeroed")

    @staticmethod
    def _write_hof_sector(image, sector_id, data):
        if len(data) != 0xF80:
            raise ValueError("Hall of Fame sector payload must be 0xF80 bytes")
        offset = sector_id * SECTOR_SIZE
        image[offset:offset + SECTOR_SIZE] = bytes(SECTOR_SIZE)
        image[offset:offset + 0xF80] = data
        put_u16(image, offset + 0xFF4, section_checksum(data))
        put_u32(image, offset + 0xFF8, SIGNATURE)
        put_u32(image, offset + 0xFFC, 0)

    def _hall_of_fame(self, image, policy, party, changes):
        if not policy or policy.get("policy") != "single-team-from-party":
            return False
        if not party:
            raise ValueError("Hall of Fame generation requires a non-empty party")
        payload = bytearray(0xF80 * 2)
        for index, mon in enumerate(party[:6]):
            offset = index * 20
            put_u32(payload, offset, mon["otId"])
            put_u32(payload, offset + 4, mon["personality"])
            packed = (int(mon["species"]["internalId"]) & 0x1FF) | ((int(mon["level"]) & 0x7F) << 9)
            put_u16(payload, offset + 8, packed)
            payload[offset + 10:offset + 20] = encode_name(mon["nickname"], 10, allow_full=True)
        self._write_hof_sector(image, 28, payload[:0xF80])
        self._write_hof_sector(image, 29, payload[0xF80:])
        changes.append(f"hall-of-fame:single-team,party={min(len(party), 6)}")
        return True

    @staticmethod
    def _flag(sb1, flag_id, value=True):
        offset = 0xEE0 + int(flag_id) // 8
        mask = 1 << (int(flag_id) % 8)
        if value: sb1[offset] |= mask
        else: sb1[offset] &= ~mask & 0xFF

    def _reset_bridge_state(self, sb1):
        for flag_id in range(0x500, 0x500 + 743): self._flag(sb1, flag_id, False)
        for flag_id in range(0x820, 0x828): self._flag(sb1, flag_id, False)
        for event in self.events.values():
            for evidence in event.get("fireRedEvidence", []):
                symbol, raw_id = evidence.get("symbol", ""), evidence.get("id")
                if not raw_id: continue
                value = int(raw_id, 16) if isinstance(raw_id, str) else int(raw_id)
                if symbol.startswith("FLAG_") and value < 0x900: self._flag(sb1, value, False)
                elif symbol.startswith("VAR_") and 0x4000 <= value < 0x4100: put_u16(sb1, 0x1000 + (value - 0x4000) * 2, 0)

    def _badges(self, sb1, badges, changes):
        for badge in badges: self._flag(sb1, 0x820 + int(badge["number"]) - 1, bool(badge["obtained"]))
        changes.append("badge-flags")

    def _trainers(self, sb1, state, changes):
        for raw in state["defeatedFlagHex"]: self._flag(sb1, int(raw, 16), True)
        changes.append(f"ordinary-trainer-defeat-flags:{len(state['defeatedFlagHex'])}")

    def _party_menu_unlock(self, sb1, party, changes):
        # FLAG_SYS_POKEMON_GET controls the pause-menu Pokémon entry. It is
        # independent of which starter was chosen and is required whenever a
        # generated save contains a usable party. Apply it after event reset so
        # an unknown Gen I starter choice cannot accidentally suppress the UI.
        self._flag(sb1, 0x828, bool(party))
        changes.append(f"pokemon-menu-unlock:party-present={bool(party)}")

    def _fly_destinations(self, sb1, policy, changes):
        if not policy:
            return
        if policy.get("policy") != "translate-red-visited-towns":
            raise ValueError("unsupported Fly-destination policy")
        # Never inherit destination state from the template. Clear all Kanto
        # and Sevii destinations, then set only the direct Red counterparts
        # whose source visited-town bits were true.
        for flag_id in KANTO_FLY_DESTINATION_FLAGS:
            self._flag(sb1, flag_id, False)
        for flag_id in range(0x89B, 0x8A2):
            self._flag(sb1, flag_id, False)
        allowed = set(RED_EQUIVALENT_FLY_DESTINATION_FLAGS)
        visited = []
        for row in policy.get("destinations", []):
            flag_id = int(row["fireRedFlagHex"], 16)
            if flag_id not in allowed:
                raise ValueError(f"Fly bridge attempted non-Red counterpart flag: {flag_id:#x}")
            if row.get("visited"):
                self._flag(sb1, flag_id, True)
                visited.append(flag_id)
        changes.append(
            f"fly-destinations:red-visited-town-translation:{len(visited)}-of-11;"
            "route4-route10-locked;sevii-locked"
        )

    def _events(self, sb1, planned_events, warnings, changes):
        # Standalone generation receives externally authored plans as well as
        # planner output. Apply the same fixed-point prerequisite closure here
        # so a later completed event cannot coexist with an open earlier scene.
        planned_by_id = {row["semanticId"]: row for row in planned_events}
        active = {
            row["semanticId"] for row in planned_events
            if row.get("sourceState") is True
        }
        changed = True
        while changed:
            changed = False
            for implication in self.event_implications:
                if implication["when"] not in active:
                    continue
                for required_id in implication.get("fireRedPrerequisites", []):
                    if required_id not in planned_by_id:
                        raise ValueError(
                            f"story closure requires missing semantic event: {required_id}"
                        )
                    if required_id not in active:
                        active.add(required_id)
                        changed = True
        closure_count = sum(
            planned_by_id[event_id].get("sourceState") is not True
            for event_id in active
        )
        # Event bridge rows are organized for research, not guaranteed to be
        # chronological. Apply executable rules in the frozen rule authority's
        # insertion order so successor scenes overwrite predecessor values
        # (for example Route 22 early=2, final=4).
        rule_order = {semantic_id: index for index, semantic_id in enumerate(self.event_rules)}
        ordered_events = sorted(
            planned_events,
            key=lambda row: rule_order.get(row["semanticId"], len(rule_order)),
        )
        applied = 0
        for planned in ordered_events:
            if planned["semanticId"] not in active: continue
            authority = self.events.get(planned["semanticId"])
            if not authority: continue
            rule = self.event_rules.get(planned["semanticId"])
            skip_generic_flags = False
            skip_generic_variables = False
            ruled_variables = set()
            if rule:
                selected = rule
                skip_generic_flags = bool(rule.get("authoritativeFlags"))
                skip_generic_variables = bool(rule.get("authoritativeVariables"))
                if rule.get("requiresSemanticValue"):
                    semantic_value = planned.get("semanticValue")
                    selected = rule.get("choices", {}).get(str(semantic_value).lower())
                    if selected is None:
                        warnings.append(
                            f"{planned['semanticId']}: explicit semanticValue required; "
                            f"allowed values are {sorted(rule.get('choices', {}))}"
                        )
                        continue
                    skip_generic_flags = True
                    skip_generic_variables = True
                for raw_id, variable_value in selected.get("variables", {}).items():
                    variable_id = int(raw_id, 16)
                    put_u16(sb1, 0x1000 + (variable_id - 0x4000) * 2, int(variable_value))
                    ruled_variables.add(variable_id)
                    applied += 1
                for raw_id in selected.get("setFlags", []):
                    self._flag(sb1, int(raw_id, 16), True)
                    applied += 1
                for raw_id in selected.get("clearFlags", []):
                    self._flag(sb1, int(raw_id, 16), False)
                    applied += 1
            for evidence in authority.get("fireRedEvidence", []):
                symbol, raw_id = evidence.get("symbol", ""), evidence.get("id")
                if not raw_id: continue
                value = int(raw_id, 16) if isinstance(raw_id, str) else int(raw_id)
                accesses = {row.get("access") for row in evidence.get("references", [])}
                if symbol.startswith("FLAG_") and value < 0x900:
                    if skip_generic_flags:
                        continue
                    if "write-set" in accesses and "write-clear" not in accesses:
                        self._flag(sb1, value, True); applied += 1
                    elif "write-clear" in accesses and "write-set" not in accesses:
                        self._flag(sb1, value, False); applied += 1
                    else:
                        warnings.append(f"{planned['semanticId']}: flag polarity unresolved for {symbol}")
                elif symbol.startswith("VAR_"):
                    if value not in ruled_variables and not skip_generic_variables:
                        warnings.append(f"{planned['semanticId']}: variable {symbol} requires an explicit event-state rule")
        changes.append(f"semantic-event-flag-actions:{applied}")
        changes.append(f"semantic-prerequisite-closure:{closure_count}")
