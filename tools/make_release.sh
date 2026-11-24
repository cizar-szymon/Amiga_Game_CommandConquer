#!/bin/sh
# The Amiga release of one of the two games, number from VERSION:
#   tools/make_release.sh td|ra [--no-full]
#
# td -> releases/amiga/      TiberianDawn/ (the public release), TiberianDawn.info (WARP drawer icon), TiberianDawn-<v>.zip
# ra -> releases-ra/amiga/   the same for RedAlert (goes to the branch "release-ra", tools/push_release_branch.sh:
#                            WARP HUB takes one game from one place, the newest releases/amiga/<Name>-<v>.zip)
# private/<Name>-<v>-full.zip  the same + the game data (runs at once; only on this PC, never in git)
#
# Build first: tools/build_all.sh (build/amiga/vanillatd, vanillara); the texts come from tools/release/gen_docs.py.
set -e
cd "$(dirname "$0")/.."
GAME=$1
[ -n "$GAME" ] || { echo "usage: tools/make_release.sh td|ra [--no-full]" >&2; exit 1; }
shift
FULL=1
for a in "$@"; do
	case "$a" in
		--no-full) FULL=0 ;;
	esac
done
case "$GAME" in
	td) N=TiberianDawn; BIN=build/amiga/vanillatd; R=releases/amiga; DATA=data/td2; ICON=td.png; CROP="0 0 76 45" ;;
	ra) N=RedAlert; BIN=build/amiga/vanillara; R=releases-ra/amiga; DATA=data/ra; ICON=ra.png; CROP="0 0 76 70" ;;
	*) echo "game: td or ra" >&2; exit 1 ;;
esac
V=$(cat VERSION)
D=docs/release/amiga
T=tools/release

python $T/gen_docs.py
grep -a -q "\$VER: $N $V " $BIN || { echo "$BIN is not $N $V (CNC_PORT_VERSION in the game's bugdlg.cpp)" >&2; exit 1; }

rm -rf "$R/$N"
mkdir -p "$R/$N/data" "$R/$N/user" "$R/$N/doc"
cp $BIN "$R/$N/$N"
# Tool Types in brackets are off, as hints
python $T/make_icon.py "$R/$N/$N.info" --frame $T/icon/$ICON --crop $CROP \
  --tooltypes "(SOUND=AHI)" "(SOUND=OFF)" "(MOVIES=OFF)" "(REPORTSERVER=83.12.152.135:6810)" >/dev/null 2>&1
cp $T/icon/warp-drawer.info "$R/$N.info"
cp "$D/${N}.txt" "$D/${N}_en.txt" "$D/${N}_release.txt" "$D/${N}_release_en.txt" "$R/$N/doc/"
cp "$D/${N}_Install.txt" "$R/$N/doc/Install.txt"
cp "$D/COPYING" "$R/$N/doc/"
python $T/make_doc_icon.py "$R/$N/doc.info" --frame $T/icon/drawer.png --crop 0 0 46 40 --transparent --drawer --window 430 230 --pos 230 12 >/dev/null 2>&1
for t in Install.txt:doc_i:72:10 ${N}.txt:doc_pl:72:80 ${N}_en.txt:doc_en:282:80 ${N}_release.txt:doc_pl:72:150 ${N}_release_en.txt:doc_en:282:150; do
	IFS=: read -r f pic x y <<EOT
$t
EOT
	# FFS names are 30 characters at most, "<name>.info" too: no icon for a text whose icon name would be longer
	[ $((${#f} + 5)) -gt 30 ] && { echo "no icon for $f (name of the icon over 30 characters)"; continue; }
	python $T/make_doc_icon.py "$R/$N/doc/$f.info" --frame "$T/icon/$pic.png" --crop 0 0 36 44 --transparent --project SYS:Utilities/MultiView --pos "$x" "$y" >/dev/null 2>&1
done
cp "$D/${N}_data_ReadMe.txt" "$R/$N/data/ReadMe.txt"
printf 'Saved games, settings and bug reports (bugNNN.*) of the game are written here.\nZapisy gry, ustawienia i zgloszenia bledow (bugNNN.*) sa zapisywane tutaj.\n' > "$R/$N/user/ReadMe.txt"

mkdir -p private/old
for f in "$R"/$N-[0-9]*.zip; do
	[ -e "$f" ] && [ "$f" != "$R/$N-$V.zip" ] && { git rm -q --cached "$f" 2>/dev/null || true; mv "$f" private/old/; }
done
python $T/make_zip.py "$R/$N-$V.zip" "$R" $N $N.info
if [ "$FULL" = 1 ]; then
	for f in private/$N-*-full.zip; do
		[ -e "$f" ] && [ "$f" != "private/$N-$V-full.zip" ] && mv "$f" private/old/
	done
	python $T/make_zip.py "private/$N-$V-full.zip" "$R" $N $N.info --add $DATA "$N/data"
fi
ls -l "$R/$N-$V.zip" private/$N-$V-full.zip 2>/dev/null
