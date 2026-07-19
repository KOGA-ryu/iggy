#include "EditorDesktopCommands.hpp"

#include <algorithm>
#include <span>
#include <string>
#include <utility>
#include <variant>

#include "EditorAssetLibrary.hpp"
#include "EditorAuthoredAssets.hpp"
#include "EditorDesktopModel.hpp"
#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorMovingPlatformPreview.hpp"
#include "EditorObjectActions.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPersistence.hpp"
#include "EditorPlayMode.hpp"
#include "EditorState.hpp"
#include "EditorTerrainGeneration.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutTerrainImpact.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    return;
  }
  commands[count].id = id;
  commands[count].payload = std::monostate{};
  ++count;
}

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id,
                                       std::string saveId) {
  push(id, CreativeDesktopCommandPayload{
               CreativeDesktopSaveAsPayload{std::move(saveId)}});
}

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id,
                                       CreativeDesktopCommandPayload payload) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    return;
  }
  commands[count].id = id;
  commands[count].payload = std::move(payload);
  ++count;
}

void CreativeDesktopCommandFrame::clear() noexcept {
  count = 0U;
  overflowed = false;
}

namespace {

// A mismatched id/payload is an explicit no-op failure — never a
// reinterpretation. Returns nullptr when the variant holds a different type.
template <typename Payload>
[[nodiscard]] const Payload* payloadAs(const CreativeDesktopCommand& command) {
  return std::get_if<Payload>(&command.payload);
}

struct GeneratedSourceScopeResolution {
  bool ancestor = false;
  bool stable = false;
};

[[nodiscard]] GeneratedSourceScopeResolution resolveGeneratedSourceScope(
    const creative::CreativeWorldLayout& layout,
    const creative::CreativeObject& object,
    creative::CreativeWorldLayoutTable table,
    std::size_t index,
    std::string_view stableKey) {
  const creative::CreativeWorldLayoutObjectProvenance provenance =
      creative::resolveCreativeWorldLayoutObjectProvenance(layout, object);
  const CreativeDesktopGeneratedSourceScopeModel scopes =
      buildCreativeDesktopGeneratedSourceScopeModel(layout, provenance);
  const std::size_t scopeIndex =
      findCreativeDesktopGeneratedSourceScope(scopes, table, index);
  if (scopeIndex >= scopes.count) {
    return {};
  }
  return {true, !stableKey.empty() &&
                    scopes.entries[scopeIndex].stableKey == stableKey};
}

[[nodiscard]] creative::CreativeObjectId findGeneratedSourceScopeObject(
    const creative::CreativeDocument& document,
    const creative::CreativeWorldLayout& layout,
    creative::CreativeWorldLayoutTable table,
    std::size_t index,
    creative::CreativeObjectKind preferredKind) noexcept {
  creative::CreativeObjectId fallback = creative::kInvalidObjectId;
  for (const creative::CreativeObject& object : document.objects()) {
    const creative::CreativeWorldLayoutObjectProvenance provenance =
        creative::resolveCreativeWorldLayoutObjectProvenance(layout, object);
    const CreativeDesktopGeneratedSourceScopeModel scopes =
        buildCreativeDesktopGeneratedSourceScopeModel(layout, provenance);
    if (findCreativeDesktopGeneratedSourceScope(scopes, table, index) >=
        scopes.count) {
      continue;
    }
    if (object.kind == preferredKind) {
      return object.id;
    }
    if (fallback == creative::kInvalidObjectId) {
      fallback = object.id;
    }
  }
  return fallback;
}

[[nodiscard]] const creative::CreativeCatalogEntry* findCatalogAsset(
    const creative::CreativeCatalogState& catalog,
    std::string_view assetId) noexcept {
  const auto found = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [assetId](const creative::CreativeCatalogEntry& entry) {
        return entry.category ==
                   creative::CreativeCatalogEntryCategory::Asset &&
               creative::creativeHotbarAssetId(entry.hotbarEntry) == assetId;
      });
  return found == catalog.entries.end() ? nullptr : &*found;
}

[[nodiscard]] bool validCatalogAsset(
    const creative::CreativeCatalogEntry& entry) noexcept {
  const creative::CreativeBoundsMetrics bounds =
      creative::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
  return entry.category == creative::CreativeCatalogEntryCategory::Asset &&
         entry.hotbarEntry.objectKind > creative::CreativeObjectKind::Unknown &&
         entry.hotbarEntry.objectKind < creative::CreativeObjectKind::Count &&
         !creative::creativeHotbarAssetId(entry.hotbarEntry).empty() &&
         entry.hotbarEntry.hasAssetBounds && bounds.valid &&
         creative::isPositiveCreativeVec3(bounds.size);
}

CreativeEditorWorldLayoutEditReceipt rejectWorldLayoutAssetRepair(
    CreativeEditorWorldLayoutState& state, std::string message,
    std::string reasonCode) {
  state.statusMessage = std::move(message);
  return {false, false, std::move(reasonCode)};
}

CreativeEditorWorldLayoutEditReceipt repairWorldLayoutAsset(
    CreativeEditorWorldLayoutState& state,
    const creative::CreativeCatalogState& catalog,
    double gridCellSizeMeters,
    const CreativeDesktopWorldLayoutAssetRepairPayload& payload) {
  if (payload.operation >=
          CreativeDesktopWorldLayoutAssetRepairOperation::Count ||
      (payload.table != creative::CreativeWorldLayoutTable::Opening &&
       payload.table != creative::CreativeWorldLayoutTable::Object) ||
      payload.stableKey.empty() || payload.expectedAssetId.empty() ||
      !creativeEditorWorldLayoutSourceStableKeyMatches(
          state, payload.table, payload.index, payload.stableKey)) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair target is stale",
        "creative_editor_world_layout_asset_repair_target_invalid");
  }

  std::string_view currentAssetId;
  if (payload.table == creative::CreativeWorldLayoutTable::Opening) {
    if (payload.index >= state.source.openings.size()) {
      return rejectWorldLayoutAssetRepair(
          state, "asset repair target is stale",
          "creative_editor_world_layout_asset_repair_target_invalid");
    }
    currentAssetId = state.source.openings[payload.index].insertAssetId;
  } else {
    if (payload.index >= state.source.objects.size()) {
      return rejectWorldLayoutAssetRepair(
          state, "asset repair target is stale",
          "creative_editor_world_layout_asset_repair_target_invalid");
    }
    currentAssetId = state.source.objects[payload.index].assetId;
  }
  if (currentAssetId != payload.expectedAssetId) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair identity changed",
        "creative_editor_world_layout_asset_repair_identity_mismatch");
  }

  if (payload.operation ==
      CreativeDesktopWorldLayoutAssetRepairOperation::UseProceduralInsert) {
    if (payload.table != creative::CreativeWorldLayoutTable::Opening) {
      return rejectWorldLayoutAssetRepair(
          state, "procedural fallback is only valid for openings",
          "creative_editor_world_layout_asset_repair_operation_invalid");
    }
    CreativeEditorWorldLayoutOpeningInsertRequest request;
    request.openingIndex = payload.index;
    request.operation =
        CreativeEditorWorldLayoutOpeningInsertOperation::UseProceduralInsert;
    return applyCreativeEditorWorldLayoutOpeningInsert(state, request);
  }

  const std::string_view requestedAssetId =
      payload.operation ==
              CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds
          ? std::string_view(payload.expectedAssetId)
          : std::string_view(payload.replacementAssetId);
  const creative::CreativeCatalogEntry* entry =
      findCatalogAsset(catalog, requestedAssetId);
  if (entry == nullptr || !validCatalogAsset(*entry)) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair catalog entry is unavailable",
        "creative_editor_world_layout_asset_repair_catalog_missing");
  }

  if (payload.table == creative::CreativeWorldLayoutTable::Opening) {
    const creative::CreativeWorldLayoutOpening& opening =
        state.source.openings[payload.index];
    if (!creativeEditorWorldLayoutCatalogAssetMatchesOpening(
            entry->assetAuthoringMetadata.categoryId, opening.kind)) {
      return rejectWorldLayoutAssetRepair(
          state, "choose a compatible door or window asset",
          "creative_editor_world_layout_asset_repair_incompatible");
    }
    CreativeEditorWorldLayoutOpeningInsertRequest request;
    request.openingIndex = payload.index;
    request.operation =
        CreativeEditorWorldLayoutOpeningInsertOperation::FitAssetToOpening;
    request.assetKind = opening.kind;
    request.assetId = creative::creativeHotbarAssetId(entry->hotbarEntry);
    request.assetSourceBoundsMeters = entry->hotbarEntry.assetSourceBounds;
    request.gridCellSizeMeters = gridCellSizeMeters;
    return applyCreativeEditorWorldLayoutOpeningInsert(state, request);
  }

  const creative::CreativeWorldLayoutObject& object =
      state.source.objects[payload.index];
  if (creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
          entry->assetAuthoringMetadata.categoryId) ||
      entry->hotbarEntry.objectKind != object.kind) {
    return rejectWorldLayoutAssetRepair(
        state, "choose a compatible object asset",
        "creative_editor_world_layout_asset_repair_incompatible");
  }
  CreativeEditorWorldLayoutObjectSettings settings;
  if (!readCreativeEditorWorldLayoutObjectSettings(state, payload.index,
                                                   settings)) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair target is stale",
        "creative_editor_world_layout_asset_repair_target_invalid");
  }
  settings.assetId = creative::creativeHotbarAssetId(entry->hotbarEntry);
  settings.assetSourceBoundsMeters = entry->hotbarEntry.assetSourceBounds;
  settings.hasAssetSourceBounds = true;
  return setCreativeEditorWorldLayoutObjectSettings(state, payload.index,
                                                    std::move(settings));
}

[[nodiscard]] bool commandAllowedDuringPlay(
    CreativeDesktopCommandId id) noexcept {
  return id == CreativeDesktopCommandId::None ||
         id == CreativeDesktopCommandId::Play ||
         id == CreativeDesktopCommandId::SelectObjects ||
         id == CreativeDesktopCommandId::ClearSelection ||
         id == CreativeDesktopCommandId::WorldLayoutFocusSource ||
         id == CreativeDesktopCommandId::WorldLayoutFocusObjectSource;
}

[[nodiscard]] float editorViewportAspectRatio(
    const CreativeEditorState& editor) noexcept {
  const iggy3d::RenderContentViewport viewport =
      editor.desktopUi.contentViewport;
  if (viewport.width > 0U && viewport.height > 0U) {
    return static_cast<float>(viewport.width) /
           static_cast<float>(viewport.height);
  }
  if (editor.lastWidth > 0U && editor.lastHeight > 0U) {
    return static_cast<float>(editor.lastWidth) /
           static_cast<float>(editor.lastHeight);
  }
  return 1.0F;
}

[[nodiscard]] bool focusEditorCameraOnBounds(
    CreativeEditorState& editor,
    creative::CreativeBounds bounds) noexcept {
  const creative::CreativeCoreVec3Conversion boundsMin =
      creative::creativeVec3ToCoreChecked(bounds.min);
  const creative::CreativeCoreVec3Conversion boundsMax =
      creative::creativeVec3ToCoreChecked(bounds.max);
  if (!boundsMin.converted || !boundsMax.converted) {
    return false;
  }
  iggy3d::ProductCreativeCameraFrameRequest request;
  request.boundsMinMeters = boundsMin.value;
  request.boundsMaxMeters = boundsMax.value;
  request.cameraYawDegrees = editor.yawDegrees;
  request.cameraPitchDegrees = editor.pitchDegrees;
  request.viewportAspectRatio = editorViewportAspectRatio(editor);
  const iggy3d::ProductCreativeCameraFrameResult frame =
      iggy3d::planProductCreativeCameraFrame(request);
  if (!frame.applied) {
    return false;
  }
  editor.flyPos = frame.anchorPositionMeters;
  return true;
}

[[nodiscard]] bool focusEditorCameraOnObject(
    CreativeEditorState& editor,
    const creative::CreativeObject& object) noexcept {
  const creative::CreativeTransformedBounds bounds =
      creative::resolveCreativeObjectBounds(object);
  const creative::CreativeBounds focusBounds =
      bounds.valid
          ? bounds.worldBounds
          : creative::CreativeBounds{object.transform.position,
                                     object.transform.position};
  return focusEditorCameraOnBounds(editor, focusBounds);
}

[[nodiscard]] bool focusEditorCameraOnTerrainGeneration(
    CreativeEditorState& editor,
    const creative::CreativeDocument& document,
    const creative::CreativeTerrainGenerationResult& generation) noexcept {
  if (!generation.receipt.accepted) {
    return false;
  }
  const creative::CreativeGridSettings grid = document.gridSettings();
  const creative::CreativeTerrainHeightFieldBounds bounds =
      generation.plan.heightField.bounds();
  const double minimumX =
      grid.origin.x + static_cast<double>(bounds.minimum.x) *
                          grid.cellSizeMeters;
  const double minimumZ =
      grid.origin.z + static_cast<double>(bounds.minimum.z) *
                          grid.cellSizeMeters;
  const double maximumX =
      grid.origin.x +
      (static_cast<double>(bounds.minimum.x) + bounds.widthCells) *
          grid.cellSizeMeters;
  const double maximumZ =
      grid.origin.z +
      (static_cast<double>(bounds.minimum.z) + bounds.depthCells) *
          grid.cellSizeMeters;
  return focusEditorCameraOnBounds(
      editor,
      {{minimumX, grid.origin.y, minimumZ},
       {maximumX,
        grid.origin.y + generation.receipt.maximumHeightCells *
                            grid.cellSizeMeters,
        maximumZ}});
}

void dispatchOne(const CreativeDesktopCommand& command,
                 const CreativeDesktopCommandContext& context,
                 CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  // Edit mutations run against whichever document is live (map document, or an
  // open authored-asset edit workspace) so an asset-edit session is never
  // corrupted. Document-lifecycle and library/instance ops stay on the map.
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  result.lastCommand = command.id;
  result.accepted = false;
  result.changed = false;
  result.documentReplaced = false;
  result.sceneChanged = false;
  result.worldLayoutChanged = false;
  result.affectedObjectCount = 0U;
  result.message.clear();

  if (context.playMode != nullptr &&
      creativeEditorPlayModeActive(*context.playMode) &&
      !commandAllowedDuringPlay(command.id)) {
    result.message = "stop play before editing";
    return;
  }

  switch (command.id) {
    case CreativeDesktopCommandId::NewDocument:
      clearToBlankScene(appState);
      clearEditHistory(appState.history, "desktop_new");
      resetCreativeEditorForDocumentReplacement(
          editor, appState.facade.document().id());
      result.accepted = true;
      result.changed = true;
      result.documentReplaced = true;
      result.message = "new document";
      break;
    case CreativeDesktopCommandId::OpenDocument: {
      const std::string saveId =
          context.activeSaveId != nullptr ? *context.activeSaveId : std::string{};
      creative::CreativeWorldLayout loadedLayout;
      const bool loaded = loadStandaloneScene(
          appState, context.saveRoot, saveId, &loadedLayout);
      if (loaded) {
        clearEditHistory(appState.history, "desktop_open");
        resetCreativeEditorForDocumentReplacement(
            editor, appState.facade.document().id());
        installCreativeEditorWorldLayout(editor.worldLayout,
                                         std::move(loadedLayout));
      }
      result.accepted = loaded;
      result.changed = loaded;
      result.documentReplaced = loaded;
      result.message = loaded ? "opened " + saveId : "open failed";
      break;
    }
    case CreativeDesktopCommandId::SaveDocument: {
      const std::string saveId =
          context.activeSaveId != nullptr ? *context.activeSaveId : std::string{};
      const iggy3d::CreativeWorldSaveResult saveResult =
          saveStandaloneScene(appState.facade, context.saveRoot, saveId,
                              &editor.worldLayout.source,
                              editor.worldLayout.generatedRevision ==
                                  editor.worldLayout.revision);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        clearEditHistory(appState.history, "desktop_save");
        markCreativeEditorWorldLayoutSaved(editor.worldLayout);
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved " + saveId : "save failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SaveDocumentAs: {
      const auto* payload = payloadAs<CreativeDesktopSaveAsPayload>(command);
      const std::string saveId = payload != nullptr ? payload->saveId : std::string{};
      if (saveId.empty()) {
        result.message = "save as: empty name";
        break;
      }
      const iggy3d::CreativeWorldSaveResult saveResult =
          saveStandaloneScene(appState.facade, context.saveRoot, saveId,
                              &editor.worldLayout.source,
                              editor.worldLayout.generatedRevision ==
                                  editor.worldLayout.revision);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        if (context.activeSaveId != nullptr) {
          *context.activeSaveId = saveId;
        }
        clearEditHistory(appState.history, "desktop_save_as");
        markCreativeEditorWorldLayoutSaved(editor.worldLayout);
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved as " + saveId
                          : "save as failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::Undo: {
      CreativeEditorWorldLayoutState* worldLayout =
          &activeAppState == &appState ? &editor.worldLayout : nullptr;
      const std::uint64_t layoutRevisionBefore =
          worldLayout != nullptr ? worldLayout->revision : 0U;
      const std::uint64_t documentRevisionBefore =
          activeAppState.facade.document().revision();
      const bool previewWasActive =
          worldLayout != nullptr &&
          creativeEditorWorldLayoutPreviewActive(*worldLayout);
      const bool ok =
          undoLastEdit(activeAppState, "desktop_undo", worldLayout);
      result.accepted = ok;
      result.changed = ok;
      result.worldLayoutChanged =
          worldLayout != nullptr &&
          worldLayout->revision != layoutRevisionBefore;
      result.sceneChanged =
          ok && (previewWasActive ||
                 activeAppState.facade.document().revision() !=
                     documentRevisionBefore);
      result.message = result.worldLayoutChanged
                           ? worldLayout->statusMessage
                           : (ok ? "undo" : "nothing to undo");
      break;
    }
    case CreativeDesktopCommandId::Redo: {
      CreativeEditorWorldLayoutState* worldLayout =
          &activeAppState == &appState ? &editor.worldLayout : nullptr;
      const std::uint64_t layoutRevisionBefore =
          worldLayout != nullptr ? worldLayout->revision : 0U;
      const std::uint64_t documentRevisionBefore =
          activeAppState.facade.document().revision();
      const bool previewWasActive =
          worldLayout != nullptr &&
          creativeEditorWorldLayoutPreviewActive(*worldLayout);
      const bool ok =
          redoLastEdit(activeAppState, "desktop_redo", worldLayout);
      result.accepted = ok;
      result.changed = ok;
      result.worldLayoutChanged =
          worldLayout != nullptr &&
          worldLayout->revision != layoutRevisionBefore;
      result.sceneChanged =
          ok && (previewWasActive ||
                 activeAppState.facade.document().revision() !=
                     documentRevisionBefore);
      result.message = result.worldLayoutChanged
                           ? worldLayout->statusMessage
                           : (ok ? "redo" : "nothing to redo");
      break;
    }
    case CreativeDesktopCommandId::DuplicateSelection: {
      const creative::CreativeDuplicateCommandReceipt receipt =
          duplicateSelectedObjectsWithUndo(
              activeAppState, activeAppState.history,
              creative::CreativeDuplicateCommandRequest{}, "desktop_duplicate");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = receipt.changed ? "duplicated selection"
                                       : "nothing to duplicate";
      break;
    }
    case CreativeDesktopCommandId::DeleteSelection: {
      const creative::CreativeDocumentRemoveReceipt receipt =
          deleteSelectedObject(activeAppState, "desktop_delete",
                               &activeAppState.history);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = receipt.changed ? "deleted selection"
                                       : "nothing to delete";
      break;
    }
    case CreativeDesktopCommandId::SelectObjects: {
      const auto* payload = payloadAs<CreativeDesktopSelectPayload>(command);
      if (payload == nullptr) {
        result.message = "select: payload mismatch";
        break;
      }
      const creative::CreativeSelectionReceipt receipt =
          activeAppState.facade.selectTargets(payload->objectIds,
                                              payload->primaryObjectId);
      result.accepted = true;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.selectedCountAfter;
      result.message = "selection updated";
      break;
    }
    case CreativeDesktopCommandId::FocusObject: {
      const auto* payload = payloadAs<CreativeDesktopSelectPayload>(command);
      if (payload == nullptr) {
        result.message = "focus: payload mismatch";
        break;
      }
      const creative::CreativeObjectId objectId =
          payload->primaryObjectId != creative::kInvalidObjectId
              ? payload->primaryObjectId
              : payload->objectIds.empty() ? creative::kInvalidObjectId
                                           : payload->objectIds.front();
      const creative::CreativeObject* object =
          activeAppState.facade.findObject(objectId);
      if (object == nullptr || !focusEditorCameraOnObject(editor, *object)) {
        result.message = "focus: object unavailable";
        break;
      }
      const creative::CreativeSelectionReceipt receipt =
          activeAppState.facade.selectTargets(
              std::span<const creative::CreativeObjectId>{&objectId, 1U},
              objectId);
      result.accepted = true;
      result.changed = true;
      result.affectedObjectCount = receipt.selectedCountAfter;
      result.message = "object focused";
      break;
    }
    case CreativeDesktopCommandId::ClearSelection: {
      const creative::CreativeSelectionReceipt receipt =
          activeAppState.facade.selectTargets(
              std::span<const creative::CreativeObjectId>{},
              creative::kInvalidObjectId);
      result.accepted = true;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.selectedCountAfter;
      result.message = "selection cleared";
      break;
    }
    case CreativeDesktopCommandId::SetLogicSource: {
      const auto* payload = payloadAs<CreativeDesktopLogicLinkPayload>(command);
      if (payload == nullptr) {
        result.message = "logic source: payload mismatch";
        break;
      }
      const creative::CreativeObjectId sourceBefore =
          editor.logicLinks.sourceObjectId;
      const CreativeEditorLogicLinkReceipt receipt =
          selectCreativeEditorLogicLinkSource(
              activeAppState, editor.logicLinks, payload->sourceObjectId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted &&
                       sourceBefore != editor.logicLinks.sourceObjectId;
      result.affectedObjectCount = receipt.accepted ? 1U : 0U;
      result.message = receipt.accepted
                           ? "logic source selected"
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::ClearLogicSource:
      result.accepted = true;
      result.changed =
          clearCreativeEditorLogicLinkSource(editor.logicLinks);
      result.message = result.changed ? "logic source cleared"
                                      : "logic source already clear";
      break;
    case CreativeDesktopCommandId::SetLogicLink: {
      const auto* payload = payloadAs<CreativeDesktopLogicLinkPayload>(command);
      if (payload == nullptr) {
        result.message = "set logic link: payload mismatch";
        break;
      }
      const CreativeEditorLogicLinkReceipt receipt =
          setCreativeEditorLogicLink(
              activeAppState, editor.logicLinks, payload->sourceObjectId,
              payload->targetObjectId, payload->action,
              "desktop_set_logic_link");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.accepted ? 1U : 0U;
      result.message = receipt.accepted
                           ? (receipt.changed ? "logic link updated"
                                              : "logic link unchanged")
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::RemoveLogicLink: {
      const auto* payload = payloadAs<CreativeDesktopLogicLinkPayload>(command);
      if (payload == nullptr) {
        result.message = "remove logic link: payload mismatch";
        break;
      }
      const CreativeEditorLogicLinkReceipt receipt =
          removeCreativeEditorLogicLink(
              activeAppState, editor.logicLinks, payload->sourceObjectId,
              payload->targetObjectId, "desktop_remove_logic_link");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.changed ? 1U : 0U;
      result.message = receipt.accepted
                           ? "logic link removed"
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::DeleteObjects: {
      const auto* payload = payloadAs<CreativeDesktopDeletePayload>(command);
      if (payload == nullptr) {
        result.message = "delete objects: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt = deleteObjectsWithUndo(
          activeAppState, activeAppState.history, payload->objectIds,
          "desktop_delete_objects");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.changed ? "deleted objects" : "nothing deleted";
      break;
    }
    case CreativeDesktopCommandId::RenameObject: {
      const auto* payload = payloadAs<CreativeDesktopRenamePayload>(command);
      if (payload == nullptr) {
        result.message = "rename: payload mismatch";
        break;
      }
      if (payload->objectId == creative::kInvalidObjectId ||
          payload->name.empty()) {
        result.message = "rename: invalid request";
        break;
      }
      const creative::CreativeDocumentMutationReceipt receipt =
          renameObjectWithUndo(activeAppState, activeAppState.history,
                               payload->objectId, payload->name,
                               "desktop_rename");
      const bool applied =
          receipt.status == creative::CreativeDocumentMutationStatus::Applied;
      result.accepted = applied;
      result.changed = applied && receipt.changed;
      result.affectedObjectCount = result.changed ? 1U : 0U;
      result.message = applied ? "renamed object" : "rename failed";
      break;
    }
    case CreativeDesktopCommandId::SetObjectsVisible: {
      const auto* payload = payloadAs<CreativeDesktopObjectFlagPayload>(command);
      if (payload == nullptr) {
        result.message = "visibility: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt =
          setObjectsVisibleWithUndo(activeAppState, activeAppState.history,
                                    payload->objectIds, payload->value,
                                    "desktop_set_visible");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.changed ? "visibility updated"
                                       : "visibility unchanged";
      break;
    }
    case CreativeDesktopCommandId::SetObjectsLocked: {
      const auto* payload = payloadAs<CreativeDesktopObjectFlagPayload>(command);
      if (payload == nullptr) {
        result.message = "lock: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt =
          setObjectsLockedWithUndo(activeAppState, activeAppState.history,
                                   payload->objectIds, payload->value,
                                   "desktop_set_locked");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.changed ? "lock updated" : "lock unchanged";
      break;
    }
    case CreativeDesktopCommandId::SetObjectTransform: {
      const auto* payload = payloadAs<CreativeDesktopTransformPayload>(command);
      if (payload == nullptr) {
        result.message = "transform: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt =
          setObjectTransformWithUndo(activeAppState, activeAppState.history,
                                     payload->objectId, payload->transform,
                                     payload->setPosition, payload->setRotation,
                                     payload->setScale, "desktop_set_transform");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.accepted
                           ? (receipt.changed ? "transform set"
                                              : "transform unchanged")
                           : receipt.message;
      break;
    }
    case CreativeDesktopCommandId::SetMovingPlatformSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopMovingPlatformPayload>(command);
      if (payload == nullptr) {
        result.message = "moving platform settings: payload mismatch";
        break;
      }
      const creative::CreativeDocumentMutationReceipt receipt =
          setMovingPlatformSettingsWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->settings, "desktop_set_moving_platform_settings");
      result.accepted =
          creative::documentMutationSucceeded(receipt.status);
      result.changed = receipt.changed;
      result.affectedObjectCount = result.changed ? 1U : 0U;
      result.message = result.accepted
                           ? (result.changed ? "platform settings updated"
                                             : "platform settings unchanged")
                           : receipt.message;
      break;
    }
    case CreativeDesktopCommandId::ToggleMovingPlatformPreview:
    case CreativeDesktopCommandId::RestartMovingPlatformPreview:
    case CreativeDesktopCommandId::SeekMovingPlatformPreview: {
      const auto* payload =
          payloadAs<CreativeDesktopMovingPlatformPreviewPayload>(command);
      if (payload == nullptr) {
        result.message = "moving platform preview: payload mismatch";
        break;
      }
      CreativeMovingPlatformPreviewCommand previewCommand =
          CreativeMovingPlatformPreviewCommand::TogglePlayback;
      if (command.id ==
          CreativeDesktopCommandId::RestartMovingPlatformPreview) {
        previewCommand = CreativeMovingPlatformPreviewCommand::Restart;
      } else if (command.id ==
                 CreativeDesktopCommandId::SeekMovingPlatformPreview) {
        previewCommand = CreativeMovingPlatformPreviewCommand::Seek;
      }
      const CreativeMovingPlatformPreviewReceipt receipt =
          applyCreativeMovingPlatformPreviewCommand(
              editor.movingPlatformPreview, previewCommand,
              payload->objectId, payload->normalizedProgress);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.accepted ? 1U : 0U;
      result.message = receipt.accepted
                           ? std::string(editor.movingPlatformPreview.reasonCode)
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::SelectMovingPlatformWaypoint:
    case CreativeDesktopCommandId::SetMovingPlatformWaypointDwell: {
      const auto* payload =
          payloadAs<CreativeDesktopMovingPlatformWaypointPayload>(command);
      if (payload == nullptr) {
        result.message = "moving platform waypoint: payload mismatch";
        break;
      }
      syncCreativeMovingPlatformPathEditState(
          activeAppState, editor.interaction.movingPlatformPathEdit);
      if (editor.interaction.movingPlatformPathEdit.objectId !=
          payload->objectId) {
        result.message = "moving platform waypoint: target mismatch";
        break;
      }
      if (command.id ==
          CreativeDesktopCommandId::SelectMovingPlatformWaypoint) {
        result.accepted = selectCreativeMovingPlatformPathPoint(
            editor.interaction.movingPlatformPathEdit, payload->pointIndex);
        result.changed = result.accepted;
        result.message = result.accepted
                             ? "moving platform waypoint selected"
                             : "moving platform waypoint selection rejected";
        break;
      }
      const CreativeMovingPlatformPathEditReceipt receipt =
          setCreativeMovingPlatformWaypointDwellWithUndo(
              activeAppState, payload->objectId, payload->pointIndex,
              payload->dwellSeconds, "desktop_inspector_waypoint_dwell");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.changed ? 1U : 0U;
      result.message = std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::EquipAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "equip: payload mismatch";
        break;
      }
      const creative::CreativeAuthoredAssetDefinition* definition =
          findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                          payload->assetId);
      if (definition == nullptr) {
        result.message = "equip: unknown asset";
        break;
      }
      const bool equipped =
          equipCreativeEditorAuthoredAssetToHotbar(appState, editor, *definition);
      result.accepted = equipped;
      result.changed = equipped;
      result.message = equipped ? "equipped " + definition->label
                                : "equip failed";
      break;
    }
    case CreativeDesktopCommandId::EditAssetSource: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "edit asset: payload mismatch";
        break;
      }
      switch (payload->editPhase) {
        case CreativeDesktopAssetEditPhase::Begin: {
          const CreativeEditorAuthoredAssetMutationReceipt receipt =
              beginCreativeEditorAuthoredAssetEdit(editor, payload->assetId);
          result.accepted = receipt.accepted;
          result.changed = receipt.accepted;
          result.message =
              receipt.accepted ? "edit session started" : "edit begin failed";
          break;
        }
        case CreativeDesktopAssetEditPhase::Save: {
          const CreativeEditorAuthoredAssetMutationReceipt receipt =
              saveCreativeEditorAuthoredAssetEdit(editor);
          result.accepted = receipt.accepted;
          result.changed = receipt.accepted;
          result.message =
              receipt.accepted ? "edit session saved" : "edit save failed";
          break;
        }
        case CreativeDesktopAssetEditPhase::Cancel: {
          const bool ok = cancelCreativeEditorAuthoredAssetEdit(editor);
          result.accepted = ok;
          result.changed = ok;
          result.message = ok ? "edit session cancelled" : "no edit session";
          break;
        }
        case CreativeDesktopAssetEditPhase::None:
          result.message = "edit asset: missing phase";
          break;
      }
      break;
    }
    case CreativeDesktopCommandId::RenameAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "rename asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetMutationReceipt receipt =
          renameCreativeEditorAuthoredAsset(editor.authoredAssets,
                                            payload->assetId, payload->name);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message =
          receipt.accepted ? "asset renamed" : "asset rename failed";
      break;
    }
    case CreativeDesktopCommandId::DuplicateAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "duplicate asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetMutationReceipt receipt =
          duplicateCreativeEditorAuthoredAsset(editor.authoredAssets,
                                               payload->assetId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message =
          receipt.accepted ? "asset duplicated" : "asset duplicate failed";
      break;
    }
    case CreativeDesktopCommandId::DeleteAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "delete asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetMutationReceipt receipt =
          deleteCreativeEditorAuthoredAsset(appState.facade.document(),
                                            editor.authoredAssets,
                                            payload->assetId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message = receipt.accepted ? "asset deleted"
                                        : "asset delete blocked: " +
                                              receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::RefreshInstances: {
      const auto* payload =
          payloadAs<CreativeDesktopInstanceRefreshPayload>(command);
      if (payload == nullptr) {
        result.message = "refresh instances: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetInstanceRefreshReceipt receipt =
          refreshCreativeEditorAuthoredAssetInstances(
              appState, editor.authoredAssets, payload->instanceRootObjectId,
              payload->mode);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message = receipt.accepted ? "instances refreshed"
                                        : "refresh failed: " + receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::UpdateAssetFromInstance: {
      const auto* payload =
          payloadAs<CreativeDesktopInstanceRefreshPayload>(command);
      if (payload == nullptr) {
        result.message = "update asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetUpdateReceipt receipt =
          updateCreativeEditorAuthoredAssetFromInstance(
              appState, editor.authoredAssets, payload->instanceRootObjectId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message = receipt.accepted ? "asset updated from instance"
                                        : "update failed: " + receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::TerrainGenerationPreview:
    case CreativeDesktopCommandId::TerrainGenerationRegenerate: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout) ||
          editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "terrain generator unavailable in this workspace";
        break;
      }
      const bool wasActive = editor.terrainGeneration.previewActive;
      const bool regenerate =
          command.id == CreativeDesktopCommandId::TerrainGenerationRegenerate;
      const CreativeEditorTerrainGenerationPreviewReceipt receipt =
          previewCreativeEditorTerrainGeneration(
              editor.terrainGeneration, activeAppState.facade.document(),
              regenerate);
      if (receipt.accepted && !wasActive) {
        static_cast<void>(focusEditorCameraOnTerrainGeneration(
            editor, activeAppState.facade.document(),
            editor.terrainGeneration.generation));
      }
      result.accepted = receipt.accepted;
      result.changed = true;
      result.affectedObjectCount =
          editor.terrainGeneration.operationPreview.receipt.replay
              .modifiedCellCount;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainGenerationApply: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout) ||
          editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "terrain generator unavailable in this workspace";
        break;
      }
      const CreativeEditorTerrainGenerationApplyReceipt receipt =
          applyCreativeEditorTerrainGeneration(activeAppState,
                                               editor.terrainGeneration);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.affectedObjectCount = receipt.operation.replay.outputCellCount;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainGenerationCancel:
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "cancel the terrain region from World Layout";
        break;
      }
      result.changed = cancelCreativeEditorTerrainGeneration(
          editor.terrainGeneration, "Terrain preview canceled");
      result.accepted = true;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    case CreativeDesktopCommandId::TerrainOperationNew:
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      result.accepted = true;
      result.changed =
          beginNewCreativeEditorTerrainOperation(editor.terrainGeneration);
      result.message = editor.terrainGeneration.statusMessage;
      break;
    case CreativeDesktopCommandId::TerrainOperationSelect: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain operation select: payload mismatch";
        break;
      }
      const bool exists = creative::findCreativeTerrainOperation(
                              activeAppState.facade.document()
                                  .terrainOperationStack(),
                              payload->operationId) != nullptr;
      result.changed = exists && selectCreativeEditorTerrainOperation(
                                     editor.terrainGeneration,
                                     activeAppState.facade.document(),
                                     payload->operationId);
      result.accepted = exists;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainOperationSetEnabled:
    case CreativeDesktopCommandId::TerrainOperationMove:
    case CreativeDesktopCommandId::TerrainOperationDuplicate:
    case CreativeDesktopCommandId::TerrainOperationDelete: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain operation edit: payload mismatch";
        break;
      }
      creative::CreativeTerrainOperationMutationRequest request;
      request.operationId = payload->operationId;
      std::string_view source = "desktop_terrain_operation_edit";
      if (command.id ==
          CreativeDesktopCommandId::TerrainOperationSetEnabled) {
        request.kind =
            creative::CreativeTerrainOperationMutationKind::SetEnabled;
        request.enabled = payload->enabled;
        source = "desktop_terrain_operation_enable";
      } else if (command.id ==
                 CreativeDesktopCommandId::TerrainOperationMove) {
        request.kind = creative::CreativeTerrainOperationMutationKind::Move;
        request.targetIndex = payload->targetIndex;
        source = "desktop_terrain_operation_move";
      } else if (command.id ==
                 CreativeDesktopCommandId::TerrainOperationDelete) {
        request.kind = creative::CreativeTerrainOperationMutationKind::Remove;
        source = "desktop_terrain_operation_delete";
      } else {
        const creative::CreativeTerrainOperation* operation =
            creative::findCreativeTerrainOperation(
                activeAppState.facade.document().terrainOperationStack(),
                payload->operationId);
        if (operation == nullptr) {
          result.message = "terrain operation duplicate: source unavailable";
          break;
        }
        request.kind = creative::CreativeTerrainOperationMutationKind::Add;
        request.generation = operation->generation;
        request.composition = operation->composition;
        request.enabled = operation->enabled;
        source = "desktop_terrain_operation_duplicate";
      }
      const CreativeEditorTerrainOperationEditReceipt receipt =
          editCreativeEditorTerrainOperation(
              activeAppState, editor.terrainGeneration, request, source);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.affectedObjectCount = receipt.operation.replay.outputCellCount;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout)) {
        result.message = "terrain region unavailable in this workspace";
        break;
      }
      const CreativeEditorTerrainGenerationPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutTerrainRegion(
              editor.worldLayoutTopography.region, editor.terrainGeneration,
              activeAppState.facade.document());
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.affectedObjectCount =
          editor.terrainGeneration.operationPreview.receipt.replay
              .modifiedCellCount;
      result.message = editor.worldLayoutTopography.region.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionApply: {
      const CreativeEditorTerrainGenerationApplyReceipt receipt =
          applyCreativeEditorWorldLayoutTerrainRegion(
              editor.worldLayoutTopography.region, editor.terrainGeneration,
              activeAppState);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.affectedObjectCount = receipt.operation.replay.outputCellCount;
      result.message = editor.worldLayoutTopography.region.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel:
      result.changed = cancelCreativeEditorWorldLayoutTerrainRegion(
          editor.worldLayoutTopography.region, editor.terrainGeneration);
      result.accepted = true;
      result.message = editor.worldLayoutTopography.region.statusMessage;
      break;
    case CreativeDesktopCommandId::WorldLayoutSetTool: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutToolPayload>(command);
      if (payload == nullptr) {
        result.message = "layout tool: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutTool(editor.worldLayout, payload->tool);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSelectCatalogAsset: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutCatalogAssetPayload>(command);
      if (payload == nullptr || payload->assetId.empty()) {
        result.message = "layout asset: payload mismatch";
        break;
      }
      const creative::CreativeCatalogEntry* found =
          findCatalogAsset(editor.catalog.model, payload->assetId);
      if (found == nullptr) {
        result.message = "layout asset: catalog entry missing";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          selectCreativeEditorWorldLayoutCatalogAsset(editor.worldLayout,
                                                      *found);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSelectBuilding: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingSelectionPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building selection: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          selectCreativeEditorWorldLayoutBuilding(
              editor.worldLayout, payload->buildingIndex);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutFocusSource: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutSourcePayload>(command);
      if (payload == nullptr) {
        result.message = "layout source focus: payload mismatch";
        break;
      }
      if (!payload->stableKey.empty() &&
          !creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->table, payload->index,
              payload->stableKey)) {
        result.message = "layout source focus: stale target";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          focusCreativeEditorWorldLayoutSource(
              editor.worldLayout, payload->table, payload->index);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutSourcePayload>(command);
      if (payload == nullptr) {
        result.message = "layout source frame: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->table, payload->index,
              payload->stableKey)) {
        result.message = "layout source frame: stale target";
        break;
      }
      if (editor.worldLayout.generatedRevision !=
          editor.worldLayout.revision) {
        result.message = "layout source frame: generate pending edits";
        break;
      }
      if (payload->table ==
              creative::CreativeWorldLayoutTable::TerrainProfile ||
          payload->table == creative::CreativeWorldLayoutTable::TerrainPath) {
        const creative::CreativeWorldLayoutTerrainImpactPlan impactPlan =
            creative::buildCreativeWorldLayoutTerrainImpactPlan(
                appState.facade.document(), editor.worldLayout.source);
        const creative::CreativeWorldLayoutTerrainSourceImpact* impact =
            creative::findCreativeWorldLayoutTerrainSourceImpact(
                impactPlan, payload->table, payload->index);
        creative::CreativeBounds bounds{};
        if (impact == nullptr ||
            !creative::creativeWorldLayoutTerrainImpactWorldBounds(
                *impact, appState.facade.document().gridSettings(), bounds)) {
          result.message = "layout source frame: no terrain impact bounds";
          break;
        }
        if (!focusEditorCameraOnBounds(editor, bounds)) {
          result.message = "layout source frame: bounds unavailable";
          break;
        }
        result.accepted = true;
        result.changed = true;
        result.affectedObjectCount =
            impact->controls.size() + impact->materials.size();
        result.message = "terrain source impact framed in 3D";
        break;
      }
      static_cast<void>(refreshCreativeDesktopGeneratedSourceScopeCache(
          editor.generatedSourceScopeCache, appState.facade.document(),
          editor.worldLayout.source, editor.worldLayout.sourceEpoch,
          editor.worldLayout.revision, editor.worldLayout.generatedRevision,
          payload->table, payload->index));
      const CreativeDesktopGeneratedSourceScopeSummary& summary =
          editor.generatedSourceScopeCache.summary;
      if (!summary.valid || !summary.hasBounds) {
        result.message = "layout source frame: no generated bounds";
        break;
      }
      if (!focusEditorCameraOnBounds(editor, summary.worldBounds)) {
        result.message = "layout source frame: bounds unavailable";
        break;
      }
      result.accepted = true;
      result.changed = true;
      result.affectedObjectCount = summary.objectCount;
      result.message = "source scope framed in 3D";
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSelectSourceScope: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutSourcePayload>(command);
      if (payload == nullptr) {
        result.message = "layout source scope: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->table, payload->index,
              payload->stableKey)) {
        result.message = "layout source scope: stale target";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          selectCreativeEditorWorldLayoutSource(
              editor.worldLayout, payload->table, payload->index);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutFocusObjectSource: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutObjectSourcePayload>(command);
      if (payload == nullptr) {
        result.message = "layout object source focus: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "layout object source focus: target missing";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      if (previewWasActive) {
        static_cast<void>(
            cancelCreativeEditorWorldLayoutPreview(editor.worldLayout));
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          focusCreativeEditorWorldLayoutObjectSource(editor.worldLayout,
                                                     *object);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = previewWasActive;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted) {
        editor.desktopUi.showWorldLayout = true;
      }
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutAdoptObjectSource: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutObjectSourcePayload>(command);
      if (payload == nullptr) {
        result.message = "layout object adoption: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutAdoptionReceipt receipt =
          adoptCreativeEditorWorldLayoutObjectSource(
              editor.worldLayout, appState, payload->objectId);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.affectedObjectCount = receipt.changed ? 1U : 0U;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted) {
        editor.desktopUi.showWorldLayout = true;
      }
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutRenameSource: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutSourceRenamePayload>(command);
      if (payload == nullptr) {
        result.message = "layout source rename: payload mismatch";
        break;
      }
      if (!payload->stableKey.empty() &&
          !creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->table, payload->index,
              payload->stableKey)) {
        result.message = "layout source rename: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          renameCreativeEditorWorldLayoutSource(
              editor.worldLayout, payload->table, payload->index,
              payload->name);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutDuplicateSource: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutSourcePayload>(command);
      if (payload == nullptr) {
        result.message = "layout source duplicate: payload mismatch";
        break;
      }
      if (!payload->stableKey.empty() &&
          !creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->table, payload->index,
              payload->stableKey)) {
        result.message = "layout source duplicate: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          duplicateCreativeEditorWorldLayoutSource(
              editor.worldLayout, payload->table, payload->index);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutDeleteSource: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutSourcePayload>(command);
      if (payload == nullptr) {
        result.message = "layout source delete: payload mismatch";
        break;
      }
      if (!payload->stableKey.empty() &&
          !creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->table, payload->index,
              payload->stableKey)) {
        result.message = "layout source delete: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          deleteCreativeEditorWorldLayoutSource(
              editor.worldLayout, payload->table, payload->index);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutLevelOperation: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutLevelOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout level operation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutLevelOperation(
              editor.worldLayout, payload->operation, payload->buildingIndex,
              payload->levelIndex);
      const bool sourceChanged =
          payload->operation !=
              CreativeEditorWorldLayoutLevelOperation::Select &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetLevelSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutLevelSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout level settings: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, cr::CreativeWorldLayoutTable::Level,
              payload->levelIndex, payload->stableKey)) {
        result.message = "layout level settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutLevelSettings(
              editor.worldLayout, payload->levelIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetBuildingGrounding: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutBuildingGroundingPayload>(command);
      if (payload == nullptr) {
        result.message = "layout building grounding: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, cr::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey)) {
        result.message = "layout building grounding: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutBuildingGroundingSettings(
              editor.worldLayout, payload->buildingIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedLevelSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated level settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated level settings: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Level,
              payload->levelIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated level settings: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated level settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutLevelSettingsToDocument(
              editor.worldLayout, appState, payload->levelIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedLevelSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedLevelSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated level preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated level preview: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Level,
              payload->levelIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated level preview: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated level preview: stale target";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutLevelSettings(
              editor.worldLayout, appState.facade.document(),
              payload->levelIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetTerrainProfileSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutTerrainProfileSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout terrain profile settings: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout,
              cr::CreativeWorldLayoutTable::TerrainProfile,
              payload->profileIndex, payload->stableKey)) {
        result.message = "layout terrain profile settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutTerrainProfileSettings(
              editor.worldLayout, payload->profileIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetTerrainPathSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutTerrainPathSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout terrain path settings: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, cr::CreativeWorldLayoutTable::TerrainPath,
              payload->pathIndex, payload->stableKey)) {
        result.message = "layout terrain path settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutTerrainPathSettings(
              editor.worldLayout, payload->pathIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetObjectSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutObjectSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout object settings: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, cr::CreativeWorldLayoutTable::Object,
              payload->objectIndex, payload->stableKey)) {
        result.message = "layout object settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutObjectSettings(
              editor.worldLayout, payload->objectIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutClearSelection: {
      const CreativeEditorWorldLayoutEditReceipt receipt =
          clearCreativeEditorWorldLayoutSelection(editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateBuilding: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingManipulationPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building manipulation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingManipulation(
              editor.worldLayout, payload->phase, payload->point,
              payload->toleranceCells);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingManipulationPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutDuplicateBuilding: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingDuplicatePayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building duplicate: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          duplicateCreativeEditorWorldLayoutBuilding(
              editor.worldLayout, payload->buildingIndex,
              payload->deltaXCells, payload->deltaZCells);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutTransformBuilding: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTransformPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building transform: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingTransform(
              editor.worldLayout, payload->phase, payload->operation);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTransformPhase::Commit &&
          receipt.changed;
      const bool exactPreviewClosed =
          previewWasActive &&
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTransformPhase::Preview &&
          receipt.accepted;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged =
          exactPreviewClosed || (previewWasActive && sourceChanged);
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutPreviewGeneratedBuildingOperation: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedBuildingOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "generated building preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated building preview: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated building preview: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated building preview: stale target";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutGeneratedBuildingOperation(
              editor.worldLayout, appState.facade.document(),
              payload->buildingIndex, payload->operation,
              payload->deltaXCells, payload->deltaZCells);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutApplyGeneratedBuildingOperation: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedBuildingOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "generated building operation: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated building operation: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated building operation: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated building operation: stale target";
        break;
      }
      const creative::CreativeObjectKind sourceObjectKind = object->kind;
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutGeneratedBuildingOperationToDocument(
              editor.worldLayout, appState, payload->buildingIndex,
              payload->operation, payload->deltaXCells,
              payload->deltaZCells);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted &&
          payload->operation ==
              CreativeEditorWorldLayoutGeneratedBuildingOperation::Duplicate) {
        const std::size_t duplicateBuildingIndex =
            editor.worldLayout.selection.kind ==
                    CreativeEditorWorldLayoutSelectionKind::Building
                ? editor.worldLayout.selection.index
                : creative::kInvalidCreativeWorldLayoutIndex;
        const creative::CreativeObjectId duplicateObjectId =
            findGeneratedSourceScopeObject(
                appState.facade.document(), editor.worldLayout.source,
                creative::CreativeWorldLayoutTable::Building,
                duplicateBuildingIndex, sourceObjectKind);
        if (duplicateObjectId != creative::kInvalidObjectId) {
          static_cast<void>(appState.facade.selectTargets(
              std::span<const creative::CreativeObjectId>{&duplicateObjectId,
                                                          1U},
              duplicateObjectId));
        } else {
          editor.worldLayout.selection = {
              CreativeEditorWorldLayoutSelectionKind::Building,
              payload->buildingIndex};
        }
      }
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutApplyGeneratedBuildingGrounding: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutBuildingGroundingPayload>(command);
      if (payload == nullptr) {
        result.message = "generated building grounding: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated building grounding: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated building grounding: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated building grounding: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingGroundingSettingsToDocument(
              editor.worldLayout, appState, payload->buildingIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCaptureBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateCapturePayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template capture: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          captureCreativeEditorWorldLayoutBuildingTemplate(
              editor.worldLayout, payload->buildingIndex, payload->label);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutUpdateBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateSyncPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template update: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          updateCreativeEditorWorldLayoutBuildingTemplateFromInstance(
              editor.worldLayout, payload->buildingIndex);
      result.accepted = receipt.accepted;
      result.changed =
          receipt.accepted &&
          receipt.reasonCode !=
              "creative_editor_world_layout_building_template_update_no_change";
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutRefreshBuildingTemplateInstances: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateSyncPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template refresh: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
              editor.worldLayout, payload->buildingIndex, payload->mode);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template selection: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          selectCreativeEditorWorldLayoutBuildingTemplate(
              editor.worldLayout, payload->templateIndex);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template placement: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
              editor.worldLayout, payload->phase, payload->point,
              payload->operation);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit &&
          receipt.changed;
      const bool exactPreviewClosed =
          previewWasActive &&
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin &&
          receipt.accepted;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged =
          exactPreviewClosed || (previewWasActive && sourceChanged);
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCanvasPoint: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutPointPayload>(command);
      if (payload == nullptr) {
        result.message = "layout point: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutPoint(editor.worldLayout,
                                              payload->point,
                                              activeAppState.facade.document()
                                                  .gridSettings());
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCanvasGesture: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutGesturePayload>(command);
      if (payload == nullptr) {
        result.message = "layout gesture: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutGesture(
              editor.worldLayout, payload->phase, payload->point);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetRoomSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutRoomSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout room settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutRoomSettings(
              editor.worldLayout, payload->roomIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedRoomSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated room settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated room settings: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Room, payload->roomIndex,
              payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated room settings: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated room settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutRoomSettingsToDocument(
              editor.worldLayout, appState, payload->roomIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedRoomSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedRoomSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated room preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated room preview: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Room, payload->roomIndex,
              payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated room preview: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated room preview: stale target";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutRoomSettings(
              editor.worldLayout, appState.facade.document(),
              payload->roomIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateRoom: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutRoomManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout room manipulation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutRoomManipulation(
              editor.worldLayout, payload->phase, payload->point,
              payload->toleranceCells);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutRoomManipulationPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetVerticalConnectorSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutVerticalConnectorSettingsPayload>(
          command);
      if (payload == nullptr) {
        result.message = "layout vertical connector settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutVerticalConnectorSettings(
              editor.worldLayout, payload->connectorIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutApplyGeneratedVerticalConnectorSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopGeneratedVerticalConnectorSettingsPayload>(command);
      if (payload == nullptr) {
        result.message =
            "generated vertical connector settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated vertical connector settings: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table !=
              creative::CreativeWorldLayoutTable::VerticalConnector) {
        result.message = "generated vertical connector settings: source mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutVerticalConnectorSettingsToDocument(
              editor.worldLayout, appState, provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutPreviewGeneratedVerticalConnectorSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopGeneratedVerticalConnectorSettingsPayload>(command);
      if (payload == nullptr) {
        result.message =
            "generated vertical connector preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated vertical connector preview: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table !=
              creative::CreativeWorldLayoutTable::VerticalConnector) {
        result.message = "generated vertical connector preview: source mismatch";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutVerticalConnectorSettings(
              editor.worldLayout, appState.facade.document(), provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload>(
          command);
      if (payload == nullptr) {
        result.message =
            "layout vertical connector manipulation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
              editor.worldLayout, payload->phase, payload->point,
              payload->toleranceCells);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                  Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetBoxSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBoxSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout floor settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutBoxSettings(
              editor.worldLayout, payload->boxIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateBox: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBoxManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout floor manipulation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutBoxManipulation(
              editor.worldLayout, payload->phase, payload->point,
              payload->toleranceCells);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBoxManipulationPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetWallSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutWallSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout partition settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutWallSettings(
              editor.worldLayout, payload->wallIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedWallSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated partition settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated partition settings: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Wall) {
        result.message = "generated partition settings: source mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutWallSettingsToDocument(
              editor.worldLayout, appState, provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedWallSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedWallSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated partition preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated partition preview: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Wall) {
        result.message = "generated partition preview: source mismatch";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutWallSettings(
              editor.worldLayout, appState.facade.document(),
              provenance.index, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateWall: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutWallManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout partition manipulation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutWallManipulation(
              editor.worldLayout, payload->phase, payload->point,
              payload->toleranceCells);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutWallManipulationPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetOpeningSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutOpeningSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout opening settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutOpeningSettings(
              editor.worldLayout, payload->openingIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedOpeningSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated opening settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated opening settings: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Opening) {
        result.message = "generated opening settings: source mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutOpeningSettingsToDocument(
              editor.worldLayout, appState, provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedOpeningSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedOpeningSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated opening preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated opening preview: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Opening) {
        result.message = "generated opening preview: source mismatch";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutOpeningSettings(
              editor.worldLayout, appState.facade.document(),
              provenance.index, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetOpeningInsert: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutOpeningInsertPayload>(command);
      if (payload == nullptr) {
        result.message = "layout opening insert: payload mismatch";
        break;
      }
      CreativeEditorWorldLayoutOpeningInsertRequest request;
      request.openingIndex = payload->openingIndex;
      request.operation = payload->operation;
      request.assetScale = payload->assetScale;
      request.gridCellSizeMeters =
          context.appState.facade.document().gridSettings().cellSizeMeters;
      if (payload->operation !=
          CreativeEditorWorldLayoutOpeningInsertOperation::
              UseProceduralInsert) {
        const creative::CreativeCatalogEntry* found =
            findCatalogAsset(editor.catalog.model, payload->assetId);
        if (found == nullptr ||
            !creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
                found->assetAuthoringMetadata.categoryId)) {
          result.message = "layout opening insert: catalog entry missing";
          break;
        }
        request.assetId = creative::creativeHotbarAssetId(found->hotbarEntry);
        request.assetSourceBoundsMeters =
            found->hotbarEntry.assetSourceBounds;
        request.assetKind = found->assetAuthoringMetadata.categoryId ==
                                    "window"
                                ? creative::CreativeBuildingOpeningKind::Window
                                : creative::CreativeBuildingOpeningKind::Door;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutOpeningInsert(editor.worldLayout,
                                                      request);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutRepairAsset: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutAssetRepairPayload>(command);
      if (payload == nullptr) {
        result.message = "layout asset repair: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          repairWorldLayoutAsset(
              editor.worldLayout, editor.catalog.model,
              context.appState.facade.document().gridSettings().cellSizeMeters,
              *payload);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateOpening: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutOpeningManipulationPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout opening manipulation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutOpeningManipulation(
              editor.worldLayout, payload->phase, payload->point,
              payload->toleranceCells);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutOpeningManipulationPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutDeleteSelection: {
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          deleteCreativeEditorWorldLayoutSelection(editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreview: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the terrain region before layout preview";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayout(editor.worldLayout,
                                           appState.facade.document());
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.accepted;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted) {
        editor.desktopUi.showWorldLayout = false;
      }
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutConfirm: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the terrain region before layout generation";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutConfirmPayload>(command);
      if (payload == nullptr &&
          !std::holds_alternative<std::monostate>(command.payload)) {
        result.message = "layout confirm: payload mismatch";
        break;
      }
      const std::span<const creative::CreativeWorldLayoutConflictDecision>
          conflictDecisions =
              payload != nullptr
                  ? std::span<const creative::CreativeWorldLayoutConflictDecision>(
                        payload->conflictDecisions)
                  : std::span<const
                        creative::CreativeWorldLayoutConflictDecision>{};
      const std::span<
          const creative::CreativeWorldLayoutTerrainConflictDecision>
          terrainConflictDecisions =
              payload != nullptr
                  ? std::span<const creative::
                                  CreativeWorldLayoutTerrainConflictDecision>(
                        payload->terrainConflictDecisions)
                  : std::span<const creative::
                                  CreativeWorldLayoutTerrainConflictDecision>{};
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          confirmCreativeEditorWorldLayout(editor.worldLayout, appState,
                                           conflictDecisions,
                                           terrainConflictDecisions);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.changed;
      result.affectedObjectCount = receipt.apply.installReceipt.nextObjectCount;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted) {
        editor.desktopUi.showWorldLayout = false;
      }
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCancelPreview: {
      const CreativeEditorWorldLayoutEditReceipt receipt =
          cancelCreativeEditorWorldLayoutPreview(editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      editor.desktopUi.showWorldLayout = true;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCancelGeneratedSettingsPreview: {
      const CreativeEditorWorldLayoutEditReceipt receipt =
          cancelCreativeEditorWorldLayoutPreview(editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::Play:
      if (context.playMode == nullptr) {
        result.message = "play owner is unavailable";
        break;
      }
      if (creativeEditorPlayModeActive(*context.playMode)) {
        const creative::CreativeRuntimeSandboxStopReceipt stopped =
            stopCreativeEditorPlayMode(*context.playMode);
        result.accepted = stopped.stopped;
        result.changed = stopped.stopped;
        result.message = stopped.stopped ? "play stopped"
                                         : "play stop failed";
        break;
      }
      if (editor.terrainGeneration.previewActive) {
        result.message = "apply or cancel the terrain preview before play";
        break;
      }
      {
        CreativeEditorPlayStartRequest request;
        request.document = &appState.facade.document();
        request.staticMeshAssetCatalog = context.staticMeshAssetCatalog;
        const CreativeEditorPlayStartReceipt started =
            startCreativeEditorPlayMode(*context.playMode, std::move(request));
        result.accepted = started.accepted;
        result.changed = started.accepted;
        result.message = started.accepted
                             ? "play started"
                             : "play failed: " + started.reasonCode;
      }
      break;
    case CreativeDesktopCommandId::None:
    case CreativeDesktopCommandId::Count:
      break;
  }
}

}  // namespace

CreativeDesktopCommandResult dispatchCreativeDesktopCommands(
    const CreativeDesktopCommandFrame& frame,
    const CreativeDesktopCommandContext& context) {
  CreativeDesktopCommandResult result;
  const std::size_t count =
      frame.count < kCreativeDesktopCommandCapacity ? frame.count
                                                    : kCreativeDesktopCommandCapacity;
  for (std::size_t index = 0U; index < count; ++index) {
    dispatchOne(frame.commands[index], context, result);
  }
  return result;
}

}  // namespace iggy3d_creative_app
