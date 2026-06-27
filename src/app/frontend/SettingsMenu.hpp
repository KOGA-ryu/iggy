#pragma once

#include <vector>
#include <string_view>

#include "app/frontend/FrontendRoute.hpp"

namespace iggy3d {

enum class FrontendInputBackend {
  Keyboard,
  Gamepad,
  Auto,
  Scripted,
};

enum class FrontendSettingsTab {
  None,
  Input,
  Controls,
  Camera,
  Gameplay,
  VideoDisplay,
  Audio,
  Accessibility,
  Developer,
};

enum class FrontendCameraMode {
  FirstPerson,
  ThirdPerson,
  Tactical,
};

enum class FrontendRendererChoice {
  Auto,
  Null,
  Vulkan,
};

enum class FrontendWindowMode {
  NoWindow,
  Window,
};

struct FrontendSettings {
  FrontendInputBackend inputBackend = FrontendInputBackend::Keyboard;
  bool gamepadAvailable = false;
  float lookSensitivity = 1.0F;
  bool invertLook = false;
  float controllerLookSensitivity = 1.0F;
  FrontendCameraMode cameraMode = FrontendCameraMode::FirstPerson;
  bool pauseOnFocusLoss = false;
  std::string_view difficulty = "normal";
  FrontendRendererChoice renderer = FrontendRendererChoice::Auto;
  FrontendWindowMode windowMode = FrontendWindowMode::NoWindow;
  float masterVolume = 1.0F;
  bool audioAvailable = false;
  bool highContrast = false;
  bool reducedMotion = false;
  bool devToolsEnabled = true;
  bool debugOverlayEnabled = false;
  bool rendererChangePending = false;
};

struct SettingsRowModel {
  std::string_view row = "none";
  bool enabled = true;
  std::string_view disabledReason = "none";
  std::string_view persistence = "runtime_only";
};

struct SettingsRouteContext {
  MenuOwner parentOwner = MenuOwner::Starter;
  FrontendSettingsTab selectedTab = FrontendSettingsTab::Input;
  SettingsRowModel selectedRow;
  bool dirty = false;
};

std::string_view frontendInputBackendName(FrontendInputBackend backend);
std::string_view frontendSettingsTabName(FrontendSettingsTab tab);
std::string_view frontendCameraModeName(FrontendCameraMode mode);
std::string_view frontendRendererChoiceName(FrontendRendererChoice renderer);
std::string_view frontendWindowModeName(FrontendWindowMode mode);
const std::vector<FrontendSettingsTab>& settingsTabOrder();
FrontendSettingsTab nextSettingsTab(FrontendSettingsTab current);
FrontendSettingsTab previousSettingsTab(FrontendSettingsTab current);
std::string_view defaultSettingsRowName(FrontendSettingsTab tab);
SettingsRowModel defaultSettingsRowModel(FrontendSettingsTab tab,
                                         const FrontendSettings& settings);
FrontendRouteResult routeSettingsAction(const SettingsRouteContext& context,
                                        FrontendAction action);
FrontendRouteResult routeSettingsBackToParent(MenuOwner parentOwner);
FrontendSettings defaultFrontendSettings();
void restoreFrontendSettingsDefaults(FrontendSettings& settings);
void applyFrontendSettingsDraft(FrontendSettings& current,
                                const FrontendSettings& draft);

}  // namespace iggy3d
