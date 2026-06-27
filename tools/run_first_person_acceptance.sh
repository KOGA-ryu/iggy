#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: tools/run_first_person_acceptance.sh [options]

Configures and builds the Vulkan-enabled product app, then prints the manual
first-person acceptance launch command and receipt fields to inspect.

The default launch command enters gameplay with --auto-new-world. A
starter-screen run is a Vulkan menu/UI draw-list diagnostic; gameplay acceptance
still requires entering gameplay or using --auto-new-world.

Default behavior does not launch a window.

Options:
  --run                 Launch the product window after configure/build.
  --starter-menu        Do not add --auto-new-world. Menu/UI diagnostic only.
  --build-dir PATH      Build directory. Default: build-vulkan
  --save-root PATH      Save root for the manual run. Default: $HOME/.iggy3d/saves
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

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
build_dir="${repo_root}/build-vulkan"
save_root="${HOME}/.iggy3d/saves"
jobs="${IGGY3D_JOBS:-8}"
run_window=0
auto_new_world=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --run)
      run_window=1
      shift
      ;;
    --starter-menu)
      auto_new_world=0
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

case "${save_root}" in
  /*) ;;
  *) save_root="${repo_root}/${save_root}" ;;
esac

configure_command=(cmake -S "${repo_root}" -B "${build_dir}" -DIGGY3D_ENABLE_VULKAN=ON)
build_command=(cmake --build "${build_dir}" --target iggy3d_app -j "${jobs}")
app_binary="${build_dir}/iggy3d"
launch_command=("${app_binary}" --window --renderer vulkan --input auto
                --save-root "${save_root}" --print-render-receipt)
if [[ "${auto_new_world}" -eq 1 ]]; then
  launch_command+=(--auto-new-world)
fi

echo "== Vulkan first-person acceptance setup =="
echo "Repo: ${repo_root}"
echo "Build dir: ${build_dir}"
echo
echo "+ $(quote_command "${configure_command[@]}")"
"${configure_command[@]}"
echo
echo "+ $(quote_command "${build_command[@]}")"
"${build_command[@]}"
echo
echo "Manual launch command:"
quote_command "${launch_command[@]}"
if [[ "${auto_new_world}" -eq 1 ]]; then
  cat <<'POLICY'

Launch policy:
  --auto-new-world is included so the Vulkan renderer receives an active gameplay
  room mesh. Without it, the app can remain on the starter screen; that path now
  proves starter UI draw-list readiness, not first-person gameplay readiness.
POLICY
else
  cat <<'POLICY'

Launch policy:
  --starter-menu was requested. This checks the Vulkan starter menu path builds
  product UI primitives. First-person room rendering still requires gameplay.
POLICY
fi
echo
cat <<'FIELDS'
Readiness receipt fields to inspect:
  auto_new_world=true
  frontend_screen=gameplay
  gameplay_active=true
  active_room_loaded=true
  renderer_request=vulkan
  product_vulkan_backend_built=true
  product_vulkan_renderer_requested=true
  product_vulkan_renderer_created=true
  product_vulkan_renderer_ready=true
  product_vulkan_frame_submitted=true
  product_vulkan_rendering_path=package_room_meshes
  product_vulkan_record_mode=room_mesh_draws
  product_vulkan_room_mesh_backend_presented=true
  product_vulkan_gameplay_ready=true
  product_vulkan_gameplay_status=product_vulkan_gameplay_ready
  product_vulkan_gameplay_reason_code=product_vulkan_gameplay_ready
  product_vulkan_menu_requested=false
  product_vulkan_menu_visible=false
  top_down_map_purpose=minimap

Starter-menu diagnostic fields:
  frontend_screen=starter
  gameplay_active=false
  product_vulkan_menu_requested=true
  product_vulkan_menu_visible=true
  product_vulkan_menu_status=product_vulkan_menu_ui_ready
  product_vulkan_menu_reason_code=product_ui_draw_list_ready
  product_vulkan_menu_surface=starter
  product_vulkan_menu_ui_ready=true
  product_vulkan_menu_ui_status=product_ui_draw_list_ready
  product_vulkan_menu_ui_primitive_count=22
  product_vulkan_menu_ui_text_count=11
  product_vulkan_menu_ui_rect_count=11
  product_vulkan_menu_ui_row_count=7

Blocker examples:
  frontend_screen=starter
  gameplay_active=false
  active_room_loaded=false
  product_vulkan_menu_ui_ready=false
  product_vulkan_menu_status=product_vulkan_menu_ui_not_ready
  product_vulkan_backend_built=false
  product_vulkan_gameplay_status=product_vulkan_backend_unavailable
  product_vulkan_gameplay_status=product_vulkan_renderer_unavailable
  product_vulkan_gameplay_status=product_vulkan_room_mesh_cpu_not_ready
  product_vulkan_gameplay_status=product_vulkan_frame_not_submitted
  product_vulkan_gameplay_status=product_vulkan_room_mesh_not_presented
FIELDS

if [[ "${run_window}" -eq 1 ]]; then
  mkdir -p "${save_root}"
  echo
  echo "Launching window:"
  echo "+ $(quote_command "${launch_command[@]}")"
  exec "${launch_command[@]}"
fi

echo
echo "Window launch skipped. Pass --run to execute the manual window gate."
