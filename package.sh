#!/bin/bash
# SPDX-License-Identifier: Unlicense
# Assembles package/ (what goes on the card under roms/ports) from build/ and the author's game files.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
BUILD="${BUILD:-$HERE/build-sf3500}"
GAME_DIR="$HERE/game"
OUT="${PKG:-$HERE/package}"
rm -rf "$OUT"; mkdir -p "$OUT/dead-mobsters"
P="$OUT/dead-mobsters"
cp "$BUILD/fxi.mipsel" "$P/"
# the author's files, contents unchanged (not the Windows interpreter and DLLs, not mobsters.bat);
# names are lower case in game/ because that is what mobsters.dcb asks for
for f in mobsters.dcb font1.fnt readme.txt; do cp "$GAME_DIR/$f" "$P/"; done
cp "$GAME_DIR"/*.fpg "$GAME_DIR"/*.wav "$P/"
cp "$HERE/package-src/run.sh" "$P/run.sh"; chmod +x "$P/run.sh"
cp "$HERE/package-src/Dead Mobsters.sh" "$OUT/Dead Mobsters.sh"; chmod +x "$OUT/Dead Mobsters.sh"
[ -f "$HERE/package-src/boxart.png" ] && cp "$HERE/package-src/boxart.png" "$P/boxart.png"
[ -f "$HERE/package-src/README.md" ] && cp "$HERE/package-src/README.md" "$P/README.md"
mkdir -p "$P/licenses"; cp "$HERE"/licenses/*.txt "$P/licenses/"; cp "$HERE/NOTICE.md" "$P/NOTICE.md"
echo "package: $OUT"; (cd "$OUT" && find . -type f | sort | wc -l); du -sh "$OUT"
