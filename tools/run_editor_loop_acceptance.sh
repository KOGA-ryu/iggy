#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: tools/run_editor_loop_acceptance.sh [options]

Builds the product app, runs the deterministic no-window editor loop proof, and
prints manual window commands for the same save root.

Default behavior does not launch a window.

Options:
  --run-window          Launch the starter window after the no-window proof.
  --skip-build          Skip configure/build and use the existing build dir.
  --build-dir PATH      Build directory. Default: build-vulkan
  --save-root PATH      Save root. Default: a temporary acceptance save root.
  --jobs N              Build parallelism. Default: $IGGY3D_JOBS or 8
  -h, --help            Show this help.
USAGE
}

quote_command() {
  local arg
  for arg in "$@"; do
    printf '%q ' "$arg"
  done
  printf '\n'
}

write_control_file() {
  local path="$1"
  local content="$2"
  printf '%s' "${content}" > "${path}"
}

require_field() {
  local receipt="$1"
  local field="$2"
  if ! grep -Fxq "${field}" "${receipt}"; then
    echo "missing receipt field in ${receipt}: ${field}" >&2
    echo "receipt excerpt:" >&2
    grep -E '^(result|frontend_screen|gameplay_active|interaction_mode|input_owner|room_editing_ready|room_editing_last_operation|room_editor_preview_visible|active_room_source|active_room_id|active_room_authored_floor_count|active_room_authored_wall_count|active_room_collision_ready|product_save_status|product_save_source|product_save_save_id|product_save_session_saved|product_save_load_status|product_save_load_source|product_save_load_save_id|product_vulkan_room_mesh_cpu_ready|product_vulkan_room_wall_draw_count)=' "${receipt}" >&2 || true
    return 1
  fi
}

require_file_contains() {
  local path="$1"
  local text="$2"
  if ! grep -Fq "${text}" "${path}"; then
    echo "missing save-file text in ${path}: ${text}" >&2
    return 1
  fi
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
build_dir="${repo_root}/build-vulkan"
save_root=""
jobs="${IGGY3D_JOBS:-8}"
run_window=0
skip_build=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --run-window)
      run_window=1
      shift
      ;;
    --skip-build)
      skip_build=1
      shift
      ;;
    --build-dir)
      if [[ $# -lt 2 ]]; then
        echo "missing value for --build-dir" >&2
        exit 2
      fi
      build_dir="$2"
      shift 2
      ;;
    --save-root)
      if [[ $# -lt 2 ]]; then
        echo "missing value for --save-root" >&2
        exit 2
      fi
      save_root="$2"
      shift 2
      ;;
    --jobs)
      if [[ $# -lt 2 ]]; then
        echo "missing value for --jobs" >&2
        exit 2
      fi
      jobs="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

case "${build_dir}" in
  /*) ;;
  *) build_dir="${repo_root}/${build_dir}" ;;
esac

if [[ -z "${save_root}" ]]; then
  save_root="$(mktemp -d "${TMPDIR:-/tmp}/iggy3d_editor_loop_acceptance.XXXXXX")"
else
  case "${save_root}" in
    /*) ;;
    *) save_root="${repo_root}/${save_root}" ;;
  esac
  rm -rf "${save_root}"
  mkdir -p "${save_root}"
fi

configure_command=(cmake -S "${repo_root}" -B "${build_dir}" -DIGGY3D_ENABLE_VULKAN=ON)
build_command=(cmake --build "${build_dir}" --target iggy3d_app -j "${jobs}")
app_binary="${build_dir}/iggy3d"

work_dir="$(mktemp -d "${TMPDIR:-/tmp}/iggy3d_editor_loop_acceptance_receipts.XXXXXX")"
create_control="${work_dir}/create_edit_leave_save.control"
continue_control="${work_dir}/continue.control"
create_receipt="${work_dir}/create_edit_leave_save.receipt"
starter_receipt="${work_dir}/fresh_starter.receipt"
continue_receipt="${work_dir}/continue.receipt"

write_control_file "${create_control}" $'frontend.execute=true\nworld.title=Editor Loop Acceptance\nworld.draft_cell=1,2,#\nworld.create=true\nsystem.pause=true\nmenu.down=true\npause.execute=true\neditor.input=editor.nudge_x_pos,editor.select_wall_tool,editor.preview,editor.preview_confirm\nmenu.back=true\nfrontend.select=leave_editor\nmenu.confirm=true\nsettings.back=true\npause.select=save\ndev_tools.execute=true\n'
write_control_file "${continue_control}" $'frontend.select=continue\nfrontend.execute=true\n'

echo "== Iggy3D editor loop acceptance =="
echo "Repo: ${repo_root}"
echo "Build dir: ${build_dir}"
echo "Save root: ${save_root}"
echo "Receipt dir: ${work_dir}"
echo

if [[ "${skip_build}" -eq 0 ]]; then
  echo "+ $(quote_command "${configure_command[@]}")"
  "${configure_command[@]}"
  echo
  echo "+ $(quote_command "${build_command[@]}")"
  "${build_command[@]}"
  echo
fi

if [[ ! -x "${app_binary}" ]]; then
  echo "app binary not found or not executable: ${app_binary}" >&2
  exit 1
fi

create_command=("${app_binary}" --no-window --renderer vulkan --input auto
                --save-root "${save_root}" --automation-control "${create_control}"
                --print-render-receipt)
starter_command=("${app_binary}" --no-window --renderer vulkan --input auto
                 --save-root "${save_root}" --print-render-receipt)
continue_command=("${app_binary}" --no-window --renderer vulkan --input auto
                  --save-root "${save_root}" --automation-control "${continue_control}"
                  --print-render-receipt)

echo "No-window proof: create -> edit -> preview confirm -> leave editor -> save"
echo "+ $(quote_command "${create_command[@]}")"
"${create_command[@]}" > "${create_receipt}"
require_field "${create_receipt}" "result=pass"
require_field "${create_receipt}" "frontend_screen=pause"
require_field "${create_receipt}" "gameplay_active=true"
require_field "${create_receipt}" "interaction_mode=player"
require_field "${create_receipt}" "input_owner=pause"
require_field "${create_receipt}" "room_editing_ready=false"
require_field "${create_receipt}" "room_editing_last_operation=pause_leave_editor"
require_field "${create_receipt}" "room_editor_preview_visible=false"
require_field "${create_receipt}" "active_room_source=editable_room"
require_field "${create_receipt}" "active_room_id=custom_dungeon_draft"
require_field "${create_receipt}" "active_room_authored_floor_count=58"
require_field "${create_receipt}" "active_room_authored_wall_count=62"
require_field "${create_receipt}" "active_room_collision_ready=true"
require_field "${create_receipt}" "product_save_status=product_save_written"
require_field "${create_receipt}" "product_save_source=pause_save"
require_field "${create_receipt}" "product_save_save_id=save_001"
require_field "${create_receipt}" "product_save_session_saved=true"
require_field "${create_receipt}" "product_vulkan_room_mesh_cpu_ready=true"
require_field "${create_receipt}" "product_vulkan_room_asset_id=custom_dungeon_draft"
require_field "${create_receipt}" "product_vulkan_room_wall_draw_count=22"

save_file="${save_root}/save_001.iggy3d.save"
if [[ ! -f "${save_file}" ]]; then
  echo "expected save file missing: ${save_file}" >&2
  exit 1
fi
require_file_contains "${save_file}" "authoredRoom.id=custom_dungeon_draft"
require_file_contains "${save_file}" "authoredRoom.wall.count=62"
require_file_contains "${save_file}" "edit_wall_1"

echo "No-window proof: fresh starter sees compatible save"
echo "+ $(quote_command "${starter_command[@]}")"
"${starter_command[@]}" > "${starter_receipt}"
require_field "${starter_receipt}" "result=pass"
require_field "${starter_receipt}" "frontend_screen=starter"
require_field "${starter_receipt}" "gameplay_active=false"
require_field "${starter_receipt}" "interaction_mode=player"
require_field "${starter_receipt}" "save_count=1"
require_field "${starter_receipt}" "compatible_save_count=1"

echo "No-window proof: Continue restores edited authored room"
echo "+ $(quote_command "${continue_command[@]}")"
"${continue_command[@]}" > "${continue_receipt}"
require_field "${continue_receipt}" "result=pass"
require_field "${continue_receipt}" "frontend_screen=gameplay"
require_field "${continue_receipt}" "gameplay_active=true"
require_field "${continue_receipt}" "interaction_mode=player"
require_field "${continue_receipt}" "input_owner=gameplay"
require_field "${continue_receipt}" "room_editing_ready=false"
require_field "${continue_receipt}" "active_room_source=saved_authored_room"
require_field "${continue_receipt}" "active_room_id=custom_dungeon_draft"
require_field "${continue_receipt}" "active_room_authored_floor_count=58"
require_field "${continue_receipt}" "active_room_authored_wall_count=62"
require_field "${continue_receipt}" "active_room_collision_ready=true"
require_field "${continue_receipt}" "product_save_load_status=product_save_loaded"
require_field "${continue_receipt}" "product_save_load_source=continue"
require_field "${continue_receipt}" "product_save_load_save_id=save_001"
require_field "${continue_receipt}" "product_vulkan_room_mesh_cpu_ready=true"
require_field "${continue_receipt}" "product_vulkan_room_asset_id=custom_dungeon_draft"
require_field "${continue_receipt}" "product_vulkan_room_wall_draw_count=22"

echo
echo "Editor loop proof: pass"
echo "Save file: ${save_file}"
echo "Receipts:"
echo "  create/edit/save: ${create_receipt}"
echo "  fresh starter:    ${starter_receipt}"
echo "  continue:         ${continue_receipt}"
echo

starter_window_command=("${app_binary}" --window --renderer vulkan --input auto
                        --save-root "${save_root}" --print-render-receipt)
continue_window_command=("${app_binary}" --window --renderer vulkan --input auto
                         --save-root "${save_root}" --print-render-receipt)

cat <<'POLICY'
Manual window acceptance:
  1. Launch the starter command below.
  2. Choose Continue to load the saved edited dungeon.
  3. Use Pause -> Edit Room for creative editing.
  4. Preview/place edits explicitly, Leave Editor, then Save.
  5. Relaunch the same command and Continue again to verify persistence.

Leave Editor does not autosave. The proof above saves only through pause Save.
POLICY
echo
echo "Starter/manual command:"
quote_command "${starter_window_command[@]}"
echo "Continue command uses the same starter path and save root:"
quote_command "${continue_window_command[@]}"

if [[ "${run_window}" -eq 1 ]]; then
  echo
  echo "Launching starter window:"
  echo "+ $(quote_command "${starter_window_command[@]}")"
  exec "${starter_window_command[@]}"
fi

echo
echo "Window launch skipped. Pass --run-window to launch the manual starter gate."
