#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorTerrainGeneration.hpp"

#include <span>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutSourceCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
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
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
