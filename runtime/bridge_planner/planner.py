from dataclasses import dataclass
from pathlib import Path

from .pokemon import PokemonConverter, is_target_shiny, pid_gender
from .policy import OriginalV1Policy, ROOT
from .util import clamp, deterministic_u16, load_json, sha256_json, value_at
from .validation import iter_source_pokemon, validate_red_document


BRIDGE_VERSION = "1.0.0"
MANIFEST_VERSION = "1.0.0"
PROPOSED_FRED_VERSION = "1.0.0"
TARGET_MASTER_SCHEMA_VERSION = "0.4.0"


# pokered's wTownVisited order at SRAM 0x29B7-0x29B8.  The first eleven
# pokefirered FLAG_WORLD_MAP_* constants intentionally use the same semantic
# city order, even though their physical encoding is entirely different.
RED_TO_FIRERED_FLY_DESTINATIONS = (
    (0, "Pallet Town", 0x890),
    (1, "Viridian City", 0x891),
    (2, "Pewter City", 0x892),
    (3, "Cerulean City", 0x893),
    (4, "Lavender Town", 0x894),
    (5, "Vermilion City", 0x895),
    (6, "Celadon City", 0x896),
    (7, "Fuchsia City", 0x897),
    (8, "Cinnabar Island", 0x898),
    (9, "Indigo Plateau", 0x899),
    (10, "Saffron City", 0x89A),
)


@dataclass
class PlanResult:
    manifest: dict
    proposed_fred: dict
    preview_markdown: str


class BridgePlanner:
    def __init__(self, root=None, salt=None):
        self.root = Path(root or ROOT)
        self.policy = OriginalV1Policy.load(salt=salt, path=self.root / "data/pokemon_policy_original_v1.json")
        self.metadata = load_json(self.root / "data/original_v1_conversion_metadata.json")
        self.events = load_json(self.root / "data/event_bridge_red_to_firered.json")
        self.event_rules = load_json(self.root / "data/firered_event_generator_rules.json")
        self.trainers = load_json(self.root / "data/trainer_bridge_red_to_firered.json")
        self.items = load_json(self.root / "data/item_bridge_red_to_firered.json")
        self.target_trainers = load_json(self.root / "data/firered_trainer_target_policy.json")
        self.domains = load_json(self.root / "data/whole_save_bridge_domains.json")

    def plan(self, source, source_sha256=None, source_name=None):
        input_warnings = validate_red_document(source)
        source_fingerprint = sha256_json(source)
        source_sha256 = source_sha256 or source_fingerprint
        decoded = source["decoded"]
        source_schema = source["schema"]
        player_name = str(value_at(decoded, "trainer", "name"))
        rival_name = str(value_at(decoded, "rival", "name", default="BLUE"))
        public_tid = int(value_at(decoded, "trainer", "trainerId"))
        player_sid = deterministic_u16(self.policy.salt, source_fingerprint, "player-sid", player_name.upper(), public_tid)

        pokemon_converter = PokemonConverter(
            self.metadata, self.policy, source_fingerprint, player_name, public_tid, player_sid
        )
        converted = []
        pokemon_audits = []
        for locator, target_locator, mon in iter_source_pokemon(source):
            target, audit = pokemon_converter.convert(mon, locator, target_locator)
            converted.append((target_locator, target))
            pokemon_audits.append(audit)

        party, storage, daycare = self._place_pokemon(converted)
        badges = self._badges(decoded)
        pokedex = self._pokedex(decoded, converted)
        inventory, item_decisions, item_warnings, omissions = self._inventory(decoded)
        semantic_events, event_decisions = self._semantic_events(decoded, badges, inventory)
        trainer_state, trainer_decisions = self._trainer_state(decoded)
        fly_destinations, fly_decision, fly_warning = self._fly_destinations(decoded)
        invariants, rejected = self._invariants(
            decoded, badges, semantic_events, converted, trainer_state, pokemon_converter
        )
        options = self._options(decoded)
        source_location = decoded.get("location", {})

        warnings = list(input_warnings) + item_warnings
        if fly_warning:
            warnings.append(fly_warning)
        warnings.extend(warning for audit in pokemon_audits for warning in audit["warnings"])
        missing_choice_values = [
            row["semanticId"] for row in semantic_events
            if row["sourceState"] is True and row.get("semanticValue") == "unknown"
        ]
        if missing_choice_values:
            warnings.append(
                "Explicit conversion policy is required for semantic choices not retained by the "
                f"current Red JSON view: {', '.join(missing_choice_values)}."
            )
        unknown_events = sum(row["sourceState"] == "unknown" for row in semantic_events)
        if unknown_events:
            warnings.append(f"{unknown_events} semantic events lack sufficient decoded Red evidence and remain unknown/defaulted.")
        planning_status = "REJECTED" if rejected else ("READY_WITH_WARNINGS" if warnings else "READY")

        identity = {
            "playerName": player_name[:7],
            "rivalName": rival_name[:7],
            "gender": "male",
            "publicTrainerId": public_tid,
            "secretTrainerId": player_sid,
            "genderPolicy": "safe-default-because-Generation-I-has-no-player-gender",
        }
        playtime = {
            "hours": clamp(decoded.get("playtime", {}).get("hours", 0), 0, 999),
            "minutes": clamp(decoded.get("playtime", {}).get("minutes", 0), 0, 59),
            "seconds": clamp(decoded.get("playtime", {}).get("seconds", 0), 0, 59),
            "vblanks": 0,
        }
        currency = {
            "money": clamp(value_at(decoded, "moneyAndCoins", "money", default=0), 0, 999999),
            "coins": clamp(value_at(decoded, "moneyAndCoins", "coins", default=0), 0, 9999),
        }
        location = {
            "policy": "INHERIT_TEMPLATEFR_BEDROOM",
            "mapName": "Pallet Town - Player's House 2F",
            "mapGroup": 4,
            "mapNumber": 1,
            "warpId": -1,
            "x": 6,
            "y": 6,
            "mapLayoutId": 2,
            "sourceLocationRetainedForAudit": source_location,
            "requiresGeneratorMapIdResolution": False,
        }

        proposed = {
            "format": "pkmn-firered-planned-save",
            "schemaVersion": PROPOSED_FRED_VERSION,
            "targetMasterSchemaVersion": TARGET_MASTER_SCHEMA_VERSION,
            "planningStatus": planning_status,
            "generatorReady": planning_status != "REJECTED",
            "doesNotContainPhysicalSaveBytes": True,
            "authority": {
                "bridgeVersion": BRIDGE_VERSION,
                "pokemonPolicy": self.policy.profile_label,
                "sourceFingerprint": source_fingerprint,
            },
            "source": {
                "format": source_schema["format"],
                "schemaVersion": source_schema["schemaVersion"],
                "sha256": source_sha256,
                "fileName": source_name,
            },
            "target": {
                "game": "Pokemon FireRed",
                "revision": "1.0-policy-default",
                "template": None,
            },
            "semantic": {
                "trainer": identity,
                "playtime": playtime,
                "options": options,
                "currency": currency,
                "location": location,
                "badges": badges,
                "pokedex": pokedex,
                "party": party,
                "storage": storage,
                "daycare": daycare,
                "inventory": inventory,
                "progression": semantic_events,
                "trainerBattles": trainer_state,
                "flyDestinations": fly_destinations,
                "hallOfFame": {"policy": "derive-minimal-from-semantic-completion-or-reset", "records": []},
                "fireRedOnly": {
                    "nationalDex": False,
                    "seviiProgression": "locked",
                    "vsSeekerRematches": "undefeated",
                    "celioNetworkMachine": "not_repaired",
                    "eventIslandEntitlements": [],
                },
            },
            "warnings": warnings,
            "rejectedInconsistencies": rejected,
        }

        decisions = event_decisions + trainer_decisions + item_decisions + [fly_decision]
        manifest = {
            "schemaVersion": MANIFEST_VERSION,
            "manifestType": "pkmn-red-to-firered-conversion-plan",
            "planningStatus": planning_status,
            "versions": {
                "bridgeSpecification": BRIDGE_VERSION,
                "pokemonPolicy": self.policy.profile_label,
                "manifestSchema": MANIFEST_VERSION,
                "proposedFredSchema": PROPOSED_FRED_VERSION,
                "targetMasterSchema": TARGET_MASTER_SCHEMA_VERSION,
            },
            "source": proposed["source"],
            "target": proposed["target"],
            "policies": {
                "pokemon": self.policy.raw,
                "storyBundle": "KANTO_EQUIVALENT_ONLY",
                "trainerConfidence": self.policy.raw["wholeSave"]["trainerConfidence"],
                "trainerAmbiguity": "DEFAULT_UNDEFEATED",
                "location": "INHERIT_TEMPLATEFR_BEDROOM",
                "randomSalt": self.policy.salt,
            },
            "domains": {
                "identity": identity,
                "playtime": playtime,
                "currency": currency,
                "options": options,
                "badges": badges,
                "pokedex": pokedex,
                "inventorySummary": {key: len(value) for key, value in inventory.items()},
                "progressionSummary": {
                    "true": sum(x["sourceState"] is True for x in semantic_events),
                    "false": sum(x["sourceState"] is False for x in semantic_events),
                    "unknown": unknown_events,
                },
                "trainerSummary": trainer_state["summary"],
                "flyDestinations": fly_destinations,
                "location": location,
                "fireRedOnly": proposed["semantic"]["fireRedOnly"],
            },
            "pokemonConversions": pokemon_audits,
            "audit": {
                "decisions": decisions,
                "warnings": warnings,
                "omissions": omissions,
                "invariantResults": invariants,
                "rejectedInconsistencies": rejected,
                "decisionCounts": self._decision_counts(decisions),
            },
            "output": {
                "proposedFredSha256": sha256_json(proposed),
                "writesSaveImage": False,
            },
        }
        preview = self._preview(manifest, proposed)
        return PlanResult(manifest=manifest, proposed_fred=proposed, preview_markdown=preview)

    def _fly_destinations(self, decoded):
        """Translate Red's saved visited-town bitset into FireRed world-map flags."""
        raw_hex = decoded.get("worldStateRaw", {}).get("visitedTownsHex")
        named = decoded.get("worldState", {}).get("visitedTowns", [])
        source = None
        visited_bits = set()
        warning = None

        if isinstance(named, list) and named:
            by_index = {
                int(row["index"]): bool(row.get("visited", row.get("isSet", False)))
                for row in named if "index" in row
            }
            visited_bits = {index for index, visited in by_index.items() if visited}
            source = "decoded.worldState.visitedTowns"
        elif isinstance(raw_hex, str):
            try:
                raw = bytes.fromhex(raw_hex)
            except ValueError:
                raw = b""
            if len(raw) == 2:
                visited_bits = {
                    bit for bit in range(11)
                    if raw[bit // 8] & (1 << (bit % 8))
                }
                source = "decoded.worldStateRaw.visitedTownsHex"

        if source is None:
            # Old/minimal red.json fixtures did not expose wTownVisited. Pallet
            # is the only safe generated default; no later city is inferred.
            visited_bits = {0}
            source = "safe-default"
            warning = (
                "Red visited-town state is unavailable; only Pallet Town is enabled as a "
                "conservative FireRed Fly destination."
            )

        destinations = [
            {
                "sourceBit": bit,
                "redName": name,
                "fireRedName": name,
                "fireRedFlagHex": f"0x{flag_id:03X}",
                "visited": bit in visited_bits,
            }
            for bit, name, flag_id in RED_TO_FIRERED_FLY_DESTINATIONS
        ]
        policy = {
            "policy": "translate-red-visited-towns",
            "sourceField": source,
            "sourceHex": raw_hex if source.endswith("visitedTownsHex") else None,
            "destinations": destinations,
            "visitedCount": sum(row["visited"] for row in destinations),
            "routePokemonCentersPolicy": "locked-no-red-counterpart",
            "seviiPolicy": "locked-until-firered-progression",
        }
        decision = {
            "domain": "flyDestinations",
            "source": source,
            "target": "FLAG_WORLD_MAP_PALLET_TOWN..FLAG_WORLD_MAP_SAFFRON_CITY",
            "action": "translate-visited-town-bits",
            "visitedCount": policy["visitedCount"],
            "extraFireRedDestinations": "default-locked",
        }
        return policy, decision, warning

    def _place_pokemon(self, converted):
        party = []
        boxes = [{"boxNumber": number + 1, "slots": []} for number in range(14)]
        daycare = {"route5": []}
        for locator, mon in converted:
            if locator.startswith("party/"):
                party.append(mon)
            elif locator.startswith("storage/"):
                parts = locator.split("/")
                box, slot = int(parts[2]), int(parts[4])
                boxes[box]["slots"].append({"slot": slot, "pokemon": mon})
            elif locator.startswith("daycare/"):
                daycare["route5"].append(mon)
        return party, {"boxes": boxes, "currentBox": 1}, daycare

    def _badges(self, decoded):
        badge_names = ["Boulder", "Cascade", "Thunder", "Rainbow", "Soul", "Marsh", "Volcano", "Earth"]
        rows = decoded.get("badges", {}).get("badges")
        if rows is None:
            rows = decoded.get("badges", {}).get("entries", [])
        return [
            {"number": int(row.get("index", index + 1)),
             "name": str(row.get("name") or badge_names[index]).replace(" Badge", ""),
             "obtained": bool(row.get("owned", False))}
            for index, row in enumerate(rows)
        ]

    def _pokedex(self, decoded, converted):
        dex = decoded.get("pokedex", {})
        species_rows = dex.get("species", [])
        owned = {int(row["nationalDexNumber"]) for row in species_rows if row.get("owned")}
        seen = {int(row["nationalDexNumber"]) for row in species_rows if row.get("seen")}
        owned.update(int(value) for value in dex.get("ownedDexNumbers", []) if 1 <= int(value) <= 151)
        seen.update(int(value) for value in dex.get("seenDexNumbers", []) if 1 <= int(value) <= 151)
        for field, target in (("ownedBitfieldHex", owned), ("seenBitfieldHex", seen)):
            raw = dex.get(field)
            if isinstance(raw, str):
                bits = bytes.fromhex(raw)
                target.update(
                    number for number in range(1, 152)
                    if bits[(number - 1) // 8] & (1 << ((number - 1) % 8))
                )
        for _, mon in converted:
            number = mon["species"]["nationalDex"]
            owned.add(number)
            seen.add(number)
        return {
            "ownedCount": len(owned), "seenCount": len(seen),
            "ownedNationalDexNumbers": sorted(owned), "seenNationalDexNumbers": sorted(seen),
            "nationalDexEnabled": False,
            "nationalDexPolicy": "not-inferred-from-Red",
        }

    def _inventory(self, decoded):
        by_id = {row["red"]["id"]: row for row in self.items["mappings"]}
        output = {"items": [], "keyItems": [], "pokeBalls": [], "tmCase": [], "pcItems": []}
        decisions, warnings, omissions = [], [], []
        containers = [
            ("bag", decoded.get("inventory", {}).get("bag", {}).get("items", [])),
            ("pcItems", decoded.get("inventory", {}).get("pcItemStorage",
                decoded.get("inventory", {}).get("pcItems", {})).get("items", [])),
        ]
        for source_container, entries in containers:
            for entry in entries:
                item = entry.get("item", {})
                source_id = int(item.get("id", entry.get("itemId", -1)))
                source_name = item.get("name", entry.get("itemName"))
                item = {"id": source_id, "name": source_name}
                mapping = by_id.get(source_id)
                locator = f"inventory/{source_container}/{entry.get('slot', 0)}"
                if not mapping or not mapping.get("fireRed"):
                    action = mapping["conversionAction"] if mapping else "omit-unknown-item"
                    omissions.append({"domain": "inventory", "sourceLocator": locator, "sourceItem": item, "action": action})
                    decisions.append({"domain": "item", "source": item.get("name"), "target": None, "action": action})
                    continue
                target_id = mapping["fireRed"]["id"]
                target_symbol = mapping["fireRed"]["symbol"]
                if 1 <= target_id <= 12:
                    pocket = "pokeBalls"
                elif 289 <= target_id <= 346:
                    pocket = "tmCase"
                elif target_id >= 259:
                    pocket = "keyItems"
                else:
                    # FireRed's normal Bag pocket is named ``items`` in the
                    # planned document; only the source PC container retains
                    # its distinct destination.
                    pocket = "pcItems" if source_container == "pcItems" else "items"
                quantity = clamp(entry.get("quantity", 1), 1, 999)
                if pocket == "keyItems":
                    quantity = 1
                output[pocket].append({"id": target_id, "symbol": target_symbol, "quantity": quantity,
                                       "sourceLocator": locator, "storyReconciliation": mapping["storyReconciliation"]})
                decisions.append({
                    "domain": "item", "source": mapping["red"]["symbol"], "target": target_symbol,
                    "action": mapping["conversionAction"], "classification": mapping["classification"],
                })
                if mapping["conversionAction"] == "translate-with-user-policy":
                    warnings.append(f"{mapping['red']['symbol']} used the documented ORIGINAL_V1 same-purpose item policy.")
        # Gen I may hold the same item in the Bag and PC. FireRed pockets use
        # one canonical stack per item ID, so merge only within each target pocket.
        for pocket, entries in output.items():
            merged = {}
            for entry in entries:
                item_id = int(entry["id"])
                if item_id not in merged:
                    merged[item_id] = entry
                else:
                    merged[item_id]["quantity"] = clamp(
                        merged[item_id]["quantity"] + entry["quantity"], 1,
                        1 if pocket == "keyItems" else 999,
                    )
                    merged[item_id]["sourceLocator"] += f"+{entry['sourceLocator']}"
            output[pocket] = list(merged.values())
        if output["tmCase"] and not any(x["symbol"] == "ITEM_TM_CASE" for x in output["keyItems"]):
            output["keyItems"].append({"id": 364, "symbol": "ITEM_TM_CASE", "quantity": 1,
                                       "sourceLocator": "generated-companion", "storyReconciliation": True})
            decisions.append({"domain": "item", "source": None, "target": "ITEM_TM_CASE", "action": "generate-required-container"})
        return output, decisions, warnings, omissions

    def _source_flag_index(self, decoded):
        index = {}
        for section, field in (("events", "flags"), ("storyProgress", "storyFlags"),
                               ("trainerBattles", "records"), ("staticBattles", "records")):
            for row in decoded.get(section, {}).get(field, []):
                value = row.get("completed", row.get("value"))
                if isinstance(value, bool):
                    index[row.get("name")] = value
        return index

    def _semantic_events(self, decoded, badges, inventory):
        index = self._source_flag_index(decoded)
        badge_semantics = {
            "BOULDER_BADGE_OBTAINED": 1,
        }
        rows, decisions = [], []
        for event in self.events["events"]:
            semantic_id = event["semanticId"]
            semantic_value = None
            evidence = []
            for source in event.get("redEvidence", []):
                symbol = source.get("symbol")
                if symbol in index:
                    evidence.append({"symbol": symbol, "value": index[symbol]})
            if semantic_id == "PLAYER_INITIALIZED":
                state = True
                evidence.append({"symbol": "decoded.trainer", "value": True})
            elif semantic_id == "STARTER_CHOSEN":
                got = value_at(decoded, "worldState", "storyEvidence", "gotStarter")
                state = got if isinstance(got, bool) else (True if index.get("EVENT_GOT_STARTER") else False if "EVENT_GOT_STARTER" in index else "unknown")
                if state is True:
                    candidate = value_at(decoded, "worldState", "storyEvidence", "starterChoice")
                    candidate = str(candidate).lower() if candidate is not None else "unknown"
                    semantic_value = candidate if candidate in {"bulbasaur", "squirtle", "charmander"} else "unknown"
            elif semantic_id == "FOSSIL_CHOSEN":
                dome = index.get("EVENT_GOT_DOME_FOSSIL") is True
                helix = index.get("EVENT_GOT_HELIX_FOSSIL") is True
                state = True if dome or helix else False if (
                    "EVENT_GOT_DOME_FOSSIL" in index and "EVENT_GOT_HELIX_FOSSIL" in index
                ) else "unknown"
                semantic_value = "dome" if dome and not helix else "helix" if helix and not dome else "unknown" if state is True else None
            elif semantic_id == "FIGHTING_DOJO_GIFT_CHOSEN":
                lee = index.get("EVENT_GOT_HITMONLEE") is True
                chan = index.get("EVENT_GOT_HITMONCHAN") is True
                state = True if lee or chan else False if (
                    "EVENT_GOT_HITMONLEE" in index and "EVENT_GOT_HITMONCHAN" in index
                ) else "unknown"
                semantic_value = "hitmonlee" if lee and not chan else "hitmonchan" if chan and not lee else "unknown" if state is True else None
            elif semantic_id == "RODS_RECEIVED":
                symbols = {
                    row.get("symbol")
                    for pocket in inventory.values()
                    for row in pocket
                }
                rods = [
                    name for name, symbol in (
                        ("old", "ITEM_OLD_ROD"),
                        ("good", "ITEM_GOOD_ROD"),
                        ("super", "ITEM_SUPER_ROD"),
                    ) if symbol in symbols
                ]
                state = bool(rods)
                semantic_value = "+".join(rods) if rods else None
            elif semantic_id == "RUNNING_SHOES_RECEIVED":
                if "EVENT_BEAT_BROCK" in index:
                    state = index["EVENT_BEAT_BROCK"] is True
                    evidence.append({
                        "symbol": "EVENT_BEAT_BROCK",
                        "value": index["EVENT_BEAT_BROCK"],
                        "treatment": "FireRed-only target companion",
                    })
                else:
                    state = False
            elif semantic_id in {"CHAMPION_DEFEATED", "HALL_OF_FAME_FIRST_ENTRY"}:
                record_count = int(decoded.get("hallOfFame", {}).get("recordCount", 0) or 0)
                if record_count:
                    state = True
                    evidence.append({
                        "symbol": "decoded.hallOfFame.recordCount",
                        "value": record_count,
                        "treatment": "persistent proof that the Champion and Hall of Fame were completed",
                    })
                elif evidence:
                    state = any(x["value"] is True for x in evidence)
                else:
                    state = "unknown"
            elif semantic_id in badge_semantics:
                number = badge_semantics[semantic_id]
                state = next((x["obtained"] for x in badges if x["number"] == number), "unknown")
            elif event["equivalenceClass"] == "FIRERED_ONLY_EXTENSION":
                state = False
            elif any(x["value"] is True for x in evidence):
                state = True
            elif evidence and all(x["value"] is False for x in evidence):
                state = False
            else:
                state = "unknown"
            target_evidence = [
                {"symbol": x.get("symbol"), "id": x.get("id")}
                for x in event.get("fireRedEvidence", []) if x.get("symbol")
            ]
            row = {
                "semanticId": semantic_id, "name": event["name"], "sourceState": state,
                "equivalenceClass": event["equivalenceClass"], "conversionAction": event["conversionAction"],
                "sourceEvidence": evidence, "targetCompanionEvidence": target_evidence,
                "generatorTreatment": event["generatorTreatment"], "invariants": event["invariants"],
            }
            if semantic_value is not None:
                row["semanticValue"] = semantic_value
            rows.append(row)
            decisions.append({
                "domain": "story", "source": semantic_id, "target": semantic_id,
                "sourceState": state, "action": event["conversionAction"],
                "equivalence": event["equivalenceClass"], "companionState": event["companionState"],
            })
        # Later milestones are proof that their normal-play prerequisites were
        # completed even when Gen I represents an exchanged/consumed item or
        # the canonical Red view cannot expose a friendly semantic field. Close
        # those prerequisites before generation so the target cannot claim a
        # completed headline while replaying an earlier map scene.
        by_id = {row["semanticId"]: row for row in rows}
        changed = True
        while changed:
            changed = False
            for implication in self.event_rules.get("implications", []):
                parent = by_id.get(implication["when"])
                if not parent or parent.get("sourceState") is not True:
                    continue
                for required_id in implication.get("redPrerequisites", []):
                    required = by_id.get(required_id)
                    if not required or required.get("sourceState") is True:
                        continue
                    previous = required.get("sourceState")
                    required["sourceState"] = True
                    required.setdefault("sourceEvidence", []).append({
                        "symbol": implication["when"],
                        "value": True,
                        "treatment": "derived Red prerequisite closure",
                        "previousState": previous,
                    })
                    decisions.append({
                        "domain": "storyInvariant",
                        "source": implication["when"],
                        "target": required_id,
                        "sourceState": True,
                        "action": "derive-required-prerequisite",
                    })
                    changed = True
        final_states = {row["semanticId"]: row["sourceState"] for row in rows}
        for decision in decisions:
            if decision.get("domain") == "story" and decision.get("source") in final_states:
                decision["sourceState"] = final_states[decision["source"]]
        return rows, decisions

    def _trainer_state(self, decoded):
        defeated = {
            row.get("name") for row in decoded.get("trainerBattles", {}).get("records", []) if row.get("completed") is True
        }
        target_ids, target_flags, decisions = [], [], []
        counts = {"sourceDefeated": len(defeated), "transferred": 0, "storyManaged": 0, "defaultedUndefeated": 0}
        for row in self.trainers["mappings"]:
            source_name = row["red"]["pretEventName"]
            source_state = source_name in defeated
            target = row.get("fireRed")
            classification = row["classification"]
            if source_state and target and classification in {"CONFIRMED_REMAKE_COUNTERPART", "STRONGLY_INFERRED_COUNTERPART"}:
                target_ids.append(target["trainerId"])
                target_flags.append(target["defeatFlagHex"])
                action = "transfer-defeated-state"
                counts["transferred"] += 1
            elif classification == "STORY_EVENT_MANAGED_SEPARATELY":
                action = "derive-from-semantic-story-state"
                counts["storyManaged"] += 1
            else:
                action = "default-undefeated"
                counts["defaultedUndefeated"] += 1
            decisions.append({
                "domain": "trainerBattle", "source": source_name,
                "sourceDefeated": source_state,
                "target": target["pretTrainerName"] if target else None,
                "targetTrainerId": target["trainerId"] if target else None,
                "action": action, "classification": classification, "confidence": row["certainty"],
            })
        return {
            "defeatedTrainerIds": sorted(target_ids), "defeatedFlagHex": sorted(target_flags),
            "allOtherTrainerTargets": "undefeated", "vsSeekerRematches": "undefeated",
            "summary": counts,
        }, decisions

    def _options(self, decoded):
        raw = int(value_at(decoded, "options", "optionsByte", default=None)
                  if value_at(decoded, "options", "optionsByte", default=None) is not None
                  else decoded.get("options", {}).get("raw", 0))
        speed = raw & 7
        return {
            "textSpeed": 2 if speed <= 1 else 1 if speed <= 3 else 0,
            "battleSceneOff": bool(raw & 0x80),
            "battleStyle": "set" if raw & 0x40 else "shift",
            "sound": "stereo" if raw & 0x10 else "mono",
            "buttonMode": 0,
            "windowFrameType": 0,
            "sourceRaw": raw,
        }

    def _invariants(self, decoded, badges, semantic_events, converted, trainer_state, converter):
        results, rejected = [], []
        checks = [
            ("party-capacity", len(decoded.get("party", {}).get("pokemon", [])) <= 6),
            ("storage-capacity", sum(1 for locator, _ in converted if locator.startswith("storage/")) <= 420),
            ("target-trainer-uniqueness", len(trainer_state["defeatedTrainerIds"]) == len(set(trainer_state["defeatedTrainerIds"]))),
            ("pokedex-pokemon-union", True),
        ]
        for name, passed in checks:
            results.append({"id": name, "status": "PASS" if passed else "FAIL"})
            if not passed:
                rejected.append({"invariant": name, "reason": "Generated target violates a hard capacity or uniqueness rule."})
        event_by_id = {row["semanticId"]: row for row in semantic_events}
        gym_pairs = [
            (1, "BROCK_DEFEATED"), (2, "MISTY_DEFEATED"), (3, "LT_SURGE_DEFEATED"),
            (4, "ERIKA_DEFEATED"), (5, "KOGA_DEFEATED"), (6, "SABRINA_DEFEATED"),
            (7, "BLAINE_DEFEATED"), (8, "GIOVANNI_GYM_DEFEATED"),
        ]
        for number, semantic_id in gym_pairs:
            badge = next((x["obtained"] for x in badges if x["number"] == number), False)
            leader = event_by_id.get(semantic_id, {}).get("sourceState", "unknown")
            status = "PASS" if leader == "unknown" or badge == leader else "FAIL"
            results.append({"id": f"gym-{number}-badge-leader-consistency", "status": status,
                            "badge": badge, "leader": leader})
            if status == "FAIL":
                rejected.append({"invariant": f"gym-{number}-badge-leader-consistency",
                                 "reason": "Red badge and leader evidence disagree; target companion state cannot be generated safely."})
        for locator, mon in converted:
            pid = mon["personality"]
            species = converter.species[str(mon["species"]["nationalDex"])]
            passed = (
                pid % 25 == mon["nature"]["id"]
                and pid_gender(species["genderRatio"], pid) == mon["gender"]
                and (pid & 1) == mon["abilitySlot"]
                and is_target_shiny(pid, mon["publicTrainerId"], mon["secretTrainerId"]) == mon["isShiny"]
                and sum(mon["evs"].values()) <= 510
            )
            results.append({"id": f"pokemon-{locator}-pid-ev-consistency", "status": "PASS" if passed else "FAIL"})
            if not passed:
                rejected.append({"invariant": f"pokemon-{locator}", "reason": "PID-derived fields or EV caps conflict."})
        return results, rejected

    def _decision_counts(self, decisions):
        counts = {}
        for row in decisions:
            action = row.get("action", "unspecified")
            counts[action] = counts.get(action, 0) + 1
        return dict(sorted(counts.items()))

    def _preview(self, manifest, proposed):
        domains = manifest["domains"]
        lines = [
            "# Pokémon Red → FireRed Conversion Preview", "",
            f"Status: **{manifest['planningStatus']}**", "",
            f"Policy: `{manifest['versions']['pokemonPolicy']}`  ",
            f"Source fingerprint: `{manifest['source']['sha256']}`  ",
            "Physical `.sav` written: **no**", "",
            "## Planned target", "",
            f"- Player: `{domains['identity']['playerName']}`",
            f"- Rival: `{domains['identity']['rivalName']}`",
            f"- Public/SID: `{domains['identity']['publicTrainerId']}` / `{domains['identity']['secretTrainerId']}`",
            f"- Party Pokémon: `{len(proposed['semantic']['party'])}`",
            f"- Stored Pokémon: `{sum(len(box['slots']) for box in proposed['semantic']['storage']['boxes'])}`",
            f"- Daycare Pokémon: `{len(proposed['semantic']['daycare']['route5'])}`",
            f"- Defeated ordinary trainer counterparts transferred: `{domains['trainerSummary']['transferred']}`",
            f"- FireRed-only progression: `locked/defaulted`", "",
            "## Audit", "",
            f"- Pokémon conversion records: `{len(manifest['pokemonConversions'])}`",
            f"- Decisions: `{len(manifest['audit']['decisions'])}`",
            f"- Warnings: `{len(manifest['audit']['warnings'])}`",
            f"- Omissions: `{len(manifest['audit']['omissions'])}`",
            f"- Rejected inconsistencies: `{len(manifest['audit']['rejectedInconsistencies'])}`", "",
        ]
        if manifest["audit"]["warnings"]:
            lines += ["## Warnings", ""] + [f"- {warning}" for warning in manifest["audit"]["warnings"]] + [""]
        if manifest["audit"]["rejectedInconsistencies"]:
            lines += ["## Blocking inconsistencies", ""] + [
                f"- `{row['invariant']}` — {row['reason']}" for row in manifest["audit"]["rejectedInconsistencies"]
            ] + [""]
        lines += [
            "## Boundary", "",
            "This is a deterministic semantic plan. It contains no FireRed sector layout, encrypted Pokémon bytes, checksums, save index, or physical template bytes. Those belong to the later FireRed Save Generator.", "",
        ]
        return "\n".join(lines)
