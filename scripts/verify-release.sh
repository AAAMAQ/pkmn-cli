#!/bin/sh
set -eu

source_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/pkmn-cli-release.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

"$source_dir/scripts/privacy-scan.sh"
mkdir "$work_dir/source"
git -C "$source_dir" archive --format=tar HEAD | tar -xf - -C "$work_dir/source"

cmake -S "$work_dir/source" -B "$work_dir/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$work_dir/build" --parallel
ctest --test-dir "$work_dir/build" --output-on-failure
cmake --install "$work_dir/build" --prefix "$work_dir/install"

cd "$work_dir"
"$work_dir/install/bin/pkmn" --help
"$work_dir/install/bin/pkmn" --version
"$work_dir/install/bin/pkmn" doctor
"$work_dir/install/bin/pkmn" doctor --deep
