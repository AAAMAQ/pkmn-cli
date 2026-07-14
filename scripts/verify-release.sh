#!/bin/sh
set -eu

source_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/pkmn-cli-release.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

cmake -S "$source_dir" -B "$work_dir/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$work_dir/build" --parallel
ctest --test-dir "$work_dir/build" --output-on-failure
cmake --install "$work_dir/build" --prefix "$work_dir/install"

cd "$work_dir"
"$work_dir/install/bin/pkmn" --help
"$work_dir/install/bin/pkmn" --version
"$work_dir/install/bin/pkmn" doctor
