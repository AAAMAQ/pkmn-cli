"""Synthetic Japanese Red layout vector; not evidence from a user save."""

import json
import copy
import subprocess
import sys
import tempfile
from pathlib import Path


def put_name(image, offset, *glyphs):
    if len(glyphs) > 5:
        raise ValueError("Japanese Gen I names have at most five glyphs")
    image[offset:offset + 6] = bytes([*glyphs, 0x50] + [0x50] * (5 - len(glyphs)))


def put_pokemon(image, record, party, species):
    image[record] = species
    image[record + 1:record + 3] = (19).to_bytes(2, "big")
    image[record + 3] = 5
    image[record + 5:record + 7] = bytes([0x16, 0x03])
    image[record + 8] = 33  # Tackle
    image[record + 0x0C:record + 0x0E] = (0x1234).to_bytes(2, "big")
    image[record + 0x0E:record + 0x11] = (135).to_bytes(3, "big")
    image[record + 0x1B:record + 0x1D] = bytes([0xAA, 0xAA])
    image[record + 0x1D] = 35
    if party:
        image[record + 0x21] = 5
        image[record + 0x22:record + 0x24] = (19).to_bytes(2, "big")


def fixture():
    image = bytearray(0x8000)
    put_name(image, 0x2598, 0x80, 0x81)  # アイ
    put_name(image, 0x25F1, 0x85, 0x86)  # カキ
    image[0x25FB:0x25FD] = (0x1234).to_bytes(2, "big")
    image[0x25EE:0x25F1] = bytes([0x00, 0x12, 0x34])
    image[0x2600] = 0  # Pallet Town
    image[0x2603:0x2605] = bytes([6, 3])
    image[0x25C4] = 0
    image[0x27DC] = 0
    image[0x2CA0:0x2CA5] = bytes([1, 0, 2, 3, 4])
    image[0x2ED5] = 1
    image[0x2ED6:0x2ED8] = bytes([0x99, 0xFF])
    put_pokemon(image, 0x2EDD, True, 0x99)  # Bulbasaur
    put_name(image, 0x2ED5 + 0x110, 0x80, 0x81)
    put_name(image, 0x2ED5 + 0x134, 0x85, 0x86)
    image[0x2842] = 0  # selected box 1
    image[0x302D] = 1
    image[0x302E:0x3030] = bytes([0x99, 0xFF])
    put_pokemon(image, 0x302D + 0x20, False, 0x99)
    put_name(image, 0x302D + 0x3FE, 0x80, 0x81)
    put_name(image, 0x302D + 0x4B2, 0x80, 0x81)
    # Permanent box 1 deliberately differs. The current cache must win.
    image[0x4000] = 0
    image[0x4001] = 0xFF
    image[0x4566] = 1  # box 2
    image[0x4567:0x4569] = bytes([0x99, 0xFF])
    put_pokemon(image, 0x4566 + 0x20, False, 0x99)
    put_name(image, 0x4566 + 0x3FE, 0x85, 0x86)
    put_name(image, 0x4566 + 0x4B2, 0x85, 0x86)
    for start in (0x4ACC, 0x5032, 0x6000, 0x6566, 0x6ACC, 0x7032):
        image[start] = 0
        image[start + 1] = 0xFF
    image[0x3594] = ~sum(image[0x2598:0x3594]) & 0xFF
    return bytes(image)


def full_box_fixture():
    image = bytearray(fixture())
    bases = [0x4000, 0x4566, 0x4ACC, 0x5032,
             0x6000, 0x6566, 0x6ACC, 0x7032]
    for base in [0x302D, *bases]:
        image[base] = 30
        image[base + 1:base + 31] = bytes([0x99] * 30)
        image[base + 31] = 0xFF
        for index in range(30):
            put_pokemon(image, base + 0x20 + index * 0x21, False, 0x99)
            put_name(image, base + 0x3FE + index * 6, 0x80, 0x81)
            put_name(image, base + 0x4B2 + index * 6, 0x85, 0x86)
    image[0x3594] = ~sum(image[0x2598:0x3594]) & 0xFF
    return bytes(image)


def nidoran_fixture():
    image = bytearray(fixture())
    image[0x2ED6] = image[0x2EDD] = 0x03  # Nidoran♂ internal ID.
    image[0x3594] = ~sum(image[0x2598:0x3594]) & 0xFF
    return bytes(image)


def hall_of_fame_fixture():
    image = bytearray(fixture())
    image[0x2844] = 1
    image[0x0598] = 0x99  # Bulbasaur
    image[0x0599] = 5
    put_name(image, 0x059A, 0x85, 0x86)
    image[0x29CC] = image[0x25F8] = 0xFF
    for flag in (119, 191, 359, 425, 601, 865, 665, 81):
        image[0x29E9 + flag // 8] |= 1 << (flag % 8)
    image[0x3594] = ~sum(image[0x2598:0x3594]) & 0xFF
    return bytes(image)


def daycare_fixture():
    image = bytearray(fixture())
    image[0x2CA7] = 1
    put_name(image, 0x2CA8, 0x85, 0x86)
    put_name(image, 0x2CAE, 0x80, 0x81)
    put_pokemon(image, 0x2CB4, False, 0x99)
    image[0x3594] = ~sum(image[0x2598:0x3594]) & 0xFF
    return bytes(image)


def run(pkmn, *args):
    result = subprocess.run([str(pkmn), *map(str, args)],
                            capture_output=True, text=True, encoding="utf-8")
    if result.returncode:
        raise AssertionError(f"{args}: {result.stdout}\n{result.stderr}")
    return result.stdout


def run_failure(pkmn, *args):
    result = subprocess.run([str(pkmn), *map(str, args)],
                            capture_output=True, text=True, encoding="utf-8")
    assert result.returncode != 0, (args, result.stdout, result.stderr)
    return result.stderr


def main():
    pkmn = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="pkmn-red-jp-synthetic-") as temp:
        root = Path(temp)
        source = root / "synthetic-jp-red.sav"
        source.write_bytes(fixture())
        run(pkmn, "red-jp", "validate", source, "--profile", "JP_RED_REV0")
        run(pkmn, "red-jp", "decode", source, "--profile", "JP_RED_REV0")
        archive = root / "synthetic-jp-red.red.jp.json"
        run(pkmn, "rjpjson", "validate", archive)
        run(pkmn, "rjpjson", "reconstruct", archive)
        assert (root / "synthetic-jp-red_reconstructed.sav").read_bytes() == fixture()
        run(pkmn, "rjpjson", "project", archive)
        projection = root / "synthetic-jp-red.red.json"
        comparison = json.loads(run(pkmn, "rjpjson", "compare", archive, projection))
        assert comparison["exactProjectionMatch"]
        assert comparison["pcPokemonCount"] == comparison["slotMappings"] == 2
        run(pkmn, "rjson", "validate", projection)
        document = json.loads(projection.read_text(encoding="utf-8"))
        assert document["sourceJapanese"]["pcPokemonCount"] == 2
        assert document["sourceJapanese"]["selectedBoxPermanentDiffers"]
        assert document["decoded"]["pcStorage"]["boxes"][0]["count"] == 2
        assert document["decoded"]["pcStorage"]["boxes"][0]["pokemon"][0]["sourceJapanese"]["nickname"]["value"] == "アイ"
        assert document["decoded"]["party"]["pokemon"][0]["sourceJapanese"]["nickname"]["value"] == "カキ"
        english_bridge = root / "english-bridge.sav"
        run(pkmn, "rjson", "generate", projection, english_bridge)
        run(pkmn, "red", "validate", english_bridge)
        run(pkmn, "red", "decode", english_bridge)
        english = json.loads((root / "english-bridge.red.json").read_text(encoding="utf-8"))
        assert english["decoded"]["party"]["pokemon"][0]["pokedexNumber"] == 1
        assert english["decoded"]["party"]["pokemon"][0]["nickname"]["value"] == "BULBASAUR"
        nidoran_path = root / "nidoran.sav"
        nidoran_path.write_bytes(nidoran_fixture())
        run(pkmn, "red-jp", "decode", nidoran_path, "--profile", "JP_RED_REV0")
        run(pkmn, "rjpjson", "project", root / "nidoran.red.jp.json")
        nidoran = json.loads((root / "nidoran.red.json").read_text(encoding="utf-8"))
        assert nidoran["decoded"]["party"]["pokemon"][0]["nickname"]["value"] == "NIDORAN♂"
        assert nidoran["decoded"]["party"]["pokemon"][0]["sourceJapanese"]["nickname"]["value"] == "カキ"
        target = root / "synthetic-fr.sav"
        run(pkmn, "rjson", "convert_to_frjson", projection)
        planned = json.loads((root / "synthetic-jp-red.fred.json").read_text(encoding="utf-8"))
        assert planned["semantic"]["party"][0]["nickname"] == "カキ", planned["semantic"]["party"][0]
        run(pkmn, "rjson", "convert", projection, "--output", target)
        manifest = json.loads((root / "synthetic-fr.conversion-manifest.json").read_text(encoding="utf-8"))
        assert manifest["route"]["capability"] == "EXPERIMENTAL"
        assert manifest["route"]["evidence"] == "SYNTHETICALLY_VALIDATED_REAL_SAVE_PENDING"
        assert manifest["japaneseSource"]["pcPokemonCount"] == 2
        assert any(change["field"] == "nickname" and change["target"] == "カキ"
                   and change["targetBytesHex"].startswith("5657FF")
                   for change in manifest["pokemonConversions"][0]["fieldChanges"])
        run(pkmn, "fred", "validate", target)
        run(pkmn, "fred", "decode", target)
        decoded = json.loads((root / "synthetic-fr.fred.json").read_text(encoding="utf-8"))
        names = [(mon["nickname"], mon["otName"], mon["language"])
                 for mon in decoded["decoded"]["semantic"]["party"]["pokemon"]]
        assert ("カキ", "RED", 1) in names, names
        pc_names = [(mon["nickname"], mon["otName"], mon["language"])
                    for box in decoded["decoded"]["semantic"]["storage"]["boxes"]
                    for mon in box["pokemon"] if mon.get("occupied")]
        assert ("アイ", "RED", 1) in pc_names, pc_names
        assert ("カキ", "カキ", 1) in pc_names, pc_names

        tampered = copy.deepcopy(document)
        tampered["decoded"]["party"]["pokemon"][0]["sourceJapanese"]["nickname"]["value"] = "アイ"
        tampered_path = root / "tampered.red.json"
        tampered_path.write_text(json.dumps(tampered, ensure_ascii=False), encoding="utf-8")
        run_failure(pkmn, "rjson", "validate", tampered_path)
        run_failure(pkmn, "rjpjson", "compare", archive, tampered_path)

        checksum_bad = bytearray(fixture())
        checksum_bad[0x2600] ^= 1
        checksum_bad_path = root / "bad-checksum.sav"
        checksum_bad_path.write_bytes(checksum_bad)
        run_failure(pkmn, "red-jp", "validate", checksum_bad_path,
                    "--profile", "JP_RED_REV0")

        bad = bytearray(fixture())
        bad[0x2ED5 + 0x134] = 0x00  # Unsupported control byte in a nickname.
        bad[0x3594] = ~sum(bad[0x2598:0x3594]) & 0xFF
        bad_path = root / "bad-name.sav"
        bad_path.write_bytes(bad)
        run(pkmn, "red-jp", "decode", bad_path, "--profile", "JP_RED_REV1")
        run_failure(pkmn, "rjpjson", "project", root / "bad-name.red.jp.json")

        alias = bytearray(fixture())
        put_name(alias, 0x2ED5 + 0x134, 0xCD)  # へ/ヘ share a Gen I tile.
        alias[0x3594] = ~sum(alias[0x2598:0x3594]) & 0xFF
        alias_path = root / "alias.sav"
        alias_path.write_bytes(alias)
        alias_target = root / "alias-fr.sav"
        run(pkmn, "red-jp", "convert", alias_path, "--profile", "JP_RED_REV1",
            "--output", alias_target)
        alias_projection = json.loads((root / "alias.red.json").read_text(encoding="utf-8"))
        assert alias_projection["decoded"]["party"]["pokemon"][0]["sourceJapanese"]["nickname"]["ambiguousGlyph"]
        run(pkmn, "fred", "decode", alias_target)
        alias_decoded = json.loads((root / "alias-fr.fred.json").read_text(encoding="utf-8"))
        assert alias_decoded["decoded"]["semantic"]["party"]["pokemon"][0]["nickname"] == "へ"
        alias_manifest = json.loads((root / "alias-fr.conversion-manifest.json").read_text(encoding="utf-8"))
        assert any("shared-tile alias" in warning for warning in
                   alias_manifest["pokemonConversions"][0]["warnings"])

        hof_path = root / "hof.sav"
        hof_path.write_bytes(hall_of_fame_fixture())
        hof_target = root / "hof-fr.sav"
        run(pkmn, "red-jp", "convert", hof_path, "--profile", "JP_RED_REV0",
            "--output", hof_target)
        run(pkmn, "fred", "validate", hof_target)
        run(pkmn, "fred", "decode", hof_target)
        hof_decoded = json.loads((root / "hof-fr.fred.json").read_text(encoding="utf-8"))
        hof_teams = hof_decoded["decoded"]["specialSectors"]["hallOfFame"]["teams"]
        assert hof_teams[0]["pokemon"][0]["nickname"] == "カキ", hof_teams

        daycare_path = root / "daycare.sav"
        daycare_path.write_bytes(daycare_fixture())
        daycare_target = root / "daycare-fr.sav"
        run(pkmn, "red-jp", "convert", daycare_path, "--profile", "JP_RED_REV0",
            "--output", daycare_target)
        run(pkmn, "fred", "decode", daycare_target)
        daycare_decoded = json.loads((root / "daycare-fr.fred.json").read_text(encoding="utf-8"))
        records = daycare_decoded["decoded"]["semantic"]["daycare"]["records"]
        assert any(entry["pokemon"]["nickname"] == "カキ" for entry in records), records

        full_path = root / "full.sav"
        full_path.write_bytes(full_box_fixture())
        run(pkmn, "red-jp", "decode", full_path, "--profile", "JP_RED_REV0")
        run(pkmn, "rjpjson", "project", root / "full.red.jp.json")
        full = json.loads((root / "full.red.json").read_text(encoding="utf-8"))
        assert full["sourceJapanese"]["pcPokemonCount"] == 240
        assert sum(box["count"] for box in full["decoded"]["pcStorage"]["boxes"]) == 240
        run(pkmn, "rjson", "validate", root / "full.red.json")

        english_cache = copy.deepcopy(document)
        english_cache.pop("sourceJapanese")
        for mon in english_cache["decoded"]["party"]["pokemon"]:
            mon.pop("sourceJapanese")
        for box in english_cache["decoded"]["pcStorage"]["boxes"]:
            for mon in box["pokemon"]:
                mon.pop("sourceJapanese")
        for mon in english_cache["decoded"]["currentBoxCache"]["cache"]["pokemon"]:
            mon.pop("sourceJapanese")
        english_cache["decoded"]["pcStorage"]["boxes"][0]["pokemon"] = []
        english_cache["decoded"]["pcStorage"]["boxes"][0]["count"] = 0
        english_cache["decoded"]["pcStorage"]["boxes"][0]["declaredCount"] = 0
        english_cache_path = root / "english-cache.red.json"
        english_cache_path.write_text(json.dumps(english_cache, ensure_ascii=False), encoding="utf-8")
        run(pkmn, "rjson", "convert_to_frjson", english_cache_path)
        english_cache_plan = json.loads((root / "english-cache.fred.json").read_text(encoding="utf-8"))
        assert len(english_cache_plan["semantic"]["storage"]["boxes"][0]["slots"]) == 2

        direct_path = root / "direct-jp.sav"
        direct_path.write_bytes(fixture())
        direct_target = root / "direct-fr.sav"
        run(pkmn, "red-jp", "convert", direct_path, "--profile", "JP_RED_REV0",
            "--output", direct_target)
        run(pkmn, "fred", "validate", direct_target)
        assert direct_path.read_bytes() == fixture()
        preserved_target = direct_target.read_bytes()
        guided_source = root / "Pkmn Red JP.sav"
        guided_source.write_bytes(fixture())
        guided = subprocess.run(
            [str(pkmn), "interactive"],
            input=f'1\n3\n"{guided_source}"\n1\nYES\nQ\n',
            text=True, encoding="utf-8", capture_output=True, check=True)
        guided_target = root / "Pkmn Red JP_fr.sav"
        assert "Converted save ready" in guided.stdout, guided.stderr
        comparison_dir = root / "guided-parity"
        comparison_dir.mkdir()
        comparison_source = comparison_dir / guided_source.name
        comparison_source.write_bytes(fixture())
        comparison_target = comparison_dir / "expected.sav"
        run(pkmn, "red-jp", "convert", comparison_source,
            "--profile", "JP_RED_REV0", "--output", comparison_target)
        assert guided_target.read_bytes() == comparison_target.read_bytes()
        assert guided_source.read_bytes() == fixture()
        assert "Experimental" in guided.stdout
        run_failure(pkmn, "red-jp", "convert", direct_path,
                    "--profile", "JP_RED_REV0", "--output", direct_target)
        assert direct_target.read_bytes() == preserved_target

        retained_path = root / "retained-jp.sav"
        retained_path.write_bytes(fixture())
        retained_target = root / "retained-fr.sav"
        retained_result = json.loads(run(
            pkmn, "red-jp", "convert", retained_path,
            "--profile", "JP_RED_REV0", "--retain-playername",
            "--output", retained_target))
        retained_projection_path = root / "retained-jp.red.json"
        retained_projection = json.loads(retained_projection_path.read_text(encoding="utf-8"))
        assert retained_projection["sourceJapanese"]["targetPlayerNamePolicy"] == "retain-japanese-raw-experimental"
        run(pkmn, "rjpjson", "validate", root / "retained-jp.red.jp.json")
        retained_comparison = json.loads(run(
            pkmn, "rjpjson", "compare", root / "retained-jp.red.jp.json",
            retained_projection_path))
        assert retained_comparison["exactProjectionMatch"]
        run(pkmn, "rjson", "validate", retained_projection_path)
        retained_manifest = json.loads((root / "retained-fr.conversion-manifest.json").read_text(encoding="utf-8"))
        assert retained_manifest["japaneseSource"]["playerNamePolicy"] == "retain-japanese-raw-experimental"
        assert retained_manifest["japaneseSource"]["originalPlayerName"] == "アイ"
        assert retained_manifest["japaneseSource"]["targetPlayerNameFieldHex"].startswith("5152FF")
        assert retained_manifest["japaneseSource"]["playerNameDisplayStatus"] == "UNVERIFIED_IN_ENGLISH_FIRERED"
        assert retained_manifest["route"]["capability"] == "EXPERIMENTAL"
        assert retained_result["status"] == "CANDIDATE_REQUIRES_EMULATOR"
        assert bytes([0x51, 0x52, 0xFF]) in retained_target.read_bytes()
        run(pkmn, "fred", "validate", retained_target)
        run(pkmn, "fred", "decode", retained_target)
        retained_decoded = json.loads((root / "retained-fr.fred.json").read_text(encoding="utf-8"))
        assert retained_decoded["decoded"]["semantic"]["party"]["pokemon"][0]["otName"] == "アイ"

        bad_policy = copy.deepcopy(retained_projection)
        bad_policy["sourceJapanese"]["targetPlayerNamePolicy"] = "unexpected"
        bad_policy_path = root / "bad-policy.red.json"
        bad_policy_path.write_text(json.dumps(bad_policy, ensure_ascii=False), encoding="utf-8")
        run_failure(pkmn, "rjson", "validate", bad_policy_path)
        print("Synthetic Japanese Red archive and projection passed")


if __name__ == "__main__":
    main()
