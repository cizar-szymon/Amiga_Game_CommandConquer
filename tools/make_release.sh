#!/bin/sh
# The release of the Amiga port, number from VERSION:
#   releases/amiga/CommandConquer/            the public release (in git): program, icons, doc/, empty data/ and user/
#   releases/amiga/CommandConquer.info        the game drawer's icon: the WARP logo
#   releases/amiga/CommandConquer-<v>.zip     the same as a ZIP (UnZip CommandConquer-<v>.zip -d Work:Games)
#   private/CommandConquer-<v>-full.zip       the same + the game data (data/td2/*.MIX): runs at once; only on this PC
# Build first: tools/build_amiga.sh, then copy the result to build/amiga/CommandConquer.
#   tools/make_release.sh [--no-full]
set -e
cd "$(dirname "$0")/.."
V=$(cat VERSION)
D=docs/release/amiga
R=releases/amiga
T=tools/release
FULL=1
for a in "$@"; do
	case "$a" in
		--no-full) FULL=0 ;;
	esac
done

grep -a -q "\$VER: CommandConquer $V " build/amiga/CommandConquer || { echo "build/amiga/CommandConquer is not $V (amiga_stack.cpp CNC_PORT_VERSION)" >&2; exit 1; }

rm -rf "$R/CommandConquer"
mkdir -p "$R/CommandConquer/data" "$R/CommandConquer/user" "$R/CommandConquer/doc"
cp build/amiga/CommandConquer "$R/CommandConquer/CommandConquer"
# Tool Types in brackets are off, as hints
python $T/make_icon.py "$R/CommandConquer/CommandConquer.info" --frame $T/icon/eagle.png --crop 0 0 76 70 \
  --tooltypes "(SOUND=AHI)" "(SOUND=PAULA)" "(SOUND=OFF)" >/dev/null 2>&1
cp $T/icon/warp-drawer.info "$R/CommandConquer.info"
for f in CommandConquer.txt CommandConquer_en.txt CommandConquer_release.txt CommandConquer_release_en.txt Install.txt COPYING; do
	cp "$D/$f" "$R/CommandConquer/doc/"
done
python $T/make_doc_icon.py "$R/CommandConquer/doc.info" --frame $T/icon/drawer.png --crop 0 0 46 40 --transparent --drawer --window 430 230 --pos 230 12 >/dev/null 2>&1
for t in Install.txt:doc_i:72:10 CommandConquer.txt:doc_pl:72:80 CommandConquer_en.txt:doc_en:282:80 CommandConquer_release.txt:doc_pl:72:150 CommandConquer_release_en.txt:doc_en:282:150; do
	IFS=: read -r f pic x y <<EOT
$t
EOT
	python $T/make_doc_icon.py "$R/CommandConquer/doc/$f.info" --frame "$T/icon/$pic.png" --crop 0 0 36 44 --transparent --project SYS:Utilities/MultiView --pos "$x" "$y" >/dev/null 2>&1
done
cp "$D/data_ReadMe.txt" "$R/CommandConquer/data/ReadMe.txt"
printf 'Saved games and settings of the game are written here.\nZapisy gry i ustawienia sa zapisywane tutaj.\n' > "$R/CommandConquer/user/ReadMe.txt"

mkdir -p private/old
for f in "$R"/CommandConquer-*.zip; do
	[ -e "$f" ] && [ "$f" != "$R/CommandConquer-$V.zip" ] && { git rm -q --cached "$f" 2>/dev/null || true; mv "$f" private/old/; }
done
python $T/make_zip.py "$R/CommandConquer-$V.zip" "$R" CommandConquer CommandConquer.info
if [ "$FULL" = 1 ]; then
	for f in private/CommandConquer-*-full.zip; do
		[ -e "$f" ] && [ "$f" != "private/CommandConquer-$V-full.zip" ] && mv "$f" private/old/
	done
	python $T/make_zip.py "private/CommandConquer-$V-full.zip" "$R" CommandConquer CommandConquer.info --add data/td2 "CommandConquer/data"
fi
ls -l "$R/CommandConquer-$V.zip" private/CommandConquer-$V-full.zip 2>/dev/null
