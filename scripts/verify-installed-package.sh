#!/bin/sh
set -eu

prefix=${1:?usage: verify-installed-package.sh INSTALL_PREFIX}
binary="$prefix/bin/pkmn"

test -x "$binary"
"$binary" --version
"$binary" doctor --deep
"$binary" config show --format json
"$binary" convert routes --format json
"$binary" frjson schema --format json

runtime_mode=$("$binary" doctor --format json | sed -n 's/.*"runtimeMode": "\([^"]*\)".*/\1/p')
if [ "$runtime_mode" != "bundled-private-executable" ]; then
  echo "installed package did not select the private bundled runtime" >&2
  exit 1
fi

work=$(mktemp -d "${TMPDIR:-/tmp}/pkmn-installed-smoke.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
source_template="$prefix/share/pkmn/resources/pokemon-red-usa-europe-v1.template.bin"
"$binary" red repair-checksums "$source_template" --output "$work/source.sav"
"$binary" red decode "$work/source.sav" --output "$work/source.red.json"
PATH=/usr/bin:/bin "$binary" rjson convert_to_frjson "$work/source.red.json" \
  "$work/target.fred.json"
PATH=/usr/bin:/bin "$binary" frjson validate "$work/target.fred.json"

echo "Installed package smoke test passed without invoking a separate Python runtime."
