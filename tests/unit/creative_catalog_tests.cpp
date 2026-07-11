#include "app/iggy3d/creative/input/Catalog.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"

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
            expect(state.page == cr::CreativeCatalogPage::Build &&
                       cr::toString(state.page) == "Build" &&
                       cr::toString(cr::CreativeCatalogPage::Actions) ==
                           "Actions",
                   "catalog defaults to named build page");

  ok = expect(cr::moveCreativeCatalogPage(state, -1) &&
                  state.page == cr::CreativeCatalogPage::Actions &&
                  cr::moveCreativeCatalogPage(state, 1) &&
                  state.page == cr::CreativeCatalogPage::Build,
              "catalog pages wrap in both directions") &&
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
  return expect(cr::creativeCatalogActionAvailable(
                    cr::CreativeInputActionId::Undo, availability) &&
                    cr::creativeCatalogActionAvailable(
                        cr::CreativeInputActionId::PasteClipboard,
                        availability),
                "history and clipboard enable their actions") &&
         ok;
}

bool catalogBuildsMaterialsAndCreatorTools() {
  const cr::CreativeCatalogState state = catalog();
  bool ok = expect(state.entries.size() == 11U,
                   "two materials plus nine catalog tools") &&
            expect(state.filteredEntryIndices.size() == state.entries.size(),
                   "empty query exposes every entry") &&
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
                           cr::CreativeHeldItemKind::ObjectSelect,
                   "tool lane follows materials");
  ok = expect(cr::toString(cr::CreativeCatalogEntryCategory::Material) ==
                  "Material" &&
                  cr::toString(cr::CreativeCatalogEntryCategory::Tool) ==
                  "Tool",
              "catalog categories have stable labels") &&
       ok;
  return ok;
}

bool catalogOmitsToolsWithoutRequiredMaterial() {
  const cr::CreativeCatalogState state = cr::makeCreativeCatalog({});
  const bool materialDependentToolPresent = std::any_of(
      state.entries.begin(), state.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind == cr::CreativeHeldItemKind::VolumeFill ||
               entry.hotbarEntry.kind ==
                   cr::CreativeHeldItemKind::VolumeHollow ||
               entry.hotbarEntry.kind ==
                   cr::CreativeHeldItemKind::VolumeReplace;
      });
  return expect(state.entries.size() == 6U,
                "empty palette retains material-independent tools") &&
         expect(!materialDependentToolPresent,
                "material-dependent tools require a valid material");
}

bool catalogAssignmentDistinguishesMaterialsFromTools() {
  const cr::CreativeCatalogState state = catalog();
  const cr::CreativeHotbarEntry material =
      cr::resolveCreativeCatalogHotbarEntry(
          state.entries[1], cr::CreativeObjectKind::Wall);
  const cr::CreativeHotbarEntry fill =
      cr::resolveCreativeCatalogHotbarEntry(
          state.entries[5], cr::CreativeObjectKind::Crate);
  return expect(material.kind == cr::CreativeHeldItemKind::Material &&
                    material.objectKind == cr::CreativeObjectKind::Crate,
                "clicked material is not replaced by active brush") &&
         expect(fill.kind == cr::CreativeHeldItemKind::VolumeFill &&
                    fill.objectKind == cr::CreativeObjectKind::Crate,
                "material-dependent tool inherits active brush");
}

bool shapeSelectionIsVisibleBoundedAndDeterministic() {
  const cr::CreativeCatalogState state = catalog();
  const cr::CreativeCatalogEntry& fill = state.entries[5];
  cr::CreativeCatalogShapeSelection selection =
      cr::normalizeCreativeCatalogShapeSelection(
          cr::CreativeShapeBrushKind::Box,
          cr::CreativeShapeBrushAxis::X);
  bool ok = expect(cr::creativeCatalogEntryUsesShapeSelection(fill),
                   "fill exposes catalog shape selection") &&
            expect(!cr::creativeCatalogEntryUsesShapeSelection(
                       state.entries[4]),
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
                   "all eight creator tools populate the bounded wheel") &&
            expect(!wheel.capacityExceeded,
                   "default tool wheel fits its fixed capacity") &&
            expect(cr::selectedCreativeToolWheelEntry(wheel, catalogState) !=
                       nullptr &&
                       cr::selectedCreativeToolWheelEntry(wheel, catalogState)
                               ->hotbarEntry.kind ==
                           cr::CreativeHeldItemKind::ObjectSelect,
                   "first tool owns the up sector");
  const auto erase = std::find_if(
      catalogState.entries.begin(), catalogState.entries.end(),
      [](const cr::CreativeCatalogEntry& entry) {
        return entry.hotbarEntry.kind ==
               cr::CreativeHeldItemKind::VolumeErase;
      });
  ok = expect(erase != catalogState.entries.end() &&
                  !erase->toolWheelEligible,
              "erase remains catalog-only") &&
       expect(wheel.catalogEntryIndices[7] < catalogState.entries.size() &&
                  catalogState.entries[wheel.catalogEntryIndices[7]]
                          .hotbarEntry.kind ==
                      cr::CreativeHeldItemKind::LinearArray,
              "array owns the eighth wheel sector") &&
       ok;

  const std::array directions{
      std::array{0.0F, 1.0F},   std::array{1.0F, 1.0F},
      std::array{1.0F, 0.0F},   std::array{1.0F, -1.0F},
      std::array{0.0F, -1.0F},  std::array{-1.0F, -1.0F},
      std::array{-1.0F, 0.0F},  std::array{-1.0F, 1.0F},
  };
  for (std::size_t index = 0; index < directions.size(); ++index) {
    static_cast<void>(cr::selectCreativeToolWheelDirection(
        wheel, directions[index][0], directions[index][1]));
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
  ok = expect(selectDegrees(22.0F) == 0U &&
                  selectDegrees(23.0F) == 1U,
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
                  wheel.selectedIndex == 7U,
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

  cr::CreativeCatalogState oversized;
  for (std::size_t index = 0; index < 9U; ++index) {
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
                   "crate query finds one material") &&
            expect(cr::selectedCreativeCatalogEntry(state) != nullptr &&
                       cr::selectedCreativeCatalogEntry(state)
                               ->hotbarEntry.objectKind ==
                           cr::CreativeObjectKind::Crate,
                   "crate result selected");

  ok = expect(cr::setCreativeCatalogQuery(state, "REGION"),
              "tool alias query accepted") &&
       expect(state.filteredEntryIndices.size() == 6U,
              "region query finds selection and five operations") &&
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

  bool ok = expect(cr::moveCreativeCatalogSelection(state, -1) &&
                       state.selectedFilteredIndex == 10U,
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
                                  cr::CreativeInputKey::GamepadDpadRight,
                                  cr::CreativeInputContext::EditorViewport),
                   "tool wheel has keyboard and controller entry actions") &&
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
                           cr::CreativeInputKey::GamepadDpadRight,
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
                  cr::CreativeInputActionId::SecondaryAction, bindings,
                  &wheelSecondaryRouted) &&
                  wheelSecondaryRouted.actionCount == 0U,
              "tool-wheel L2 cannot leak into world secondary action") &&
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

}  // namespace

int main() {
  bool ok = true;
  ok = actionPageIsFixedNavigableAndConfirmationSafe() && ok;
  ok = actionAvailabilityUsesExplicitFacts() && ok;
  ok = catalogBuildsMaterialsAndCreatorTools() && ok;
  ok = catalogOmitsToolsWithoutRequiredMaterial() && ok;
  ok = catalogAssignmentDistinguishesMaterialsFromTools() && ok;
  ok = shapeSelectionIsVisibleBoundedAndDeterministic() && ok;
  ok = toolWheelIsBoundedDirectionalAndAssignable() && ok;
  ok = searchIsCaseInsensitiveBoundedAndStable() && ok;
  ok = selectionWrapsAndAssignmentsAreExplicit() && ok;
  ok = modalBindingsAreIsolatedAndDoNotRetrigger() && ok;
  return ok ? 0 : 1;
}
