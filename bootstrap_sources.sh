#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Fetch only the official, commit-pinned seL4 sources required by HelixOS.
# This script does not build or execute upstream code.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$ROOT/src"
mkdir -p "$SRC/projects" "$SRC/tools"

fetch() {
  local name="$1"
  local url="$2"
  local revision="$3"
  local dest="$4"

  if [ -e "$dest/.git" ]; then
    echo "[verify] $name"
    git -C "$dest" fetch --quiet --tags origin
  else
    echo "[clone]  $name"
    git clone --quiet "$url" "$dest"
  fi
  git -C "$dest" checkout --quiet --detach "$revision"
  actual="$(git -C "$dest" rev-parse HEAD)"
  if [ "$actual" != "$revision" ]; then
    echo "error: $name checkout mismatch: $actual != $revision" >&2
    exit 1
  fi
  printf '%s  %s\n' "$actual" "$name" >> "$ROOT/.sources.lock.tmp"
}

fetch_worktree() {
  local name="$1"
  local url="$2"
  local revision="$3"
  local dest="$4"
  local cache="$ROOT/.source-cache/${name##*/}"

  mkdir -p "$ROOT/.source-cache"
  if [ -d "$cache/.git" ]; then
    echo "[verify] $name source cache"
    git -C "$cache" fetch --quiet --tags origin
  else
    echo "[clone]  $name source cache"
    git clone --quiet "$url" "$cache"
  fi
  if [ -e "$dest" ]; then
    git -C "$cache" worktree remove --force "$dest" 2>/dev/null || rm -rf "$dest"
  fi
  git -C "$cache" worktree prune
  git -C "$cache" worktree add --quiet --detach "$dest" "$revision"
  actual="$(git -C "$dest" rev-parse HEAD)"
  if [ "$actual" != "$revision" ]; then
    echo "error: $name checkout mismatch: $actual != $revision" >&2
    exit 1
  fi
  printf '%s  %s\n' "$actual" "$name" >> "$ROOT/.sources.lock.tmp"
}

rm -f "$ROOT/.sources.lock.tmp"
# Pinned commits from seL4/sel4test-manifest default.xml retrieved 2026-08-13.
fetch "kernel/seL4" "https://github.com/seL4/seL4.git" "1326364bc9135d9445d936ebc01e38a402c1f4c6" "$SRC/kernel"
fetch "tools/seL4_tools" "https://github.com/seL4/seL4_tools.git" "7dd5ba144b1fecf1358a12d2bef3eb365aab35c7" "$SRC/tools/seL4"
fetch_worktree "projects/musllibc" "https://github.com/seL4/musllibc.git" "b0005f86fecbd6d0257b15363a5b013446914265" "$SRC/projects/musllibc"
fetch "projects/seL4_libs" "https://github.com/seL4/seL4_libs.git" "d8abd95e7114c852f6636e3084eb1f66091a75ee" "$SRC/projects/seL4_libs"
fetch "projects/sel4runtime" "https://github.com/seL4/sel4runtime.git" "86489cf6efab9f314964e79468c036e9035394c7" "$SRC/projects/sel4runtime"
fetch "projects/util_libs" "https://github.com/seL4/util_libs.git" "6e55b3c62687779692150e1de411ce61b9d2919a" "$SRC/projects/util_libs"
fetch "projects/sel4_projects_libs" "https://github.com/seL4/sel4_projects_libs.git" "5b3c81127b191232489df09a59b22edead1c9db7" "$SRC/projects/sel4_projects_libs"

{
  echo "# HelixOS official source lock"
  echo "# Source manifest: seL4/sel4test-manifest default.xml, retrieved 2026-08-13"
  sort "$ROOT/.sources.lock.tmp"
} > "$ROOT/sources.lock"
rm -f "$ROOT/.sources.lock.tmp"
printf 'All pinned sources are present. Lockfile: %s\n' "$ROOT/sources.lock"
