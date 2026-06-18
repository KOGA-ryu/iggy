#!/usr/bin/env bash
set -euo pipefail

SESSION="${IGGY_DEPT_SESSION:-iggy-depts}"
ROOT="${IGGY_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
SHELL_BIN="${SHELL:-/bin/zsh}"

create_window() {
  local name="$1"
  local dir="$2"
  local note="$3"

  if tmux list-windows -t "$SESSION" -F '#{window_name}' 2>/dev/null | grep -qx "$name"; then
    return 0
  fi

  tmux new-window -t "$SESSION" -n "$name" -c "$dir" \
    "printf '\\033[1;36m%s\\033[0m\\n%s\\n\\n' '$name' '$note'; git status --short --branch 2>/dev/null || true; exec '$SHELL_BIN' -l"
}

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux is required" >&2
  exit 1
fi

if ! tmux has-session -t "$SESSION" 2>/dev/null; then
  tmux new-session -d -s "$SESSION" -n hub -c "$ROOT" \
    "printf '\\033[1;35mhub\\033[0m\\nHead planner / integration control.\\n\\n'; git worktree list; printf '\\n'; git status --short --branch; exec '$SHELL_BIN' -l"
fi

create_window "runtime" "$ROOT" "Runtime department: frames, reports, sessions, save/load."
create_window "ai-npc" "$ROOT" "AI/NPC department: AI maps, profiles, movement, navigation."
create_window "authoring" "$ROOT" "Authoring department: TOML, package, preview, fixtures."
create_window "ui-product" "$ROOT" "UI/Product department: surface specs, preview consumption, shell."
create_window "platform" "$ROOT" "Platform/Integration department: scripts, CMake, CI lanes."
create_window "integration" "$ROOT" "Integration gate: merge, full build, full CTest, scans."
create_window "research" "$ROOT" "Research/review: read-only scouts, risk maps, merge gates."

tmux select-window -t "$SESSION:hub"

echo "tmux session ready: $SESSION"
echo "Attach with: tmux attach -t $SESSION"
