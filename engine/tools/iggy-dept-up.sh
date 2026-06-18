#!/usr/bin/env bash
set -euo pipefail

ROOT="${IGGY_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
SHELL_BIN="${SHELL:-/bin/zsh}"
OPEN_TERMINAL=1

if [[ "${1:-}" == "--no-open" ]]; then
  OPEN_TERMINAL=0
fi

session_name() {
  printf 'iggy-%s' "$1"
}

existing_or_root() {
  local path="$1"
  if [[ -d "$path/.git" || -f "$path/.git" ]]; then
    printf '%s' "$path"
  else
    printf '%s' "$ROOT"
  fi
}

role_note() {
  local dept="$1"
  local role="$2"
  case "$role" in
    planner) printf 'Own the %s department bucket, refine work with reviewer/researcher, dispatch.' "$dept" ;;
    builder) printf 'Implement scoped %s work only in this department branch.' "$dept" ;;
    reviewer) printf 'Read-only adversarial gate for %s ownership, risk, and merge fit.' "$dept" ;;
    researcher) printf 'Read-only scout for %s semantics, prior art, compute cost, and repo fit.' "$dept" ;;
    finisher) printf 'Cleanup, docs, test-support, and merge polish for %s.' "$dept" ;;
    apprentice) printf 'Spark slot for small bounded %s scans or mechanical tasks.' "$dept" ;;
  esac
}

window_command() {
  local dept="$1"
  local role="$2"
  local note
  note="$(role_note "$dept" "$role")"

  cat <<CMD
printf '\\033[1;36m%s / %s\\033[0m\\n%s\\n\\n' '$dept' '$role' '$note'
printf 'Docs: engine/research/departments/%s/README.md\\n' '$dept'
printf 'Worktree: '; pwd
printf '\\n\\n'
git status --short --branch 2>/dev/null || true
exec '$SHELL_BIN' -l
CMD
}

ensure_department_session() {
  local dept="$1"
  local dir="$2"
  local session
  session="$(session_name "$dept")"

  if ! tmux has-session -t "$session" 2>/dev/null; then
    tmux new-session -d -s "$session" -n planner -c "$dir" \
      "$(window_command "$dept" planner)"
  fi

  local role
  for role in builder reviewer researcher finisher apprentice; do
    if tmux list-windows -t "$session" -F '#{window_name}' 2>/dev/null | grep -qx "$role"; then
      continue
    fi
    tmux new-window -t "$session" -n "$role" -c "$dir" \
      "$(window_command "$dept" "$role")"
  done

  tmux select-window -t "$session:planner" >/dev/null
}

open_terminal_window() {
  local session="$1"

  if [[ "$OPEN_TERMINAL" -eq 0 ]]; then
    return 0
  fi

  osascript >/dev/null <<APPLESCRIPT
tell application "Terminal"
  activate
  do script "cd '$ROOT' && tmux attach -t '$session'"
end tell
APPLESCRIPT
}

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux is required" >&2
  exit 1
fi

RUNTIME_DIR="$(existing_or_root "/Users/kogaryu/iggy-finisher")"
AI_NPC_DIR="$(existing_or_root "/Users/kogaryu/iggy-builder-aimap")"
AUTHORING_DIR="$(existing_or_root "/Users/kogaryu/iggy-authoring")"
UI_PRODUCT_DIR="$(existing_or_root "/Users/kogaryu/iggy-ui-product")"
PLATFORM_DIR="$(existing_or_root "/Users/kogaryu/iggy-platform")"
INTEGRATION_DIR="$ROOT"

ensure_department_session runtime "$RUNTIME_DIR"
ensure_department_session ai_npc "$AI_NPC_DIR"
ensure_department_session authoring "$AUTHORING_DIR"
ensure_department_session ui_product "$UI_PRODUCT_DIR"
ensure_department_session platform "$PLATFORM_DIR"
ensure_department_session integration "$INTEGRATION_DIR"

for dept in runtime ai_npc authoring ui_product platform integration; do
  open_terminal_window "$(session_name "$dept")"
done

echo "Department sessions ready:"
for dept in runtime ai_npc authoring ui_product platform integration; do
  printf '  %-12s tmux attach -t %s\n' "$dept" "$(session_name "$dept")"
done
