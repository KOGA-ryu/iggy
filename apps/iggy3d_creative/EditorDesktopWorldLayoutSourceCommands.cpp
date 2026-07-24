#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorTerrainGeneration.hpp"
#include "EditorWorldLayoutRoofs.hpp"

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

      CreativeEditorWorldLayoutState selectionCandidate = editor.worldLayout;
      const CreativeEditorWorldLayoutEditReceipt candidateReceipt =
          selectCreativeEditorWorldLayoutSource(
              selectionCandidate, payload->table, payload->index,
              payload->preferredLevelIndex);
      if (!candidateReceipt.accepted) {
        editor.worldLayout.statusMessage = selectionCandidate.statusMessage;
        result.message = editor.worldLayout.statusMessage;
        break;
      }

      editor.generatedSourceScopeCache = {};
      if (editor.worldLayout.generatedRevision == editor.worldLayout.revision) {
        static_cast<void>(refreshCreativeDesktopGeneratedSourceScopeCache(
            editor.generatedSourceScopeCache, appState.facade.document(),
            editor.worldLayout.source, editor.worldLayout.sourceEpoch,
            editor.worldLayout.revision,
            editor.worldLayout.generatedRevision, payload->table,
            payload->index));
      }
      if (editor.generatedSourceScopeCache.objectIds.size() >
          creative::kCreativeSelectionTargetCapacity) {
        result.message = "layout source scope exceeds selection capacity";
        break;
      }

      const std::span<const creative::CreativeObjectId> generatedObjectIds =
          editor.generatedSourceScopeCache.objectIds;
      const creative::CreativeObjectId primaryObjectId =
          generatedObjectIds.empty() ? creative::kInvalidObjectId
                                     : generatedObjectIds.front();
      const creative::CreativeSelectionReceipt objectSelection =
          appState.facade.selectTargets(generatedObjectIds, primaryObjectId);
      if (!objectSelection.accepted) {
        result.message = objectSelection.message;
        break;
      }

      const CreativeEditorWorldLayoutEditReceipt receipt =
          selectCreativeEditorWorldLayoutSource(
              editor.worldLayout, payload->table, payload->index,
              payload->preferredLevelIndex);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed || objectSelection.changed;
      result.affectedObjectCount = generatedObjectIds.size();
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
              editor.worldLayout, payload->table, payload->index,
              activeAppState.facade.document().gridSettings());
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
      if (payload->table == cr::CreativeWorldLayoutTable::RoofAperture) {
        const CreativeDesktopWorldLayoutLiveEditResult deleted =
            dispatchCreativeDesktopWorldLayoutImmediateEdit(
                editor.worldLayout, appState,
                [&](CreativeEditorWorldLayoutState& target) {
                  return deleteCreativeEditorWorldLayoutSource(
                      target, payload->table, payload->index);
                },
                "desktop_world_layout_roof_aperture_delete");
        result.accepted = deleted.accepted;
        result.changed = deleted.changed;
        result.worldLayoutChanged = deleted.worldLayoutChanged;
        result.sceneChanged = deleted.sceneChanged;
      } else {
        const bool previewWasActive =
            creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
        const CreativeEditorWorldLayoutEditReceipt receipt =
            deleteCreativeEditorWorldLayoutSource(
                editor.worldLayout, payload->table, payload->index);
        result.accepted = receipt.accepted;
        result.changed = receipt.changed;
        result.worldLayoutChanged = receipt.changed;
        result.sceneChanged = previewWasActive && receipt.changed;
      }
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
    case CreativeDesktopCommandId::WorldLayoutCreateRoofAperture: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutRoofApertureCreatePayload>(command);
      if (payload == nullptr) {
        result.message = "layout roof aperture: payload mismatch";
        break;
      }
      const auto createAperture = [&](CreativeEditorWorldLayoutState& target) {
        return createCreativeEditorWorldLayoutRoofAperture(
            target, payload->levelIndex, payload->kind);
      };
      const CreativeDesktopWorldLayoutLiveEditResult created =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, appState, createAperture,
              "desktop_world_layout_roof_aperture_create");
      result.accepted = created.accepted;
      result.changed = created.changed;
      result.worldLayoutChanged = created.worldLayoutChanged;
      result.sceneChanged = created.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutRoofApertureManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout roof aperture manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
              CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update,
              CreativeEditorWorldLayoutRoofApertureManipulationPhase::Commit,
              CreativeEditorWorldLayoutRoofApertureManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutRoofApertureManipulationPhase
                      phase) {
                return applyCreativeEditorWorldLayoutRoofApertureManipulation(
                    target, phase, payload->point, payload->toleranceCells,
                    appState.facade.document().gridSettings());
              },
              "desktop_world_layout_roof_aperture_drag",
              "roof aperture drag preview ready in 3D",
              "roof aperture updated in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateRoof: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutRoofManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout roof manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
              CreativeEditorWorldLayoutRoofManipulationPhase::Update,
              CreativeEditorWorldLayoutRoofManipulationPhase::Commit,
              CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutRoofManipulationPhase phase) {
                return applyCreativeEditorWorldLayoutRoofManipulation(
                    target, phase, payload->target,
                    payload->coordinateCells,
                    appState.facade.document().gridSettings());
              },
              "desktop_world_layout_roof_drag",
              "roof drag preview ready in 3D",
              "roof updated in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutClearSelection: {
      const CreativeEditorWorldLayoutEditReceipt receipt =
          clearCreativeEditorWorldLayoutSelection(editor.worldLayout);
      editor.generatedSourceScopeCache = {};
      const creative::CreativeSelectionReceipt objectSelection =
          appState.facade.selectTargets(
              std::span<const creative::CreativeObjectId>{});
      result.accepted = receipt.accepted && objectSelection.accepted;
      result.changed = receipt.changed || objectSelection.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
