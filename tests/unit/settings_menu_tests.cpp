#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"

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
         expect(!settings.debugOverlayEnabled, "debug overlay disabled by default");
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

bool settingsRowsHaveUsefulState() {
  const iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  const iggy3d::SettingsRowModel audio =
      iggy3d::defaultSettingsRowModel(iggy3d::FrontendSettingsTab::Audio, settings);
  const iggy3d::SettingsRowModel input =
      iggy3d::defaultSettingsRowModel(iggy3d::FrontendSettingsTab::Input, settings);
  const iggy3d::SettingsRowModel gameplay =
      iggy3d::defaultSettingsRowModel(iggy3d::FrontendSettingsTab::Gameplay, settings);
  return expect(input.row == "input_backend", "input row") &&
         expect(input.enabled, "input enabled") &&
         expect(input.persistence == "runtime_only", "input runtime only") &&
         expect(!audio.enabled, "audio disabled without audio system") &&
         expect(audio.disabledReason == "audio_unavailable",
                "audio disabled reason") &&
         expect(gameplay.enabled, "gameplay runtime tuning enabled") &&
         expect(gameplay.disabledReason == "none",
                "gameplay runtime tuning reason");
}

bool movementTuningFieldDescriptorsAreStable() {
  iggy3d::ProductGameplayMovementTuning tuning =
      iggy3d::productGameplayMovementTuning();
  const float walk = tuning.walkSpeedMetersPerSecond;
  const float adjusted = iggy3d::adjustProductGameplayMovementTuning(
      tuning, iggy3d::ProductGameplayMovementTuningField::WalkSpeed, 1);
  const float invert = iggy3d::adjustProductGameplayMovementTuning(
      tuning, iggy3d::ProductGameplayMovementTuningField::InvertLook, 1);
  return expect(iggy3d::productGameplayMovementTuningFieldCount() == 12U,
                "movement tuning field count") &&
         expect(iggy3d::productGameplayMovementTuningFieldName(
                    iggy3d::ProductGameplayMovementTuningField::WalkSpeed) ==
                    "walk_speed_mps",
                "walk speed field name") &&
         expect(iggy3d::nextProductGameplayMovementTuningField(
                    iggy3d::ProductGameplayMovementTuningField::WalkSpeed) ==
                    iggy3d::ProductGameplayMovementTuningField::SprintSpeed,
                "next movement tuning field") &&
         expect(iggy3d::previousProductGameplayMovementTuningField(
                    iggy3d::ProductGameplayMovementTuningField::WalkSpeed) ==
                    iggy3d::ProductGameplayMovementTuningField::DashCooldown,
                "previous movement tuning wraps") &&
         expect(adjusted > walk, "walk tuning increments") &&
         expect(tuning.walkSpeedMetersPerSecond == adjusted,
                "walk tuning stores increment") &&
         expect(invert == 1.0F, "invert tuning toggles on") &&
         expect(iggy3d::productGameplayMovementTuningInvertLook(tuning),
                "invert tuning bool helper");
}

bool settingsTabNavigationClamps() {
  return expect(iggy3d::previousSettingsTab(iggy3d::FrontendSettingsTab::Input) ==
                    iggy3d::FrontendSettingsTab::Input,
                "previous input clamps") &&
         expect(iggy3d::nextSettingsTab(iggy3d::FrontendSettingsTab::Input) ==
                    iggy3d::FrontendSettingsTab::Controls,
                "next input controls") &&
         expect(iggy3d::previousSettingsTab(iggy3d::FrontendSettingsTab::Controls) ==
                    iggy3d::FrontendSettingsTab::Input,
                "previous controls input") &&
         expect(iggy3d::nextSettingsTab(iggy3d::FrontendSettingsTab::Developer) ==
                    iggy3d::FrontendSettingsTab::Developer,
                "next developer clamps") &&
         expect(iggy3d::nextSettingsTab(iggy3d::FrontendSettingsTab::None) ==
                    iggy3d::FrontendSettingsTab::Input,
                "next none maps input") &&
         expect(iggy3d::previousSettingsTab(iggy3d::FrontendSettingsTab::None) ==
                    iggy3d::FrontendSettingsTab::Input,
                "previous none maps input");
}

bool settingsApplyRoutesArePure() {
  iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  iggy3d::SettingsRouteContext context;
  context.parentOwner = iggy3d::MenuOwner::Starter;
  context.selectedTab = iggy3d::FrontendSettingsTab::Input;
  context.selectedRow =
      iggy3d::defaultSettingsRowModel(iggy3d::FrontendSettingsTab::Input, settings);
  context.dirty = true;
  settings.inputBackend = iggy3d::FrontendInputBackend::Keyboard;

  const auto applied =
      iggy3d::routeSettingsAction(context, iggy3d::FrontendAction::Apply);
  bool ok = expect(applied.accepted, "dirty apply accepted") &&
            expect(applied.inputOwner == iggy3d::MenuOwner::Settings,
                   "dirty apply owner") &&
            expect(applied.nextScreen == iggy3d::FrontendScreen::Settings,
                   "dirty apply stays settings") &&
            expect(applied.nextChildScreen == iggy3d::FrontendScreen::Starter,
                   "dirty apply starter parent") &&
            expect(applied.requestedTransition ==
                       iggy3d::FrontendTransitionRequest::None,
                   "dirty apply no transition") &&
            expect(!applied.closeRequested, "dirty apply no close") &&
            expect(applied.gameplayInputSuppressed, "dirty apply suppresses gameplay") &&
            expect(applied.status == "settings_apply_requested",
                   "dirty apply status") &&
            expect(applied.selectedAction == iggy3d::FrontendAction::Apply,
                   "dirty apply action") &&
            expect(settings.inputBackend == iggy3d::FrontendInputBackend::Keyboard,
                   "dirty apply helper does not mutate settings");

  context.dirty = false;
  const auto clean =
      iggy3d::routeSettingsAction(context, iggy3d::FrontendAction::Apply);
  ok = expect(!clean.accepted, "clean apply ignored") &&
       expect(clean.status == "settings_no_changes", "clean apply status") &&
       expect(clean.requestedTransition == iggy3d::FrontendTransitionRequest::None,
              "clean apply transition") && ok;

  context.selectedTab = iggy3d::FrontendSettingsTab::Audio;
  context.selectedRow =
      iggy3d::defaultSettingsRowModel(iggy3d::FrontendSettingsTab::Audio, settings);
  context.dirty = true;
  const auto disabled =
      iggy3d::routeSettingsAction(context, iggy3d::FrontendAction::Apply);
  ok = expect(!disabled.accepted, "disabled apply ignored") &&
       expect(disabled.status == "audio_unavailable", "disabled apply status") &&
       expect(disabled.receiptReason == "audio_unavailable",
              "disabled apply reason") && ok;

  return ok;
}

bool settingsRestoreAndBackRoutesArePure() {
  const iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  iggy3d::SettingsRouteContext context;
  context.parentOwner = iggy3d::MenuOwner::Pause;
  context.selectedTab = iggy3d::FrontendSettingsTab::Controls;
  context.selectedRow =
      iggy3d::defaultSettingsRowModel(iggy3d::FrontendSettingsTab::Controls, settings);
  context.dirty = false;

  const auto restored =
      iggy3d::routeSettingsAction(context, iggy3d::FrontendAction::RestoreDefaults);
  bool ok = expect(restored.accepted, "restore accepted") &&
            expect(restored.nextScreen == iggy3d::FrontendScreen::Settings,
                   "restore stays settings") &&
            expect(restored.nextChildScreen == iggy3d::FrontendScreen::Pause,
                   "restore pause parent") &&
            expect(restored.status == "settings_restore_defaults_requested",
                   "restore status") &&
            expect(restored.selectedAction == iggy3d::FrontendAction::RestoreDefaults,
                   "restore action");

  const auto backPause =
      iggy3d::routeSettingsAction(context, iggy3d::FrontendAction::Back);
  ok = expect(backPause.accepted, "back pause accepted") &&
       expect(backPause.inputOwner == iggy3d::MenuOwner::Settings,
              "back pause settings owner") &&
       expect(backPause.nextScreen == iggy3d::FrontendScreen::Pause,
              "back pause next screen") &&
       expect(backPause.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
              "back pause child sentinel") &&
       expect(backPause.gameplayInputSuppressed, "back pause suppresses gameplay") &&
       expect(backPause.status == "settings_back_requested", "back pause status") &&
       expect(backPause.selectedAction == iggy3d::FrontendAction::Back,
              "back pause action") && ok;

  const auto backStarter = iggy3d::routeSettingsBackToParent(iggy3d::MenuOwner::Starter);
  ok = expect(backStarter.accepted, "back starter accepted") &&
       expect(backStarter.nextScreen == iggy3d::FrontendScreen::Starter,
              "back starter screen") &&
       expect(backStarter.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
              "back starter child sentinel") &&
       expect(backStarter.gameplayInputSuppressed,
              "back starter suppresses gameplay") && ok;

  return ok;
}

bool invalidAndUnsupportedSettingsRoutesAreIgnored() {
  const iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  iggy3d::SettingsRouteContext context;
  context.parentOwner = iggy3d::MenuOwner::Gameplay;
  context.selectedRow =
      iggy3d::defaultSettingsRowModel(iggy3d::FrontendSettingsTab::Input, settings);
  context.dirty = true;

  const auto invalid =
      iggy3d::routeSettingsAction(context, iggy3d::FrontendAction::Apply);
  bool ok = expect(!invalid.accepted, "invalid parent ignored") &&
            expect(invalid.inputOwner == iggy3d::MenuOwner::Settings,
                   "invalid parent owner") &&
            expect(invalid.status == "settings_invalid_parent",
                   "invalid parent status") &&
            expect(invalid.gameplayInputSuppressed,
                   "invalid parent suppresses gameplay");

  context.parentOwner = iggy3d::MenuOwner::Starter;
  const auto unsupported =
      iggy3d::routeSettingsAction(context, iggy3d::FrontendAction::Save);
  ok = expect(!unsupported.accepted, "unsupported ignored") &&
       expect(unsupported.status == "not_settings_action", "unsupported status") &&
       expect(unsupported.receiptReason == "not_settings_action",
              "unsupported reason") &&
       expect(unsupported.gameplayInputSuppressed,
              "unsupported suppresses gameplay") && ok;

  return ok;
}

}  // namespace

int main() {
  const bool ok = settingsTabOrderIsExact() && settingsNamesAreStable() &&
                  defaultsArePacketDefaults() && restoreAndApplyStayFrontendOnly() &&
                  settingsRowsHaveUsefulState() && settingsTabNavigationClamps() &&
                  movementTuningFieldDescriptorsAreStable() &&
                  settingsApplyRoutesArePure() &&
                  settingsRestoreAndBackRoutesArePure() &&
                  invalidAndUnsupportedSettingsRoutesAreIgnored();
  return ok ? 0 : 1;
}
