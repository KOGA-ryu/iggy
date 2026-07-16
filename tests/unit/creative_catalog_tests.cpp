#include "app/iggy3d/creative/input/Catalog.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeCatalogState catalog() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectKind::Crate};
  return cr::makeCreativeCatalog(palette);
}

cr::CreativeCatalogPage expectedCatalogPage(
    cr::CreativeObjectCategory category) {
  switch (category) {
    case cr::CreativeObjectCategory::Structural:
      return cr::CreativeCatalogPage::Structure;
    case cr::CreativeObjectCategory::TerrainOrVolume:
      return cr::CreativeCatalogPage::Terrain;
    case cr::CreativeObjectCategory::NavigationOrMovement:
      return cr::CreativeCatalogPage::Movement;
    case cr::CreativeObjectCategory::Logic:
      return cr::CreativeCatalogPage::Logic;
    case cr::CreativeObjectCategory::VisualDressing:
      return cr::CreativeCatalogPage::Dressing;
    case cr::CreativeObjectCategory::LightSoundOrCamera:
      return cr::CreativeCatalogPage::Media;
    case cr::CreativeObjectCategory::Gameplay:
      return cr::CreativeCatalogPage::Gameplay;
    case cr::CreativeObjectCategory::Testing:
      return cr::CreativeCatalogPage::Testing;
    case cr::CreativeObjectCategory::AuthoringMeta:
      return cr::CreativeCatalogPage::Helpers;
    case cr::CreativeObjectCategory::Unknown:
      return cr::CreativeCatalogPage::Count;
  }
  return cr::CreativeCatalogPage::Count;
}

bool actionPageIsFixedNavigableAndConfirmationSafe() {
  const std::span<const cr::CreativeCatalogActionEntry> actions =
      cr::creativeCatalogActionEntries();
  constexpr std::array expectedActions{
      cr::CreativeInputActionId::Undo,
      cr::CreativeInputActionId::Redo,
      cr::CreativeInputActionId::CopySelection,
      cr::CreativeInputActionId::CutSelection,
      cr::CreativeInputActionId::PasteClipboard,
      cr::CreativeInputActionId::DuplicateSelection,
      cr::CreativeInputActionId::Save,
      cr::CreativeInputActionId::NewDocument,
      cr::CreativeInputActionId::Load,
  };
  cr::CreativeCatalogState state = catalog();
  bool ok = expect(actions.size() == cr::kCreativeCatalogActionCapacity &&
                       actions.size() == expectedActions.size(),
                   "catalog action registry has fixed capacity") &&
            expect(std::equal(
                       actions.begin(), actions.end(), expectedActions.begin(),
                       [](const cr::CreativeCatalogActionEntry& entry,
                          cr::CreativeInputActionId expected) {
                         return entry.action == expected;
                       }),
                   "catalog action order is stable") &&
            expect(!actions[6].confirmationRequired &&
                       actions[7].confirmationRequired &&
                       actions[8].confirmationRequired,
                   "only new and load require confirmation") &&
            expect(state.page == cr::CreativeCatalogPage::Structure &&
                       cr::toString(state.page) == "Structure" &&
                       cr::toString(cr::CreativeCatalogPage::Actions) ==
                           "Actions",
                   "catalog defaults to named structure category");

  ok = expect(cr::moveCreativeCatalogPage(state, -1) &&
                  state.page == cr::CreativeCatalogPage::Actions &&
                  cr::moveCreativeCatalogPage(state, 1) &&
                  state.page == cr::CreativeCatalogPage::Structure,
              "catalog categories wrap in both directions") &&
       expect(!cr::setCreativeCatalogPage(state,
                                          cr::CreativeCatalogPage::Count),
              "catalog rejects sentinel page") &&
       ok;

  static_cast<void>(cr::setCreativeCatalogPage(
      state, cr::CreativeCatalogPage::Actions));
  ok = expect(cr::moveCreativeCatalogActionSelection(state, -1) &&
                  state.selectedActionIndex == actions.size() - 1U &&
                  cr::selectedCreativeCatalogAction(state)->action ==
                      cr::CreativeInputActionId::Load,
              "action selection wraps backward") &&
       expect(!cr::selectCreativeCatalogActionIndex(state, actions.size()),
              "invalid action selection is rejected") &&
       ok;

  static_cast<void>(cr::selectCreativeCatalogActionIndex(state, 7U));
  const cr::CreativeCatalogActionActivation armed =
      cr::activateSelectedCreativeCatalogAction(state);
  const bool pendingAfterArm =
      state.pendingActionConfirmation ==
      cr::CreativeInputActionId::NewDocument;
  const cr::CreativeCatalogActionActivation confirmed =
      cr::activateSelectedCreativeCatalogAction(state);
  ok = expect(armed.confirmationArmed && !armed.requested &&
                  pendingAfterArm &&
                  state.pendingActionConfirmation == std::nullopt,
              "first destructive activation arms and second requests") &&
       expect(confirmed.requested &&
                  confirmed.action == cr::CreativeInputActionId::NewDocument,
              "confirmed destructive activation returns semantic command") &&
       ok;

  static_cast<void>(cr::selectCreativeCatalogActionIndex(state, 6U));
  const cr::CreativeCatalogActionActivation saved =
      cr::activateSelectedCreativeCatalogAction(state);
  ok = expect(saved.requested && !saved.confirmationArmed &&
                  saved.action == cr::CreativeInputActionId::Save,
              "non-destructive action requests immediately") &&
       ok;

  static_cast<void>(cr::selectCreativeCatalogActionIndex(state, 7U));
  static_cast<void>(cr::activateSelectedCreativeCatalogAction(state));
  static_cast<void>(cr::moveCreativeCatalogActionSelection(state, 1));
  ok = expect(!state.pendingActionConfirmation.has_value(),
              "selection movement clears destructive confirmation") &&
       ok;
  static_cast<void>(cr::selectCreativeCatalogActionIndex(state, 7U));
  static_cast<void>(cr::activateSelectedCreativeCatalogAction(state));
  state.open = true;
  static_cast<void>(cr::setCreativeCatalogOpen(state, false));
  return expect(!state.pendingActionConfirmation.has_value(),
                "closing catalog clears destructive confirmation") &&
         ok;
}

bool actionAvailabilityUsesExplicitFacts() {
  cr::CreativeCatalogActionAvailability availability;
  bool ok = expect(!cr::creativeCatalogActionAvailable(
                       cr::CreativeInputActionId::Undo, availability) &&
                       !cr::creativeCatalogActionAvailable(
                           cr::CreativeInputActionId::CopySelection,
                           availability) &&
                       !cr::creativeCatalogActionAvailable(
                           cr::CreativeInputActionId::PasteClipboard,
                           availability),
                   "empty editor disables state-dependent actions") &&
            expect(cr::creativeCatalogActionAvailable(
                       cr::CreativeInputActionId::Save, availability) &&
                       cr::creativeCatalogActionAvailable(
                           cr::CreativeInputActionId::NewDocument,
                           availability) &&
                       cr::creativeCatalogActionAvailable(
                           cr::CreativeInputActionId::Load, availability),
                   "file actions remain available");
  availability.undoAvailable = true;
  availability.clipboardAvailable = true;
  availability.copyAvailable = true;
  bool enabled = expect(cr::creativeCatalogActionAvailable(
                            cr::CreativeInputActionId::Undo, availability) &&
                            cr::creativeCatalogActionAvailable(
                                cr::CreativeInputActionId::PasteClipboard,
                                availability) &&
                            cr::creativeCatalogActionAvailable(
                                cr::CreativeInputActionId::CopySelection,
                                availability) &&
                            !cr::creativeCatalogActionAvailable(
                                cr::CreativeInputActionId::CutSelection,
                                availability) &&
                            !cr::creativeCatalogActionAvailable(
                                cr::CreativeInputActionId::DuplicateSelection,
                                availability),
                        "selection actions consume independent availability facts");
  availability.cutAvailable = true;
  availability.duplicateAvailable = true;
  return expect(cr::creativeCatalogActionAvailable(
                    cr::CreativeInputActionId::CutSelection, availability) &&
                    cr::creativeCatalogActionAvailable(
                        cr::CreativeInputActionId::DuplicateSelection,
                        availability),
                "cut and duplicate enable without changing copy or paste") &&
         enabled &&
         ok;
}

bool catalogBuildsMaterialsAndCreatorTools() {
  const cr::CreativeCatalogState state = catalog();
  const auto terrainRegion = std::find_if(
      state.entries.begin(), state.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::TerrainRegion;
      });
  const auto logicLink = std::find_if(
      state.entries.begin(), state.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::LogicLink;
      });
  bool ok = expect(state.entries.size() == 24U,
                   "materials and tools retain one asset reload command") &&
            expect(state.filteredEntryIndices.size() == 1U,
                   "structure category exposes only structural materials") &&
            expect(state.entries[0].category ==
                       cr::CreativeCatalogEntryCategory::Material &&
                       state.entries[0].hotbarEntry.kind ==
                           cr::CreativeHeldItemKind::Material &&
                       state.entries[0].hotbarEntry.objectKind ==
                           cr::CreativeObjectKind::Wall,
                   "first palette material retained") &&
            expect(state.entries[2].category ==
                       cr::CreativeCatalogEntryCategory::Tool &&
                       state.entries[2].hotbarEntry.kind ==
                           cr::CreativeHeldItemKind::MaterialBrush,
                   "material brush starts the tool lane") &&
            expect(state.entries[3].hotbarEntry.kind ==
                       cr::CreativeHeldItemKind::ObjectSelect,
                   "object tools follow the material brush") &&
            expect(terrainRegion != state.entries.end() &&
                       terrainRegion->label == "Terrain Region",
                   "terrain region remains a selectable tool") &&
            expect(logicLink != state.entries.end() &&
                       logicLink->label == "Logic Link" &&
                       !logicLink->toolWheelEligible,
                   "Logic Link closes the catalog lane without shifting the wheel");
  ok = expect(cr::toString(cr::CreativeCatalogEntryCategory::Material) ==
                  "Material" &&
                  cr::toString(cr::CreativeCatalogEntryCategory::Command) ==
                      "Command" &&
                  cr::toString(cr::CreativeCatalogEntryCategory::Tool) ==
                  "Tool",
              "catalog categories have stable labels") &&
       ok;
  return ok;
}

bool catalogPartitionsDescriptorPaletteAndSearchesCreatorCategories() {
  std::vector<cr::CreativeObjectKind> palette;
  for (const cr::CreativeObjectDescriptor& descriptor :
       cr::allObjectDescriptors()) {
    if (cr::descriptorShowsInAuthoringBrushPalette(descriptor)) {
      palette.push_back(descriptor.kind);
    }
  }

  cr::CreativeCatalogState state = cr::makeCreativeCatalog(palette);
  constexpr std::array creatorPages{
      cr::CreativeCatalogPage::Structure,
      cr::CreativeCatalogPage::Terrain,
      cr::CreativeCatalogPage::Movement,
      cr::CreativeCatalogPage::Logic,
      cr::CreativeCatalogPage::Dressing,
      cr::CreativeCatalogPage::Media,
      cr::CreativeCatalogPage::Gameplay,
      cr::CreativeCatalogPage::Testing,
      cr::CreativeCatalogPage::Helpers,
  };
  bool ok = true;
  std::size_t categorizedMaterialCount = 0U;
  for (cr::CreativeCatalogPage page : creatorPages) {
    static_cast<void>(cr::setCreativeCatalogPage(state, page));
    ok = expect(!state.filteredEntryIndices.empty(),
                "every descriptor category has visible catalog entries") &&
         ok;
    for (std::size_t entryIndex : state.filteredEntryIndices) {
      const cr::CreativeCatalogEntry& entry = state.entries[entryIndex];
      ok = expect(entry.category ==
                          cr::CreativeCatalogEntryCategory::Material &&
                      entry.page == page &&
                      entry.page == expectedCatalogPage(
                                        cr::describeObject(
                                            entry.hotbarEntry.objectKind)
                                            .category),
                  "material page follows descriptor category") &&
           ok;
      ++categorizedMaterialCount;
    }
  }

  static_cast<void>(
      cr::setCreativeCatalogPage(state, cr::CreativeCatalogPage::Tools));
  ok = expect(!state.filteredEntryIndices.empty() &&
                  std::all_of(
                      state.filteredEntryIndices.begin(),
                      state.filteredEntryIndices.end(),
                      [&state](std::size_t index) {
                        return state.entries[index].page ==
                                   cr::CreativeCatalogPage::Tools &&
                               state.entries[index].category ==
                                   cr::CreativeCatalogEntryCategory::Tool;
                      }),
              "creator tools occupy one dedicated category") &&
       expect(categorizedMaterialCount == palette.size(),
              "category pages partition the visible descriptor palette") &&
       ok;

  static_cast<void>(
      cr::setCreativeCatalogPage(state, cr::CreativeCatalogPage::Structure));
  const bool searched = cr::setCreativeCatalogQuery(state, "crate");
  const cr::CreativeCatalogEntry* crate =
      cr::selectedCreativeCatalogEntry(state);
  return expect(searched && state.filteredEntryIndices.size() == 1U &&
                    crate != nullptr &&
                    crate->hotbarEntry.objectKind ==
                        cr::CreativeObjectKind::Crate &&
                    crate->page == cr::CreativeCatalogPage::Dressing,
                "typing searches all creator categories from the active tab") &&
         ok;
}

bool catalogOmitsToolsWithoutRequiredMaterial() {
  cr::CreativeCatalogState state = cr::makeCreativeCatalog({});
  static_cast<void>(
      cr::setCreativeCatalogPage(state, cr::CreativeCatalogPage::Tools));
  const bool materialDependentToolPresent = std::any_of(
      state.entries.begin(), state.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
                   cr::CreativeHeldItemKind::MaterialBrush ||
               entry.hotbarEntry.kind == cr::CreativeHeldItemKind::VolumeFill ||
               entry.hotbarEntry.kind ==
                   cr::CreativeHeldItemKind::VolumeHollow ||
               entry.hotbarEntry.kind ==
                   cr::CreativeHeldItemKind::VolumeReplace ||
               entry.hotbarEntry.kind ==
                   cr::CreativeHeldItemKind::ConnectedFill ||
               entry.hotbarEntry.kind ==
                   cr::CreativeHeldItemKind::SurfaceExtrude;
      });
  return expect(state.entries.size() == 16U &&
                    state.filteredEntryIndices.size() == 15U,
                "empty palette retains tools plus the asset reload command") &&
         expect(!materialDependentToolPresent,
                "material-dependent tools require a valid material");
}

bool catalogAssignmentDistinguishesMaterialsFromTools() {
  const cr::CreativeCatalogState state = catalog();
  const cr::CreativeHotbarEntry material =
      cr::resolveCreativeCatalogHotbarEntry(
          state.entries[1], cr::CreativeObjectKind::Wall);
  const cr::CreativeHotbarEntry brush =
      cr::resolveCreativeCatalogHotbarEntry(
          state.entries[2], cr::CreativeObjectKind::Wall);
  const cr::CreativeHotbarEntry fill =
      cr::resolveCreativeCatalogHotbarEntry(
          state.entries[7], cr::CreativeObjectKind::Crate);
  return expect(material.kind == cr::CreativeHeldItemKind::Material &&
                    material.objectKind == cr::CreativeObjectKind::Crate,
                "clicked material is not replaced by active brush") &&
         expect(fill.kind == cr::CreativeHeldItemKind::VolumeFill &&
                    fill.objectKind == cr::CreativeObjectKind::Crate,
                "material-dependent tool inherits active brush") &&
         expect(brush.kind == cr::CreativeHeldItemKind::MaterialBrush &&
                    brush.objectKind == cr::CreativeObjectKind::Wall,
                "material brush inherits the active voxel material");
}

bool shapeSelectionIsVisibleBoundedAndDeterministic() {
  const cr::CreativeCatalogState state = catalog();
  const cr::CreativeCatalogEntry& fill = state.entries[7];
  cr::CreativeCatalogShapeSelection selection =
      cr::normalizeCreativeCatalogShapeSelection(
          cr::CreativeShapeBrushKind::Box,
          cr::CreativeShapeBrushAxis::X);
  bool ok = expect(cr::creativeCatalogEntryUsesShapeSelection(fill),
                   "fill exposes catalog shape selection") &&
            expect(!cr::creativeCatalogEntryUsesShapeSelection(
                       state.entries[6]),
                   "region wand has no shape configuration") &&
            expect(selection.kind == cr::CreativeShapeBrushKind::Box &&
                       selection.axis == cr::CreativeShapeBrushAxis::Y &&
                       cr::creativeCatalogShapeSelectionLabel(selection) ==
                           "Box",
                   "non-axis shapes normalize to canonical preset");

  constexpr std::array expectedLabels{
      std::string_view{"Line"}, std::string_view{"Ellipsoid"},
      std::string_view{"Cylinder X"}, std::string_view{"Cylinder Y"},
      std::string_view{"Cylinder Z"}, std::string_view{"Box"},
  };
  for (std::string_view label : expectedLabels) {
    ok = expect(cr::moveCreativeCatalogShapeSelection(selection, 1) &&
                    cr::creativeCatalogShapeSelectionLabel(selection) == label,
                "shape preset advances in visible catalog order") &&
         ok;
  }
  ok = expect(cr::moveCreativeCatalogShapeSelection(selection, -1) &&
                  cr::creativeCatalogShapeSelectionLabel(selection) ==
                      "Cylinder Z",
              "shape preset wraps backward") &&
       ok;

  const cr::CreativeCatalogShapeSelection invalid =
      cr::normalizeCreativeCatalogShapeSelection(
          static_cast<cr::CreativeShapeBrushKind>(255U),
          static_cast<cr::CreativeShapeBrushAxis>(255U));
  return expect(invalid.kind == cr::CreativeShapeBrushKind::Box &&
                    invalid.axis == cr::CreativeShapeBrushAxis::Y,
                "invalid shape configuration fails closed to box") &&
         ok;
}

bool toolWheelIsBoundedDirectionalAndAssignable() {
  const cr::CreativeCatalogState catalogState = catalog();
  cr::CreativeToolWheelState wheel =
      cr::makeCreativeToolWheel(catalogState);
  bool ok = expect(wheel.entryCount == cr::kCreativeToolWheelCapacity,
                   "all nine wheel tools populate the bounded wheel") &&
            expect(!wheel.capacityExceeded,
                   "default tool wheel fits its fixed capacity") &&
            expect(cr::selectedCreativeToolWheelEntry(wheel, catalogState) !=
                       nullptr &&
                       cr::selectedCreativeToolWheelEntry(wheel, catalogState)
                               ->hotbarEntry.kind ==
                           cr::CreativeHeldItemKind::MaterialBrush,
                   "material brush owns the up sector");
  const auto erase = std::find_if(
      catalogState.entries.begin(), catalogState.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::VolumeErase;
      });
  const auto regionSelect = std::find_if(
      catalogState.entries.begin(), catalogState.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::VolumeSelect;
      });
  const auto objectSelect = std::find_if(
      catalogState.entries.begin(), catalogState.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::ObjectSelect;
      });
  const auto terrain = std::find_if(
      catalogState.entries.begin(), catalogState.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::TerrainControl;
      });
  const auto clone = std::find_if(
      catalogState.entries.begin(), catalogState.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::VolumeClone;
      });
  const auto group = std::find_if(
      catalogState.entries.begin(), catalogState.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::ObjectGroup;
      });
  ok = expect(erase != catalogState.entries.end() &&
                  !erase->toolWheelEligible &&
                  regionSelect != catalogState.entries.end() &&
                  !regionSelect->toolWheelEligible &&
                  objectSelect != catalogState.entries.end() &&
                  !objectSelect->toolWheelEligible &&
                  terrain != catalogState.entries.end() &&
                  !terrain->toolWheelEligible &&
                  clone != catalogState.entries.end() &&
                  !clone->toolWheelEligible,
              "specialized tools remain assignable but outside defaults") &&
       expect(group != catalogState.entries.end() &&
                  group->toolWheelEligible &&
                  wheel.catalogEntryIndices[2] < catalogState.entries.size() &&
                  catalogState.entries[wheel.catalogEntryIndices[2]]
                          .hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::ObjectGroup,
              "group replaces clone in the default R3 wheel") &&
       expect(wheel.catalogEntryIndices[8] < catalogState.entries.size() &&
                  catalogState.entries[wheel.catalogEntryIndices[8]]
                          .hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::LinearArray,
              "array owns the ninth wheel sector") &&
       expect(wheel.catalogEntryIndices[6] < catalogState.entries.size() &&
                  catalogState.entries[wheel.catalogEntryIndices[6]]
                          .hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::ConnectedFill,
              "connected fill owns the seventh wheel sector") &&
       expect(wheel.catalogEntryIndices[7] < catalogState.entries.size() &&
                  catalogState.entries[wheel.catalogEntryIndices[7]]
                          .hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::SurfaceExtrude,
              "surface extrude owns the wheel sector before array") &&
       ok;

  constexpr float kTurnRadians = 6.28318530717958647692F;
  for (std::size_t index = 0; index < wheel.entryCount; ++index) {
    const float angle = kTurnRadians * static_cast<float>(index) /
                        static_cast<float>(wheel.entryCount);
    static_cast<void>(cr::selectCreativeToolWheelDirection(
        wheel, std::sin(angle), std::cos(angle)));
    ok = expect(wheel.selectedIndex == index,
                "tool wheel sectors run clockwise from up") &&
         ok;
  }

  const auto selectDegrees = [&wheel](float degrees) {
    constexpr float kDegreesToRadians =
        3.14159265358979323846F / 180.0F;
    const float radians = degrees * kDegreesToRadians;
    static_cast<void>(cr::selectCreativeToolWheelDirection(
        wheel, std::sin(radians), std::cos(radians)));
    return wheel.selectedIndex;
  };
  ok = expect(selectDegrees(19.0F) == 0U &&
                  selectDegrees(21.0F) == 1U,
              "sector boundary has deterministic half-sector rounding") &&
       ok;

  const std::size_t beforeDeadzone = wheel.selectedIndex;
  ok = expect(!cr::selectCreativeToolWheelDirection(wheel, 0.1F, 0.1F) &&
                  wheel.selectedIndex == beforeDeadzone,
              "deadzone preserves radial selection") &&
       expect(!cr::selectCreativeToolWheelDirection(
                  wheel, std::numeric_limits<float>::infinity(), 0.0F) &&
                  wheel.selectedIndex == beforeDeadzone,
              "non-finite radial direction is rejected") &&
       expect(!cr::selectCreativeToolWheelDirection(
                  wheel, 1.0F, 0.0F, -1.0F) &&
                  wheel.selectedIndex == beforeDeadzone,
              "invalid deadzone is rejected") &&
       ok;

  static_cast<void>(cr::selectCreativeToolWheelForHotbarEntry(
      wheel, catalogState,
      {cr::CreativeHeldItemKind::VolumeReplace,
       cr::CreativeObjectKind::Crate}));
  ok = expect(wheel.selectedIndex == 5U,
              "held tool kind restores radial selection") &&
       expect(cr::moveCreativeToolWheelSelection(wheel, -6) &&
                  wheel.selectedIndex == 8U,
              "radial keyboard navigation wraps") &&
       ok;

  constexpr std::array palette{cr::CreativeObjectKind::Wall};
  cr::CreativeHotbarState hotbar = cr::makeDefaultCreativeHotbar(palette);
  hotbar.selectedSlot = 3U;
  static_cast<void>(cr::selectCreativeToolWheelForHotbarEntry(
      wheel, catalogState,
      {cr::CreativeHeldItemKind::VolumeFill,
       cr::CreativeObjectKind::Wall}));
  ok = expect(cr::assignSelectedCreativeToolWheelEntry(
                  wheel, catalogState, hotbar) &&
                  hotbar.selectedSlot == 3U &&
                  hotbar.entries[3].kind ==
                      cr::CreativeHeldItemKind::VolumeFill,
              "wheel equips the active hotbar slot") &&
       ok;

  const std::size_t objectSelectIndex = static_cast<std::size_t>(
      std::distance(catalogState.entries.begin(), objectSelect));
  const std::size_t terrainIndex = static_cast<std::size_t>(
      std::distance(catalogState.entries.begin(), terrain));
  const std::size_t extrudeIndex = wheel.catalogEntryIndices[7];
  ok = expect(cr::assignCreativeToolWheelCatalogEntry(
                  wheel, catalogState, 0U, objectSelectIndex) &&
                  wheel.catalogEntryIndices[0] == objectSelectIndex &&
                  cr::isValidCreativeToolWheel(wheel, catalogState),
              "catalog-only tools can replace a favorite sector") &&
       expect(cr::assignCreativeToolWheelCatalogEntry(
                  wheel, catalogState, 1U, terrainIndex) &&
                  wheel.catalogEntryIndices[1] == terrainIndex,
              "terrain rod can be assigned as a favorite") &&
       expect(cr::assignCreativeToolWheelCatalogEntry(
                  wheel, catalogState, 0U, extrudeIndex) &&
                  wheel.catalogEntryIndices[0] == extrudeIndex &&
                  wheel.catalogEntryIndices[7] == objectSelectIndex,
              "moving an existing favorite swaps sectors without duplicates") &&
       expect(!cr::assignCreativeToolWheelCatalogEntry(
                  wheel, catalogState, wheel.entryCount, objectSelectIndex) &&
                  !cr::assignCreativeToolWheelCatalogEntry(
                      wheel, catalogState, 1U, 0U),
              "invalid sectors and material entries cannot enter the wheel") &&
       expect(cr::resetCreativeToolWheel(wheel, catalogState) &&
                  wheel.catalogEntryIndices[0] < catalogState.entries.size() &&
                  catalogState.entries[wheel.catalogEntryIndices[0]]
                          .hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::MaterialBrush &&
                  cr::isValidCreativeToolWheel(wheel, catalogState),
              "wheel reset restores the bounded default ordering") &&
       ok;

  cr::CreativeToolWheelState duplicate = wheel;
  duplicate.catalogEntryIndices[1] = duplicate.catalogEntryIndices[0];
  ok = expect(!cr::isValidCreativeToolWheel(duplicate, catalogState),
              "duplicate favorites fail structural validation") &&
       ok;

  cr::CreativeCatalogState oversized;
  for (std::size_t index = 0;
       index < cr::kCreativeToolWheelCapacity + 1U; ++index) {
    cr::CreativeCatalogEntry entry;
    entry.category = cr::CreativeCatalogEntryCategory::Tool;
    entry.hotbarEntry.kind = cr::CreativeHeldItemKind::ObjectSelect;
    entry.toolWheelEligible = true;
    oversized.entries.push_back(entry);
  }
  const cr::CreativeToolWheelState bounded =
      cr::makeCreativeToolWheel(oversized);
  const cr::CreativeToolWheelState empty;
  return expect(bounded.entryCount == cr::kCreativeToolWheelCapacity &&
                    bounded.capacityExceeded,
                "tool overflow is reported instead of reallocating") &&
         expect(!cr::assignSelectedCreativeToolWheelEntry(
                    empty, catalogState, hotbar),
                "empty wheel cannot equip") &&
         ok;
}

bool searchIsCaseInsensitiveBoundedAndStable() {
  cr::CreativeCatalogState state = catalog();
  bool ok = expect(cr::setCreativeCatalogQuery(state, "cRaTe"),
                   "mixed-case query changes filter") &&
            expect(state.filteredEntryIndices.size() == 1U,
                   "exact crate query excludes the crater profile alias") &&
            expect(cr::selectedCreativeCatalogEntry(state) != nullptr &&
                       cr::selectedCreativeCatalogEntry(state)
                               ->hotbarEntry.objectKind ==
                           cr::CreativeObjectKind::Crate,
                   "crate result selected");

  ok = expect(cr::setCreativeCatalogQuery(state, "REGION"),
              "tool alias query accepted") &&
       expect(state.filteredEntryIndices.size() == 8U,
              "region query includes terrain and voxel region tools") &&
       expect(cr::setCreativeCatalogQuery(state, "raise") &&
                  state.filteredEntryIndices.size() == 2U &&
                  std::any_of(
                      state.filteredEntryIndices.begin(),
                      state.filteredEntryIndices.end(),
                      [&state](std::size_t index) {
                        return state.entries[index].hotbarEntry.kind ==
                               cr::CreativeHeldItemKind::TerrainRegion;
                      }),
              "raise alias exposes brush and bounded-region terrain tools") &&
       expect(cr::setCreativeCatalogQuery(state, "crater") &&
                  state.filteredEntryIndices.size() == 1U &&
                  cr::selectedCreativeCatalogEntry(state) != nullptr &&
                  cr::selectedCreativeCatalogEntry(state)->hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::TerrainProfile,
              "crater alias resolves uniquely to terrain profile") &&
       expect(cr::setCreativeCatalogQuery(state, "road") &&
                  state.filteredEntryIndices.size() == 1U &&
                  cr::selectedCreativeCatalogEntry(state) != nullptr &&
                  cr::selectedCreativeCatalogEntry(state)->hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::TerrainPath,
              "road alias resolves uniquely to terrain path") &&
       expect(cr::setCreativeCatalogQuery(state, "no-such-entry"),
              "empty-result query accepted") &&
       expect(state.filteredEntryIndices.empty() &&
                  cr::selectedCreativeCatalogEntry(state) == nullptr,
              "empty result has no selected entry") &&
       ok;

  static_cast<void>(cr::setCreativeCatalogQuery(state, {}));
  ok = expect(cr::appendCreativeCatalogQueryText(state, "Cr"),
              "query text appended") &&
       expect(cr::appendCreativeCatalogQueryText(state, "ate\n"),
              "printable suffix appended") &&
       expect(state.query == "Crate",
              "control characters excluded from query") &&
       expect(cr::eraseCreativeCatalogQuery(state, 2U) && state.query == "Cra",
              "backspace count erases bounded suffix") &&
       ok;

  const std::string oversized(200U, 'x');
  static_cast<void>(cr::setCreativeCatalogQuery(state, oversized));
  return expect(state.query.size() == cr::kCreativeCatalogQueryCapacity,
                "query is capped") &&
         ok;
}

bool selectionWrapsAndAssignmentsAreExplicit() {
  cr::CreativeCatalogState state = catalog();
  constexpr std::array palette{cr::CreativeObjectKind::Wall};
  cr::CreativeHotbarState hotbar = cr::makeDefaultCreativeHotbar(palette);
  static_cast<void>(
      cr::setCreativeCatalogPage(state, cr::CreativeCatalogPage::Tools));

  bool ok = expect(cr::moveCreativeCatalogSelection(state, -1) &&
                       state.selectedFilteredIndex + 1U ==
                           state.filteredEntryIndices.size(),
                   "previous wraps to final result") &&
            expect(cr::moveCreativeCatalogSelection(state, 1) &&
                       state.selectedFilteredIndex == 0U,
                   "next wraps to first result") &&
            expect(!cr::selectCreativeCatalogFilteredIndex(state, 100U),
                   "invalid result index rejected");

  static_cast<void>(cr::setCreativeCatalogQuery(state, "Object Move"));
  ok = expect(state.filteredEntryIndices.size() == 1U,
              "move tool query unique") &&
       expect(cr::assignSelectedCreativeCatalogEntry(state, hotbar),
              "selected tool equips current slot") &&
       expect(hotbar.selectedSlot == 0U &&
                  hotbar.entries[0].kind ==
                      cr::CreativeHeldItemKind::ObjectMove,
              "move tool assigned to active slot") &&
       ok;

  static_cast<void>(cr::setCreativeCatalogQuery(state, "Erase"));
  ok = expect(cr::assignSelectedCreativeCatalogEntry(state, hotbar, 8U),
              "numbered assignment accepted") &&
       expect(hotbar.selectedSlot == 8U &&
                  hotbar.entries[8].kind ==
                      cr::CreativeHeldItemKind::VolumeErase,
              "numbered assignment selects and replaces slot") &&
       expect(!cr::assignSelectedCreativeCatalogEntry(state, hotbar, 99U),
              "out-of-range slot rejected") &&
       ok;

  static_cast<void>(cr::setCreativeCatalogQuery(state, "missing"));
  ok = expect(!cr::assignSelectedCreativeCatalogEntry(state, hotbar),
              "empty search cannot assign") &&
       ok;

  static_cast<void>(cr::setCreativeCatalogQuery(state, "Wall"));
  hotbar.selectedSlot = 99U;
  return expect(!cr::assignSelectedCreativeCatalogEntry(state, hotbar),
                "invalid active slot is rejected") &&
         ok;
}

bool modalBindingsAreIsolatedAndDoNotRetrigger() {
  const std::span<const cr::CreativeInputBinding> bindings =
      cr::defaultCreativeInputBindings();
  const cr::CreativeInputBindingAuditResult audit =
      cr::auditCreativeInputBindings(bindings);
  const auto hasBinding = [bindings](cr::CreativeInputActionId action,
                                     cr::CreativeInputKey key,
                                     cr::CreativeInputContext context) {
    return std::any_of(bindings.begin(), bindings.end(),
                       [=](const cr::CreativeInputBinding& binding) {
                         return binding.action == action &&
                                binding.trigger == key &&
                                binding.context == context;
                       });
  };
  bool ok = expect(!bindings.empty() &&
                       bindings.size() <= cr::kCreativeInputBindingCapacity,
                   "catalog registry remains populated within fixed capacity") &&
            expect(!audit.bindingCapacityExceeded && audit.conflictCount == 0U,
                   "catalog bindings remain conflict free") &&
            expect(hasBinding(cr::CreativeInputActionId::ToggleCatalog,
                              cr::CreativeInputKey::E,
                              cr::CreativeInputContext::EditorViewport) &&
                       hasBinding(cr::CreativeInputActionId::ToggleCatalog,
                                  cr::CreativeInputKey::E,
                                  cr::CreativeInputContext::Catalog),
                   "E toggles in viewport and modal contexts") &&
            expect(hasBinding(cr::CreativeInputActionId::ToggleCatalog,
                              cr::CreativeInputKey::GamepadInventory,
                              cr::CreativeInputContext::EditorViewport),
                   "controller inventory opens catalog") &&
            expect(hasBinding(cr::CreativeInputActionId::CatalogConfirm,
                              cr::CreativeInputKey::GamepadConfirm,
                              cr::CreativeInputContext::Catalog) &&
                       hasBinding(cr::CreativeInputActionId::CatalogClose,
                                  cr::CreativeInputKey::GamepadCancel,
                                  cr::CreativeInputContext::Catalog),
                   "controller confirm and cancel own catalog actions") &&
            expect(hasBinding(cr::CreativeInputActionId::CatalogPrevious,
                              cr::CreativeInputKey::GamepadDpadUp,
                              cr::CreativeInputContext::AssetLibrary) &&
                       hasBinding(cr::CreativeInputActionId::CatalogNextVariant,
                                  cr::CreativeInputKey::GamepadDpadRight,
                                  cr::CreativeInputContext::AssetLibrary) &&
                       hasBinding(cr::CreativeInputActionId::CatalogConfirm,
                                  cr::CreativeInputKey::GamepadConfirm,
                                  cr::CreativeInputContext::AssetLibrary) &&
                       hasBinding(cr::CreativeInputActionId::CatalogClose,
                                  cr::CreativeInputKey::GamepadCancel,
                                  cr::CreativeInputContext::AssetLibrary),
                   "asset library owns directional, confirm, and cancel input") &&
            expect(hasBinding(
                       cr::CreativeInputActionId::ToolOptionsPrevious,
                       cr::CreativeInputKey::GamepadDpadUp,
                       cr::CreativeInputContext::AuthoredAssetEditMenu) &&
                       hasBinding(
                           cr::CreativeInputActionId::ConfirmActiveTool,
                           cr::CreativeInputKey::GamepadConfirm,
                           cr::CreativeInputContext::AuthoredAssetEditMenu) &&
                       hasBinding(
                           cr::CreativeInputActionId::CancelActiveTool,
                           cr::CreativeInputKey::GamepadCancel,
                           cr::CreativeInputContext::AuthoredAssetEditMenu),
                   "asset edit menu follows shared select and back semantics") &&
            expect(hasBinding(
                       cr::CreativeInputActionId::CatalogAssignToolWheel,
                       cr::CreativeInputKey::GamepadWest,
                       cr::CreativeInputContext::Catalog),
                   "catalog Square owns contextual wheel assignment") &&
            expect(hasBinding(
                       cr::CreativeInputActionId::CatalogPreviousPage,
                       cr::CreativeInputKey::LeftBracket,
                       cr::CreativeInputContext::Catalog) &&
                       hasBinding(
                           cr::CreativeInputActionId::CatalogNextPage,
                           cr::CreativeInputKey::RightBracket,
                           cr::CreativeInputContext::Catalog) &&
                       hasBinding(
                           cr::CreativeInputActionId::CatalogPreviousPage,
                           cr::CreativeInputKey::GamepadLeftShoulder,
                           cr::CreativeInputContext::Catalog) &&
                       hasBinding(
                           cr::CreativeInputActionId::CatalogNextPage,
                           cr::CreativeInputKey::GamepadRightShoulder,
                           cr::CreativeInputContext::Catalog),
                   "catalog page navigation has keyboard and shoulder bindings") &&
            expect(hasBinding(
                       cr::CreativeInputActionId::CatalogPreviousVariant,
                       cr::CreativeInputKey::ArrowLeft,
                       cr::CreativeInputContext::Catalog) &&
                       hasBinding(
                           cr::CreativeInputActionId::CatalogNextVariant,
                           cr::CreativeInputKey::ArrowRight,
                           cr::CreativeInputContext::Catalog) &&
                       hasBinding(
                           cr::CreativeInputActionId::CatalogPreviousVariant,
                           cr::CreativeInputKey::GamepadDpadLeft,
                           cr::CreativeInputContext::Catalog) &&
                       hasBinding(
                           cr::CreativeInputActionId::CatalogNextVariant,
                           cr::CreativeInputKey::GamepadDpadRight,
                           cr::CreativeInputContext::Catalog),
                   "catalog shape variants own horizontal keyboard and dpad input") &&
            expect(hasBinding(cr::CreativeInputActionId::ToggleToolWheel,
                              cr::CreativeInputKey::R,
                              cr::CreativeInputContext::EditorViewport) &&
                       hasBinding(cr::CreativeInputActionId::ToggleToolWheel,
                                  cr::CreativeInputKey::R,
                                  cr::CreativeInputContext::ToolWheel) &&
                       hasBinding(cr::CreativeInputActionId::ToggleToolWheel,
                                  cr::CreativeInputKey::GamepadRightStick,
                                  cr::CreativeInputContext::EditorViewport) &&
                       hasBinding(cr::CreativeInputActionId::ToggleToolWheel,
                                  cr::CreativeInputKey::GamepadRightStick,
                                  cr::CreativeInputContext::ToolWheel) &&
                       !hasBinding(cr::CreativeInputActionId::ToggleToolWheel,
                                   cr::CreativeInputKey::GamepadDpadRight,
                                   cr::CreativeInputContext::EditorViewport),
                   "tool wheel has keyboard and controller entry actions") &&
            expect(hasBinding(cr::CreativeInputActionId::QuickEditPrevious,
                              cr::CreativeInputKey::GamepadDpadUp,
                              cr::CreativeInputContext::EditorViewport) &&
                       hasBinding(cr::CreativeInputActionId::QuickEditNext,
                                  cr::CreativeInputKey::GamepadDpadDown,
                                  cr::CreativeInputContext::EditorViewport) &&
                       hasBinding(cr::CreativeInputActionId::QuickEditNext,
                                  cr::CreativeInputKey::GamepadWest,
                                  cr::CreativeInputContext::EditorViewport) &&
                       hasBinding(cr::CreativeInputActionId::QuickEditNext,
                                  cr::CreativeInputKey::GamepadWest,
                                  cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(cr::CreativeInputActionId::QuickEditDecrease,
                                  cr::CreativeInputKey::GamepadDpadLeft,
                                  cr::CreativeInputContext::EditorViewport) &&
                       hasBinding(cr::CreativeInputActionId::QuickEditIncrease,
                                  cr::CreativeInputKey::GamepadDpadRight,
                                  cr::CreativeInputContext::EditorViewport),
                   "viewport dpad and Square own bounded quick editing") &&
            expect(hasBinding(cr::CreativeInputActionId::PickAction,
                              cr::CreativeInputKey::GamepadTouchpad,
                              cr::CreativeInputContext::EditorViewport) &&
                       !hasBinding(cr::CreativeInputActionId::PickAction,
                                   cr::CreativeInputKey::GamepadWest,
                                   cr::CreativeInputContext::EditorViewport),
                   "touchpad owns controller pick after Square cycles settings") &&
            expect(hasBinding(cr::CreativeInputActionId::ToolWheelConfirm,
                              cr::CreativeInputKey::GamepadConfirm,
                              cr::CreativeInputContext::ToolWheel) &&
                       hasBinding(cr::CreativeInputActionId::ToolWheelClose,
                                  cr::CreativeInputKey::GamepadCancel,
                                  cr::CreativeInputContext::ToolWheel),
                   "tool wheel confirm and cancel stay in wheel context") &&
            expect(hasBinding(cr::CreativeInputActionId::ToolWheelOptions,
                              cr::CreativeInputKey::O,
                              cr::CreativeInputContext::ToolWheel) &&
                       hasBinding(cr::CreativeInputActionId::ToolWheelOptions,
                                  cr::CreativeInputKey::GamepadWest,
                                  cr::CreativeInputContext::ToolWheel),
                   "tool wheel options has dedicated keyboard and PS5 input") &&
            expect(hasBinding(cr::CreativeInputActionId::ToolOptionsPrevious,
                              cr::CreativeInputKey::GamepadDpadUp,
                              cr::CreativeInputContext::ToolOptions) &&
                       hasBinding(cr::CreativeInputActionId::ToolOptionsDecrease,
                                  cr::CreativeInputKey::GamepadDpadLeft,
                                  cr::CreativeInputContext::ToolOptions) &&
                       hasBinding(cr::CreativeInputActionId::ToolOptionsIncrease,
                                  cr::CreativeInputKey::GamepadDpadRight,
                                  cr::CreativeInputContext::ToolOptions) &&
                       hasBinding(cr::CreativeInputActionId::ToolOptionsConfirm,
                                  cr::CreativeInputKey::GamepadConfirm,
                                  cr::CreativeInputContext::ToolOptions),
                   "tool options own directional editing and confirm") &&
            expect(hasBinding(cr::CreativeInputActionId::ConfirmActiveTool,
                              cr::CreativeInputKey::Enter,
                              cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(cr::CreativeInputActionId::ConfirmActiveTool,
                                  cr::CreativeInputKey::GamepadConfirm,
                                  cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(cr::CreativeInputActionId::CancelActiveTool,
                                  cr::CreativeInputKey::Escape,
                                  cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(cr::CreativeInputActionId::CancelActiveTool,
                                  cr::CreativeInputKey::GamepadCancel,
                                  cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(cr::CreativeInputActionId::CancelActiveTool,
                                  cr::CreativeInputKey::Delete,
                                  cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(cr::CreativeInputActionId::CancelActiveTool,
                                  cr::CreativeInputKey::Backspace,
                                  cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::ToggleTransformControls,
                           cr::CreativeInputKey::R,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::ToggleTransformControls,
                           cr::CreativeInputKey::GamepadRightStick,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::TransformControlPrevious,
                           cr::CreativeInputKey::GamepadDpadUp,
                           cr::CreativeInputContext::TransformControls) &&
                       hasBinding(
                           cr::CreativeInputActionId::TransformControlNext,
                           cr::CreativeInputKey::GamepadDpadDown,
                           cr::CreativeInputContext::TransformControls) &&
                       hasBinding(
                           cr::CreativeInputActionId::TransformConstraintX,
                           cr::CreativeInputKey::X,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::TransformConstraintY,
                           cr::CreativeInputKey::Y,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::TransformConstraintZ,
                           cr::CreativeInputKey::Z,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::TransformNudgePositive,
                           cr::CreativeInputKey::GamepadDpadUp,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::QuickEditDecrease,
                           cr::CreativeInputKey::GamepadDpadLeft,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::QuickEditIncrease,
                           cr::CreativeInputKey::GamepadDpadRight,
                           cr::CreativeInputContext::TransformPreview) &&
                       hasBinding(
                           cr::CreativeInputActionId::ConfirmActiveTool,
                           cr::CreativeInputKey::GamepadConfirm,
                           cr::CreativeInputContext::TransformControls),
                   "transform preview and controls have isolated semantic input");

  cr::CreativeInputRouterState router;
  cr::CreativeInputFrame frame;
  frame.context = cr::CreativeInputContext::EditorViewport;
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::E, true);
  const cr::CreativeInputRouteResult opened =
      cr::routeCreativeInput(router, frame, bindings);
  frame.context = cr::CreativeInputContext::Catalog;
  const cr::CreativeInputRouteResult held =
      cr::routeCreativeInput(router, frame, bindings);
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::E, false);
  static_cast<void>(cr::routeCreativeInput(router, frame, bindings));
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::E, true);
  const cr::CreativeInputRouteResult closed =
      cr::routeCreativeInput(router, frame, bindings);

  ok = expect(opened.actionCount == 1U &&
                  opened.actions[0].action ==
                      cr::CreativeInputActionId::ToggleCatalog,
              "viewport E emits open action") &&
       expect(held.actionCount == 0U,
              "held E does not retrigger after modal transition") &&
       expect(closed.actionCount == 1U &&
                  closed.actions[0].action ==
                      cr::CreativeInputActionId::ToggleCatalog,
              "released modal E emits close on next press") &&
       ok;

  cr::CreativeInputRouterState pageRouter;
  cr::CreativeInputFrame pageFrame;
  pageFrame.context = cr::CreativeInputContext::Catalog;
  cr::setCreativeInputKey(pageFrame,
                          cr::CreativeInputKey::GamepadRightShoulder, true);
  const cr::CreativeInputRouteResult pagePressed =
      cr::routeCreativeInput(pageRouter, pageFrame, bindings);
  const cr::CreativeInputRouteResult pageHeld =
      cr::routeCreativeInput(pageRouter, pageFrame, bindings);
  ok = expect(pagePressed.actionCount == 1U &&
                  pagePressed.actions[0].action ==
                      cr::CreativeInputActionId::CatalogNextPage,
              "controller shoulder emits catalog page action") &&
       expect(pageHeld.actionCount == 0U,
              "held controller shoulder does not repeat page action") &&
       ok;

  cr::CreativeInputRouterState wheelRouter;
  cr::CreativeInputFrame wheelFrame;
  wheelFrame.context = cr::CreativeInputContext::EditorViewport;
  cr::setCreativeInputKey(wheelFrame, cr::CreativeInputKey::R, true);
  const cr::CreativeInputRouteResult wheelOpened =
      cr::routeCreativeInput(wheelRouter, wheelFrame, bindings);
  wheelFrame.context = cr::CreativeInputContext::ToolWheel;
  const cr::CreativeInputRouteResult wheelHeld =
      cr::routeCreativeInput(wheelRouter, wheelFrame, bindings);
  ok = expect(wheelOpened.actionCount == 1U &&
                  wheelOpened.actions[0].action ==
                      cr::CreativeInputActionId::ToggleToolWheel,
              "viewport R emits wheel-open action") &&
       expect(wheelHeld.actionCount == 0U,
              "held R does not retrigger in tool-wheel context") &&
       ok;

  cr::CreativeInputRouterState wheelOptionsRouter;
  cr::CreativeInputFrame wheelOptionsFrame;
  wheelOptionsFrame.context = cr::CreativeInputContext::ToolWheel;
  cr::setCreativeInputKey(wheelOptionsFrame,
                          cr::CreativeInputKey::GamepadWest, true);
  const cr::CreativeInputRouteResult wheelOptionsPressed =
      cr::routeCreativeInput(wheelOptionsRouter, wheelOptionsFrame, bindings);
  const cr::CreativeInputRouteResult wheelOptionsHeld =
      cr::routeCreativeInput(wheelOptionsRouter, wheelOptionsFrame, bindings);
  ok = expect(wheelOptionsPressed.actionCount == 1U &&
                  wheelOptionsPressed.actions[0].action ==
                      cr::CreativeInputActionId::ToolWheelOptions,
              "tool-wheel Square emits dedicated options action") &&
       expect(wheelOptionsHeld.actionCount == 0U,
              "held Square does not repeat tool options") &&
       ok;

  cr::CreativeInputRouterState quickEditRouter;
  cr::CreativeInputFrame quickEditFrame;
  quickEditFrame.context = cr::CreativeInputContext::EditorViewport;
  cr::setCreativeInputKey(quickEditFrame,
                          cr::CreativeInputKey::GamepadWest, true);
  const cr::CreativeInputRouteResult quickEditPressed =
      cr::routeCreativeInput(quickEditRouter, quickEditFrame, bindings);
  const cr::CreativeInputRouteResult quickEditHeld =
      cr::routeCreativeInput(quickEditRouter, quickEditFrame, bindings);
  ok = expect(quickEditPressed.actionCount == 1U &&
                  quickEditPressed.actions[0].action ==
                      cr::CreativeInputActionId::QuickEditNext,
              "viewport Square emits one next-setting action") &&
       expect(quickEditHeld.actionCount == 0U,
              "held viewport Square does not repeat setting changes") &&
       ok;

  cr::CreativeInputRouterState transformModeRouter;
  cr::CreativeInputFrame transformModeFrame;
  transformModeFrame.context = cr::CreativeInputContext::TransformPreview;
  cr::setCreativeInputKey(transformModeFrame,
                          cr::CreativeInputKey::GamepadWest, true);
  const cr::CreativeInputRouteResult transformModePressed =
      cr::routeCreativeInput(transformModeRouter, transformModeFrame,
                             bindings);
  ok = expect(transformModePressed.actionCount == 1U &&
                  transformModePressed.actions[0].action ==
                      cr::CreativeInputActionId::QuickEditNext,
              "transform-preview Square emits one mode-cycle action") &&
       ok;

  cr::CreativeInputFrame touchpadFrame;
  touchpadFrame.context = cr::CreativeInputContext::EditorViewport;
  cr::setCreativeInputKey(touchpadFrame,
                          cr::CreativeInputKey::GamepadTouchpad, true);
  ok = expect(cr::creativeInputActionDown(
                  touchpadFrame, cr::CreativeInputActionId::PickAction,
                  bindings),
              "touchpad preserves continuous pick and sample input") &&
       ok;

  cr::CreativeInputRouterState wheelSecondaryRouter;
  cr::CreativeInputFrame wheelSecondaryFrame;
  wheelSecondaryFrame.context = cr::CreativeInputContext::ToolWheel;
  cr::setCreativeInputKey(wheelSecondaryFrame,
                          cr::CreativeInputKey::GamepadLeftTrigger, true);
  const cr::CreativeInputRouteResult wheelSecondaryRouted =
      cr::routeCreativeInput(wheelSecondaryRouter, wheelSecondaryFrame,
                             bindings);
  ok = expect(!cr::creativeInputActionDown(
                  wheelSecondaryFrame,
                  cr::CreativeInputActionId::FlyDown, bindings,
                  &wheelSecondaryRouted) &&
                  wheelSecondaryRouted.actionCount == 0U,
              "tool-wheel L2 cannot leak into flight") &&
       ok;

  cr::CreativeInputRouterState optionRouter;
  cr::CreativeInputFrame optionFrame;
  optionFrame.context = cr::CreativeInputContext::ToolOptions;
  cr::setCreativeInputKey(optionFrame, cr::CreativeInputKey::ArrowRight, true);
  const cr::CreativeInputRouteResult optionAdjusted =
      cr::routeCreativeInput(optionRouter, optionFrame, bindings);
  ok = expect(optionAdjusted.actionCount == 1U &&
                  optionAdjusted.actions[0].action ==
                      cr::CreativeInputActionId::ToolOptionsIncrease,
              "tool-options right arrow emits semantic increase") &&
       ok;

  cr::CreativeInputRouterState previewRouter;
  cr::CreativeInputFrame previewFrame;
  previewFrame.context = cr::CreativeInputContext::TransformPreview;
  cr::setCreativeInputKey(previewFrame, cr::CreativeInputKey::Enter, true);
  const cr::CreativeInputRouteResult previewConfirmed =
      cr::routeCreativeInput(previewRouter, previewFrame, bindings);
  cr::setCreativeInputKey(previewFrame, cr::CreativeInputKey::V, true);
  previewFrame.modifiers = cr::kCreativeInputModifierControl;
  const cr::CreativeInputRouteResult previewIsolated =
      cr::routeCreativeInput(previewRouter, previewFrame, bindings);
  ok = expect(previewConfirmed.actionCount == 1U &&
                  previewConfirmed.actions[0].action ==
                      cr::CreativeInputActionId::ConfirmActiveTool,
              "transform preview Enter emits confirm") &&
       expect(previewIsolated.actionCount == 0U,
              "transform preview suppresses viewport paste shortcut") &&
       ok;

  cr::CreativeInputRouterState transformRouter;
  cr::CreativeInputFrame transformFrame;
  transformFrame.context = cr::CreativeInputContext::TransformPreview;
  cr::setCreativeInputKey(transformFrame, cr::CreativeInputKey::R, true);
  const cr::CreativeInputRouteResult transformOpened =
      cr::routeCreativeInput(transformRouter, transformFrame, bindings);
  transformFrame.context = cr::CreativeInputContext::TransformControls;
  const cr::CreativeInputRouteResult transformHeld =
      cr::routeCreativeInput(transformRouter, transformFrame, bindings);
  ok = expect(transformOpened.actionCount == 1U &&
                  transformOpened.actions[0].action ==
                      cr::CreativeInputActionId::ToggleTransformControls,
              "transform preview R opens contextual controls") &&
       expect(transformHeld.actionCount == 0U,
              "held transform toggle does not retrigger after context change") &&
       ok;

  cr::CreativeInputRouterState precisionRouter;
  cr::CreativeInputFrame precisionFrame;
  precisionFrame.context = cr::CreativeInputContext::TransformPreview;
  cr::setCreativeInputKey(precisionFrame, cr::CreativeInputKey::X, true);
  const cr::CreativeInputRouteResult axisPressed =
      cr::routeCreativeInput(precisionRouter, precisionFrame, bindings);
  cr::setCreativeInputKey(precisionFrame, cr::CreativeInputKey::X, false);
  static_cast<void>(
      cr::routeCreativeInput(precisionRouter, precisionFrame, bindings));
  precisionFrame.modifiers = cr::kCreativeInputModifierShift;
  cr::setCreativeInputKey(precisionFrame,
                          cr::CreativeInputKey::GamepadDpadUp, true);
  const cr::CreativeInputRouteResult firstNudge =
      cr::routeCreativeInput(precisionRouter, precisionFrame, bindings);
  const cr::CreativeInputRouteResult heldNudge =
      cr::routeCreativeInput(precisionRouter, precisionFrame, bindings);
  cr::setCreativeInputKey(precisionFrame,
                          cr::CreativeInputKey::GamepadDpadUp, false);
  static_cast<void>(
      cr::routeCreativeInput(precisionRouter, precisionFrame, bindings));
  cr::setCreativeInputKey(precisionFrame,
                          cr::CreativeInputKey::GamepadDpadUp, true);
  const cr::CreativeInputRouteResult repeatedNudge =
      cr::routeCreativeInput(precisionRouter, precisionFrame, bindings);
  ok = expect(axisPressed.actionCount == 1U &&
                  axisPressed.actions[0].action ==
                      cr::CreativeInputActionId::TransformConstraintX,
              "preview X emits axis constraint") &&
       expect(firstNudge.actionCount == 1U &&
                  firstNudge.actions[0].action ==
                      cr::CreativeInputActionId::TransformNudgePositive &&
                  heldNudge.actionCount == 0U &&
                  repeatedNudge.actionCount == 1U,
              "fine nudge emits once per physical controller-style press") &&
       ok;
  return ok;
}

bool assetRemovalCompactsCatalogAndToolWheelIndices() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall};
  std::array<cr::CreativeCatalogAsset, 3> assets{};
  assets[0].objectKind = cr::CreativeObjectKind::Crate;
  assets[0].assetId = "authored_0001";
  assets[0].label = "First";
  assets[0].sourceBounds = {{-0.5, 0.0, -0.5}, {0.5, 1.0, 0.5}};
  assets[1] = assets[0];
  assets[1].assetId = "authored_0002";
  assets[1].label = "Second";
  assets[2] = assets[0];
  assets[2].assetId = "authored_0003";
  assets[2].label = "Third";
  cr::CreativeCatalogState state = cr::makeCreativeCatalog(palette, assets);
  const auto second = std::find_if(
      state.entries.begin(), state.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return cr::creativeHotbarAssetId(entry.hotbarEntry) == "authored_0002";
      });
  if (!expect(second != state.entries.end(),
              "asset removal fixture contains the middle asset")) {
    return false;
  }
  const std::size_t removedIndex =
      static_cast<std::size_t>(std::distance(state.entries.begin(), second));
  cr::CreativeToolWheelState wheel;
  wheel.open = true;
  wheel.entryCount = 3U;
  wheel.selectedIndex = 2U;
  wheel.catalogEntryIndices[0] = removedIndex - 1U;
  wheel.catalogEntryIndices[1] = removedIndex;
  wheel.catalogEntryIndices[2] = removedIndex + 1U;

  cr::CreativeToolWheelState shiftedSelection = wheel;
  shiftedSelection.selectedIndex = 2U;
  shiftedSelection.catalogEntryIndices[0] = removedIndex;
  shiftedSelection.catalogEntryIndices[1] = removedIndex + 1U;
  shiftedSelection.catalogEntryIndices[2] = removedIndex + 2U;

  std::size_t reportedIndex = std::numeric_limits<std::size_t>::max();
  const bool removed =
      cr::removeCreativeCatalogAsset(state, "authored_0002", &reportedIndex);
  const bool wheelChanged =
      cr::removeCreativeToolWheelCatalogEntry(wheel, reportedIndex);
  const bool shiftedSelectionChanged =
      cr::removeCreativeToolWheelCatalogEntry(shiftedSelection, reportedIndex);

  return expect(removed && reportedIndex == removedIndex &&
                    std::none_of(state.entries.begin(), state.entries.end(),
                                 [](const cr::CreativeCatalogEntry& entry) {
                                   return cr::creativeHotbarAssetId(
                                              entry.hotbarEntry) ==
                                          "authored_0002";
                                 }),
                "catalog removes exactly the requested authored identity") &&
         expect(
             wheelChanged && wheel.entryCount == 2U &&
                 wheel.catalogEntryIndices[0] == removedIndex - 1U &&
                 wheel.catalogEntryIndices[1] == removedIndex &&
                 wheel.selectedIndex == 1U,
             "tool wheel drops the removed index and shifts later entries") &&
         expect(shiftedSelectionChanged &&
                    shiftedSelection.entryCount == 2U &&
                    shiftedSelection.selectedIndex == 1U &&
                    shiftedSelection.catalogEntryIndices[1] == removedIndex + 1U,
                "tool wheel preserves a selected sector shifted by deletion") &&
         expect(!cr::removeCreativeCatalogAsset(state, "authored_0002") &&
                    !cr::removeCreativeToolWheelCatalogEntry(
                        wheel, state.entries.size() + 10U),
                "repeated and unrelated removals are no-ops");
}

bool assetPagePreservesBuildIndicesAndEquipsDurableIdentity() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectKind::Crate};
  std::array<cr::CreativeCatalogAsset, 2> assets{};
  assets[0].objectKind = cr::CreativeObjectKind::Rock;
  assets[0].assetId = "boulder_01";
  assets[0].label = "Boulder 01";
  assets[0].sourceBounds = {{-0.25, -0.2, -1.0}, {1.25, 1.2, 0.2}};
  assets[0].authoringMetadata.status =
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored;
  assets[0].authoringMetadata.categoryId = "boulder";
  assets[0].authoringMetadata.collisionSpecified = true;
  assets[1].objectKind = cr::CreativeObjectKind::Bridge;
  assets[1].assetId = "walkway_stone_01";
  assets[1].label = "Walkway Stone 01";
  assets[1].sourceBounds = {{-1.5, 0.0, -0.6}, {1.5, 0.3, 0.6}};
  assets[1].authoringMetadata.status =
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored;
  assets[1].authoringMetadata.categoryId = "walkway";
  assets[1].authoringMetadata.collisionSpecified = true;
  assets[1].authoringMetadata.walkable = true;
  assets[1].authoringMetadata.walkableSpecified = true;
  cr::CreativeCatalogState state =
      cr::makeCreativeCatalog(palette, assets, 1U);
  const std::size_t structureCount = state.filteredEntryIndices.size();
  const bool pageChanged = cr::setCreativeCatalogPage(
      state, cr::CreativeCatalogPage::Assets);
  const cr::CreativeCatalogEntry* first =
      cr::selectedCreativeCatalogEntry(state);
  const bool assetPageReady =
      pageChanged && state.page == cr::CreativeCatalogPage::Assets &&
      state.filteredEntryIndices.size() == assets.size() + 1U &&
      state.rejectedAssetCount == 1U;
  cr::CreativeHotbarState hotbar = cr::makeDefaultCreativeHotbar(palette);
  const bool assigned = cr::assignSelectedCreativeCatalogEntry(state, hotbar);
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(hotbar);
  const bool searched =
      cr::setCreativeCatalogQuery(state, "walkway");
  const cr::CreativeCatalogEntry* walkway =
      cr::selectedCreativeCatalogEntry(state);
  iggy3d::StaticMeshAuthoringMetadata decor;
  decor.status = iggy3d::StaticMeshAuthoringMetadataStatus::Authored;
  decor.collisionMode = iggy3d::StaticMeshCollisionMode::None;
  decor.collisionSpecified = true;
  iggy3d::StaticMeshAuthoringMetadata unsupported;
  unsupported.status =
      iggy3d::StaticMeshAuthoringMetadataStatus::UnsupportedCollision;
  unsupported.collisionMode = iggy3d::StaticMeshCollisionMode::Convex;
  unsupported.collisionSpecified = true;
  iggy3d::StaticMeshAuthoringMetadata compound;
  compound.status = iggy3d::StaticMeshAuthoringMetadataStatus::Authored;
  compound.collisionMode = iggy3d::StaticMeshCollisionMode::CompoundBounds;
  compound.collisionSpecified = true;
  compound.walkable = true;
  iggy3d::StaticMeshAuthoringMetadata invalid;
  invalid.status = iggy3d::StaticMeshAuthoringMetadataStatus::Invalid;
  invalid.collisionMode = iggy3d::StaticMeshCollisionMode::Invalid;
  static_cast<void>(cr::setCreativeCatalogPage(
      state, cr::CreativeCatalogPage::Actions));

  return expect(structureCount == 1U,
                "assets do not shift the selected creator category") &&
         expect(assetPageReady, "asset page is reachable before actions") &&
         expect(first != nullptr &&
                    first->category == cr::CreativeCatalogEntryCategory::Asset &&
                    cr::creativeHotbarAssetId(first->hotbarEntry) ==
                        "boulder_01",
                "asset page exposes the first discovered mesh") &&
         expect(assigned && held.kind == cr::CreativeHeldItemKind::Material &&
                    held.objectKind == cr::CreativeObjectKind::Rock &&
                    cr::creativeHotbarAssetId(held) == "boulder_01" &&
                    held.hasAssetBounds &&
                    cr::creativeBoundsExactlyEqual(
                        held.assetSourceBounds, assets[0].sourceBounds),
                "asset assignment carries identity and source-space bounds") &&
         expect(searched && walkway != nullptr &&
                    cr::creativeHotbarAssetId(walkway->hotbarEntry) ==
                        "walkway_stone_01" &&
                    walkway->assetAuthoringMetadata.walkable &&
                    walkway->assetAuthoringMetadata.categoryId == "walkway",
                "asset search retains physics status in the asset lane") &&
         expect(cr::creativeCatalogAssetPhysicsLabel(
                    assets[0].authoringMetadata) == "SOLID" &&
                    cr::creativeCatalogAssetPhysicsLabel(
                        assets[1].authoringMetadata) == "SOLID WALKABLE" &&
                    cr::creativeCatalogAssetPhysicsLabel({}) ==
                        "SOLID DEFAULT" &&
                    cr::creativeCatalogAssetPhysicsLabel(decor) == "DECOR" &&
                 cr::creativeCatalogAssetPhysicsLabel(compound) ==
                     "COMPOUND WALKABLE" &&
                    cr::creativeCatalogAssetPhysicsLabel(unsupported) ==
                        "UNSUPPORTED" &&
                    cr::creativeCatalogAssetPhysicsLabel(invalid) == "INVALID",
                "asset physics labels expose every catalog state") &&
         expect(state.filteredEntryIndices.empty(),
                "actions page does not expose build or asset rows");
}

bool assetReloadCommandAndFailuresAreExplicitAndNotAssignable() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall};
  cr::CreativeCatalogAsset asset;
  asset.objectKind = cr::CreativeObjectKind::Rock;
  asset.assetId = "boulder_01";
  asset.label = "Boulder 01";
  asset.sourceBounds = {{-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}};
  const cr::CreativeCatalogAssetFailure failure{
      "broken_prop", "assets/creative/broken_prop.glb",
      "static_mesh_glb_parse_failed"};
  cr::CreativeCatalogState state =
      cr::makeCreativeCatalog(palette, std::span{&asset, 1U}, 1U,
                              std::span{&failure, 1U});
  static_cast<void>(
      cr::setCreativeCatalogPage(state, cr::CreativeCatalogPage::Assets));
  const bool selectedFailure =
      cr::setCreativeCatalogQuery(state, "broken_prop");
  const cr::CreativeCatalogEntry* failureEntry =
      cr::selectedCreativeCatalogEntry(state);
  cr::CreativeHotbarState hotbar = cr::makeDefaultCreativeHotbar(palette);
  const bool failureAssigned =
      cr::assignSelectedCreativeCatalogEntry(state, hotbar);
  const bool selectedReload = cr::setCreativeCatalogQuery(state, "reload");
  const cr::CreativeCatalogEntry* reload =
      cr::selectedCreativeCatalogEntry(state);
  const bool reloadAssigned =
      cr::assignSelectedCreativeCatalogEntry(state, hotbar);

  return expect(selectedFailure && failureEntry != nullptr &&
                    failureEntry->category ==
                        cr::CreativeCatalogEntryCategory::AssetFailure &&
                    failureEntry->detail.find("static_mesh_glb_parse_failed") !=
                        std::string::npos &&
                    !failureAssigned,
                "failed imports remain visible with a reason and cannot equip") &&
         expect(selectedReload && reload != nullptr &&
                    cr::creativeCatalogEntryRequestsAssetReload(*reload) &&
                    !cr::creativeCatalogEntryAssignable(*reload) &&
                    !reloadAssigned,
                "reload is a typed command rather than a fake hotbar item");
}

}  // namespace

int main() {
  bool ok = true;
  ok = actionPageIsFixedNavigableAndConfirmationSafe() && ok;
  ok = actionAvailabilityUsesExplicitFacts() && ok;
  ok = catalogBuildsMaterialsAndCreatorTools() && ok;
  ok = catalogPartitionsDescriptorPaletteAndSearchesCreatorCategories() && ok;
  ok = catalogOmitsToolsWithoutRequiredMaterial() && ok;
  ok = catalogAssignmentDistinguishesMaterialsFromTools() && ok;
  ok = shapeSelectionIsVisibleBoundedAndDeterministic() && ok;
  ok = toolWheelIsBoundedDirectionalAndAssignable() && ok;
  ok = searchIsCaseInsensitiveBoundedAndStable() && ok;
  ok = selectionWrapsAndAssignmentsAreExplicit() && ok;
  ok = modalBindingsAreIsolatedAndDoNotRetrigger() && ok;
  ok = assetRemovalCompactsCatalogAndToolWheelIndices() && ok;
  ok = assetPagePreservesBuildIndicesAndEquipsDurableIdentity() && ok;
  ok = assetReloadCommandAndFailuresAreExplicitAndNotAssignable() && ok;
  return ok ? 0 : 1;
}
