#include "EditorControls.hpp"
#include "EditorCatalog.hpp"
#include "EditorState.hpp"
#include "EditorToolWheelPreferences.hpp"

#include <algorithm>
#include <array>
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

  std::string serialized;
  const app::CreativeEditorControlPersistenceReceipt serializedReceipt =
      app::serializeCreativeEditorControlProfile(source, serialized);
  cr::CreativeControlProfile parsed = cr::makeDefaultCreativeControlProfile();
  const app::CreativeEditorControlPersistenceReceipt parsedReceipt =
      app::parseCreativeEditorControlProfile(serialized, parsed);

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
         expect(serializedReceipt.accepted && !serialized.empty() &&
                    parsedReceipt.accepted &&
                    parsed.lookStick.deadzone == source.lookStick.deadzone &&
                    parsed.lookStick.invertY == source.lookStick.invertY &&
                    parsed.menuRepeatDelayMilliseconds == 375U,
                "pure control codec round trips without filesystem IO") &&
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

bool toolWheelPreferenceRoundTripIsAtomic() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectKind::Crate};
  const cr::CreativeCatalogState catalog =
      cr::makeCreativeCatalog(palette);
  cr::CreativeToolWheelState source = cr::makeCreativeToolWheel(catalog);
  const auto terrainPath = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::TerrainPath;
      });
  if (terrainPath == catalog.entries.end()) {
    return expect(false, "terrain path exists for wheel preference test");
  }
  const std::size_t terrainPathIndex = static_cast<std::size_t>(
      std::distance(catalog.entries.begin(), terrainPath));
  if (!cr::assignCreativeToolWheelCatalogEntry(
          source, catalog, 0U, terrainPathIndex)) {
    return expect(false, "custom wheel assignment applies before save");
  }

  const std::filesystem::path root = temporaryRoot();
  const std::filesystem::path path = root / "tool_wheel.cfg";
  const app::CreativeEditorToolWheelPersistenceReceipt saved =
      app::saveCreativeEditorToolWheel(source, catalog, path);
  cr::CreativeToolWheelState loaded = cr::makeCreativeToolWheel(catalog);
  const app::CreativeEditorToolWheelPersistenceReceipt loadedReceipt =
      app::loadCreativeEditorToolWheel(loaded, catalog, path);
  bool same = loaded.entryCount == source.entryCount;
  for (std::size_t index = 0; index < source.entryCount && same; ++index) {
    same = loaded.catalogEntryIndices[index] ==
           source.catalogEntryIndices[index];
  }

  const std::filesystem::path malformed = root / "malformed_wheel.cfg";
  {
    std::ofstream output(malformed);
    output << "iggy3d_creative_tool_wheel 1\n"
              "entry_count 9\n"
              "slot 0 Brush\n"
              "slot 1 Brush\n"
              "slot 2 Fill\n"
              "slot 3 Hollow\n"
              "slot 4 Replace\n"
              "slot 5 Clone\n"
              "slot 6 Flood\n"
              "slot 7 Extrude\n"
              "slot 8 Array\n";
  }
  const cr::CreativeToolWheelState beforeMalformed = loaded;
  const app::CreativeEditorToolWheelPersistenceReceipt malformedReceipt =
      app::loadCreativeEditorToolWheel(loaded, catalog, malformed);
  const bool preserved =
      loaded.entryCount == beforeMalformed.entryCount &&
      std::equal(loaded.catalogEntryIndices.begin(),
                 loaded.catalogEntryIndices.end(),
                 beforeMalformed.catalogEntryIndices.begin());
  std::error_code error;
  std::filesystem::remove_all(root, error);

  return expect(saved.status ==
                        app::CreativeEditorToolWheelPersistenceStatus::Saved &&
                    saved.accepted,
                "custom wheel saves outside document state") &&
         expect(loadedReceipt.status ==
                        app::CreativeEditorToolWheelPersistenceStatus::Loaded &&
                    loadedReceipt.accepted,
                "custom wheel preference loads") &&
         expect(same, "custom wheel order survives preference round trip") &&
         expect(malformedReceipt.status ==
                        app::CreativeEditorToolWheelPersistenceStatus::Invalid &&
                    preserved,
                "duplicate persisted favorites fail atomically");
}

bool controlsOverlayUsesTheStandardWidgetFrame() {
  app::CreativeEditorState editor;
  editor.controls.open = true;
  static_cast<void>(app::selectCreativeEditorControlsTab(
      editor, cr::CreativeControlDevice::Gamepad));
  static_cast<void>(app::selectCreativeEditorControlsTab(
      editor, cr::CreativeControlDevice::KeyboardMouse));
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

bool deviceTabsPartitionBindingsAndResetOnlyViewState() {
  app::CreativeEditorState editor;
  const cr::CreativeControlBindingList all =
      cr::buildCreativeControlBindingList(editor.controlProfile);
  editor.controls.selectedIndex = 8U;
  editor.controls.scrollOffset = 5U;
  editor.controls.focusedWidgetId = 9U;

  const bool selectedPs5 = app::selectCreativeEditorControlsTab(
      editor, cr::CreativeControlDevice::Gamepad);
  const std::size_t ps5Count = editor.controls.bindingList.count;
  const bool ps5Only = std::all_of(
      editor.controls.bindingList.items().begin(),
      editor.controls.bindingList.items().end(),
      [](const cr::CreativeControlBindingRow& row) {
        return row.device == cr::CreativeControlDevice::Gamepad;
      });
  const bool repeatedPs5 = app::selectCreativeEditorControlsTab(
      editor, cr::CreativeControlDevice::Gamepad);

  const bool selectedKeyboard = app::selectCreativeEditorControlsTab(
      editor, cr::CreativeControlDevice::KeyboardMouse);
  const std::size_t keyboardCount = editor.controls.bindingList.count;
  const bool keyboardOnly = std::all_of(
      editor.controls.bindingList.items().begin(),
      editor.controls.bindingList.items().end(),
      [](const cr::CreativeControlBindingRow& row) {
        return row.device == cr::CreativeControlDevice::KeyboardMouse;
      });
  const bool rejectedInvalid = app::selectCreativeEditorControlsTab(
      editor, cr::CreativeControlDevice::Count);

  return expect(selectedPs5 && !repeatedPs5 && selectedKeyboard &&
                    !rejectedInvalid,
                "controls tabs accept only real device transitions") &&
         expect(ps5Count > 0U && keyboardCount > 0U &&
                    ps5Count + keyboardCount == all.count && ps5Only &&
                    keyboardOnly,
                "keyboard and PS5 tabs partition configurable bindings") &&
         expect(editor.controls.activeDevice ==
                        cr::CreativeControlDevice::KeyboardMouse &&
                    editor.controls.selectedIndex == 0U &&
                    editor.controls.scrollOffset == 0U &&
                    editor.controls.focusedWidgetId !=
                        cr::kInvalidCreativeUiWidgetId,
                "tab switch resets navigation without changing profile");
}

}  // namespace

int main() {
  bool ok = true;
  ok = profileRoundTripPreservesBindingsAndTuning() && ok;
  ok = missingAndMalformedFilesDoNotReplaceLiveProfile() && ok;
  ok = toolWheelPreferenceRoundTripIsAtomic() && ok;
  ok = controlsOverlayUsesTheStandardWidgetFrame() && ok;
  ok = deviceTabsPartitionBindingsAndResetOnlyViewState() && ok;
  return ok ? 0 : 1;
}
