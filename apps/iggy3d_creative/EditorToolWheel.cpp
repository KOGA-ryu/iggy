#include "EditorCatalogInternal.hpp"

#include "EditorToolWheelPreferences.hpp"

#include <algorithm>
#include <cmath>

#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] bool assignToolWheelSelection(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeToolWheelEntry(editor.catalog.toolWheel,
                                         editor.catalog.model);
  if (selected == nullptr) {
    return false;
  }
  const std::size_t targetSlot =
      std::min<std::size_t>(editor.interaction.hotbar.selectedSlot,
                            cr::kCreativeHotbarSlotCount - 1U);
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  const cr::CreativeHeldItemKind previousKind =
      editor.interaction.hotbar.entries[targetSlot].kind;
  static_cast<void>(cr::assignSelectedCreativeToolWheelEntry(
      editor.catalog.toolWheel, editor.catalog.model,
      editor.interaction.hotbar));
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  held = cr::resolveCreativeCatalogHotbarEntry(*selected, editor.placeBrush);
  if (previousKind != held.kind) {
    clearCreativeMaterialBrushPresetSlot(
        editor.interaction.materialBrushPresets, targetSlot);
  }
  static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  syncCreativeEditorHeldItem(appState, editor);
  return true;
}

}  // namespace

void finishToolWheelAssignment(CreativeEditorCatalogState& state,
                               bool reopenCatalog) noexcept {
  state.toolWheelAssignmentCatalogEntryIndex.reset();
  static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, false));
  if (reopenCatalog) {
    static_cast<void>(cr::setCreativeCatalogOpen(state.model, true));
  }
}

namespace {

[[nodiscard]] bool commitToolWheelAssignment(
    const CreativeEditorCatalogFrameRequest& request,
    CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& state = request.editor.catalog;
  if (!state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
    return false;
  }
  const std::size_t sector = state.toolWheel.selectedIndex;
  const bool changed = cr::assignCreativeToolWheelCatalogEntry(
      state.toolWheel, state.model, sector,
      *state.toolWheelAssignmentCatalogEntryIndex);
  result.toolWheelChanged = changed;
  if (changed) {
    const CreativeEditorToolWheelPersistenceReceipt saved =
        saveCreativeEditorToolWheel(state.toolWheel, state.model,
                                    request.toolWheelSettingsPath);
    result.toolWheelSaved =
        saved.status == CreativeEditorToolWheelPersistenceStatus::Saved;
    state.statusLabel = result.toolWheelSaved ? "TOOL WHEEL UPDATED"
                                              : "TOOL WHEEL SAVE FAILED";
  } else {
    state.statusLabel = "TOOL WHEEL UNCHANGED";
  }
  finishToolWheelAssignment(state, true);
  return true;
}

}  // namespace

[[nodiscard]] bool beginToolWheelAssignment(
    CreativeEditorCatalogState& state) noexcept {
  const std::optional<std::size_t> catalogIndex =
      cr::selectedCreativeCatalogEntryIndex(state.model);
  if (!catalogIndex.has_value() ||
      state.model.entries[*catalogIndex].category !=
          cr::CreativeCatalogEntryCategory::Tool ||
      state.toolWheel.entryCount == 0U) {
    return false;
  }
  state.toolWheelAssignmentCatalogEntryIndex = *catalogIndex;
  if (const std::optional<std::size_t> existing =
          cr::creativeToolWheelSectorForCatalogEntry(state.toolWheel,
                                                     *catalogIndex);
      existing.has_value()) {
    state.toolWheel.selectedIndex = *existing;
  }
  state.statusLabel.clear();
  static_cast<void>(cr::setCreativeCatalogOpen(state.model, false));
  static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, true));
  return true;
}

void processToolWheelInput(const CreativeEditorCatalogFrameRequest& request,
                           CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& state = request.editor.catalog;
  for (const cr::CreativeInputActionEvent& event :
       request.routedInput.actionEvents()) {
    if (!state.toolWheel.open) {
      break;
    }
    switch (event.action) {
      case cr::CreativeInputActionId::ToolWheelPrevious:
        static_cast<void>(
            cr::moveCreativeToolWheelSelection(state.toolWheel, -1));
        break;
      case cr::CreativeInputActionId::ToolWheelNext:
        static_cast<void>(
            cr::moveCreativeToolWheelSelection(state.toolWheel, 1));
        break;
      case cr::CreativeInputActionId::ToolWheelConfirm:
        if (state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
          static_cast<void>(commitToolWheelAssignment(request, result));
          break;
        }
        result.assigned =
            assignToolWheelSelection(request.appState, request.editor) ||
            result.assigned;
        if (result.assigned) {
          static_cast<void>(
              cr::setCreativeToolWheelOpen(state.toolWheel, false));
        }
        break;
      case cr::CreativeInputActionId::ToolWheelOptions: {
        if (state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
          break;
        }
        const cr::CreativeCatalogEntry* selected =
            cr::selectedCreativeToolWheelEntry(state.toolWheel, state.model);
        if (selected == nullptr) {
          break;
        }
        const cr::CreativeToolOptionList options =
            creativeEditorToolOptionsForEntry(selected->hotbarEntry,
                                              request.editor.toolSettings);
        const CreativeEditorToolOptionsCommandList commands =
            creativeEditorToolOptionCommandsForEntry(selected->hotbarEntry);
        if ((options.count == 0U && commands.count == 0U) ||
            options.capacityExceeded) {
          break;
        }
        result.openToolOptionsRequested = true;
        result.toolOptionsEntry = selected->hotbarEntry;
        static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, false));
        return;
      }
      default:
        break;
    }
  }

  if (!state.toolWheel.open) {
    return;
  }
  const iggy3d::SdlWindowEventState& events = request.window.eventState();
  const std::int32_t wheelSteps = cr::quantizeCreativeWheelSteps(
      events.mouseWheelY, kCatalogWheelProfile);
  if (wheelSteps != 0) {
    static_cast<void>(
        cr::moveCreativeToolWheelSelection(state.toolWheel, wheelSteps));
  }

  const bool gamepadDirected =
      std::isfinite(request.toolWheelDirectionX) &&
      std::isfinite(request.toolWheelDirectionY) &&
      std::hypot(request.toolWheelDirectionX,
                 request.toolWheelDirectionY) > 0.35F;
  if (gamepadDirected) {
    static_cast<void>(cr::selectCreativeToolWheelDirection(
        state.toolWheel, request.toolWheelDirectionX,
        request.toolWheelDirectionY));
  } else {
    const cr::CreativeDrawablePointer pointer =
        cr::resolveCreativeDrawablePointer(
            {events.pointerX, events.pointerY, events.windowWidth,
             events.windowHeight, request.drawableWidth,
             request.drawableHeight, events.pointerMoved,
             events.primaryPointerPressed});
    if (pointer.valid && pointer.moved) {
      static_cast<void>(cr::selectCreativeToolWheelDirection(
          state.toolWheel, pointer.centeredX, pointer.centeredY, 24.0F));
    }
  }

  if (events.primaryPointerPressed) {
    if (state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
      static_cast<void>(commitToolWheelAssignment(request, result));
      return;
    }
    result.assigned =
        assignToolWheelSelection(request.appState, request.editor) ||
        result.assigned;
    if (result.assigned) {
      static_cast<void>(
          cr::setCreativeToolWheelOpen(state.toolWheel, false));
    }
  }
}

}  // namespace iggy3d_creative_app
