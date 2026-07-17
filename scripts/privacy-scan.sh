#!/bin/sh
set -eu

source_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$source_dir"

files=$(git ls-files; git ls-files --others --exclude-standard)
prohibited=$(printf '%s\n' "$files" | \
  grep -E -i '\.(sav|srm|gb|gbc|gba|png|jpe?g|gif|webp|pdf|red\.json|fred\.json|zip|7z|tar|tar\.gz)$|(^|/)(evidence|private)/|\.pkmn-proof/' || true)
if [ -n "$prohibited" ]; then
  printf '%s\n' "Prohibited private/generated artifacts:" "$prohibited" >&2
  exit 1
fi

if rg -n --hidden \
  -g '!.git/**' -g '!build/**' -g '!build-*/**' \
  -g '!Pkmn Unified CLI Plan/**' -g '!scripts/privacy-scan.sh' \
  '/Users/[^/]+/|BEGIN (RSA|OPENSSH|EC|DSA) PRIVATE KEY|AKIA[0-9A-Z]{16}|gh[pousr]_[A-Za-z0-9_]{20,}' .; then
  printf '%s\n' "Private path or likely secret found." >&2
  exit 1
fi

printf '%s\n' "Privacy scan passed: no prohibited artifacts, private paths, or common secret forms found."
