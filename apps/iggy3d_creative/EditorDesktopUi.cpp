#include "EditorDesktopUi.hpp"

#include <cmath>

#include "imgui.h"
#include "imgui_internal.h"

#include "app/iggy3d/creative/input/UiInput.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {
namespace {

// Reads the central dock node rect and maps it to a drawable-pixel content
// viewport. Returns the full-frame sentinel until docked panels actually
// shrink the central node, so the scene keeps rendering to the whole window.
iggy3d::RenderContentViewport resolveCentralNodeContentViewport(
    ImGuiID dockspaceId) {
  const ImGuiIO& io = ImGui::GetIO();
  const float scaleX = io.DisplayFramebufferScale.x;
  const float scaleY = io.DisplayFramebufferScale.y;
  const auto drawableWidth =
      static_cast<std::uint32_t>(std::lround(io.DisplaySize.x * scaleX));
  const auto drawableHeight =
      static_cast<std::uint32_t>(std::lround(io.DisplaySize.y * scaleY));
  const ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockspaceId);
  if (central == nullptr || drawableWidth == 0U || drawableHeight == 0U) {
    return {};
  }
  const iggy3d::creative::CreativeContentRect rect =
      iggy3d::creative::resolveCreativeContentViewport(
          central->Pos.x, central->Pos.y, central->Pos.x + central->Size.x,
          central->Pos.y + central->Size.y, scaleX, scaleY, drawableWidth,
          drawableHeight);
  // Full-window central node collapses to the sentinel (cheaper, and keeps the
  // scene + capture byte-identical); only an actual sub-rect is carried.
  if (!rect.valid ||
      (rect.x == 0 && rect.y == 0 && rect.width == drawableWidth &&
       rect.height == drawableHeight)) {
    return {};
  }
  return {rect.x, rect.y, rect.width, rect.height};
}

}  // namespace

bool beginCreativeEditorDesktopUiFrame(CreativeEditorDesktopUiState& desktopUi,
                                       iggy3d::VulkanBackend& backend) {
  desktopUi.frameActive = false;
  desktopUi.contentViewport = {};  // sentinel: scene fills the whole window
  if (!desktopUi.shellEnabled) {
    return false;
  }
  if (!backend.beginExternalUiFrame()) {
    return false;
  }
  desktopUi.frameActive = true;
  return true;
}

void layoutCreativeEditorDesktopDockspace(
    CreativeEditorDesktopUiState& desktopUi) {
  if (!desktopUi.frameActive) {
    return;
  }

  // Full-window host with a passthru central node: the 3D scene renders
  // straight to the swapchain underneath, so the host window must not paint
  // a background and mouse input over the central node must reach the app.
  // Uses the FULL viewport (not WorkPos/WorkSize) so the main menu bar and
  // status bar overlay the scene edges rather than shrinking the central node
  // — the content rect stays full-frame, keeping the scene, crosshair, and
  // pick ray aligned until UI-3 introduces real side panels and the app-side
  // content-rect fan-out together.
  const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(mainViewport->Pos);
  ImGui::SetNextWindowSize(mainViewport->Size);
  const ImGuiWindowFlags hostFlags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
  ImGui::Begin("##creative_desktop_shell", nullptr, hostFlags);
  ImGui::PopStyleVar(3);
  const ImGuiID dockspaceId = ImGui::GetID("creative_desktop_dockspace");
  ImGui::DockSpace(dockspaceId, ImVec2(0.0F, 0.0F),
                   ImGuiDockNodeFlags_PassthruCentralNode);
  desktopUi.contentViewport = resolveCentralNodeContentViewport(dockspaceId);
  ImGui::End();
}

void endCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi) {
  if (!desktopUi.frameActive) {
    return;
  }
  ImGui::Render();
  desktopUi.frameActive = false;
}

CreativeDesktopPointerDecision decideCreativeDesktopPointerCapture(
    bool shellEnabled,
    bool currentlyCaptured,
    bool primaryPressedOverViewport,
    bool viewportContext,
    bool windowFocused) noexcept {
  CreativeDesktopPointerDecision decision;
  if (!shellEnabled) {
    decision.captured = false;
    decision.changed = currentlyCaptured;
    return decision;
  }
  bool next = currentlyCaptured;
  if (currentlyCaptured) {
    if (!windowFocused || !viewportContext) {
      next = false;
    }
  } else if (windowFocused && viewportContext && primaryPressedOverViewport) {
    next = true;
  }
  decision.captured = next;
  decision.changed = next != currentlyCaptured;
  return decision;
}

bool creativeDesktopUiWantsInput(bool viewportPointerCaptured,
                                 bool imguiWantsMouse,
                                 bool imguiWantsKeyboard) noexcept {
  return !viewportPointerCaptured && (imguiWantsMouse || imguiWantsKeyboard);
}

}  // namespace iggy3d_creative_app
