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
  if (!desktopUi.shellEnabled) {
    desktopUi.contentViewport = {};  // sentinel: scene fills the whole window
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

// Builds the docked arrangement. World Layout receives a sibling drafting node
// rather than consuming the central passthrough node, so 2D authoring and 3D
// inspection remain visible together.
void buildDefaultDesktopLayout(ImGuiID dockspaceId, ImVec2 workspaceSize,
                               CreativeDesktopDockLayoutSpec layout) {
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

  ImGuiID worldLayoutId = 0U;
  if (layout.worldLayoutPanelVisible) {
    worldLayoutId = ImGui::DockBuilderSplitNode(
        centerId, ImGuiDir_Left, layout.worldLayoutPanelFraction, nullptr,
        &centerId);
  }

  ImGui::DockBuilderDockWindow("Project", leftId);
  ImGui::DockBuilderDockWindow("Inspector", rightId);
  ImGui::DockBuilderDockWindow("Diagnostics##bottom", bottomId);
  ImGui::DockBuilderDockWindow("History##bottom", bottomId);
  ImGui::DockBuilderDockWindow("Toolbar##desktop", toolbarId);
  if (worldLayoutId != 0U) {
    ImGui::DockBuilderDockWindow("World Layout", worldLayoutId);
  }
  ImGui::DockBuilderFinish(dockspaceId);
}

}  // namespace

CreativeDesktopDockLayoutSpec creativeDesktopDockLayoutSpec(
    bool showWorldLayout) noexcept {
  if (showWorldLayout) {
    return {CreativeDesktopDockLayoutMode::WorldLayoutSplit,
            /*worldLayoutPanelVisible=*/true,
            /*threeDimensionalViewportVisible=*/true,
            /*worldLayoutPanelFraction=*/0.5F};
  }
  return {CreativeDesktopDockLayoutMode::Standard,
          /*worldLayoutPanelVisible=*/false,
          /*threeDimensionalViewportVisible=*/true,
          /*worldLayoutPanelFraction=*/0.0F};
}

bool creativeDesktopDockLayoutRebuildRequired(
    bool dockLayoutBuilt, bool resetLayoutRequested) noexcept {
  return !dockLayoutBuilt || resetLayoutRequested;
}

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
  if (creativeDesktopDockLayoutRebuildRequired(
          desktopUi.dockLayoutBuilt, desktopUi.resetLayoutRequested)) {
    // Always seed the latent World Layout dock. ImGui collapses its empty node
    // while the panel is hidden and restores the same user-adjusted node when
    // it reopens; visibility changes must never rebuild the dockspace.
    buildDefaultDesktopLayout(dockspaceId, hostSize,
                              creativeDesktopDockLayoutSpec(true));
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
    CreativeDesktopPointerCaptureMode currentMode,
    bool primaryPressedOverViewport,
    bool primaryDown,
    iggy3d::creative::CreativeInputModifierMask modifiers,
    bool viewportContext,
    bool windowFocused) noexcept {
  CreativeDesktopPointerDecision decision;
  const bool orbitModifier =
      (modifiers & iggy3d::creative::kCreativeInputModifierAlt) != 0U;
  const bool panModifier =
      orbitModifier &&
      (modifiers & iggy3d::creative::kCreativeInputModifierShift) != 0U;
  if (!shellEnabled) {
    decision.changed = creativeDesktopPointerCaptured(currentMode);
    return decision;
  }
  CreativeDesktopPointerCaptureMode next = currentMode;
  if (creativeDesktopPointerCaptured(currentMode)) {
    const bool dragGesture =
        creativeDesktopPointerModeOwnsPrimaryAction(currentMode);
    if (!windowFocused || !viewportContext ||
        (dragGesture && !primaryDown)) {
      next = CreativeDesktopPointerCaptureMode::None;
    }
  } else if (windowFocused && viewportContext && primaryPressedOverViewport) {
    next = panModifier ? CreativeDesktopPointerCaptureMode::Pan
                       : orbitModifier
                             ? CreativeDesktopPointerCaptureMode::Orbit
                             : CreativeDesktopPointerCaptureMode::FlyLook;
    decision.consumePrimaryPress = true;
    decision.discardNextMouseDelta = true;
  }
  decision.mode = next;
  decision.captured = creativeDesktopPointerCaptured(next);
  decision.changed = next != currentMode;
  return decision;
}

bool creativeDesktopPointerCaptured(
    CreativeDesktopPointerCaptureMode mode) noexcept {
  return mode > CreativeDesktopPointerCaptureMode::None &&
         mode < CreativeDesktopPointerCaptureMode::Count;
}

bool creativeDesktopPointerModeOwnsPrimaryAction(
    CreativeDesktopPointerCaptureMode mode) noexcept {
  return mode == CreativeDesktopPointerCaptureMode::Orbit ||
         mode == CreativeDesktopPointerCaptureMode::Pan;
}

bool creativeDesktopMouseLookActive(
    bool shellEnabled,
    CreativeDesktopPointerCaptureMode mode) noexcept {
  return !shellEnabled || mode == CreativeDesktopPointerCaptureMode::FlyLook;
}

bool creativeDesktopViewportDollyRequested(
    CreativeDesktopPointerCaptureMode mode,
    iggy3d::creative::CreativeInputModifierMask modifiers,
    float wheelDelta,
    bool viewportContext) noexcept {
  return viewportContext && mode == CreativeDesktopPointerCaptureMode::None &&
         (modifiers & iggy3d::creative::kCreativeInputModifierAlt) != 0U &&
         std::isfinite(wheelDelta) && std::fabs(wheelDelta) > 1.0e-4F;
}

bool creativeDesktopUiWantsInput(
                                 CreativeDesktopPointerCaptureMode pointerCaptureMode,
                                 bool imguiWantsMouse,
                                 bool imguiWantsKeyboard) noexcept {
  return !creativeDesktopPointerCaptured(pointerCaptureMode) &&
         (imguiWantsMouse || imguiWantsKeyboard);
}

}  // namespace iggy3d_creative_app
