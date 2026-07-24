#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include "EditorWorldLayoutBuildings.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutSources.hpp"

#include <span>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

namespace {

CreativeEditorWorldLayoutPreviewReceipt previewBuildingCandidateInEveryView(
    CreativeEditorWorldLayoutState& state,
    const creative::CreativeDocument& document,
    const creative::CreativeWorldLayout& source,
    std::string_view successMessage) {
  CreativeEditorWorldLayoutState candidate =
      makeCreativeEditorWorldLayoutLiveEditCandidate(state);
  candidate.source = source;
  ++candidate.revision;
  CreativeEditorWorldLayoutPreviewReceipt preview =
      previewCreativeEditorWorldLayoutLiveEditCandidate(
          state, document, std::move(candidate),
          {true, true,
           "creative_editor_world_layout_building_candidate_preview"},
          successMessage);
  state.liveEditPreviewVisible = preview.accepted;
  return preview;
}

}  // namespace

bool dispatchCreativeDesktopWorldLayoutBuildingCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  switch (command.id) {
    case CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingBlockoutPayload>(command);
      if (payload == nullptr) {
        result.message = "layout building blockout: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          createCreativeEditorWorldLayoutBuildingBlockout(
              editor.worldLayout, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutUpdateBuildingBlockout: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingBlockoutUpdatePayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building blockout update: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          updateCreativeEditorWorldLayoutBuildingBlockout(
              editor.worldLayout, payload->buildingIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
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
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
              CreativeEditorWorldLayoutBuildingManipulationPhase::Update,
              CreativeEditorWorldLayoutBuildingManipulationPhase::Commit,
              CreativeEditorWorldLayoutBuildingManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutBuildingManipulationPhase phase) {
                return applyCreativeEditorWorldLayoutBuildingManipulation(
                    target, phase, payload->point, payload->toleranceCells);
              },
              "desktop_world_layout_building_move",
              "building move preview ready in 3D", "building moved in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
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
      bool exactPreviewChanged = false;
      if (receipt.accepted &&
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTransformPhase::Preview &&
          editor.worldLayout.buildingTransform.active &&
          (receipt.changed || !previewWasActive)) {
        const CreativeEditorWorldLayoutPreviewReceipt preview =
            previewBuildingCandidateInEveryView(
                editor.worldLayout, appState.facade.document(),
                editor.worldLayout.buildingTransform.candidate,
                "building transform preview ready in every view");
        exactPreviewChanged = preview.changed;
      } else if (receipt.accepted &&
                 payload->phase ==
                     CreativeEditorWorldLayoutBuildingTransformPhase::Cancel) {
        exactPreviewChanged =
            clearCreativeEditorWorldLayoutLiveEditPreview(editor.worldLayout);
      }
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTransformPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed || exactPreviewChanged;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged =
          exactPreviewChanged || (previewWasActive && sourceChanged);
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
    case CreativeDesktopCommandId::WorldLayoutPreviewBuildingArchitecture: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutBuildingArchitecturePayload>(command);
      if (payload == nullptr) {
        result.message = "building architecture preview: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey)) {
        result.message = "building architecture preview: stale target";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutBuildingArchitecture(
              editor.worldLayout, appState.facade.document(),
              payload->buildingIndex, payload->profile);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyBuildingArchitecture: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutBuildingArchitecturePayload>(command);
      if (payload == nullptr) {
        result.message = "building architecture apply: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey)) {
        result.message = "building architecture apply: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingArchitectureToDocument(
              editor.worldLayout, appState, payload->buildingIndex,
              payload->profile);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
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
    case CreativeDesktopCommandId::WorldLayoutRepairBuildingUsability: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingRepairPayload>(command);
      if (payload == nullptr) {
        result.message = "layout building repair: payload mismatch";
        break;
      }
      if (!payload->stableKey.empty() &&
          !creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->issue.table,
              payload->issue.index, payload->stableKey)) {
        result.message = "layout building repair: stale target";
        break;
      }
      const creative::CreativeGridSettings grid =
          activeAppState.facade.document().gridSettings();
      const CreativeDesktopWorldLayoutLiveEditResult repair =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, activeAppState,
              [&](CreativeEditorWorldLayoutState& target) {
                return applyCreativeEditorWorldLayoutBuildingRepair(
                    target, grid, payload->issue);
              },
              "desktop_world_layout_building_repair");
      result.accepted = repair.accepted;
      result.changed = repair.changed;
      result.worldLayoutChanged = repair.worldLayoutChanged;
      result.sceneChanged = repair.sceneChanged;
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
    case CreativeDesktopCommandId::WorldLayoutDetachBuildingTemplateInstance: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateSyncPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template detach: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          detachCreativeEditorWorldLayoutBuildingTemplateInstance(
              editor.worldLayout, payload->buildingIndex);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
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
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingTemplateRefreshToDocument(
              editor.worldLayout, appState, payload->buildingIndex,
              payload->mode);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = receipt.apply.changed ||
                            (previewWasActive && receipt.changed);
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
              payload->operation, &appState.facade.document());
      const bool candidatePhase =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin ||
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Update ||
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Transform;
      bool exactPreviewChanged = false;
      if (receipt.accepted && candidatePhase &&
          editor.worldLayout.buildingTemplatePlacement.active &&
          editor.worldLayout.buildingTemplatePlacement.previewValid &&
          (receipt.changed || !previewWasActive)) {
        const CreativeEditorWorldLayoutPreviewReceipt preview =
            previewBuildingCandidateInEveryView(
                editor.worldLayout, appState.facade.document(),
                editor.worldLayout.buildingTemplatePlacement.candidate,
                "building template preview ready in every view");
        exactPreviewChanged = preview.changed;
      } else if (receipt.accepted &&
                 ((candidatePhase &&
                   !editor.worldLayout.buildingTemplatePlacement.previewValid) ||
                  payload->phase ==
                      CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Cancel)) {
        exactPreviewChanged =
            clearCreativeEditorWorldLayoutLiveEditPreview(editor.worldLayout);
      }
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed || exactPreviewChanged;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged =
          exactPreviewChanged || (previewWasActive && sourceChanged);
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
