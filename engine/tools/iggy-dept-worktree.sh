#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  engine/tools/iggy-dept-worktree.sh <department> <topic> [base]

Examples:
  engine/tools/iggy-dept-worktree.sh runtime report-cleanup
  engine/tools/iggy-dept-worktree.sh ai region-ai-map
  engine/tools/iggy-dept-worktree.sh ui preview-consumption master

Creates:
  /Users/kogaryu/iggy-<department>-<topic>
  branch codex/<department>-<topic>
USAGE
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

if [[ $# -lt 2 || $# -gt 3 ]]; then
  usage >&2
  exit 2
fi

DEPT="$1"
TOPIC="$2"
BASE="${3:-HEAD}"

ROOT="$(git rev-parse --show-toplevel)"
PARENT="$(dirname "$ROOT")"
SAFE_DEPT="$(printf '%s' "$DEPT" | tr -c 'A-Za-z0-9._-' '-')"
SAFE_TOPIC="$(printf '%s' "$TOPIC" | tr -c 'A-Za-z0-9._-' '-')"
BRANCH="codex/${SAFE_DEPT}-${SAFE_TOPIC}"
PATH_OUT="${PARENT}/iggy-${SAFE_DEPT}-${SAFE_TOPIC}"

if git show-ref --verify --quiet "refs/heads/${BRANCH}"; then
  echo "Branch already exists: ${BRANCH}" >&2
  exit 1
fi

if [[ -e "$PATH_OUT" ]]; then
  echo "Path already exists: ${PATH_OUT}" >&2
  exit 1
fi

git worktree add -b "$BRANCH" "$PATH_OUT" "$BASE"

echo "Created worktree:"
echo "  path:   ${PATH_OUT}"
echo "  branch: ${BRANCH}"
echo "  base:   ${BASE}"
