#!/bin/sh
# WARP HUB takes one game from one repo place: releases/amiga/<Name>-<v>.zip of the repo (branch). Tiberian Dawn is on the
# main branch (releases/amiga); the Red Alert release (releases-ra/amiga here) goes to the branch "release-ra", as
# releases/amiga, and only that. This makes/updates the local branch (a new commit with the present release); push it
# with: git push origin release-ra   (never merge it into main).
set -e
cd "$(dirname "$0")/.."
BR=release-ra
V=$(cat VERSION)
[ -d releases-ra/amiga ] || { echo "no releases-ra/amiga: tools/make_release.sh ra first" >&2; exit 1; }
TMP=$(mktemp -d)
rmdir "$TMP"
if git rev-parse --verify -q "$BR" >/dev/null; then
	git worktree add -q "$TMP" "$BR"
else
	git worktree add -q --detach "$TMP" HEAD
	git -C "$TMP" checkout -q --orphan "$BR"
fi
git -C "$TMP" rm -rq --cached . 2>/dev/null || true
find "$TMP" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +
mkdir -p "$TMP/releases"
cp -r releases-ra/amiga "$TMP/releases/amiga"
printf '# release files are byte for byte what is in the ZIP\nreleases/** -text\n' > "$TMP/.gitattributes"
cp docs/release/amiga/COPYING "$TMP/COPYING" 2>/dev/null || true
printf 'Red Alert for the Amiga: the release (releases/amiga) of this branch is taken by WARP HUB. Sources and notes: main branch.\n' > "$TMP/README.md"
git -C "$TMP" add -A
git -C "$TMP" commit -q -m "Red Alert release $V" -m "Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_017g4PoTADs6Z5A5aMeTycB9"
git worktree remove --force "$TMP"
git log --oneline -1 "$BR"
