#include "EditorControls.hpp"
#include "EditorState.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const cr::CreativeControlBindingRow* findRow(
    const cr::CreativeControlBindingList& list,
    cr::CreativeInputActionId action,
    cr::CreativeControlDevice device) {
  const auto found = std::find_if(
      list.items().begin(), list.items().end(),
      [=](const cr::CreativeControlBindingRow& row) {
        return row.action == action && row.device == device;
      });
  return found == list.items().end() ? nullptr : &*found;
}

std::filesystem::path temporaryRoot() {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("iggy3d_creative_controls_test_" + std::to_string(stamp));
}

bool profileRoundTripPreservesBindingsAndTuning() {
  const std::filesystem::path root = temporaryRoot();
  const std::filesystem::path path = root / "controls.cfg";
  cr::CreativeControlProfile source = cr::makeDefaultCreativeControlProfile();
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(source);
  const cr::CreativeControlBindingRow* copy =
      findRow(rows, cr::CreativeInputActionId::CopySelection,
              cr::CreativeControlDevice::KeyboardMouse);
  const cr::CreativeControlBindingRow* paste =
      findRow(rows, cr::CreativeInputActionId::PasteClipboard,
              cr::CreativeControlDevice::KeyboardMouse);
  if (copy == nullptr || paste == nullptr) {
    return expect(false, "copy and paste rows exist");
  }
  const cr::CreativeControlRebindReceipt swapped = cr::rebindCreativeControl(
      source,
      {paste->group, cr::CreativeInputKey::C,
       cr::kCreativeInputModifierCommand,
       cr::CreativeControlConflictPolicy::Swap});
  static_cast<void>(cr::adjustCreativeControlSetting(
      source, cr::CreativeControlSettingId::LookDeadzone, 3));
  static_cast<void>(cr::adjustCreativeControlSetting(
      source, cr::CreativeControlSettingId::InvertLookY, 1));
  source.menuRepeatDelayMilliseconds = 375;
  source.menuRepeatIntervalMilliseconds = 90;

  const app::CreativeEditorControlPersistenceReceipt saved =
      app::saveCreativeEditorControlProfile(source, path);
  cr::CreativeControlProfile loaded = cr::makeDefaultCreativeControlProfile();
  const app::CreativeEditorControlPersistenceReceipt loadedReceipt =
      app::loadCreativeEditorControlProfile(loaded, path);
  const cr::CreativeInputBinding* loadedCopy =
      cr::creativeControlGroupBinding(loaded, copy->group);
  const cr::CreativeInputBinding* loadedPaste =
      cr::creativeControlGroupBinding(loaded, paste->group);
  std::error_code error;
  std::filesystem::remove_all(root, error);

  return expect(swapped.changed, "source binding swap applied") &&
         expect(saved.status ==
                    app::CreativeEditorControlPersistenceStatus::Saved &&
                    saved.accepted,
                "valid profile saves") &&
         expect(loadedReceipt.status ==
                    app::CreativeEditorControlPersistenceStatus::Loaded &&
                    loadedReceipt.accepted,
                "saved profile loads") &&
         expect(loadedCopy != nullptr && loadedPaste != nullptr &&
                    loadedCopy->trigger == cr::CreativeInputKey::V &&
                    loadedPaste->trigger == cr::CreativeInputKey::C,
                "swapped semantic bindings survive round trip") &&
         expect(loaded.lookStick.deadzone == source.lookStick.deadzone &&
                    loaded.lookStick.invertY == source.lookStick.invertY &&
                    loaded.menuRepeatDelayMilliseconds == 375U &&
                    loaded.menuRepeatIntervalMilliseconds == 90U,
                "controller and repeat tuning survive round trip") &&
         expect(cr::isValidCreativeControlProfile(loaded),
                "loaded profile remains valid");
}

bool missingAndMalformedFilesDoNotReplaceLiveProfile() {
  const std::filesystem::path root = temporaryRoot();
  const std::filesystem::path missing = root / "missing.cfg";
  cr::CreativeControlProfile profile = cr::makeDefaultCreativeControlProfile();
  profile.mouseLookSensitivity = 0.42F;
  const app::CreativeEditorControlPersistenceReceipt missingReceipt =
      app::loadCreativeEditorControlProfile(profile, missing);

  std::filesystem::create_directories(root);
  const std::filesystem::path malformed = root / "malformed.cfg";
  {
    std::ofstream output(malformed);
    output << "iggy3d_creative_controls 1\n"
              "mouse_sensitivity not-a-number\n";
  }
  const app::CreativeEditorControlPersistenceReceipt malformedReceipt =
      app::loadCreativeEditorControlProfile(profile, malformed);

  const std::filesystem::path trailing = root / "trailing.cfg";
  {
    std::ofstream output(trailing);
    output << "iggy3d_creative_controls 1\n"
              "mouse_sensitivity 0.4 unexpected\n";
  }
  const app::CreativeEditorControlPersistenceReceipt trailingReceipt =
      app::loadCreativeEditorControlProfile(profile, trailing);
  std::error_code error;
  std::filesystem::remove_all(root, error);

  return expect(missingReceipt.status ==
                    app::CreativeEditorControlPersistenceStatus::Missing &&
                    profile.mouseLookSensitivity == 0.42F,
                "missing file preserves live defaults") &&
         expect(malformedReceipt.status ==
                    app::CreativeEditorControlPersistenceStatus::Invalid &&
                    profile.mouseLookSensitivity == 0.42F,
                "malformed file fails atomically") &&
         expect(trailingReceipt.status ==
                    app::CreativeEditorControlPersistenceStatus::Invalid &&
                    profile.mouseLookSensitivity == 0.42F,
                "unexpected persisted fields fail atomically");
}

bool controlsOverlayUsesTheStandardWidgetFrame() {
  app::CreativeEditorState editor;
  editor.controls.open = true;
  editor.controls.bindingList =
      cr::buildCreativeControlBindingList(editor.controlProfile);
  editor.controls.focusedWidgetId = 1U;
  std::vector<iggy3d::RenderUiRect> rects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  app::appendCreativeEditorControlsOverlay(editor, 1280U, 720U, rects,
                                           glyphs);

  return expect(!rects.empty() && !glyphs.empty(),
                "open Controls projects standard widget visuals") &&
         expect(rects.front().x == 0 && rects.front().y == 0 &&
                    rects.front().width == 1280U &&
                    rects.front().height == 720U &&
                    rects.front().a == 0.72F,
                "standard scrim covers the drawable with bounded opacity");
}

}  // namespace

int main() {
  bool ok = true;
  ok = profileRoundTripPreservesBindingsAndTuning() && ok;
  ok = missingAndMalformedFilesDoNotReplaceLiveProfile() && ok;
  ok = controlsOverlayUsesTheStandardWidgetFrame() && ok;
  return ok ? 0 : 1;
}
