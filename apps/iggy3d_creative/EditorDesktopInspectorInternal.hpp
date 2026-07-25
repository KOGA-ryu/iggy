#pragma once

#include "EditorDesktopWidgets.hpp"
#include "EditorMovingPlatformPreview.hpp"
#include "EditorPathEditing.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopInspectorSelectionModel {
  const iggy3d::creative::CreativeDocument& document;
  const CreativeDesktopSelectionResolution& selection;
  const CreativeDesktopObjectActionContext& objectActions;
  const iggy3d::creative::CreativeObject* object = nullptr;
  bool playModeActive = false;
};

void appendCreativeDesktopInspectorHoverTooltip(const char* text);

void refreshCreativeDesktopInspectorDraft(
    CreativeDesktopInspectorDraft& draft,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeObject& object);

void appendCreativeDesktopMeasurementInspector(
    const iggy3d::creative::CreativeMeasurementState& measurement,
    const iggy3d::creative::CreativeMeasurementAnnotationStore& annotations,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopLogicInspector(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorLogicLinkState& logicLinks,
    const CreativeDesktopInspectorSelectionModel& selection,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopMultiSelectionInspector(
    const CreativeDesktopInspectorSelectionModel& selection,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopPlayerSpawnFields(
    CreativeDesktopInspectorDraft& draft,
    const iggy3d::creative::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopNpcSpawnFields(
    CreativeDesktopInspectorDraft& draft,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopLootPointFields(
    CreativeDesktopInspectorDraft& draft,
    const iggy3d::creative::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopExitPointFields(
    CreativeDesktopInspectorDraft& draft,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopMovingPlatformFields(
    CreativeDesktopInspectorDraft& draft,
    const iggy3d::creative::CreativeObject& object,
    const CreativeMovingPlatformPreviewState& preview,
    const CreativeMovingPlatformPathEditState& pathEdit,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopSingleObjectInspector(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeDesktopInspectorSelectionModel& selection,
    CreativeEditorWorldLayoutState& worldLayout,
    CreativeDesktopGeneratedSourceScopeCache& generatedSourceScopeCache,
    const CreativeMovingPlatformPreviewState& preview,
    const CreativeMovingPlatformPathEditState& pathEdit,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
