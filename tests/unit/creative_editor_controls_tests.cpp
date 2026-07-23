#include "EditorControls.hpp"
#include "EditorCatalog.hpp"
#include "EditorCatalogLayout.hpp"
#include "EditorState.hpp"
#include "EditorToolDescriptor.hpp"
#include "EditorToolWheelPreferences.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

const cr::CreativeControlBindingRow* findRow(
    const cr::CreativeControlBindingList& list,
    cr::CreativeInputActionId action,
    cr::CreativeControlDevice device,
    std::uint16_t ordinal) {
  const auto found = std::find_if(
      list.items().begin(), list.items().end(),
      [=](const cr::CreativeControlBindingRow& row) {
        return row.action == action && row.device == device &&
               row.ordinal == ordinal;
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

bool activationShadowFileDoesNotReplaceLiveProfile() {
  const cr::CreativeControlProfile defaults =
      cr::makeDefaultCreativeControlProfile();
  std::string serialized;
  const app::CreativeEditorControlPersistenceReceipt encoded =
      app::serializeCreativeEditorControlProfile(defaults, serialized);
  constexpr std::string_view kUndoPrefix =
      "bind Undo KeyboardMouse 0 ";
  const std::size_t lineBegin = serialized.find(kUndoPrefix);
  const std::size_t lineEnd =
      lineBegin == std::string::npos
          ? std::string::npos
          : serialized.find('\n', lineBegin);
  if (!encoded.accepted || lineBegin == std::string::npos ||
      lineEnd == std::string::npos) {
    return expect(false, "default Undo row serializes");
  }
  serialized.replace(
      lineBegin, lineEnd - lineBegin,
      "bind Undo KeyboardMouse 0 MousePrimary 0 0 0");

  cr::CreativeControlProfile live = defaults;
  live.mouseLookSensitivity = 0.42F;
  const app::CreativeEditorControlPersistenceReceipt parsed =
      app::parseCreativeEditorControlProfile(serialized, live);
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(live);
  const cr::CreativeControlBindingRow* undo =
      findRow(rows, cr::CreativeInputActionId::Undo,
              cr::CreativeControlDevice::KeyboardMouse);

  return expect(parsed.status ==
                        app::CreativeEditorControlPersistenceStatus::Invalid &&
                    !parsed.accepted,
                "persisted activation shadow is rejected") &&
         expect(live.mouseLookSensitivity == 0.42F && undo != nullptr &&
                    undo->trigger == cr::CreativeInputKey::Z,
                "invalid persisted profile leaves live controls untouched");
}

bool legacySquarePickMigratesWithoutDiscardingProfileTuning() {
  std::ostringstream legacy;
  constexpr cr::CreativeInputModifierMask kEveryModifier =
      cr::kCreativeInputModifierShift |
      cr::kCreativeInputModifierControl |
      cr::kCreativeInputModifierAlt |
      cr::kCreativeInputModifierCommand;
  legacy << "iggy3d_creative_controls 1\n"
            "gamepad_sensitivity 2.75\n"
            "bind PickAction Gamepad 0 GamepadWest 0 0 "
         << static_cast<unsigned>(kEveryModifier)
         << "\n"
            "bind QuickEditNext Gamepad 0 GamepadDpadDown 0 0 0\n";
  cr::CreativeControlProfile profile = cr::makeDefaultCreativeControlProfile();
  const app::CreativeEditorControlPersistenceReceipt loaded =
      app::parseCreativeEditorControlProfile(legacy.str(), profile);
  const cr::CreativeControlBindingList rows =
      cr::buildCreativeControlBindingList(profile);
  const cr::CreativeControlBindingRow* pick =
      findRow(rows, cr::CreativeInputActionId::PickAction,
              cr::CreativeControlDevice::Gamepad);
  const cr::CreativeControlBindingRow* quickEditDown =
      findRow(rows, cr::CreativeInputActionId::QuickEditNext,
              cr::CreativeControlDevice::Gamepad, 0U);
  const cr::CreativeControlBindingRow* quickEditSquare =
      findRow(rows, cr::CreativeInputActionId::QuickEditNext,
              cr::CreativeControlDevice::Gamepad, 1U);
  return expect(loaded.accepted && profile.gamepadLookSensitivity == 2.75F,
                "legacy profile tuning survives the Square migration") &&
         expect(pick != nullptr &&
                    pick->trigger == cr::CreativeInputKey::GamepadTouchpad,
                "legacy default Square pick migrates to Touchpad") &&
         expect(quickEditDown != nullptr && quickEditSquare != nullptr &&
                    quickEditDown->trigger ==
                        cr::CreativeInputKey::GamepadDpadDown &&
                    quickEditSquare->trigger ==
                        cr::CreativeInputKey::GamepadWest &&
                    cr::isValidCreativeControlProfile(profile),
                "legacy D-pad setting binding and new Square binding coexist");
}

bool toolWheelPreferenceRoundTripIsAtomic() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectKind::Crate};
  const cr::CreativeCatalogState catalog =
      cr::makeCreativeCatalog(palette, {}, 0U, {},
                              app::creativeEditorCatalogToolSpecs());
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

bool catalogOverlayFitsAndEmitsEveryCategoryTab() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectKind::Crate};
  cr::CreativeAppState appState;
  app::CreativeEditorState editor;
  editor.catalog.model = cr::makeCreativeCatalog(
      palette, {}, 0U, {}, app::creativeEditorCatalogToolSpecs());
  editor.catalog.model.open = true;

  bool ok = true;
  for (const std::array<std::uint32_t, 2> drawable :
       {std::array<std::uint32_t, 2>{1280U, 720U},
        std::array<std::uint32_t, 2>{480U, 640U}}) {
    const app::CatalogLayout layout =
        app::catalogLayout(drawable[0], drawable[1]);
    std::vector<iggy3d::RenderUiRect> rects;
    std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
    app::appendCreativeEditorCatalogOverlay(
        appState, editor, drawable[0], drawable[1], rects, glyphs);
    const std::size_t emittedTabs = static_cast<std::size_t>(std::count_if(
        rects.begin(), rects.end(), [&layout](const iggy3d::RenderUiRect& rect) {
          return rect.y == layout.tabsY && rect.width == layout.tabWidth &&
                 rect.height == layout.tabHeight;
        }));
    const std::uint32_t tabCount =
        static_cast<std::uint32_t>(cr::CreativeCatalogPage::Count);
    ok = expect(emittedTabs == tabCount,
                "catalog overlay emits every category tab") &&
         expect(layout.tabWidth * tabCount <= layout.panelWidth &&
                    layout.tabsX >= layout.panelX &&
                    layout.tabsX + static_cast<std::int32_t>(
                                       layout.tabWidth * tabCount) <=
                        layout.panelX +
                            static_cast<std::int32_t>(layout.panelWidth),
                "category tabs remain inside desktop and compact panels") &&
         ok;
  }
  return ok;
}

bool assetCatalogOverlayRendersCachedThumbnailAndVariantControls() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall};
  cr::CreativeCatalogAsset asset;
  asset.objectKind = cr::CreativeObjectKind::Prop;
  asset.assetId = "props/test_asset";
  asset.label = "Test Asset";
  asset.sourceBounds = {{-1.0, 0.0, -0.5}, {1.0, 2.0, 0.5}};
  asset.materialVariants = {{"Weathered"}};
  asset.thumbnail.valid = true;
  asset.thumbnail.coveredPixelCount = 2U;
  asset.thumbnail.pixels[0] = {255U, 0U, 0U, 255U};
  asset.thumbnail.pixels[1] = {255U, 0U, 0U, 255U};
  cr::CreativeAppState appState;
  app::CreativeEditorState editor;
  editor.catalog.model = cr::makeCreativeCatalog(
      palette, std::span{&asset, 1U}, 0U, {},
      app::creativeEditorCatalogToolSpecs());
  static_cast<void>(cr::setCreativeCatalogPage(
      editor.catalog.model, cr::CreativeCatalogPage::Assets));
  editor.catalog.model.open = true;
  constexpr std::uint32_t width = 1280U;
  constexpr std::uint32_t height = 720U;
  const app::CatalogLayout layout = app::catalogLayout(width, height);
  const app::CatalogRect thumbnail = app::catalogAssetThumbnailRect(layout);
  const app::CatalogRect previous =
      app::previousAssetMaterialVariantButton(layout);
  const app::CatalogRect next = app::nextAssetMaterialVariantButton(layout);
  std::vector<iggy3d::RenderUiRect> rects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  app::appendCreativeEditorCatalogOverlay(appState, editor, width, height,
                                          rects, glyphs);
  const bool thumbnailRun = std::any_of(
      rects.begin(), rects.end(), [&](const iggy3d::RenderUiRect& rect) {
        return rect.x == thumbnail.x && rect.y == thumbnail.y &&
               rect.r == 1.0F && rect.g == 0.0F && rect.b == 0.0F &&
               rect.width == thumbnail.width * 2U /
                                 iggy3d::kStaticMeshThumbnailExtent;
      });
  const auto hasRect = [&](app::CatalogRect target) {
    return std::any_of(
        rects.begin(), rects.end(),
        [&](const iggy3d::RenderUiRect& rect) {
          return rect.x == target.x && rect.y == target.y &&
                 rect.width == target.width && rect.height == target.height;
        });
  };
  return expect(layout.showDetails && thumbnailRun,
                "catalog renders the cached asset thumbnail without a mesh "
                "rebuild") &&
         expect(hasRect(previous) && hasRect(next) && !glyphs.empty(),
                "mouse and controller material variant controls share the "
                "asset detail surface");
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
  ok = activationShadowFileDoesNotReplaceLiveProfile() && ok;
  ok = legacySquarePickMigratesWithoutDiscardingProfileTuning() && ok;
  ok = toolWheelPreferenceRoundTripIsAtomic() && ok;
  ok = controlsOverlayUsesTheStandardWidgetFrame() && ok;
  ok = catalogOverlayFitsAndEmitsEveryCategoryTab() && ok;
  ok = assetCatalogOverlayRendersCachedThumbnailAndVariantControls() && ok;
  ok = deviceTabsPartitionBindingsAndResetOnlyViewState() && ok;
  return ok ? 0 : 1;
}
