#!/bin/sh
set -eu

prefix=${1:?usage: build-appimage.sh INSTALL_PREFIX OUTPUT.AppImage}
output=${2:?usage: build-appimage.sh INSTALL_PREFIX OUTPUT.AppImage}
appdir=$(mktemp -d "${TMPDIR:-/tmp}/pkmn-appdir.XXXXXX")
trap 'rm -rf "$appdir"' EXIT HUP INT TERM

mkdir -p "$appdir/usr"
cp -R "$prefix/." "$appdir/usr/"
cp packaging/linux/pkmn-interactive.desktop "$appdir/pkmn-interactive.desktop"
cp packaging/linux/pkmn.svg "$appdir/pkmn.svg"
ln -s pkmn.svg "$appdir/.DirIcon"
cp packaging/linux/AppRun "$appdir/AppRun"
chmod +x "$appdir/AppRun"

tool=${APPIMAGETOOL:-appimagetool}
ARCH=x86_64 "$tool" "$appdir" "$output"
