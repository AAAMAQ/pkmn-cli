#!/bin/sh
set -eu

source_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/pkmn-cli-release.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

"$source_dir/scripts/privacy-scan.sh"
mkdir "$work_dir/source"
git -C "$source_dir" ls-files --cached --others --exclude-standard |
  tar -C "$source_dir" -cf - -T - | tar -xf - -C "$work_dir/source"

python3 -m PyInstaller --version >/dev/null
python3 "$work_dir/source/scripts/build-bundled-runtime.py" \
  --source "$work_dir/source" --output "$work_dir/runtime-dist"
runtime="$work_dir/runtime-dist/pkmn-runtime/pkmn-runtime"
cmake -S "$work_dir/source" -B "$work_dir/build" -DCMAKE_BUILD_TYPE=Release \
  -DPKMN_BUNDLED_RUNTIME_EXECUTABLE="$runtime"
cmake --build "$work_dir/build" --parallel
ctest --test-dir "$work_dir/build" --output-on-failure
cmake --install "$work_dir/build" --prefix "$work_dir/install"

cd "$work_dir"
"$work_dir/install/bin/pkmn" --help
"$work_dir/install/bin/pkmn" --version
"$work_dir/install/bin/pkmn" doctor
"$work_dir/install/bin/pkmn" doctor --deep
"$work_dir/source/scripts/verify-installed-package.sh" "$work_dir/install"

cmake --build "$work_dir/build" --target package

printf '%s\n' "Phase 3 release verification passed from the current working tree."
