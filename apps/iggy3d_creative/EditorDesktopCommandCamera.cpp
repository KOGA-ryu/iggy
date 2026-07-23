#include "EditorDesktopCommandsInternal.hpp"

#include <algorithm>

#include "EditorFrame.hpp"
#include "EditorTerrainGeneration.hpp"

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/camera/ViewportNavigation.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

namespace {

void includeBounds(creative::CreativeBounds source,
                   creative::CreativeBounds& aggregate,
                   bool& initialized) noexcept {
  if (!initialized) {
    aggregate = source;
    initialized = true;
    return;
  }
  aggregate.min.x = std::min(aggregate.min.x, source.min.x);
  aggregate.min.y = std::min(aggregate.min.y, source.min.y);
  aggregate.min.z = std::min(aggregate.min.z, source.min.z);
  aggregate.max.x = std::max(aggregate.max.x, source.max.x);
  aggregate.max.y = std::max(aggregate.max.y, source.max.y);
  aggregate.max.z = std::max(aggregate.max.z, source.max.z);
}

void includeObjectBounds(const creative::CreativeObject& object,
                         creative::CreativeBounds& aggregate,
                         bool& initialized) noexcept {
  const creative::CreativeTransformedBounds resolved =
      creative::resolveCreativeObjectBounds(object);
  if (resolved.valid) {
    includeBounds(resolved.worldBounds, aggregate, initialized);
  }
}

}  // namespace

float editorViewportAspectRatio(const CreativeEditorState& editor) noexcept {
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

bool focusEditorCameraOnBounds(CreativeEditorState& editor,
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
  editor.viewportFocus = iggy3d::makeProductCreativeViewportFocus(
      (boundsMin.value + boundsMax.value) * 0.5F, frame.distanceMeters);
  return true;
}

bool focusEditorCameraOnObject(
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

bool focusEditorCameraOnSelection(
    CreativeEditorState& editor,
    const creative::CreativeDocument& document,
    const creative::CreativeSelectionState& selection) noexcept {
  creative::CreativeBounds bounds;
  bool initialized = false;
  for (creative::TargetRef target : creative::selectedTargetList(selection)) {
    const creative::CreativeObject* object = document.findObject(
        static_cast<creative::CreativeObjectId>(target.value));
    if (object != nullptr && creative::creativeObjectEffectivelyVisible(
                                 document, object->id)) {
      includeObjectBounds(*object, bounds, initialized);
    }
  }
  return initialized && focusEditorCameraOnBounds(editor, bounds);
}

bool focusEditorCameraOnDocument(
    CreativeEditorState& editor,
    const creative::CreativeDocument& document) noexcept {
  creative::CreativeBounds bounds;
  bool initialized = false;
  for (const creative::CreativeObject& object : document.objects()) {
    if (creative::creativeObjectEffectivelyVisible(document, object.id)) {
      includeObjectBounds(object, bounds, initialized);
    }
  }
  return initialized && focusEditorCameraOnBounds(editor, bounds);
}

bool focusEditorCameraOnTerrainGeneration(
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

}  // namespace iggy3d_creative_app
