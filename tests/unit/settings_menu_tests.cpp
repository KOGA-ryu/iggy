#include "app/frontend/SettingsMenu.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool settingsTabOrderIsExact() {
  const std::vector<iggy3d::FrontendSettingsTab>& tabs = iggy3d::settingsTabOrder();
  return expect(tabs.size() == 8U, "settings tab count") &&
         expect(tabs[0] == iggy3d::FrontendSettingsTab::Input, "input first") &&
         expect(tabs[1] == iggy3d::FrontendSettingsTab::Controls, "controls second") &&
         expect(tabs[2] == iggy3d::FrontendSettingsTab::Camera, "camera third") &&
         expect(tabs[3] == iggy3d::FrontendSettingsTab::Gameplay, "gameplay fourth") &&
         expect(tabs[4] == iggy3d::FrontendSettingsTab::VideoDisplay,
                "video display fifth") &&
         expect(tabs[5] == iggy3d::FrontendSettingsTab::Audio, "audio sixth") &&
         expect(tabs[6] == iggy3d::FrontendSettingsTab::Accessibility,
                "accessibility seventh") &&
         expect(tabs[7] == iggy3d::FrontendSettingsTab::Developer,
                "developer eighth");
}

bool settingsNamesAreStable() {
  return expect(iggy3d::frontendSettingsTabName(
                    iggy3d::FrontendSettingsTab::VideoDisplay) == "video_display",
                "video display tab name") &&
         expect(iggy3d::frontendCameraModeName(
                    iggy3d::FrontendCameraMode::FirstPerson) == "first_person",
                "first person camera name") &&
         expect(iggy3d::frontendRendererChoiceName(
                    iggy3d::FrontendRendererChoice::Vulkan) == "vulkan",
                "vulkan renderer name") &&
         expect(iggy3d::frontendWindowModeName(
                    iggy3d::FrontendWindowMode::NoWindow) == "no_window",
                "no window mode name") &&
         expect(iggy3d::defaultSettingsRowName(
                    iggy3d::FrontendSettingsTab::Developer) == "dev_tools_enabled",
                "developer default row");
}

bool defaultsArePacketDefaults() {
  const iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  return expect(settings.inputBackend == iggy3d::FrontendInputBackend::Keyboard,
                "default input") &&
         expect(!settings.gamepadAvailable, "default gamepad unavailable") &&
         expect(settings.lookSensitivity == 1.0F, "default look sensitivity") &&
         expect(!settings.invertLook, "default invert look") &&
         expect(settings.controllerLookSensitivity == 1.0F,
                "default controller sensitivity") &&
         expect(settings.cameraMode == iggy3d::FrontendCameraMode::FirstPerson,
                "default camera") &&
         expect(settings.renderer == iggy3d::FrontendRendererChoice::Auto,
                "default renderer") &&
         expect(settings.windowMode == iggy3d::FrontendWindowMode::NoWindow,
                "default window mode") &&
         expect(settings.masterVolume == 1.0F, "default volume") &&
         expect(!settings.audioAvailable, "audio unavailable") &&
         expect(settings.devToolsEnabled, "dev tools enabled") &&
         expect(settings.debugOverlayEnabled, "debug overlay enabled");
}

bool restoreAndApplyStayFrontendOnly() {
  iggy3d::FrontendSettings current = iggy3d::defaultFrontendSettings();
  iggy3d::FrontendSettings draft = current;
  draft.inputBackend = iggy3d::FrontendInputBackend::Gamepad;
  draft.lookSensitivity = 1.5F;
  draft.controllerLookSensitivity = 0.75F;
  draft.invertLook = true;
  draft.renderer = iggy3d::FrontendRendererChoice::Vulkan;
  iggy3d::applyFrontendSettingsDraft(current, draft);
  bool ok = expect(current.inputBackend == iggy3d::FrontendInputBackend::Gamepad,
                   "apply input") &&
            expect(current.lookSensitivity == 1.5F, "apply look") &&
            expect(current.controllerLookSensitivity == 0.75F,
                   "apply controller look") &&
            expect(current.invertLook, "apply invert") &&
            expect(current.rendererChangePending, "renderer change pending");
  iggy3d::restoreFrontendSettingsDefaults(current);
  ok = expect(current.inputBackend == iggy3d::FrontendInputBackend::Keyboard,
              "restore input") &&
       expect(current.lookSensitivity == 1.0F, "restore look") &&
       expect(!current.invertLook, "restore invert") &&
       expect(!current.rendererChangePending, "restore pending") && ok;
  return ok;
}

}  // namespace

int main() {
  const bool ok = settingsTabOrderIsExact() && settingsNamesAreStable() &&
                  defaultsArePacketDefaults() && restoreAndApplyStayFrontendOnly();
  return ok ? 0 : 1;
}
