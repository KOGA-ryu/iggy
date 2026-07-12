#include "EditorPattern.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] std::vector<cr::CreativeObjectId>
selectedHierarchyObjectIds(const cr::CreativeAppState& appState) {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const std::span<const cr::TargetRef> targets =
      cr::selectedTargetList(selection);
  std::vector<cr::CreativeObjectId> selected;
  selected.reserve(targets.empty() ? 1U : targets.size());
  for (cr::TargetRef target : targets) {
    if (target.value != cr::kInvalidId) {
      selected.push_back(static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  if (selected.empty() && selection.selectedTarget.value != cr::kInvalidId) {
    selected.push_back(static_cast<cr::CreativeObjectId>(
        selection.selectedTarget.value));
  }
  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(appState.facade.document(), selected);
  return hierarchy.accepted ? hierarchy.objectIds : selected;
}

[[nodiscard]] iggy3d::Vec3 rotateAroundPivot(
    iggy3d::Vec3 point,
    iggy3d::Vec3 pivot,
    cr::CreativeAxis3 axis,
    double radians) noexcept {
  const iggy3d::Vec3 local = point - pivot;
  const cr::CreativeVec3 rotated = cr::rotateCreativeVectorAxisAngle(
      {static_cast<double>(local.x), static_cast<double>(local.y),
       static_cast<double>(local.z)},
      axis, radians);
  return pivot + iggy3d::Vec3{static_cast<float>(rotated.x),
                              static_cast<float>(rotated.y),
                              static_cast<float>(rotated.z)};
}

[[nodiscard]] VisualBounds rotateVisualBounds(
    VisualBounds bounds,
    iggy3d::Vec3 pivot,
    cr::CreativeAxis3 axis,
    double radians) noexcept {
  VisualBounds output;
  output.min = {std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  output.max = {std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()};
  for (std::size_t cornerIndex = 0; cornerIndex < 8U; ++cornerIndex) {
    const iggy3d::Vec3 corner{
        (cornerIndex & 1U) != 0U ? bounds.max.x : bounds.min.x,
        (cornerIndex & 2U) != 0U ? bounds.max.y : bounds.min.y,
        (cornerIndex & 4U) != 0U ? bounds.max.z : bounds.min.z,
    };
    const iggy3d::Vec3 rotated =
        rotateAroundPivot(corner, pivot, axis, radians);
    output.min.x = std::min(output.min.x, rotated.x);
    output.min.y = std::min(output.min.y, rotated.y);
    output.min.z = std::min(output.min.z, rotated.z);
    output.max.x = std::max(output.max.x, rotated.x);
    output.max.y = std::max(output.max.y, rotated.y);
    output.max.z = std::max(output.max.z, rotated.z);
  }
  return output;
}

[[nodiscard]] VisualBounds rotateObjectVisualBounds(
    const cr::CreativeObject& object,
    iggy3d::Vec3 pivot,
    cr::CreativeAxis3 axis,
    double radians) noexcept {
  const cr::CreativeTransformedBounds resolved =
      cr::resolveCreativeObjectBounds(object);
  if (!resolved.valid) {
    return rotateVisualBounds(visualBoundsForObject(object), pivot, axis,
                              radians);
  }
  VisualBounds output;
  output.min = {std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  output.max = {std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()};
  for (cr::CreativeVec3 corner : resolved.corners) {
    const cr::CreativeCoreVec3Conversion core =
        cr::creativeVec3ToCoreChecked(corner);
    if (!core.converted) {
      return rotateVisualBounds(visualBoundsForObject(object), pivot, axis,
                                radians);
    }
    const iggy3d::Vec3 rotated =
        rotateAroundPivot(core.value, pivot, axis, radians);
    output.min.x = std::min(output.min.x, rotated.x);
    output.min.y = std::min(output.min.y, rotated.y);
    output.min.z = std::min(output.min.z, rotated.z);
    output.max.x = std::max(output.max.x, rotated.x);
    output.max.y = std::max(output.max.y, rotated.y);
    output.max.z = std::max(output.max.z, rotated.z);
  }
  return output;
}

}  // namespace

cr::CreativeLinearArrayRequest creativeEditorLinearArrayRequest(
    const cr::CreativeToolSettings& settings,
    double cellSize) noexcept {
  cr::CreativeLinearArrayRequest request;
  request.direction = settings.arrayDirection;
  request.copyCount = settings.arrayCopyCount;
  request.spacing = settings.arraySpacing;
  request.cellSize = cellSize;
  return request;
}

cr::CreativeRadialArrayRequest creativeEditorRadialArrayRequest(
    const cr::CreativeToolSettings& settings,
    cr::CreativeVec3 pivot) noexcept {
  cr::CreativeRadialArrayRequest request;
  request.pivot = pivot;
  request.axis = settings.radialArrayAxis;
  request.instanceCount = settings.radialArrayInstanceCount;
  request.sweep = settings.radialArraySweep;
  return request;
}

cr::CreativeLinearArrayReceipt applyCreativeEditorLinearArrayWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const cr::CreativeToolSettings& settings,
    double cellSize,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  state.lastReceipt = appState.facade.createLinearArrayFromSelection(
      creativeEditorLinearArrayRequest(settings, cellSize));
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastReceipt.accepted && state.lastReceipt.changed,
      state.lastReceipt.message));

  SDL_Log("iggy3d_creative: LINEAR_ARRAY status='%s' accepted=%d changed=%d "
          "sourceObjects=%llu generatedObjects=%llu direction='%s' copies='%s' "
          "spacing='%s' reasonCode='%s'",
          std::string(cr::toString(state.lastReceipt.status)).c_str(),
          state.lastReceipt.accepted ? 1 : 0,
          state.lastReceipt.changed ? 1 : 0,
          static_cast<unsigned long long>(state.lastReceipt.sourceObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.generatedObjectCount),
          std::string(cr::toString(settings.arrayDirection)).c_str(),
          std::string(cr::toString(settings.arrayCopyCount)).c_str(),
          std::string(cr::toString(settings.arraySpacing)).c_str(),
          state.lastReceipt.message.c_str());
  return state.lastReceipt;
}

cr::CreativeRadialArrayReceipt applyCreativeEditorRadialArrayWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const cr::CreativeToolSettings& settings,
    cr::CreativeVec3 pivot,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  state.lastRadialReceipt = appState.facade.createRadialArrayFromSelection(
      creativeEditorRadialArrayRequest(settings, pivot));
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastRadialReceipt.accepted && state.lastRadialReceipt.changed,
      state.lastRadialReceipt.message));

  SDL_Log("iggy3d_creative: RADIAL_ARRAY status='%s' accepted=%d changed=%d "
          "sourceObjects=%llu generatedObjects=%llu axis='%s' instances='%s' "
          "sweep='%s' pivot=(%.3f,%.3f,%.3f) reasonCode='%s'",
          std::string(cr::toString(state.lastRadialReceipt.status)).c_str(),
          state.lastRadialReceipt.accepted ? 1 : 0,
          state.lastRadialReceipt.changed ? 1 : 0,
          static_cast<unsigned long long>(
              state.lastRadialReceipt.sourceObjectCount),
          static_cast<unsigned long long>(
              state.lastRadialReceipt.generatedObjectCount),
          std::string(cr::toString(settings.radialArrayAxis)).c_str(),
          std::string(cr::toString(settings.radialArrayInstanceCount)).c_str(),
          std::string(cr::toString(settings.radialArraySweep)).c_str(), pivot.x,
          pivot.y, pivot.z, state.lastRadialReceipt.message.c_str());
  return state.lastRadialReceipt;
}

bool applyCreativeEditorArrayWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const cr::CreativeToolSettings& settings,
    double cellSize,
    bool pivotValid,
    cr::CreativeVec3 pivot,
    std::string_view source) {
  switch (settings.arrayMode) {
    case cr::CreativeArrayMode::Linear:
      return applyCreativeEditorLinearArrayWithHistory(
                 appState, state, settings, cellSize, source)
          .accepted;
    case cr::CreativeArrayMode::Radial:
      if (!pivotValid || !cr::isFiniteCreativeVec3(pivot)) {
        state.lastRadialReceipt = {};
        state.lastRadialReceipt.requested = true;
        state.lastRadialReceipt.status =
            cr::CreativeRadialArrayStatus::InvalidRequest;
        state.lastRadialReceipt.message =
            "creative_radial_array_pivot_unavailable";
        return false;
      }
      return applyCreativeEditorRadialArrayWithHistory(
                 appState, state, settings, pivot, source)
          .accepted;
    case cr::CreativeArrayMode::Count:
      return false;
  }
  return false;
}

std::size_t appendCreativeEditorLinearArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::LinearArray) {
    return 0U;
  }

  const std::vector<cr::CreativeObjectId> selected =
      selectedHierarchyObjectIds(appState);
  const std::uint64_t sourceObjectCount = selected.size();

  const cr::CreativeLinearArrayRequest request =
      creativeEditorLinearArrayRequest(editor.toolSettings,
                                       editor.placeCellSize);
  cr::CreativeLinearArrayPlanRequest planRequest;
  planRequest.sourceObjectCount = sourceObjectCount;
  planRequest.direction = request.direction;
  planRequest.copyCount = request.copyCount;
  planRequest.spacing = request.spacing;
  planRequest.cellSize = request.cellSize;
  planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
  const cr::CreativeLinearArrayPlanReceipt plan =
      cr::planCreativeLinearArray(planRequest);
  if (!plan.accepted) {
    return 0U;
  }

  const std::size_t before = wireLines.size();
  const iggy3d::RenderLineColor color{0.22F, 0.88F, 1.0F, 0.90F};
  for (const cr::CreativeLinearArrayInstance& instance :
       plan.plannedInstances()) {
    const cr::CreativeCoreVec3Conversion offset =
        cr::creativeVec3ToCoreChecked(instance.offset);
    if (!offset.converted) {
      continue;
    }
    for (cr::CreativeObjectId objectId : selected) {
      const cr::CreativeObject* object =
          appState.facade.findObject(objectId);
      if (object == nullptr) {
        continue;
      }
      const VisualBounds bounds = visualBoundsForObject(*object);
      appendStandaloneWireframeBoxEdges(
          wireLines, bounds.min + offset.value, bounds.max + offset.value,
          color,
          std::max(0.025F, wireThickness * 0.8F));
    }
  }
  return wireLines.size() - before;
}

std::size_t appendCreativeEditorRadialArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::LinearArray ||
      !editor.interaction.target.grid.valid) {
    return 0U;
  }
  const cr::CreativeVec3 pivot =
      editor.interaction.target.grid.placementAnchor;
  const cr::CreativeCoreVec3Conversion corePivot =
      cr::creativeVec3ToCoreChecked(pivot);
  if (!corePivot.converted) {
    return 0U;
  }

  const std::vector<cr::CreativeObjectId> selected =
      selectedHierarchyObjectIds(appState);
  const std::uint64_t sourceObjectCount = selected.size();
  cr::CreativeObjectWorldExtent selectionExtent;
  for (cr::CreativeObjectId objectId : selected) {
    const cr::CreativeObject* object = appState.facade.findObject(objectId);
    if (object == nullptr) {
      continue;
    }
    const cr::CreativeObjectWorldExtent objectExtent =
        cr::resolveCreativeObjectWorldExtent(*object);
    if (!objectExtent.valid) {
      return 0U;
    }
    if (!selectionExtent.valid) {
      selectionExtent = objectExtent;
    } else {
      selectionExtent.min.x =
          std::min(selectionExtent.min.x, objectExtent.min.x);
      selectionExtent.min.y =
          std::min(selectionExtent.min.y, objectExtent.min.y);
      selectionExtent.min.z =
          std::min(selectionExtent.min.z, objectExtent.min.z);
      selectionExtent.max.x =
          std::max(selectionExtent.max.x, objectExtent.max.x);
      selectionExtent.max.y =
          std::max(selectionExtent.max.y, objectExtent.max.y);
      selectionExtent.max.z =
          std::max(selectionExtent.max.z, objectExtent.max.z);
    }
  }

  const cr::CreativeRadialArrayRequest request =
      creativeEditorRadialArrayRequest(editor.toolSettings, pivot);
  cr::CreativeRadialArrayPlanRequest planRequest;
  planRequest.sourceObjectCount = sourceObjectCount;
  planRequest.pivot = request.pivot;
  planRequest.axis = request.axis;
  planRequest.instanceCount = request.instanceCount;
  planRequest.sweep = request.sweep;
  planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
  const cr::CreativeRadialArrayPlanReceipt plan =
      cr::planCreativeRadialArray(planRequest);
  if (!plan.accepted || !selectionExtent.valid) {
    return 0U;
  }

  const cr::CreativeVec3 selectionAnchor{
      std::midpoint(selectionExtent.min.x, selectionExtent.max.x),
      selectionExtent.min.y,
      std::midpoint(selectionExtent.min.z, selectionExtent.max.z)};
  const bool degenerate =
      cr::creativeSquaredDistanceFromAxis(selectionAnchor, pivot,
                                          request.axis) <= 1.0e-12;
  const iggy3d::RenderLineColor copyColor =
      degenerate ? iggy3d::RenderLineColor{1.0F, 0.18F, 0.14F, 0.95F}
                 : iggy3d::RenderLineColor{0.22F, 0.88F, 1.0F, 0.90F};

  const std::size_t before = wireLines.size();
  for (const cr::CreativeRadialArrayInstance& instance :
       plan.plannedInstances()) {
    for (cr::CreativeObjectId objectId : selected) {
      const cr::CreativeObject* object = appState.facade.findObject(objectId);
      if (object == nullptr) {
        continue;
      }
      const VisualBounds bounds = rotateObjectVisualBounds(
          *object, corePivot.value, request.axis, instance.angleRadians);
      appendStandaloneWireframeBoxEdges(
          wireLines, bounds.min, bounds.max, copyColor,
          std::max(0.025F, wireThickness * 0.8F));
    }
  }

  const float pivotHalfExtent = std::max(0.06F, wireThickness * 1.5F);
  appendStandaloneWireframeBoxEdges(
      wireLines, corePivot.value - iggy3d::Vec3{pivotHalfExtent,
                                                pivotHalfExtent,
                                                pivotHalfExtent},
      corePivot.value + iggy3d::Vec3{pivotHalfExtent, pivotHalfExtent,
                                     pivotHalfExtent},
      {0.96F, 0.74F, 0.18F, 1.0F}, std::max(0.03F, wireThickness));
  return wireLines.size() - before;
}

std::size_t appendCreativeEditorArrayPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  switch (editor.toolSettings.arrayMode) {
    case cr::CreativeArrayMode::Linear:
      return appendCreativeEditorLinearArrayPreview(
          appState, editor, wireThickness, wireLines);
    case cr::CreativeArrayMode::Radial:
      return appendCreativeEditorRadialArrayPreview(
          appState, editor, wireThickness, wireLines);
    case cr::CreativeArrayMode::Count:
      return 0U;
  }
  return 0U;
}

}  // namespace iggy3d_creative_app
