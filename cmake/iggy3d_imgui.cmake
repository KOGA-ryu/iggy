# Vendored Dear ImGui (docking branch) static library.
#
# Pin: v1.92.8-docking @ b61e56346a92cfcaf1f43a545ca37b0b32239654 — see
# third_party/imgui/README.md for the file-set hashes and update procedure.
#
# Third-party discipline (docs/creative_desktop_ui_plan.md DL-8):
#   - no iggy3d_apply_warnings on this target
#   - SYSTEM PUBLIC includes so consumer TUs see no third-party warnings
#   - only the DL-2 allowlist files may include these headers
#
# Requires IGGY3D_SDL3_TARGET / IGGY3D_VULKAN_TARGET from
# cmake/iggy3d_vulkan_deps.cmake — include this module after it.

set(IGGY3D_IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")

if(NOT IGGY3D_HAS_SDL3_TARGET OR NOT IGGY3D_HAS_VULKAN_TARGET)
  message(FATAL_ERROR "iggy3d_imgui requires SDL3 and Vulkan (include iggy3d_vulkan_deps.cmake first)")
endif()

add_library(iggy3d_imgui STATIC
  "${IGGY3D_IMGUI_ROOT}/imgui.cpp"
  "${IGGY3D_IMGUI_ROOT}/imgui_draw.cpp"
  "${IGGY3D_IMGUI_ROOT}/imgui_tables.cpp"
  "${IGGY3D_IMGUI_ROOT}/imgui_widgets.cpp"
  "${IGGY3D_IMGUI_ROOT}/backends/imgui_impl_sdl3.cpp"
  "${IGGY3D_IMGUI_ROOT}/backends/imgui_impl_vulkan.cpp"
)

target_include_directories(iggy3d_imgui SYSTEM PUBLIC
  "${IGGY3D_IMGUI_ROOT}"
  "${IGGY3D_IMGUI_ROOT}/backends"
)

target_link_libraries(iggy3d_imgui PUBLIC
  "${IGGY3D_SDL3_TARGET}"
  "${IGGY3D_VULKAN_TARGET}"
)
