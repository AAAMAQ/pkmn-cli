#!/usr/bin/env python3
"""Regression test for bundled and user-supplied FireRed templates."""

import json
import sys
from pathlib import Path


def main():
    root = Path(sys.argv[1])
    sys.path.insert(0, str(root / "runtime"))
    from firered_generator.binary import analyze_slots, assemble_logical, scatter_logical
    from firered_generator.generator import validate_clean_template

    profile = json.loads(
        (root / "runtime/data/firered_v1_clean_template_profile.json").read_text()
    )
    bundled = (
        root / "resources/pokemon-firered-usa-europe-v1.template.bin"
    ).read_bytes()
    standard = validate_clean_template(bundled, profile)
    assert standard["kind"] == "bundled-standard"

    custom = bytearray(bundled)
    slot = analyze_slots(custom)
    sb2, sb1, storage = assemble_logical(custom, slot)
    sb2[0x11] = (sb2[0x11] + 1) % 60  # Harmless clean-save timing may differ.
    scatter_logical(custom, slot, (sb2, sb1, storage))
    accepted = validate_clean_template(bytes(custom), profile)
    assert accepted["kind"] == "user-clean-dump"

    progressed = bytearray(custom)
    slot = analyze_slots(progressed)
    sb2, sb1, storage = assemble_logical(progressed, slot)
    sb1[0xEE0] ^= 1  # Any progression flag drift must reject the template.
    scatter_logical(progressed, slot, (sb2, sb1, storage))
    try:
        validate_clean_template(bytes(progressed), profile)
    except ValueError as error:
        assert "saved event flags" in str(error)
    else:
        raise AssertionError("progressed FireRed template was accepted")


if __name__ == "__main__":
    main()
