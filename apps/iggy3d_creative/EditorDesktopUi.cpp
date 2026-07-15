#include "EditorDesktopUi.hpp"

#include "imgui.h"

#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {

bool beginCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi,
                                     iggy3d::VulkanBackend& backend) {
  desktopUi.frameActive = false;
  if (!desktopUi.shellEnabled) {
    return false;
  }
  if (!backend.beginExternalUiFrame()) {
    return false;
  }
  desktopUi.frameActive = true;

  // Full-window host with a passthru central node: the 3D scene renders
  // straight to the swapchain underneath, so the host window must not paint
  // a background and mouse input over the central node must reach the app.
  const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(mainViewport->WorkPos);
  ImGui::SetNextWindowSize(mainViewport->WorkSize);
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
  ImGui::End();
  return true;
}

void endCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi) {
  if (!desktopUi.frameActive) {
    return;
  }
  ImGui::Render();
  desktopUi.frameActive = false;
}

}  // namespace iggy3d_creative_app
