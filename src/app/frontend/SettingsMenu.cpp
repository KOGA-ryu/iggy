#include "app/frontend/SettingsMenu.hpp"

#include <algorithm>

namespace iggy3d {
namespace {

FrontendScreen parentScreenForSettings(MenuOwner parentOwner) {
  if (parentOwner == MenuOwner::Pause) {
    return FrontendScreen::Pause;
  }
  return FrontendScreen::Starter;
}

FrontendScreen settingsChildForParent(MenuOwner parentOwner) {
  if (parentOwner == MenuOwner::Pause) {
    return FrontendScreen::Pause;
  }
  return FrontendScreen::Starter;
}

bool validSettingsParent(MenuOwner parentOwner) {
  return parentOwner == MenuOwner::Starter || parentOwner == MenuOwner::Pause;
}

FrontendRouteResult invalidSettingsParentRoute(FrontendAction action) {
  FrontendRouteResult result = makeIgnoredFrontendRouteResult(
      MenuOwner::Settings,
      FrontendScreen::Settings,
      FrontendScreen::Gameplay,
      action);
  result.gameplayInputSuppressed = true;
  result.status = "settings_invalid_parent";
  result.receiptReason = "settings_invalid_parent";
  return result;
}

FrontendRouteResult ignoredSettingsRoute(const SettingsRouteContext& context,
                                         FrontendAction action,
                                         std::string_view status) {
  FrontendRouteResult result = makeIgnoredFrontendRouteResult(
      MenuOwner::Settings,
      FrontendScreen::Settings,
      settingsChildForParent(context.parentOwner),
      action);
  result.gameplayInputSuppressed = true;
  result.status = status;
  result.receiptReason = status;
  return result;
}

FrontendRouteResult acceptedSettingsStayRoute(const SettingsRouteContext& context,
                                              FrontendAction action,
                                              std::string_view status) {
  return makeAcceptedFrontendRouteResult(MenuOwner::Settings,
                                         FrontendScreen::Settings,
                                         settingsChildForParent(context.parentOwner),
                                         FrontendTransitionRequest::None,
                                         false,
                                         true,
                                         status,
                                         status,
                                         action);
}

}  // namespace

std::string_view frontendInputBackendName(FrontendInputBackend backend) {
  switch (backend) {
    case FrontendInputBackend::Keyboard:
      return "keyboard";
    case FrontendInputBackend::Gamepad:
      return "gamepad";
    case FrontendInputBackend::Auto:
      return "auto";
    case FrontendInputBackend::Scripted:
      return "scripted";
  }
  return "keyboard";
}

std::string_view frontendSettingsTabName(FrontendSettingsTab tab) {
  switch (tab) {
    case FrontendSettingsTab::None:
      return "none";
    case FrontendSettingsTab::Input:
      return "input";
    case FrontendSettingsTab::Controls:
      return "controls";
    case FrontendSettingsTab::Camera:
      return "camera";
    case FrontendSettingsTab::Gameplay:
      return "gameplay";
    case FrontendSettingsTab::VideoDisplay:
      return "video_display";
    case FrontendSettingsTab::Audio:
      return "audio";
    case FrontendSettingsTab::Accessibility:
      return "accessibility";
    case FrontendSettingsTab::Developer:
      return "developer";
  }
  return "none";
}

std::string_view frontendCameraModeName(FrontendCameraMode mode) {
  switch (mode) {
    case FrontendCameraMode::FirstPerson:
      return "first_person";
    case FrontendCameraMode::ThirdPerson:
      return "third_person";
    case FrontendCameraMode::Tactical:
      return "tactical";
  }
  return "first_person";
}

std::string_view frontendRendererChoiceName(FrontendRendererChoice renderer) {
  switch (renderer) {
    case FrontendRendererChoice::Auto:
      return "auto";
    case FrontendRendererChoice::Null:
      return "null";
    case FrontendRendererChoice::Vulkan:
      return "vulkan";
  }
  return "auto";
}

std::string_view frontendWindowModeName(FrontendWindowMode mode) {
  switch (mode) {
    case FrontendWindowMode::NoWindow:
      return "no_window";
    case FrontendWindowMode::Window:
      return "window";
  }
  return "no_window";
}

const std::vector<FrontendSettingsTab>& settingsTabOrder() {
  static const std::vector<FrontendSettingsTab> tabs = {
      FrontendSettingsTab::Input,
      FrontendSettingsTab::Controls,
      FrontendSettingsTab::Camera,
      FrontendSettingsTab::Gameplay,
      FrontendSettingsTab::VideoDisplay,
      FrontendSettingsTab::Audio,
      FrontendSettingsTab::Accessibility,
      FrontendSettingsTab::Developer,
  };
  return tabs;
}

FrontendSettingsTab nextSettingsTab(FrontendSettingsTab current) {
  const auto& tabs = settingsTabOrder();
  const auto currentIt = std::find(tabs.begin(), tabs.end(), current);
  if (currentIt == tabs.end()) {
    return FrontendSettingsTab::Input;
  }
  const auto nextIt = currentIt + 1;
  return nextIt == tabs.end() ? tabs.back() : *nextIt;
}

FrontendSettingsTab previousSettingsTab(FrontendSettingsTab current) {
  const auto& tabs = settingsTabOrder();
  const auto currentIt = std::find(tabs.begin(), tabs.end(), current);
  if (currentIt == tabs.end() || currentIt == tabs.begin()) {
    return FrontendSettingsTab::Input;
  }
  return *(currentIt - 1);
}

std::string_view defaultSettingsRowName(FrontendSettingsTab tab) {
  switch (tab) {
    case FrontendSettingsTab::Input:
      return "input_backend";
    case FrontendSettingsTab::Controls:
      return "look_sensitivity";
    case FrontendSettingsTab::Camera:
      return "camera_mode";
    case FrontendSettingsTab::Gameplay:
      return "difficulty";
    case FrontendSettingsTab::VideoDisplay:
      return "renderer";
    case FrontendSettingsTab::Audio:
      return "master_volume";
    case FrontendSettingsTab::Accessibility:
      return "high_contrast";
    case FrontendSettingsTab::Developer:
      return "dev_tools_enabled";
    case FrontendSettingsTab::None:
      break;
  }
  return "none";
}

SettingsRowModel defaultSettingsRowModel(FrontendSettingsTab tab,
                                         const FrontendSettings& settings) {
  SettingsRowModel model;
  model.row = defaultSettingsRowName(tab);
  model.persistence = "runtime_only";
  switch (tab) {
    case FrontendSettingsTab::Audio:
      model.enabled = settings.audioAvailable;
      model.disabledReason = model.enabled ? "none" : "audio_unavailable";
      break;
    case FrontendSettingsTab::VideoDisplay:
      model.enabled = true;
      model.disabledReason = "none";
      model.persistence = "runtime_only";
      break;
    case FrontendSettingsTab::Gameplay:
      model.enabled = false;
      model.disabledReason = "read_only_v1";
      model.persistence = "runtime_only";
      break;
    case FrontendSettingsTab::None:
      model.enabled = false;
      model.disabledReason = "not_settings_row";
      model.persistence = "deferred";
      break;
    case FrontendSettingsTab::Input:
    case FrontendSettingsTab::Controls:
    case FrontendSettingsTab::Camera:
    case FrontendSettingsTab::Accessibility:
    case FrontendSettingsTab::Developer:
      model.enabled = true;
      model.disabledReason = "none";
      break;
  }
  return model;
}

FrontendRouteResult routeSettingsBackToParent(MenuOwner parentOwner) {
  if (!validSettingsParent(parentOwner)) {
    return invalidSettingsParentRoute(FrontendAction::Back);
  }
  return makeAcceptedFrontendRouteResult(MenuOwner::Settings,
                                         parentScreenForSettings(parentOwner),
                                         FrontendScreen::Gameplay,
                                         FrontendTransitionRequest::None,
                                         false,
                                         true,
                                         "settings_back_requested",
                                         "settings_back_requested",
                                         FrontendAction::Back);
}

FrontendRouteResult routeSettingsAction(const SettingsRouteContext& context,
                                        FrontendAction action) {
  if (!validSettingsParent(context.parentOwner)) {
    return invalidSettingsParentRoute(action);
  }

  switch (action) {
    case FrontendAction::Apply:
      if (!context.selectedRow.enabled) {
        return ignoredSettingsRoute(context, action, context.selectedRow.disabledReason);
      }
      if (!context.dirty) {
        return ignoredSettingsRoute(context, action, "settings_no_changes");
      }
      return acceptedSettingsStayRoute(context, action, "settings_apply_requested");
    case FrontendAction::RestoreDefaults:
      return acceptedSettingsStayRoute(context, action,
                                       "settings_restore_defaults_requested");
    case FrontendAction::Back:
      return routeSettingsBackToParent(context.parentOwner);
    case FrontendAction::None:
    case FrontendAction::Continue:
    case FrontendAction::NewWorld:
    case FrontendAction::LoadSave:
    case FrontendAction::Settings:
    case FrontendAction::DevTools:
    case FrontendAction::Exit:
    case FrontendAction::CreateAndEnter:
    case FrontendAction::Load:
    case FrontendAction::Delete:
    case FrontendAction::Resume:
    case FrontendAction::EditRoom:
    case FrontendAction::LeaveEditor:
    case FrontendAction::Save:
    case FrontendAction::SaveAndExit:
    case FrontendAction::ReturnToTitle:
    case FrontendAction::ExitGame:
      break;
  }

  return ignoredSettingsRoute(context, action, "not_settings_action");
}

FrontendSettings defaultFrontendSettings() {
  return {};
}

void restoreFrontendSettingsDefaults(FrontendSettings& settings) {
  settings = defaultFrontendSettings();
}

void applyFrontendSettingsDraft(FrontendSettings& current,
                                const FrontendSettings& draft) {
  current = draft;
  current.rendererChangePending = current.renderer != defaultFrontendSettings().renderer;
}

}  // namespace iggy3d
