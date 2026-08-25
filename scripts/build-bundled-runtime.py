#!/usr/bin/env python3
"""Build the private, self-contained pkmn conversion runtime with PyInstaller.

Python is a release-build dependency only. End users run the resulting helper
through pkmn and do not need a separate Python installation.
"""

import argparse
from importlib.metadata import distribution
import os
import shutil
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    source = args.source.resolve()
    runtime = source / "runtime"
    output = args.output.resolve()
    work = output.parent / "pyinstaller-work"
    spec = output.parent / "pyinstaller-spec"
    separator = ";" if os.name == "nt" else ":"
    executable_name = "pkmn-runtime.exe" if os.name == "nt" else "pkmn-runtime"

    output.mkdir(parents=True, exist_ok=True)
    previous = output / "pkmn-runtime"
    if previous.is_dir():
        shutil.rmtree(previous)
    elif previous.exists():
        previous.unlink()
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(spec, ignore_errors=True)
    command = [
        sys.executable,
        "-m",
        "PyInstaller",
        "--noconfirm",
        "--clean",
        "--onedir",
        "--name",
        "pkmn-runtime",
        "--distpath",
        str(output),
        "--workpath",
        str(work),
        "--specpath",
        str(spec),
        "--paths",
        str(runtime),
        "--add-data",
        f"{runtime / 'data'}{separator}data",
        "--add-data",
        f"{runtime / 'schemas'}{separator}schemas",
        "--collect-submodules",
        "bridge_planner",
        "--collect-submodules",
        "firered_generator",
        str(runtime / "pkmn_v2_runtime.py"),
    ]
    environment = os.environ.copy()
    environment["PYINSTALLER_CONFIG_DIR"] = str(output.parent / "pyinstaller-cache")
    subprocess.run(command, check=True, env=environment, cwd=source)
    built = output / "pkmn-runtime" / executable_name
    if not built.is_file():
        raise RuntimeError(f"PyInstaller did not produce {built}")
    licenses = built.parent / "_licenses"
    licenses.mkdir(exist_ok=True)
    python_license = Path(sys.base_prefix) / "LICENSE"
    if not python_license.is_file():
        python_license = Path(sys.base_prefix) / "lib" / (
            f"python{sys.version_info.major}.{sys.version_info.minor}"
        ) / "LICENSE.txt"
    if python_license.is_file():
        shutil.copy2(python_license, licenses / "PYTHON-LICENSE.txt")
    pyinstaller = distribution("pyinstaller")
    for item in pyinstaller.files or ():
        if str(item).endswith("licenses/COPYING.txt"):
            shutil.copy2(pyinstaller.locate_file(item),
                         licenses / "PYINSTALLER-COPYING.txt")
            break
    print(built)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
