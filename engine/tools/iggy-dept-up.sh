#!/usr/bin/env bash
set -euo pipefail

ROOT="${IGGY_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
SHELL_BIN="${SHELL:-/bin/zsh}"
OPEN_TERMINAL=1
RESET=0
START_CODEX=1
CHECK_ONLY=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-open)
      OPEN_TERMINAL=0
      ;;
    --reset)
      RESET=1
      ;;
    --shell-only)
      START_CODEX=0
      ;;
    --check)
      CHECK_ONLY=1
      OPEN_TERMINAL=0
      ;;
    --help|-h)
      cat <<'EOF'
Usage: engine/tools/iggy-dept-up.sh [--no-open] [--reset] [--shell-only] [--check]

Creates the local Mac department floor as visible department stations:
runtime, ai_npc, authoring, ui_product, platform, and integration.

Each department station has Codex-hosted planner, builder, and reviewer tabs.
Researchers, finishers, and apprentices remain on-demand through Codex threads,
subagents, branch worktrees, and durable bus files.

Options:
  --no-open  Create/update tmux sessions without opening Terminal windows.
  --reset    Recreate department sessions so stale worker tabs are removed.
  --shell-only
             Leave each planner station at a shell instead of launching Codex.
  --check    Verify each department has planner, builder, and reviewer tabs,
             each hosted by Codex CLI. Does not create sessions.

Set IGGY_CODEX_ARGS to pass local Codex CLI flags, for example:
  IGGY_CODEX_ARGS='--model gpt-5.4-codex' engine/tools/iggy-dept-up.sh --reset
EOF
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
  shift
done

session_name() {
  printf 'iggy-%s' "$1"
}

departments() {
  printf '%s\n' runtime ai_npc authoring ui_product platform integration
}

required_roles() {
  printf '%s\n' planner builder reviewer
}

existing_or_root() {
  local path="$1"
  if [[ -d "$path/.git" || -f "$path/.git" ]]; then
    printf '%s' "$path"
  else
    printf '%s' "$ROOT"
  fi
}

planner_note() {
  local dept="$1"
  case "$dept" in
    runtime) printf 'Runtime planner: owns frame/session/save/report cleanup and dispatches runtime workers on demand.' ;;
    ai_npc) printf 'AI/NPC planner: owns AI maps, profiles, movement, navigation, and legacy NPC migration gates.' ;;
    authoring) printf 'Authoring planner: owns TOML, package, facade, preview, fixture, and diagnostic lanes.' ;;
    ui_product) printf 'UI/Product planner: owns surface design, preview UX, editor handoff, and play/debug display choices.' ;;
    platform) printf 'Platform planner: owns scripts, tmux floor, CMake lanes, macros, and developer workflow.' ;;
    integration) printf 'Integration planner: owns branch health, merge order, reviewer gates, and final verification.' ;;
  esac
}

role_note() {
  local dept="$1"
  local role="$2"
  case "$role" in
    planner) planner_note "$dept" ;;
    builder) printf '%s builder: implement only planner-scoped packets in this department worktree.' "$dept" ;;
    reviewer) printf '%s reviewer: read-only gate for ownership, risk, semantics, tests, and merge fit.' "$dept" ;;
  esac
}

window_command() {
  local dept="$1"
  local role="$2"
  local note
  note="$(role_note "$dept" "$role")"

  cat <<CMD
printf '\\033[1;36m%s department / %s\\033[0m\\n%s\\n\\n' '$dept' '$role' '$note'
printf 'Head planner stays in the Codex app. This tmux window is the department desk.\\n'
printf 'Codex CLI hosts this role with full local access.\\n'
printf 'Docs: engine/research/departments/%s/README.md\\n' '$dept'
printf 'Bus:  engine/research/departments/%s/{inbox,outbox,replies,decisions}/\\n' '$dept'
printf 'Worktree: '; pwd
printf '\\n\\n'
git status --short --branch 2>/dev/null || true
if [[ '$START_CODEX' -eq 1 ]] && command -v codex >/dev/null 2>&1; then
  printf '\\nStarting Codex CLI planner host. No task is sent until you type one.\\n'
  if [[ -n "\${IGGY_CODEX_ARGS:-}" ]]; then
    # shellcheck disable=SC2086
    exec codex -C "\$(pwd)" --sandbox danger-full-access --ask-for-approval never \${IGGY_CODEX_ARGS}
  fi
  exec codex -C "\$(pwd)" --sandbox danger-full-access --ask-for-approval never
fi
printf '\\nCodex CLI not started; falling back to shell.\\n'
exec '$SHELL_BIN' -l
CMD
}

ensure_department_session() {
  local dept="$1"
  local dir="$2"
  local session
  session="$(session_name "$dept")"

  if [[ "$RESET" -eq 1 ]] && tmux has-session -t "$session" 2>/dev/null; then
    tmux kill-session -t "$session"
  fi

  if ! tmux has-session -t "$session" 2>/dev/null; then
    tmux new-session -d -s "$session" -n planner -c "$dir" \
      "$(window_command "$dept" planner)"
  fi

  local role
  for role in planner builder reviewer; do
    if ! tmux list-windows -t "$session" -F '#{window_name}' 2>/dev/null | grep -qx "$role"; then
      tmux new-window -t "$session" -n "$role" -c "$dir" \
        "$(window_command "$dept" "$role")"
    fi
  done

  tmux list-windows -t "$session" -F '#{window_id} #{window_name}' 2>/dev/null |
    while read -r window_id window_name; do
      if [[ "$window_name" != "planner" && "$window_name" != "builder" && "$window_name" != "reviewer" ]]; then
        tmux kill-window -t "$window_id"
      fi
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

check_department_sessions() {
  local failed=0
  local dept session role command windows

  for dept in $(departments); do
    session="$(session_name "$dept")"
    if ! tmux has-session -t "$session" 2>/dev/null; then
      echo "missing session: $session" >&2
      failed=1
      continue
    fi

    windows="$(tmux list-windows -t "$session" -F '#{window_name}' | sort | paste -sd ',' -)"
    if [[ "$windows" != "builder,planner,reviewer" ]]; then
      echo "$session has wrong tabs: $windows" >&2
      failed=1
    fi

    for role in $(required_roles); do
      if ! tmux list-windows -t "$session" -F '#{window_name}' | grep -qx "$role"; then
        echo "$session missing tab: $role" >&2
        failed=1
        continue
      fi

      command="$(tmux list-panes -t "$session:$role" -F '#{pane_current_command}' | head -n 1)"
      if [[ "$command" != codex* ]]; then
        echo "$session:$role is not Codex-hosted: $command" >&2
        failed=1
      else
        printf '%-18s %-8s %s\n' "$session" "$role" "$command"
      fi
    done
  done

  return "$failed"
}

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux is required" >&2
  exit 1
fi

if [[ "$CHECK_ONLY" -eq 1 ]]; then
  check_department_sessions
  exit $?
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

for dept in $(departments); do
  open_terminal_window "$(session_name "$dept")"
done

echo "Department sessions ready:"
for dept in $(departments); do
  printf '  %-12s tmux attach -t %s\n' "$dept" "$(session_name "$dept")"
done
