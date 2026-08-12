"""Phase 4 Red -> FireRed conversion orchestration."""

import copy
import hashlib
from dataclasses import dataclass

from bridge_planner import BridgePlanner
from bridge_planner.util import sha256_json

from .generator import FireRedTemplateGenerator, GenerationResult


@dataclass
class ConversionResult:
    source_red_json: dict
    proposed_fred: dict
    manifest: dict
    preview_markdown: str
    generation: GenerationResult


def without_physical_image(document):
    """Remove archival source bytes; semantic conversion must never use them."""
    cleaned = copy.deepcopy(document)
    cleaned.pop("physicalImage", None)
    return cleaned


def complete_generator_policy(proposed, source=None):
    """Attach the Phase 3 policies proven by MAQ's emulator acceptance."""
    plan = copy.deepcopy(proposed)
    semantic = plan["semantic"]
    semantic["hiddenItems"] = {"policy": "safe-default-uncollected"}
    # BridgePlanner derives this from Red's saved visited-town bitset. Never
    # replace it with a blanket unlock: Route 4/10 and Sevii have no direct Red
    # visited-town counterparts.
    if "flyDestinations" not in semantic:
        semantic["flyDestinations"] = {
            "policy": "translate-red-visited-towns",
            "sourceField": "safe-default",
            "destinations": [{
                "sourceBit": 0, "redName": "Pallet Town", "fireRedName": "Pallet Town",
                "fireRedFlagHex": "0x890", "visited": True,
            }],
            "visitedCount": 1,
            "routePokemonCentersPolicy": "locked-no-red-counterpart",
            "seviiPolicy": "locked-until-firered-progression",
        }
    semantic["fireRedOnlyDefaults"] = {
        "policy": "locked",
        "domains": [
            "national-dex", "sevii-islands", "celio-network-machine",
            "vs-seeker-rematches", "trainer-tower", "event-islands",
        ],
    }

    progression = {row["semanticId"]: row.get("sourceState") for row in semantic["progression"]}
    hall_complete = progression.get("HALL_OF_FAME_FIRST_ENTRY") is True
    champion_complete = progression.get("CHAMPION_DEFEATED") is True
    if hall_complete or champion_complete:
        if not semantic["party"]:
            raise ValueError("completed League state requires a non-empty party for Hall of Fame generation")
        play = semantic["playtime"]
        first_hof_time = (
            (int(play["hours"]) << 16)
            | (int(play["minutes"]) << 8)
            | int(play["seconds"])
        )
        semantic["hallOfFame"] = {
            "policy": "single-team-from-party",
            "records": [],
            "derivation": "single target record from the converted current party",
            "sourceRecordCount": int((source or {}).get("decoded", {}).get("hallOfFame", {}).get("recordCount", 0) or 0),
        }
        semantic["gameStats"] = {
            "policy": "explicit-zero-default",
            "values": {
                "FIRST_HOF_PLAY_TIME": first_hof_time,
                "ENTERED_HOF": 1,
                "RECEIVED_RIBBONS": 1,
            },
        }
        for mon in semantic["party"]:
            names = mon.setdefault("ribbons", {}).setdefault("names", [])
            if "Champion Ribbon" not in names:
                names.append("Champion Ribbon")
    else:
        semantic["hallOfFame"] = {"policy": "preserve-clean-template-empty", "records": []}
        semantic["gameStats"] = {"policy": "explicit-zero-default", "values": {}}
    return plan


def convert_red_json(root, red_document, template_bytes, source_name=None, source_sha256=None,
                     template_name=None, salt=None, generator=None):
    """Plan and physically generate one deterministic FireRed save."""
    source = without_physical_image(red_document)
    planned = BridgePlanner(root=root, salt=salt).plan(
        source, source_sha256=source_sha256, source_name=source_name
    )
    if planned.manifest["planningStatus"] == "REJECTED":
        reasons = "; ".join(
            row["reason"] for row in planned.manifest["audit"]["rejectedInconsistencies"]
        )
        raise ValueError(f"bridge plan rejected: {reasons}")

    proposed = complete_generator_policy(planned.proposed_fred, source)
    source_hof_count = int(source.get("decoded", {}).get("hallOfFame", {}).get("recordCount", 0) or 0)
    hof_warning = None
    if source_hof_count > 1:
        hof_warning = (
            f"Source Red contains {source_hof_count} Hall of Fame records; Generator v1 creates one "
            "FireRed Hall of Fame team from the converted current party and records this historical reduction."
        )
        proposed["warnings"].append(hof_warning)
    generation = (generator or FireRedTemplateGenerator.from_repository(root)).generate(
        proposed, template_bytes, template_name
    )
    manifest = copy.deepcopy(planned.manifest)
    manifest["manifestType"] = "pkmn-red-to-firered-completed-conversion"
    manifest["output"] = {
        "proposedFredSha256": sha256_json(proposed),
        "writesSaveImage": True,
        "generatedSaveSha256": hashlib.sha256(generation.image).hexdigest(),
        "generatedSaveSize": len(generation.image),
        "generationStatus": generation.report["status"],
        "sourcePhysicalImageUsed": False,
        "approvedCleanTemplateUsedAsContainer": True,
    }
    manifest["generator"] = {
        "kind": "template-backed-semantic-generator",
        "templateSha256": hashlib.sha256(template_bytes).hexdigest(),
        "templateContribution": "container-layout-and-safe-baseline-only",
    }
    if hof_warning:
        manifest["audit"]["warnings"].append(hof_warning)
        manifest["audit"]["decisions"].append({
            "domain": "hallOfFame", "sourceRecords": source_hof_count,
            "targetRecords": 1, "action": "derive-single-current-party-team",
        })
    preview = planned.preview_markdown.replace(
        "Physical `.sav` written: **no**", "Physical `.sav` written: **yes**"
    ).replace(
        "This is a deterministic semantic plan. It contains no FireRed sector layout, encrypted Pokémon bytes, checksums, save index, or physical template bytes. Those belong to the later FireRed Save Generator.",
        "This conversion was materialized by the emulator-proven FireRed generator. The Red source physical image was not used; an approved clean FireRed template supplied only the container baseline.",
    )
    return ConversionResult(source, proposed, manifest, preview, generation)
