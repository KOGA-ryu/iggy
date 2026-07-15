#include "EditorAssetLibrary.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <span>
#include <string>

#include "EditorState.hpp"
#include "app/iggy3d/creative/input/Catalog.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] char asciiLower(char value) noexcept {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

[[nodiscard]] bool containsFolded(std::string_view value,
                                  std::string_view query) {
  if (query.empty()) {
    return true;
  }
  std::string loweredValue(value);
  std::string loweredQuery(query);
  std::transform(loweredValue.begin(), loweredValue.end(), loweredValue.begin(),
                 asciiLower);
  std::transform(loweredQuery.begin(), loweredQuery.end(), loweredQuery.begin(),
                 asciiLower);
  return loweredValue.find(loweredQuery) != std::string::npos;
}

void refreshLibraryFilter(CreativeEditorAssetLibraryState& state,
                          const CreativeEditorAuthoredAssetLibrary& library,
                          std::string_view selectedAssetId = {}) {
  state.filteredDefinitionIndices.clear();
  for (std::size_t index = 0U; index < library.definitions.size(); ++index) {
    const cr::CreativeAuthoredAssetDefinition& definition =
        library.definitions[index];
    if (containsFolded(definition.label, state.query) ||
        containsFolded(definition.assetId, state.query)) {
      state.filteredDefinitionIndices.push_back(index);
    }
  }
  if (!selectedAssetId.empty()) {
    const auto selected = std::find_if(
        state.filteredDefinitionIndices.begin(),
        state.filteredDefinitionIndices.end(),
        [&library, selectedAssetId](std::size_t index) {
          return library.definitions[index].assetId == selectedAssetId;
        });
    if (selected != state.filteredDefinitionIndices.end()) {
      state.selectedFilteredIndex = static_cast<std::size_t>(
          std::distance(state.filteredDefinitionIndices.begin(), selected));
    }
  }
  if (state.filteredDefinitionIndices.empty()) {
    state.selectedFilteredIndex = 0U;
  } else {
    state.selectedFilteredIndex =
        std::min(state.selectedFilteredIndex,
                 state.filteredDefinitionIndices.size() - 1U);
  }
}

[[nodiscard]] cr::CreativeAuthoredAssetDefinition*
selectedDefinition(CreativeEditorState& editor) noexcept {
  CreativeEditorAssetLibraryState& state = editor.assetLibrary;
  if (state.selectedFilteredIndex >= state.filteredDefinitionIndices.size()) {
    return nullptr;
  }
  const std::size_t definitionIndex =
      state.filteredDefinitionIndices[state.selectedFilteredIndex];
  return definitionIndex < editor.authoredAssets.definitions.size()
             ? &editor.authoredAssets.definitions[definitionIndex]
             : nullptr;
}

void refreshSelectedReferences(CreativeEditorState& editor,
                               const cr::CreativeDocument& mapDocument) {
  const cr::CreativeAuthoredAssetDefinition* definition =
      selectedDefinition(editor);
  editor.assetLibrary.selectedReferences =
      definition == nullptr
          ? CreativeEditorAuthoredAssetReferenceSummary{}
          : summarizeCreativeEditorAuthoredAssetReferences(
                mapDocument, editor.authoredAssets, definition->assetId);
}

[[nodiscard]] bool actionPresent(const cr::CreativeInputRouteResult& input,
                                 cr::CreativeInputActionId action) noexcept {
  return std::any_of(input.actionEvents().begin(), input.actionEvents().end(),
                     [action](const cr::CreativeInputActionEvent& event) {
                       return event.action == action;
                     });
}

void removeActions(cr::CreativeInputRouteResult& input,
                   std::span<const cr::CreativeInputActionId> actions) {
  std::size_t output = 0U;
  for (const cr::CreativeInputActionEvent& event : input.actionEvents()) {
    if (std::find(actions.begin(), actions.end(), event.action) !=
        actions.end()) {
      continue;
    }
    input.actions[output++] = event;
  }
  input.actionCount = output;
}

void moveWrapped(std::size_t& value, std::size_t count,
                 std::int32_t direction) noexcept {
  if (count == 0U || direction == 0) {
    return;
  }
  const std::int64_t signedCount = static_cast<std::int64_t>(count);
  const std::int64_t moved =
      (static_cast<std::int64_t>(value) + direction) % signedCount;
  value = static_cast<std::size_t>(moved < 0 ? moved + signedCount : moved);
}

void syncAsset(CreativeEditorState& editor, std::string_view assetId) {
  if (const cr::CreativeAuthoredAssetDefinition* definition =
          findCreativeEditorAuthoredAsset(editor.authoredAssets, assetId);
      definition != nullptr) {
    static_cast<void>(
        refreshCreativeEditorAuthoredAssetReferences(editor, *definition));
  }
}

void clearDeletedAssetReferences(CreativeEditorState& editor,
                                 std::string_view assetId) {
  std::size_t removedCatalogIndex = 0U;
  if (cr::removeCreativeCatalogAsset(editor.catalog.model, assetId,
                                     &removedCatalogIndex)) {
    static_cast<void>(cr::removeCreativeToolWheelCatalogEntry(
        editor.catalog.toolWheel, removedCatalogIndex));
    if (editor.catalog.toolWheelAssignmentCatalogEntryIndex ==
        removedCatalogIndex) {
      editor.catalog.toolWheelAssignmentCatalogEntryIndex.reset();
    } else if (editor.catalog.toolWheelAssignmentCatalogEntryIndex
                   .has_value() &&
               *editor.catalog.toolWheelAssignmentCatalogEntryIndex >
                   removedCatalogIndex) {
      --*editor.catalog.toolWheelAssignmentCatalogEntryIndex;
    }
  }
  for (cr::CreativeHotbarEntry& entry : editor.interaction.hotbar.entries) {
    if (cr::creativeHotbarAssetId(entry) == assetId) {
      entry = {cr::CreativeHeldItemKind::Material, editor.placeBrush};
    }
  }
}

void closeLibrary(iggy3d::SdlWindow& window,
                  CreativeEditorAssetLibraryState& state) {
  state.open = false;
  state.mode = CreativeEditorAssetLibraryMode::Browse;
  static_cast<void>(window.setTextInputActive(false));
  static_cast<void>(window.setRelativeMouseMode(true));
}

} // namespace

bool beginCreativeEditorAssetLibrary(CreativeEditorState& editor,
                                     const cr::CreativeDocument& mapDocument,
                                     std::string_view assetId) {
  if (editor.assetEdit.active ||
      findCreativeEditorAuthoredAsset(editor.authoredAssets, assetId) ==
          nullptr) {
    return false;
  }
  editor.assetLibrary = {};
  editor.assetLibrary.open = true;
  refreshLibraryFilter(editor.assetLibrary, editor.authoredAssets, assetId);
  refreshSelectedReferences(editor, mapDocument);
  editor.assetLibrary.statusLabel = "creative_asset_library_ready";
  return true;
}

CreativeEditorAssetLibraryFrameResult processCreativeEditorAssetLibraryFrame(
    const CreativeEditorAssetLibraryFrameRequest& request) {
  CreativeEditorAssetLibraryFrameResult result;
  result.remainingInput = request.routedInput;
  CreativeEditorState& editor = request.editor;
  const bool editWasActive = editor.assetEdit.active;
  const bool panelWasOpen = editor.assetLibrary.open;
  const bool menuWasOpen = editor.assetEdit.menuOpen;

  if (editor.assetEdit.active && !editor.assetEdit.menuOpen &&
      request.routedInput.context == cr::CreativeInputContext::EditorViewport) {
    constexpr std::array intercepted{
        cr::CreativeInputActionId::ToggleControls,
        cr::CreativeInputActionId::Save,
        cr::CreativeInputActionId::NewDocument,
        cr::CreativeInputActionId::Load,
    };
    if (actionPresent(request.routedInput,
                      cr::CreativeInputActionId::ToggleControls)) {
      editor.assetEdit.menuOpen = true;
      editor.assetEdit.menuAction =
          CreativeEditorAssetEditMenuAction::ContinueEditing;
      static_cast<void>(request.window.setTextInputActive(false));
      static_cast<void>(request.window.setRelativeMouseMode(false));
    }
    if (actionPresent(request.routedInput, cr::CreativeInputActionId::Save)) {
      static_cast<void>(saveCreativeEditorAuthoredAssetEdit(editor));
    }
    if (actionPresent(request.routedInput,
                      cr::CreativeInputActionId::NewDocument) ||
        actionPresent(request.routedInput, cr::CreativeInputActionId::Load)) {
      editor.assetEdit.statusLabel =
          "creative_authored_asset_edit_file_action_blocked";
    }
    removeActions(result.remainingInput, intercepted);
  }

  if (editor.assetEdit.active && editor.assetEdit.menuOpen &&
      request.routedInput.context ==
          cr::CreativeInputContext::AuthoredAssetEditMenu) {
    for (const cr::CreativeInputActionEvent& event :
         request.routedInput.actionEvents()) {
      if (event.action == cr::CreativeInputActionId::ToolOptionsPrevious) {
        std::size_t action =
            static_cast<std::size_t>(editor.assetEdit.menuAction);
        moveWrapped(
            action,
            static_cast<std::size_t>(CreativeEditorAssetEditMenuAction::Count),
            -1);
        editor.assetEdit.menuAction =
            static_cast<CreativeEditorAssetEditMenuAction>(action);
      } else if (event.action == cr::CreativeInputActionId::ToolOptionsNext) {
        std::size_t action =
            static_cast<std::size_t>(editor.assetEdit.menuAction);
        moveWrapped(
            action,
            static_cast<std::size_t>(CreativeEditorAssetEditMenuAction::Count),
            1);
        editor.assetEdit.menuAction =
            static_cast<CreativeEditorAssetEditMenuAction>(action);
      } else if (event.action == cr::CreativeInputActionId::CancelActiveTool) {
        editor.assetEdit.menuOpen = false;
        static_cast<void>(request.window.setRelativeMouseMode(true));
      } else if (event.action == cr::CreativeInputActionId::ConfirmActiveTool) {
        switch (editor.assetEdit.menuAction) {
          case CreativeEditorAssetEditMenuAction::ContinueEditing:
            editor.assetEdit.menuOpen = false;
            static_cast<void>(request.window.setRelativeMouseMode(true));
            break;
          case CreativeEditorAssetEditMenuAction::SaveAndExit:
            if (saveCreativeEditorAuthoredAssetEdit(editor).accepted) {
              static_cast<void>(request.window.setRelativeMouseMode(true));
            }
            break;
          case CreativeEditorAssetEditMenuAction::DiscardAndExit:
            static_cast<void>(cancelCreativeEditorAuthoredAssetEdit(
                editor, "creative_authored_asset_edit_discarded"));
            static_cast<void>(request.window.setRelativeMouseMode(true));
            break;
          case CreativeEditorAssetEditMenuAction::Count:
            break;
        }
      }
      if (!editor.assetEdit.active || !editor.assetEdit.menuOpen) {
        break;
      }
    }
  }

  if (editor.assetLibrary.open &&
      request.routedInput.context == cr::CreativeInputContext::AssetLibrary) {
    CreativeEditorAssetLibraryState& state = editor.assetLibrary;
    if (state.mode == CreativeEditorAssetLibraryMode::Rename) {
      state.renameDraft.append(request.window.eventState().textInput);
      state.renameDraft =
          normalizeCreativeEditorAssetLibraryText(state.renameDraft);
      for (std::size_t index = 0U;
           index < request.window.eventState().backspacePressCount &&
           !state.renameDraft.empty();
           ++index) {
        state.renameDraft.pop_back();
      }
    } else {
      state.query.append(request.window.eventState().textInput);
      state.query = normalizeCreativeEditorAssetLibraryText(state.query);
      for (std::size_t index = 0U;
           index < request.window.eventState().backspacePressCount &&
           !state.query.empty();
           ++index) {
        state.query.pop_back();
      }
      refreshLibraryFilter(state, editor.authoredAssets);
      refreshSelectedReferences(editor, request.mapAppState.facade.document());
    }

    for (const cr::CreativeInputActionEvent& event :
         request.routedInput.actionEvents()) {
      if (!state.open) {
        break;
      }
      if (event.action == cr::CreativeInputActionId::CatalogClose) {
        if (state.mode == CreativeEditorAssetLibraryMode::Browse) {
          closeLibrary(request.window, state);
        } else {
          state.mode = CreativeEditorAssetLibraryMode::Browse;
          state.renameDraft.clear();
        }
        continue;
      }
      if (state.mode == CreativeEditorAssetLibraryMode::Rename) {
        if (event.action == cr::CreativeInputActionId::CatalogConfirm) {
          cr::CreativeAuthoredAssetDefinition* selected =
              selectedDefinition(editor);
          const CreativeEditorAuthoredAssetMutationReceipt renamed =
              selected == nullptr
                  ? CreativeEditorAuthoredAssetMutationReceipt{}
                  : renameCreativeEditorAuthoredAsset(editor.authoredAssets,
                                                      selected->assetId,
                                                      state.renameDraft);
          state.statusLabel = renamed.reasonCode;
          if (renamed.accepted) {
            syncAsset(editor, renamed.assetId);
            state.mode = CreativeEditorAssetLibraryMode::Browse;
            refreshLibraryFilter(state, editor.authoredAssets, renamed.assetId);
          }
        }
        continue;
      }
      if (state.mode == CreativeEditorAssetLibraryMode::ConfirmDelete) {
        if (event.action == cr::CreativeInputActionId::CatalogConfirm) {
          const cr::CreativeAuthoredAssetDefinition* selected =
              selectedDefinition(editor);
          const std::string assetId =
              selected == nullptr ? std::string{} : selected->assetId;
          const CreativeEditorAuthoredAssetMutationReceipt removed =
              selected == nullptr ? CreativeEditorAuthoredAssetMutationReceipt{}
                                  : deleteCreativeEditorAuthoredAsset(
                                        request.mapAppState.facade.document(),
                                        editor.authoredAssets, assetId);
          state.statusLabel = removed.reasonCode;
          state.mode = CreativeEditorAssetLibraryMode::Browse;
          if (removed.accepted) {
            clearDeletedAssetReferences(editor, assetId);
            refreshLibraryFilter(state, editor.authoredAssets);
          }
          refreshSelectedReferences(editor,
                                    request.mapAppState.facade.document());
        }
        continue;
      }

      if (event.action == cr::CreativeInputActionId::CatalogPrevious) {
        moveWrapped(state.selectedFilteredIndex,
                    state.filteredDefinitionIndices.size(), -1);
        refreshSelectedReferences(editor,
                                  request.mapAppState.facade.document());
      } else if (event.action == cr::CreativeInputActionId::CatalogNext) {
        moveWrapped(state.selectedFilteredIndex,
                    state.filteredDefinitionIndices.size(), 1);
        refreshSelectedReferences(editor,
                                  request.mapAppState.facade.document());
      } else if (event.action ==
                     cr::CreativeInputActionId::CatalogPreviousVariant ||
                 event.action ==
                     cr::CreativeInputActionId::CatalogNextVariant) {
        std::size_t action = static_cast<std::size_t>(state.action);
        moveWrapped(
            action,
            static_cast<std::size_t>(CreativeEditorAssetLibraryAction::Count),
            event.action == cr::CreativeInputActionId::CatalogPreviousVariant
                ? -1
                : 1);
        state.action = static_cast<CreativeEditorAssetLibraryAction>(action);
      } else if (event.action == cr::CreativeInputActionId::CatalogConfirm) {
        cr::CreativeAuthoredAssetDefinition* selected =
            selectedDefinition(editor);
        if (selected == nullptr) {
          continue;
        }
        const std::string assetId = selected->assetId;
        switch (state.action) {
          case CreativeEditorAssetLibraryAction::Edit: {
            const CreativeEditorAuthoredAssetMutationReceipt begun =
                beginCreativeEditorAuthoredAssetEdit(editor, assetId);
            state.statusLabel = begun.reasonCode;
            if (begun.accepted) {
              closeLibrary(request.window, state);
            }
            break;
          }
          case CreativeEditorAssetLibraryAction::Rename:
            state.mode = CreativeEditorAssetLibraryMode::Rename;
            state.renameDraft = selected->label;
            break;
          case CreativeEditorAssetLibraryAction::Duplicate: {
            const CreativeEditorAuthoredAssetMutationReceipt duplicated =
                duplicateCreativeEditorAuthoredAsset(editor.authoredAssets,
                                                     assetId);
            state.statusLabel = duplicated.reasonCode;
            if (duplicated.accepted) {
              syncAsset(editor, duplicated.assetId);
              refreshLibraryFilter(state, editor.authoredAssets,
                                   duplicated.assetId);
              refreshSelectedReferences(editor,
                                        request.mapAppState.facade.document());
            }
            break;
          }
          case CreativeEditorAssetLibraryAction::Delete:
            refreshSelectedReferences(editor,
                                      request.mapAppState.facade.document());
            state.statusLabel =
                state.selectedReferences.total() == 0U
                    ? "creative_asset_library_delete_confirm"
                    : "creative_asset_library_delete_referenced";
            if (state.selectedReferences.total() == 0U) {
              state.mode = CreativeEditorAssetLibraryMode::ConfirmDelete;
            }
            break;
          case CreativeEditorAssetLibraryAction::Count:
            break;
        }
      }
    }
  }

  refreshCreativeEditorAuthoredAssetEditDirtyState(editor);
  result.blockWorldActions =
      panelWasOpen || menuWasOpen || editor.assetLibrary.open ||
      editor.assetEdit.menuOpen || editWasActive != editor.assetEdit.active;
  result.activeDocumentChanged = editWasActive != editor.assetEdit.active;
  return result;
}

} // namespace iggy3d_creative_app
