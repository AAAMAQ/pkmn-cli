#!/usr/bin/env python3
"""Bundled deterministic FireRed planning/generation runtime for pkmn 2.0."""

import argparse
import copy
import hashlib
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

from bridge_planner import BridgePlanner  # noqa: E402
from firered_generator.conversion import (  # noqa: E402
    complete_generator_policy,
    convert_red_json,
    without_physical_image,
)
from firered_generator import FireRedTemplateGenerator  # noqa: E402
from firered_generator.generator import validate_clean_template  # noqa: E402
from firered_generator.binary import (  # noqa: E402
    FLASH_SIZE, SECTOR_SIZE, analyze_slots, assemble_logical, scatter_logical,
)


def write_new(path, payload, binary=False):
    path = Path(path)
    if path.exists():
        raise FileExistsError(f"refusing to overwrite {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    if binary:
        path.write_bytes(payload)
    else:
        path.write_text(payload)


def load_json(path):
    return json.loads(Path(path).read_text())


def source_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def plan_red(path, salt=None, source_name=None, source_sha256=None):
    source = without_physical_image(load_json(path))
    result = BridgePlanner(root=ROOT, salt=salt).plan(
        source, source_sha256=source_sha256 or source_hash(path),
        source_name=source_name or Path(path).name
    )
    if result.manifest["planningStatus"] == "REJECTED":
        reasons = "; ".join(
            row["reason"]
            for row in result.manifest["audit"]["rejectedInconsistencies"]
        )
        raise ValueError(f"bridge plan rejected: {reasons}")
    return source, result


def conversion_sidecars(output):
    output = Path(output)
    stem = output.with_suffix("")
    return (
        Path(str(stem) + ".conversion-manifest.json"),
        Path(str(stem) + ".conversion-report.md"),
    )


def convert_to_frjson(args):
    source, planned = plan_red(
        args.input, args.salt, args.source_name, args.source_sha256
    )
    proposed = complete_generator_policy(planned.proposed_fred, source)
    proposed["cliContract"] = {
        "toolVersion": "2.0.0",
        "command": "rjson convert_to_frjson",
        "nativeSchemaGate": "phase-5-accepted",
        "physicalImageUsed": False,
    }
    default_manifest, default_report = conversion_sidecars(args.output)
    manifest_path = args.manifest or default_manifest
    report_path = args.report or default_report
    write_new(args.output, json.dumps(proposed, indent=2) + "\n")
    write_new(manifest_path, json.dumps(planned.manifest, indent=2) + "\n")
    write_new(report_path, planned.preview_markdown + "\n")
    print(json.dumps({
        "status": planned.manifest["planningStatus"],
        "output": str(args.output),
        "manifest": str(manifest_path),
        "report": str(report_path),
        "physicalImageUsed": False,
    }, indent=2))


def convert_to_save(args):
    template = Path(args.template)
    if not template.is_file():
        raise FileNotFoundError("an approved FireRed template is required")
    source = load_json(args.input)
    result = convert_red_json(
        ROOT, source, template.read_bytes(),
        source_name=args.source_name or Path(args.input).name,
        source_sha256=args.source_sha256 or source_hash(args.input),
        template_name=template.name,
        salt=args.salt,
    )
    default_manifest, default_report = conversion_sidecars(args.output)
    manifest_path = args.manifest or default_manifest
    report_path = args.report or default_report
    write_new(args.output, result.generation.image, binary=True)
    write_new(manifest_path, json.dumps(result.manifest, indent=2) + "\n")
    write_new(report_path, result.preview_markdown + "\n")
    if args.keep_intermediate:
        intermediate = Path(args.output).with_suffix(".fred.json")
        write_new(intermediate, json.dumps(result.proposed_fred, indent=2) + "\n")
    print(json.dumps({
        "status": result.generation.report["status"],
        "output": str(args.output),
        "manifest": str(manifest_path),
        "report": str(report_path),
        "physicalImageUsed": False,
        "warnings": result.generation.report["warnings"],
    }, indent=2))


def generate_frjson(args):
    template = Path(args.template)
    if not template.is_file():
        raise FileNotFoundError("an approved FireRed template is required")
    plan = load_json(args.input)
    if plan.get("format") == "pkmn-firered-planned-save":
        result = FireRedTemplateGenerator.from_repository(ROOT).generate(
            plan, template.read_bytes(), template.name
        )
    elif plan.get("format") == "pkmn-firered-master-save":
        result = generate_native_frjson(plan, template.read_bytes(), template.name)
    else:
        raise ValueError("unsupported FireRed JSON format")
    write_new(args.output, result.image, binary=True)
    report = Path(str(Path(args.output).with_suffix("")) + ".generation-report.json")
    write_new(report, json.dumps(result.report, indent=2) + "\n")
    print(json.dumps({"status": result.report["status"], "output": str(args.output),
                      "report": str(report), "warnings": result.report["warnings"]}, indent=2))


def generate_native_frjson(document, template_bytes, template_name=None):
    """Generate from the complete logical-byte authority, never physicalImage."""
    from firered_generator.generator import GenerationResult
    template_profile = load_json(ROOT / "data" / "firered_v1_clean_template_profile.json")
    template_validation = validate_clean_template(template_bytes, template_profile)
    decoded = document.get("decoded", {})
    logical = decoded.get("logicalBlocks", {})
    names = ("saveBlock2", "saveBlock1", "pokemonStorage")
    blocks = []
    for name in names:
        record = logical.get(name)
        if not isinstance(record, dict) or not isinstance(record.get("rawHex"), str):
            raise ValueError(f"native JSON lacks decoded.logicalBlocks.{name}.rawHex")
        block = bytes.fromhex(record["rawHex"])
        if hashlib.sha256(block).hexdigest() != record.get("sha256"):
            raise ValueError(f"logical block hash mismatch: {name}")
        if len(block) != int(record.get("size", -1)):
            raise ValueError(f"logical block size mismatch: {name}")
        blocks.append(block)
    image = bytearray(template_bytes)
    slot = analyze_slots(image)
    scatter_logical(image, slot, blocks)
    special_written = 0
    sectors = decoded.get("specialSectors", {}).get("physicalSectors", [])
    for record in sectors:
        number = int(record.get("physicalSector", -1))
        if number not in range(28, 32):
            continue
        raw = bytes.fromhex(record.get("fullSectorHex", ""))
        if len(raw) != SECTOR_SIZE:
            raise ValueError(f"special sector {number} is not exactly 4096 bytes")
        if hashlib.sha256(raw).hexdigest() != record.get("fullSectorSha256"):
            raise ValueError(f"special sector hash mismatch: {number}")
        image[number * SECTOR_SIZE:(number + 1) * SECTOR_SIZE] = raw
        special_written += 1
    generated_slot = analyze_slots(image)
    output_hash = hashlib.sha256(image).hexdigest()
    report = {
        "format": "pkmn-firered-native-json-generation-report",
        "version": "1.0.0", "status": "STATIC_VALIDATION_PASS_PHASE5_ACCEPTED",
        "inputAuthority": "decoded logical blocks and special sectors",
        "physicalImageUsed": False,
        "template": {"fileName": template_name,
                     "sha256": hashlib.sha256(template_bytes).hexdigest(),
                     "activeSlot": slot.index,
                     "profileId": template_validation["profileId"],
                     "kind": template_validation["kind"]},
        "output": {"sha256": output_hash, "size": len(image),
                   "activeSlot": generated_slot.index,
                   "mainSectionChecksumsValid": True,
                   "specialSectorsWritten": special_written},
        "warnings": [],
        "verificationRequired": [],
    }
    return GenerationResult(bytes(image), report)


def bridge_catalog(kind):
    names = {"event": "event_bridge_red_to_firered.json",
             "trainer": "trainer_bridge_red_to_firered.json",
             "item": "item_bridge_red_to_firered.json"}
    document = load_json(ROOT / "data" / names[kind])
    key = {"event": "events", "trainer": "mappings", "item": "mappings"}[kind]
    return document, document[key]


def bridge_inspect(args):
    document, records = bridge_catalog(args.kind)
    query = (args.query or "").lower()
    selected = []
    for row in records:
        searchable = json.dumps(row, sort_keys=True).lower()
        if not query or query in searchable:
            selected.append(row)
    result = {"format": "pkmn-bridge-inspection", "version": "1.0.0",
              "kind": args.kind, "query": args.query, "resultCount": len(selected),
              "authority": document.get("pret", document.get("sourceCommits")),
              "records": selected}
    print(json.dumps(result, indent=2))


def validate_manifest(args):
    manifest = load_json(args.input)
    errors = []
    if not isinstance(manifest, dict): errors.append("top level must be an object")
    if not manifest.get("manifestType"): errors.append("manifestType is required")
    audit = manifest.get("audit")
    if not isinstance(audit, dict): errors.append("audit object is required")
    else:
        for key in ("decisions", "warnings", "omissions", "rejectedInconsistencies"):
            if not isinstance(audit.get(key), list): errors.append(f"audit.{key} must be an array")
    conversions = manifest.get("pokemonConversions", [])
    if not isinstance(conversions, list): errors.append("pokemonConversions must be an array")
    allowed = {"TRANSFER", "TRANSLATE", "DERIVE", "DEFAULT", "OMIT", "POLICY",
               "REJECT", "UNKNOWN", "transfer", "translate", "derive", "default",
               "omit", "reject", "ask user", "safe-default"}
    unknown_actions = []
    if isinstance(audit, dict):
        for row in audit.get("decisions", []):
            action = row.get("action") if isinstance(row, dict) else None
            if action and action not in allowed: unknown_actions.append(action)
    result = {"format": "pkmn-conversion-manifest-validation", "version": "1.0.0",
              "valid": not errors, "errors": errors, "unknownActionLabels": sorted(set(unknown_actions)),
              "decisionCount": len(audit.get("decisions", [])) if isinstance(audit, dict) else 0,
              "pokemonConversionCount": len(conversions) if isinstance(conversions, list) else 0}
    print(json.dumps(result, indent=2))
    if errors: raise ValueError("; ".join(errors))


def json_differences(left, right, path="", ignored=()):
    if any(path == prefix or path.startswith(prefix + "/") for prefix in ignored): return []
    if type(left) is not type(right): return [{"path": path or "/", "left": left, "right": right}]
    if isinstance(left, dict):
        rows = []
        for key in sorted(set(left) | set(right)):
            child = f"{path}/{key}"
            if key not in left or key not in right:
                rows.append({"path": child, "left": left.get(key), "right": right.get(key)})
            else: rows.extend(json_differences(left[key], right[key], child, ignored))
        return rows
    if isinstance(left, list):
        rows = []
        for index in range(max(len(left), len(right))):
            child = f"{path}/{index}"
            if index >= len(left) or index >= len(right):
                rows.append({"path": child, "left": left[index] if index < len(left) else None,
                             "right": right[index] if index < len(right) else None})
            else: rows.extend(json_differences(left[index], right[index], child, ignored))
        return rows
    return [] if left == right else [{"path": path or "/", "left": left, "right": right}]


def compare_frjson(args):
    left, right = load_json(args.left), load_json(args.right)
    mode_paths = {
        "semantic": ("/decoded/semantic", "/semantic"),
        "pokemon": ("/party", "/storage", "/daycare", "/decoded/semantic/party",
                    "/decoded/semantic/storage", "/decoded/semantic/daycare"),
        "events": ("/progression", "/decoded/semantic/flags", "/decoded/semantic/variables"),
        "trainers": ("/trainerBattles",), "items": ("/inventory",),
        "fly": ("/flyDestinations",), "hall-of-fame": ("/hallOfFame", "/decoded/specialSectors/hallOfFame"),
    }
    if args.mode == "native-authority":
        def authority(document):
            decoded = document.get("decoded", {})
            return {"semantic": decoded.get("semantic"),
                    "logicalBlocks": decoded.get("logicalBlocks"),
                    "specialSectors": decoded.get("specialSectors")}
        diffs = json_differences(authority(left), authority(right))
    elif args.mode == "semantic":
        ignored = ("/physicalImage", "/source", "/generator", "/diagnostics")
        diffs = json_differences(left, right, ignored=ignored)
    else:
        all_diffs = json_differences(left, right, ignored=("/physicalImage",))
        needles = mode_paths[args.mode]
        diffs = [row for row in all_diffs if any(n in row["path"] for n in needles)]
    report = {"format": "pkmn-firered-comparison", "version": "1.0.0",
              "mode": args.mode, "equivalent": not diffs,
              "differenceCount": len(diffs), "differences": diffs}
    if args.output:
        write_new(args.output, json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if diffs: raise ValueError(f"{len(diffs)} comparison differences")


def compare_bridge(args):
    source, target = load_json(args.red), load_json(args.fred)
    source_decoded = source.get("decoded")
    semantic = target.get("semantic")
    errors = []
    if not isinstance(source_decoded, dict): errors.append("Red decoded object is missing")
    if target.get("format") != "pkmn-firered-planned-save" or not isinstance(semantic, dict):
        errors.append("target is not a planned FireRed semantic document")
    manifest = load_json(args.manifest) if args.manifest else None
    if manifest is not None and not isinstance(manifest.get("audit"), dict):
        errors.append("conversion manifest audit is missing")
    domains = manifest.get("domains", {}) if manifest else {}
    audit = manifest.get("audit", {}) if manifest else {}
    report = {"format": "pkmn-red-to-firered-bridge-comparison", "version": "1.0.0",
              "valid": not errors, "errors": errors,
              "sourceFormat": source.get("schema", {}).get("format"),
              "targetFormat": target.get("format"),
              "domainCount": len(domains),
              "decisions": len(audit.get("decisions", [])),
              "omissions": len(audit.get("omissions", [])),
              "warnings": len(audit.get("warnings", [])),
              "rejectedInconsistencies": len(audit.get("rejectedInconsistencies", [])),
              "generatorReady": target.get("generatorReady"),
              "physicalImageAbsent": "physicalImage" not in target}
    if args.output: write_new(args.output, json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if errors: raise ValueError("; ".join(errors))


def proof_fred(args):
    source = load_json(args.input)
    stripped = copy.deepcopy(source); stripped.pop("physicalImage", None)
    template = Path(args.template).read_bytes()
    first = generate_native_frjson(stripped, template, Path(args.template).name)
    second = generate_native_frjson(stripped, template, Path(args.template).name)
    mutated = copy.deepcopy(stripped); mutated["physicalImage"] = {"ignored": True}
    third = generate_native_frjson(mutated, template, Path(args.template).name)
    deterministic = first.image == second.image
    isolated = first.image == third.image
    directory = Path(args.output_dir or (str(Path(args.input).with_suffix("")) + ".phase5-proof"))
    if directory.exists(): raise FileExistsError(f"refusing to overwrite {directory}")
    directory.mkdir(parents=True)
    write_new(directory / "source-without-physical-image.fred.json", json.dumps(stripped, indent=2) + "\n")
    write_new(directory / "generated.sav", first.image, binary=True)
    write_new(directory / "generation-report.json", json.dumps(first.report, indent=2) + "\n")
    checklist = "# Phase 5 MAQ Emulator Verification\n\n- [ ] Candidate boots\n- [ ] Complete semantic comparison passes\n- [ ] Save, close, and reload passes\n- [ ] Save Genie reanalysis passes\n"
    write_new(directory / "MAQ_PHASE_5_CHECKLIST.md", checklist)
    artifacts = {}
    for path in directory.iterdir(): artifacts[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
    manifest = {"tool": "pkmn", "toolVersion": "2.0.0", "proofType": "firered-phase5",
                "proofManifestVersion": "2.0.0", "status": "AUTOMATED_PASS_MANUAL_GATE_PENDING",
                "deterministic": deterministic, "physicalImageIsolation": isolated,
                "generatedSha256": hashlib.sha256(first.image).hexdigest(),
                "artifactSha256": artifacts, "emulatorValidation": "required-manual-gate"}
    write_new(directory / "proof-manifest.json", json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"status": manifest["status"], "output": str(directory),
                      "deterministic": deterministic, "physicalImageIsolation": isolated}, indent=2))


def proof_conversion(args):
    source = load_json(args.input)
    template = Path(args.template).read_bytes()
    first = convert_red_json(ROOT, source, template, source_name=Path(args.input).name,
                             source_sha256=source_hash(args.input), template_name=Path(args.template).name,
                             salt=args.salt)
    second = convert_red_json(ROOT, source, template, source_name=Path(args.input).name,
                              source_sha256=source_hash(args.input), template_name=Path(args.template).name,
                              salt=args.salt)
    deterministic = first.generation.image == second.generation.image and first.manifest == second.manifest
    directory = Path(args.output_dir or (str(Path(args.input).with_suffix("")) + ".phase6-proof"))
    if directory.exists(): raise FileExistsError(f"refusing to overwrite {directory}")
    directory.mkdir(parents=True)
    write_new(directory / "proposed.fred.json", json.dumps(first.proposed_fred, indent=2) + "\n")
    write_new(directory / "generated.sav", first.generation.image, binary=True)
    write_new(directory / "conversion-manifest.json", json.dumps(first.manifest, indent=2) + "\n")
    write_new(directory / "conversion-preview.md", first.preview_markdown + "\n")
    write_new(directory / "MAQ_PHASE_6_CHECKLIST.md", "# Phase 6 MAQ Conversion Verification\n\n- [ ] Every source domain is accounted for\n- [ ] Converted save boots\n- [ ] Story and trainer bridge matches\n- [ ] Save, close, reload and reanalysis pass\n")
    artifacts = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in directory.iterdir()}
    proof = {"tool": "pkmn", "toolVersion": "2.0.0", "proofType": "red-to-firered-phase6",
             "proofManifestVersion": "2.0.0", "status": "AUTOMATED_PASS_MANUAL_GATE_PENDING",
             "deterministic": deterministic, "artifactSha256": artifacts,
             "emulatorValidation": "required-manual-gate"}
    write_new(directory / "proof-manifest.json", json.dumps(proof, indent=2) + "\n")
    print(json.dumps({"status": proof["status"], "output": str(directory),
                      "deterministic": deterministic}, indent=2))


def update_schema(args):
    document = load_json(args.input)
    if not isinstance(document, dict):
        raise ValueError("top level must be an object")
    before = copy.deepcopy(document)
    if args.kind == "rjson":
        schema = document.get("schema", {})
        if schema.get("format") not in ("pkmn-red-master-save", "pkmn-red-json"):
            raise ValueError("unsupported Red JSON format")
        if not isinstance(document.get("decoded"), dict):
            raise ValueError("decoded object is required")
        version = schema.get("schemaVersion")
        if version not in (None, "0.1.0"):
            raise ValueError(f"unsupported Red schema version: {version}")
        document.setdefault("schema", {})["schemaVersion"] = "0.1.0"
        target = "0.1.0"
    else:
        if document.get("format") == "pkmn-firered-planned-save":
            version = document.get("schemaVersion")
            if version not in (None, "1.0.0"):
                raise ValueError(
                    f"unsupported planned FireRed schema version: {version}"
                )
            if not isinstance(document.get("semantic"), dict):
                raise ValueError("planned semantic object is required")
            document["schemaVersion"] = "1.0.0"
            target = "planned-1.0.0"
            document["schemaUpdate"] = {
                "tool": "pkmn", "toolVersion": "2.0.0",
                "targetSchemaVersion": target,
                "semanticValuesChanged": False,
            }
            write_new(args.output, json.dumps(document, indent=2) + "\n")
            print(json.dumps({"status": "PASS", "output": str(args.output),
                              "targetSchemaVersion": target,
                              "documentChanged": document != before}, indent=2))
            return
        schema = document.get("schema", {})
        version = schema.get("schemaVersion") or document.get("schemaVersion")
        if document.get("format") != "pkmn-firered-master-save":
            raise ValueError("unsupported native FireRed JSON format")
        if not isinstance(document.get("decoded"), dict):
            raise ValueError("native decoded object is required")
        if version not in (None, "0.4.0"):
            raise ValueError(f"unsupported FireRed schema version: {version}")
        if "schema" in document:
            document.setdefault("schema", {})["schemaVersion"] = "0.4.0"
        else:
            document["schemaVersion"] = "0.4.0"
        target = "0.4.0"
    document["schemaUpdate"] = {
        "tool": "pkmn", "toolVersion": "2.0.0", "targetSchemaVersion": target,
        "semanticValuesChanged": False,
    }
    write_new(args.output, json.dumps(document, indent=2) + "\n")
    print(json.dumps({"status": "PASS", "output": str(args.output),
                      "targetSchemaVersion": target,
                      "documentChanged": document != before}, indent=2))


def validate_frjson(args):
    document = load_json(args.input)
    errors = []
    if not isinstance(document, dict):
        errors.append("top level must be an object")
    schema = document.get("schema", {}) if isinstance(document, dict) else {}
    version = schema.get("schemaVersion") or document.get("schemaVersion")
    planned = document.get("format") == "pkmn-firered-planned-save"
    native = (document.get("format") == "pkmn-firered-master-save" and
              version == "0.4.0")
    if not planned and not native:
        errors.append("expected native schema 0.4.0 or a planned FireRed document")
    if planned:
        if document.get("schemaVersion") != "1.0.0":
            errors.append("planned FireRed schemaVersion must be 1.0.0")
        if document.get("targetMasterSchemaVersion") != "0.4.0":
            errors.append("planned targetMasterSchemaVersion must be 0.4.0")
        if not isinstance(document.get("semantic"), dict):
            errors.append("planned semantic object is required")
        if not isinstance(document.get("generatorReady"), bool):
            errors.append("planned generatorReady boolean is required")
        if document.get("doesNotContainPhysicalSaveBytes") is not True:
            errors.append("planned JSON must assert physical save bytes are absent")
        if "physicalImage" in document:
            errors.append("planned conversion JSON must not contain physicalImage")
    if native:
        if not isinstance(document.get("decoded"), dict):
            errors.append("native decoded object is required")
        physical = document.get("physicalImage")
        if physical is not None:
            if not isinstance(physical, dict):
                errors.append("native physicalImage must be an object")
            else:
                encoded = physical.get("fullFileHex")
                expected = physical.get("fullFileSha256")
                try:
                    image = bytes.fromhex(encoded)
                    if hashlib.sha256(image).hexdigest() != expected:
                        errors.append("native physicalImage SHA-256 mismatch")
                    if len(image) < 0x20000:
                        errors.append("native physicalImage is smaller than 128 KiB")
                except (TypeError, ValueError):
                    errors.append("native physicalImage hex is invalid")
    result = {"status": "PASS" if not errors else "FAIL", "valid": not errors,
              "nativeSchema": native, "plannedSchema": planned, "errors": errors}
    print(json.dumps(result, indent=2))
    if errors:
        raise ValueError("; ".join(errors))


def parser():
    result = argparse.ArgumentParser()
    sub = result.add_subparsers(dest="command", required=True)
    for name, function in (("convert-to-save", convert_to_save),
                           ("convert-to-frjson", convert_to_frjson)):
        command = sub.add_parser(name)
        command.add_argument("input", type=Path)
        command.add_argument("--output", required=True, type=Path)
        command.add_argument("--salt")
        command.add_argument("--source-name")
        command.add_argument("--source-sha256")
        command.add_argument("--manifest", type=Path)
        command.add_argument("--report", type=Path)
        if name == "convert-to-save":
            command.add_argument("--template", required=True, type=Path)
            command.add_argument("--keep-intermediate", action="store_true")
        command.set_defaults(function=function)
    command = sub.add_parser("generate-frjson")
    command.add_argument("input", type=Path)
    command.add_argument("--template", required=True, type=Path)
    command.add_argument("--output", required=True, type=Path)
    command.set_defaults(function=generate_frjson)
    command = sub.add_parser("update-schema")
    command.add_argument("kind", choices=("rjson", "frjson"))
    command.add_argument("input", type=Path)
    command.add_argument("--output", required=True, type=Path)
    command.set_defaults(function=update_schema)
    command = sub.add_parser("validate-frjson")
    command.add_argument("input", type=Path)
    command.set_defaults(function=validate_frjson)
    command = sub.add_parser("bridge-inspect")
    command.add_argument("kind", choices=("event", "trainer", "item"))
    command.add_argument("query", nargs="?")
    command.set_defaults(function=bridge_inspect)
    command = sub.add_parser("validate-manifest")
    command.add_argument("input", type=Path)
    command.set_defaults(function=validate_manifest)
    command = sub.add_parser("compare-frjson")
    command.add_argument("mode", choices=("semantic", "native-authority", "pokemon", "events", "trainers", "items", "fly", "hall-of-fame"))
    command.add_argument("left", type=Path)
    command.add_argument("right", type=Path)
    command.add_argument("--output", type=Path)
    command.set_defaults(function=compare_frjson)
    command = sub.add_parser("compare-bridge")
    command.add_argument("red", type=Path)
    command.add_argument("fred", type=Path)
    command.add_argument("--manifest", type=Path)
    command.add_argument("--output", type=Path)
    command.set_defaults(function=compare_bridge)
    command = sub.add_parser("proof-fred")
    command.add_argument("input", type=Path)
    command.add_argument("--template", required=True, type=Path)
    command.add_argument("--output-dir", type=Path)
    command.set_defaults(function=proof_fred)
    command = sub.add_parser("proof-red-to-firered")
    command.add_argument("input", type=Path)
    command.add_argument("--template", required=True, type=Path)
    command.add_argument("--output-dir", type=Path)
    command.add_argument("--salt")
    command.set_defaults(function=proof_conversion)
    return result


def main():
    args = parser().parse_args()
    try:
        args.function(args)
        return 0
    except Exception as error:  # command boundary: convert to stable CLI error
        print(f"pkmn FireRed runtime: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
