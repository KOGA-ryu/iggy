#include "EditorDesktopCommands.hpp"

#include "EditorDesktopCommandsInternal.hpp"

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

  if (dispatchCreativeDesktopDocumentCommand(command, context, result) ||
      dispatchCreativeDesktopObjectCommand(command, context, result) ||
      dispatchCreativeDesktopAssetCommand(command, context, result) ||
      dispatchCreativeDesktopTerrainCommand(command, context, result)) {
    return;
  }

  switch (command.id) {
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
    default:
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
