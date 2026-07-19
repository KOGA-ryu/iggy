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

bool creativeDesktopShellEnabledForLaunch(bool desktopUiRequested,
                                          bool captureMode) noexcept {
  return desktopUiRequested && !captureMode;
}

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

namespace {

// Reserved height for the bottom status bar so the dockspace never overlaps it.
constexpr float kStatusBarReserveScale = 1.0F;

// Builds the default docked arrangement: Project left (~18%), Inspector right
// (~24%), Diagnostics bottom (~22%), a thin toolbar above the central 3D
// viewport. Ratios are of the shrinking node, so later splits compensate for
// earlier ones to hit the target fractions of the whole workspace.
void buildDefaultDesktopLayout(ImGuiID dockspaceId, ImVec2 workspaceSize) {
  ImGui::DockBuilderRemoveNode(dockspaceId);
  ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_PassthruCentralNode |
                                             ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspaceId, workspaceSize);

  ImGuiID centerId = dockspaceId;
  const ImGuiID leftId =
      ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, 0.18F, nullptr, &centerId);
  // Right is 24% of the whole; after the left split the remainder is 82%.
  const ImGuiID rightId = ImGui::DockBuilderSplitNode(
      centerId, ImGuiDir_Right, 0.24F / 0.82F, nullptr, &centerId);
  // Bottom is 22% of the whole height.
  const ImGuiID bottomId =
      ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.22F, nullptr, &centerId);
  // A thin toolbar strip above the central viewport.
  const float toolbarFraction =
      workspaceSize.y > 1.0F ? 44.0F / workspaceSize.y : 0.06F;
  const ImGuiID toolbarId = ImGui::DockBuilderSplitNode(
      centerId, ImGuiDir_Up, toolbarFraction, nullptr, &centerId);

  ImGui::DockBuilderDockWindow("Project", leftId);
  ImGui::DockBuilderDockWindow("Inspector", rightId);
  ImGui::DockBuilderDockWindow("Diagnostics##bottom", bottomId);
  ImGui::DockBuilderDockWindow("Toolbar##desktop", toolbarId);
  ImGui::DockBuilderDockWindow("World Layout", centerId);
  ImGui::DockBuilderFinish(dockspaceId);
}

}  // namespace

void layoutCreativeEditorDesktopDockspace(
    CreativeEditorDesktopUiState& desktopUi) {
  if (!desktopUi.frameActive) {
    return;
  }

  // Host spans the work area (below the menu bar) minus a reserved strip for
  // the bottom status bar, so docked panels never collide with either. The
  // host paints no background; the central node stays passthru so the 3D scene
  // shows through it while the docked panels frame it.
  const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
  const float statusBarHeight =
      ImGui::GetFrameHeight() * kStatusBarReserveScale;
  const ImVec2 hostPos = mainViewport->WorkPos;
  const ImVec2 hostSize(mainViewport->WorkSize.x,
                        mainViewport->WorkSize.y - statusBarHeight);
  ImGui::SetNextWindowPos(hostPos);
  ImGui::SetNextWindowSize(hostSize);
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
  if (!desktopUi.dockLayoutBuilt || desktopUi.resetLayoutRequested) {
    buildDefaultDesktopLayout(dockspaceId, hostSize);
    desktopUi.dockLayoutBuilt = true;
    desktopUi.resetLayoutRequested = false;
  }
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
    decision.consumePrimaryPress = true;
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
