#include "EditorOverlayAssemblersInternal.hpp"

#include <algorithm>
#include <cmath>

#include "EditorPreviewProxies.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "projection/debug/DebugProjection.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
using namespace iggy3d;
namespace {

[[nodiscard]] bool objectSelected(
    const CreativeEditorOverlaySelectionSnapshot& selection,
    cr::CreativeObjectId objectId) noexcept {
  return std::find(selection.selectedObjectIds.begin(),
                   selection.selectedObjectIds.end(),
                   objectId) != selection.selectedObjectIds.end();
}

[[nodiscard]] bool generatedScopeContains(
    const CreativeEditorOverlayDocumentAssemblyRequest& request,
    const CreativeEditorOverlayDocumentPlan& plan,
    cr::CreativeObjectId objectId) noexcept {
  return plan.generatedScopeSummary.valid &&
         creativeDesktopGeneratedSourceScopeCacheContains(
             request.generatedSourceScopeCache, objectId);
}

}  // namespace

CreativeEditorOverlayDocumentPlan prepareCreativeEditorOverlayDocument(
    const CreativeEditorOverlayDocumentAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorOverlayDocumentPlan plan;
  plan.hasSelection = request.selection.hasSelection && !request.volumeActive;

  CreativeDesktopGeneratedSourceScopeModel generatedScopes;
  cr::CreativeWorldLayoutTable generatedScopeTable =
      cr::CreativeWorldLayoutTable::None;
  std::size_t generatedScopeIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t generatedActiveScopeIndex = 0U;
  if (plan.hasSelection && request.selection.selected != nullptr &&
      request.worldLayout.generatedRevision == request.worldLayout.revision) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(
            request.worldLayout.source, *request.selection.selected);
    generatedScopes = buildCreativeDesktopGeneratedSourceScopeModel(
        request.worldLayout.source, provenance);
    if (generatedScopes.count > 0U) {
      generatedActiveScopeIndex =
          resolveCreativeDesktopGeneratedSourceActiveScope(
              generatedScopes,
              creativeEditorWorldLayoutSelectionTable(
                  request.worldLayout.selection.kind),
              request.worldLayout.selection.index);
      const CreativeDesktopGeneratedSourceScopeEntry& scope =
          generatedScopes.entries[generatedActiveScopeIndex];
      generatedScopeTable = scope.table;
      generatedScopeIndex = scope.index;
      static_cast<void>(refreshCreativeDesktopGeneratedSourceScopeCache(
          request.generatedSourceScopeCache, request.document,
          request.worldLayout.source, request.worldLayout.sourceEpoch,
          request.worldLayout.revision,
          request.worldLayout.generatedRevision, generatedScopeTable,
          generatedScopeIndex));
      plan.generatedScopeSummary =
          request.generatedSourceScopeCache.summary;
      if (plan.generatedScopeSummary.valid) {
        const CreativeDesktopGeneratedSourceScopeTint tint =
            creativeDesktopGeneratedSourceScopeTint(generatedScopeTable);
        plan.generatedScopeColor = {tint.r, tint.g, tint.b, tint.a};
        output.generatedScopeActive = true;
        output.generatedScopeObjectCount =
            plan.generatedScopeSummary.objectCount;
        output.generatedScopeVisibleObjectCount =
            plan.generatedScopeSummary.visibleObjectCount;
        plan.generatedScopeBroad =
            generatedActiveScopeIndex != generatedScopes.directEntryIndex ||
            generatedScopeTable == cr::CreativeWorldLayoutTable::Building ||
            generatedScopeTable == cr::CreativeWorldLayoutTable::Level ||
            generatedScopeTable == cr::CreativeWorldLayoutTable::Room;
      }
    }
  }

  const cr::CreativeDocumentWireframeSegmentBuildResult segments =
      cr::buildCreativeDocumentWireframeSegments(
          request.document, request.wireProjectionRequest);
  ProductCreativeWireframeDebugLineBuildResult lines =
      buildProductCreativeWireframeDebugLines(segments.segmentList);
  for (ProductCreativeWireframeDebugLine& line : lines.lineList.lines) {
    const bool selected =
        plan.hasSelection && objectSelected(request.selection, line.objectId);
    const bool inGeneratedScope =
        generatedScopeContains(request, plan, line.objectId);
    line.thickness =
        inGeneratedScope ? 0.055F : selected ? 0.06F : 0.03F;
    if (inGeneratedScope) {
      line.color = {
          plan.generatedScopeColor.r, plan.generatedScopeColor.g,
          plan.generatedScopeColor.b, plan.generatedScopeColor.a};
    } else if (selected) {
      line.color = {1.0F, 1.0F, 0.0F, 1.0F};
    }
  }
  plan.wireframe = buildCreativeWireframeDebugRenderFrame(&lines.lineList);
  return plan;
}

void appendCreativeEditorOverlayDocumentBase(
    const CreativeEditorOverlayDocumentAssemblyRequest& request,
    const CreativeEditorOverlayDocumentPlan& plan,
    CreativeEditorOverlayFrame& output) {
  std::vector<RenderCreativeWireframeDebugLine>& combinedWireLines =
      output.combinedWireLines;
  for (const RenderCreativeWireframeDebugLine& line :
       plan.wireframe.lines) {
    const cr::CreativeObject* object =
        line.objectId != cr::kInvalidObjectId
            ? request.document.findObject(line.objectId)
            : nullptr;
    if (object != nullptr &&
        cr::describeObject(object->kind).shapeKind ==
            cr::CreativeObjectShapeKind::Line) {
      continue;
    }
    combinedWireLines.push_back(line);
    if (generatedScopeContains(request, plan, line.objectId)) {
      ++output.generatedScopeEdgeCount;
    }
  }
  output.documentWireLineCount = combinedWireLines.size();
  if (plan.generatedScopeSummary.valid &&
      plan.generatedScopeSummary.hasBounds) {
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(
            plan.generatedScopeSummary.worldBounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(
            plan.generatedScopeSummary.worldBounds.max);
    if (minimum.converted && maximum.converted) {
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines, minimum.value, maximum.value,
          plan.generatedScopeColor, 0.07F);
      output.generatedScopeEdgeCount +=
          combinedWireLines.size() - before;
    }
  }
}

void appendCreativeEditorOverlayDocumentInteraction(
    const CreativeEditorOverlayDocumentAssemblyRequest& request,
    const CreativeEditorOverlayDocumentPlan& plan,
    CreativeEditorOverlayFrame& output) {
  std::vector<RenderCreativeWireframeDebugLine>& combinedWireLines =
      output.combinedWireLines;
  const cr::CreativeObjectId focusedGroupId =
      activeCreativeEditorGroupFocusId(request.groupFocus);
  if (focusedGroupId != cr::kInvalidObjectId) {
    bool haveFocusedBounds = false;
    VisualBounds focusedBounds{};
    for (const cr::CreativeObject& object : request.document.objects()) {
      if (!cr::creativeObjectEffectivelyVisible(request.document, object.id) ||
          object.kind == cr::CreativeObjectKind::Group ||
          !creativeEditorObjectInsideActiveGroup(
              request.document, request.groupFocus, object.id)) {
        continue;
      }
      const VisualBounds objectBounds = visualBoundsForObject(object);
      if (!haveFocusedBounds) {
        focusedBounds = objectBounds;
        haveFocusedBounds = true;
        continue;
      }
      focusedBounds.min.x =
          std::min(focusedBounds.min.x, objectBounds.min.x);
      focusedBounds.min.y =
          std::min(focusedBounds.min.y, objectBounds.min.y);
      focusedBounds.min.z =
          std::min(focusedBounds.min.z, objectBounds.min.z);
      focusedBounds.max.x =
          std::max(focusedBounds.max.x, objectBounds.max.x);
      focusedBounds.max.y =
          std::max(focusedBounds.max.y, objectBounds.max.y);
      focusedBounds.max.z =
          std::max(focusedBounds.max.z, objectBounds.max.z);
    }
    if (haveFocusedBounds) {
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines, focusedBounds.min, focusedBounds.max,
          RenderLineColor{0.18F, 0.90F, 1.0F, 1.0F}, 0.045F);
      for (std::size_t index = before;
           index < combinedWireLines.size(); ++index) {
        combinedWireLines[index].objectId = focusedGroupId;
      }
    }
  }

  for (const cr::CreativeObject& object : request.document.objects()) {
    const cr::CreativeObjectDescriptor& descriptor =
        cr::describeObject(object.kind);
    if (!cr::creativeObjectEffectivelyVisible(request.document, object.id)) {
      continue;
    }
    const bool selected =
        plan.hasSelection && objectSelected(request.selection, object.id);
    const bool inGeneratedScope =
        generatedScopeContains(request, plan, object.id);
    if (descriptor.shapeKind != cr::CreativeObjectShapeKind::Point &&
        descriptor.shapeKind != cr::CreativeObjectShapeKind::Line) {
      continue;
    }
    const VisualBounds markerBounds = visualBoundsForObject(object);
    const std::size_t before = combinedWireLines.size();
    appendStandaloneWireframeBoxEdges(
        combinedWireLines, markerBounds.min, markerBounds.max,
        inGeneratedScope
            ? plan.generatedScopeColor
            : selected ? RenderLineColor{1.0F, 1.0F, 0.0F, 1.0F}
            : descriptor.shapeKind == cr::CreativeObjectShapeKind::Line
                  ? RenderLineColor{0.86F, 0.68F, 0.28F, 1.0F}
                  : RenderLineColor{0.34F, 0.62F, 0.88F, 1.0F},
        inGeneratedScope ? 0.055F : selected ? 0.06F : 0.035F);
    for (std::size_t index = before;
         index < combinedWireLines.size(); ++index) {
      combinedWireLines[index].objectId = object.id;
    }
    if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Line) {
      output.lineMarkerEdgeCount += combinedWireLines.size() - before;
    } else {
      output.pointMarkerEdgeCount += combinedWireLines.size() - before;
    }
    if (inGeneratedScope) {
      output.generatedScopeEdgeCount +=
          combinedWireLines.size() - before;
    }
  }

  const cr::CreativeObject* selected = request.selection.selected;
  if (request.gizmoFrame.selectedIsPathForHandles) {
    const auto appendSegmentSpeedMarker =
        [&](Vec3 from, Vec3 to, double speedMultiplier) {
          if (std::fabs(speedMultiplier - 1.0) <= 1.0e-9) {
            return;
          }
          RenderCreativeWireframeDebugLine speedMarker;
          speedMarker.start = (from + to) * 0.5F;
          speedMarker.end =
              speedMarker.start +
              Vec3{0.0F,
                   0.20F +
                       0.10F * static_cast<float>(
                                   std::fabs(speedMultiplier - 1.0)),
                   0.0F};
          speedMarker.color = {0.92F, 0.32F, 1.0F, 1.0F};
          speedMarker.objectId =
              static_cast<cr::CreativeObjectId>(
                  request.selection.selectedId);
          speedMarker.thickness = 0.06F;
          combinedWireLines.push_back(speedMarker);
          ++output.pathPointHandleEdgeCount;
        };
    for (std::size_t index = 1U;
         index < selected->pathPoints.size(); ++index) {
      const cr::CreativeCoreVec3Conversion from =
          cr::creativeVec3ToCoreChecked(
              selected->pathPoints[index - 1U].position);
      const cr::CreativeCoreVec3Conversion to =
          cr::creativeVec3ToCoreChecked(
              selected->pathPoints[index].position);
      if (!from.converted || !to.converted) {
        continue;
      }
      RenderCreativeWireframeDebugLine routeLine;
      routeLine.start = from.value;
      routeLine.end = to.value;
      routeLine.color = {0.20F, 0.88F, 1.0F, 1.0F};
      routeLine.objectId =
          static_cast<cr::CreativeObjectId>(
              request.selection.selectedId);
      routeLine.thickness = 0.045F;
      combinedWireLines.push_back(routeLine);
      ++output.pathPointHandleEdgeCount;
      if (selected->kind == cr::CreativeObjectKind::MovingPlatform) {
        appendSegmentSpeedMarker(
            from.value, to.value,
            selected->pathPoints[index - 1U].outgoingSpeedMultiplier);
      }
    }
    if (selected->kind == cr::CreativeObjectKind::MovingPlatform &&
        selected->movingPlatform.traversalMode ==
            cr::CreativeMovingPlatformTraversalMode::Loop &&
        selected->pathPoints.size() > 1U) {
      const cr::CreativeCoreVec3Conversion from =
          cr::creativeVec3ToCoreChecked(
              selected->pathPoints.back().position);
      const cr::CreativeCoreVec3Conversion to =
          cr::creativeVec3ToCoreChecked(
              selected->pathPoints.front().position);
      if (from.converted && to.converted &&
          lengthSquared(to.value - from.value) > 1.0e-10F) {
        RenderCreativeWireframeDebugLine closureLine;
        closureLine.start = from.value;
        closureLine.end = to.value;
        closureLine.color = {0.20F, 0.88F, 1.0F, 1.0F};
        closureLine.objectId =
            static_cast<cr::CreativeObjectId>(
                request.selection.selectedId);
        closureLine.thickness = 0.045F;
        combinedWireLines.push_back(closureLine);
        ++output.pathPointHandleEdgeCount;
        appendSegmentSpeedMarker(
            from.value, to.value,
            selected->pathPoints.back().outgoingSpeedMultiplier);
      }
    }
    for (std::size_t index = 0U;
         index < selected->pathPoints.size(); ++index) {
      const cr::CreativePathPoint& point = selected->pathPoints[index];
      const bool pointSelected =
          request.movingPlatformPathEdit.pointSelected &&
          request.movingPlatformPathEdit.objectId == selected->id &&
          request.movingPlatformPathEdit.selectedPointIndex == index;
      const VisualBounds handleBounds =
          pathPointHandleBounds(point.position);
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines, handleBounds.min, handleBounds.max,
          pointSelected ? RenderLineColor{1.0F, 0.92F, 0.20F, 1.0F}
                        : RenderLineColor{0.20F, 0.88F, 1.0F, 1.0F},
          pointSelected ? 0.06F : 0.035F);
      for (std::size_t lineIndex = before;
           lineIndex < combinedWireLines.size(); ++lineIndex) {
        combinedWireLines[lineIndex].objectId =
            static_cast<cr::CreativeObjectId>(
                request.selection.selectedId);
      }
      output.pathPointHandleEdgeCount +=
          combinedWireLines.size() - before;
      if (point.dwellSeconds > 0.0) {
        const cr::CreativeCoreVec3Conversion markerBase =
            cr::creativeVec3ToCoreChecked(point.position);
        if (markerBase.converted) {
          RenderCreativeWireframeDebugLine dwellMarker;
          dwellMarker.start = markerBase.value;
          dwellMarker.end =
              markerBase.value + Vec3{0.0F, 0.4F, 0.0F};
          dwellMarker.color = {1.0F, 0.62F, 0.12F, 1.0F};
          dwellMarker.objectId =
              static_cast<cr::CreativeObjectId>(
                  request.selection.selectedId);
          dwellMarker.thickness = 0.06F;
          combinedWireLines.push_back(dwellMarker);
          ++output.pathPointHandleEdgeCount;
        }
      }
    }
  }

  if (plan.hasSelection && !plan.generatedScopeBroad) {
    for (const GizmoAxisShaft& shaft : request.gizmoFrame.shafts) {
      RenderCreativeWireframeDebugLine gizmoLine;
      gizmoLine.start = request.gizmoFrame.center;
      gizmoLine.end = shaft.tip;
      gizmoLine.color = shaft.color;
      gizmoLine.objectId =
          static_cast<cr::CreativeObjectId>(
              request.selection.selectedId);
      gizmoLine.thickness = request.gizmoThickness;
      combinedWireLines.push_back(gizmoLine);
    }
  }
}

}  // namespace iggy3d_creative_app
