"""Green 1.0 command/profile coverage using generated Japanese-layout bytes.

This is synthetic evidence only, not a real Green save or emulator acceptance.
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from japanese_red_synthetic_test import fixture, run, run_failure


def main():
    pkmn = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="pkmn-green-jp-") as tmp:
        root = Path(tmp)
        source = root / "Pkmn Green JP.sav"
        original = fixture() + b"synthetic trailer"
        source.write_bytes(original)
        run(pkmn, "green-jp", "validate", source, "--profile", "JP_GREEN_REV0")
        run_failure(pkmn, "green-jp", "validate", source, "--profile", "JP_RED_REV0")
        run_failure(pkmn, "green-jp", "validate", source, "--profile", "JP_GREEN_REV1")
        run_failure(pkmn, "red-jp", "validate", source, "--profile", "JP_GREEN_REV0")
        run(pkmn, "green-jp", "decode", source, "--profile", "JP_GREEN_REV0")
        archive = root / "Pkmn Green JP.green.jp.json"
        data = json.loads(archive.read_text())
        assert data["schema"]["format"] == "pkmn-green-jp-master-save"
        assert data["source"]["profile"] == "JP_GREEN_REV0"
        for action in ("inspect", "validate"):
            run(pkmn, "gjpjson", action, archive)
        run_failure(pkmn, "rjpjson", "validate", archive)
        run(pkmn, "gjpjson", "reconstruct", archive)
        assert (root / "Pkmn Green JP_reconstructed.sav").read_bytes() == original
        run(pkmn, "gjpjson", "project", archive)
        projection = root / "Pkmn Green JP.red.json"
        run(pkmn, "rjson", "validate", projection)
        run(pkmn, "gjpjson", "compare", archive, projection)
        assert json.loads(projection.read_text())["sourceJapanese"]["profile"] == "JP_GREEN_REV0"
        data["schema"]["format"] = "pkmn-red-jp-master-save"
        invalid = root / "mismatched.json"
        invalid.write_text(json.dumps(data))
        run_failure(pkmn, "gjpjson", "validate", invalid)
        no_image = root / "no-image.green.jp.json"
        run(pkmn, "green-jp", "decode", source, "--profile", "JP_GREEN_REV0",
            "--no-physical-image", "--output", no_image)
        run_failure(pkmn, "gjpjson", "reconstruct", no_image)
        converted_source = root / "convert-green.sav"
        converted_source.write_bytes(original)
        target = root / "green-fr.sav"
        run(pkmn, "green-jp", "convert", converted_source,
            "--profile", "JP_GREEN_REV0", "--output", target)
        run(pkmn, "fred", "validate", target)
        run(pkmn, "fred", "decode", target)
        decoded = json.loads((root / "green-fr.fred.json").read_text())
        assert any(mon["nickname"] == "カキ" and mon["language"] == 1
                   for mon in decoded["decoded"]["semantic"]["party"]["pokemon"])
        repeated = root / "green-fr-repeat.sav"
        run(pkmn, "rjson", "convert", root / "convert-green.red.json", "--output", repeated)
        assert target.read_bytes() == repeated.read_bytes()
        manifest = json.loads((root / "green-fr.conversion-manifest.json").read_text())
        assert manifest["route"]["id"] == "green-jp-firered"
        assert manifest["route"]["sourceProfile"] == "JP_GREEN_REV0"
        assert manifest["route"]["capability"] == "EXPERIMENTAL"
        run(pkmn, "convert", "validate-manifest", root / "green-fr.conversion-manifest.json")
        manifest["route"]["sourceProfile"] = "JP_RED_REV0"
        bad_manifest = root / "bad-manifest.json"
        bad_manifest.write_text(json.dumps(manifest))
        run_failure(pkmn, "convert", "validate-manifest", bad_manifest)
        before = target.read_bytes()
        run_failure(pkmn, "green-jp", "convert", converted_source,
                    "--profile", "JP_GREEN_REV0", "--output", target)
        assert target.read_bytes() == before
        guided_source = root / "simple-green.sav"
        guided_source.write_bytes(original)
        guided = subprocess.run([str(pkmn), "interactive"],
            input=f'1\n4\n"{guided_source}"\n1\nYES\nQ\n',
            capture_output=True, text=True, check=True)
        assert "Converted save ready" in guided.stdout, guided.stderr
        run(pkmn, "fred", "validate", root / "simple-green_fr.sav")
        assert source.read_bytes() == original == guided_source.read_bytes()


if __name__ == "__main__":
    main()
