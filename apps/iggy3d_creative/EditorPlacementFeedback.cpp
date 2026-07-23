#include "EditorPlacementFeedback.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"
#include "core/math/OrientedBox.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::array<std::array<std::size_t, 2U>, 12U> kBoxEdgeIndices{{
    {0U, 1U}, {2U, 3U}, {4U, 5U}, {6U, 7U},
    {0U, 2U}, {1U, 3U}, {4U, 6U}, {5U, 7U},
    {0U, 4U}, {1U, 5U}, {2U, 6U}, {3U, 7U},
}};

void assignFeedbackText(
    CreativeEditorPlacementFeedbackText& output,
    std::string_view prefix,
    std::string_view detail = {}) noexcept {
  output = {};
  const auto append = [&output](std::string_view text) {
    const std::size_t available = output.bytes.size() - output.length;
    const std::size_t count = std::min(available, text.size());
    if (count > 0U) {
      std::copy_n(text.data(), count,
                  output.bytes.data() + output.length);
    }
    output.length = static_cast<std::uint8_t>(output.length + count);
  };
  append(prefix);
  append(detail);
}

[[nodiscard]] CreativeEditorPlacementRejectionReason rejectionReasonFor(
    CreativeBrushPlacementAdmissionStatus status) noexcept {
  switch (status) {
    case CreativeBrushPlacementAdmissionStatus::InvalidTarget:
      return CreativeEditorPlacementRejectionReason::InvalidTarget;
    case CreativeBrushPlacementAdmissionStatus::FloorRequired:
      return CreativeEditorPlacementRejectionReason::FloorRequired;
    case CreativeBrushPlacementAdmissionStatus::WallRequired:
      return CreativeEditorPlacementRejectionReason::WallRequired;
    case CreativeBrushPlacementAdmissionStatus::SurfaceRequired:
      return CreativeEditorPlacementRejectionReason::SurfaceRequired;
    case CreativeBrushPlacementAdmissionStatus::UnsupportedBrush:
      return CreativeEditorPlacementRejectionReason::UnsupportedBrush;
    case CreativeBrushPlacementAdmissionStatus::InvalidGeometry:
      return CreativeEditorPlacementRejectionReason::InvalidGeometry;
    case CreativeBrushPlacementAdmissionStatus::UnsupportedPolicy:
      return CreativeEditorPlacementRejectionReason::UnsupportedPolicy;
    case CreativeBrushPlacementAdmissionStatus::FaceDisallowed:
      return CreativeEditorPlacementRejectionReason::FaceDisallowed;
    case CreativeBrushPlacementAdmissionStatus::TargetIncompatible:
      return CreativeEditorPlacementRejectionReason::TargetIncompatible;
    case CreativeBrushPlacementAdmissionStatus::AttachmentUnavailable:
      return CreativeEditorPlacementRejectionReason::AttachmentUnavailable;
    case CreativeBrushPlacementAdmissionStatus::AttachmentIncompatible:
      return CreativeEditorPlacementRejectionReason::AttachmentIncompatible;
    case CreativeBrushPlacementAdmissionStatus::AttachmentOccupied:
      return CreativeEditorPlacementRejectionReason::AttachmentOccupied;
    case CreativeBrushPlacementAdmissionStatus::ClearanceBlocked:
      return CreativeEditorPlacementRejectionReason::ClearanceBlocked;
    case CreativeBrushPlacementAdmissionStatus::Ready:
      return CreativeEditorPlacementRejectionReason::ActionRejected;
  }
  return CreativeEditorPlacementRejectionReason::ActionRejected;
}

[[nodiscard]] CreativeEditorPlacementRejectionReason rejectionReasonFor(
    CreativeBrushPlacementMutationStatus status) noexcept {
  switch (status) {
    case CreativeBrushPlacementMutationStatus::InvalidPlan:
      return CreativeEditorPlacementRejectionReason::InvalidPlan;
    case CreativeBrushPlacementMutationStatus::Occupied:
      return CreativeEditorPlacementRejectionReason::Occupied;
    case CreativeBrushPlacementMutationStatus::ClearanceRejected:
      return CreativeEditorPlacementRejectionReason::ClearanceBlocked;
    case CreativeBrushPlacementMutationStatus::ObjectRejected:
      return CreativeEditorPlacementRejectionReason::ObjectRejected;
    case CreativeBrushPlacementMutationStatus::VoxelRejected:
      return CreativeEditorPlacementRejectionReason::VoxelRejected;
    case CreativeBrushPlacementMutationStatus::NotRequested:
    case CreativeBrushPlacementMutationStatus::Applied:
      return CreativeEditorPlacementRejectionReason::ActionRejected;
  }
  return CreativeEditorPlacementRejectionReason::ActionRejected;
}

void assignRejectionReasonText(
    CreativeEditorPlacementFeedbackText& output,
    CreativeEditorPlacementRejectionReason reason) noexcept {
  switch (reason) {
    case CreativeEditorPlacementRejectionReason::None:
    case CreativeEditorPlacementRejectionReason::ActionRejected:
      assignFeedbackText(output, "Action rejected");
      return;
    case CreativeEditorPlacementRejectionReason::InvalidTarget:
      assignFeedbackText(output, "No placement target");
      return;
    case CreativeEditorPlacementRejectionReason::FloorRequired:
      assignFeedbackText(output, "Aim at a floor");
      return;
    case CreativeEditorPlacementRejectionReason::WallRequired:
      assignFeedbackText(output, "Aim at a wall");
      return;
    case CreativeEditorPlacementRejectionReason::SurfaceRequired:
      assignFeedbackText(output, "Aim at a surface");
      return;
    case CreativeEditorPlacementRejectionReason::UnsupportedBrush:
      assignFeedbackText(output, "Unsupported shape");
      return;
    case CreativeEditorPlacementRejectionReason::InvalidGeometry:
      assignFeedbackText(output, "Invalid geometry");
      return;
    case CreativeEditorPlacementRejectionReason::UnsupportedPolicy:
      assignFeedbackText(output, "Placement mode unsupported");
      return;
    case CreativeEditorPlacementRejectionReason::FaceDisallowed:
      assignFeedbackText(output, "Face not supported");
      return;
    case CreativeEditorPlacementRejectionReason::TargetIncompatible:
      assignFeedbackText(output, "Incompatible target");
      return;
    case CreativeEditorPlacementRejectionReason::AttachmentUnavailable:
      assignFeedbackText(output, "Aim closer to a socket");
      return;
    case CreativeEditorPlacementRejectionReason::AttachmentIncompatible:
      assignFeedbackText(output, "Incompatible socket");
      return;
    case CreativeEditorPlacementRejectionReason::AttachmentOccupied:
      assignFeedbackText(output, "Socket occupied");
      return;
    case CreativeEditorPlacementRejectionReason::ClearanceBlocked:
      assignFeedbackText(output, "Placement blocked");
      return;
    case CreativeEditorPlacementRejectionReason::Occupied:
      assignFeedbackText(output, "Target occupied");
      return;
    case CreativeEditorPlacementRejectionReason::InvalidPlan:
      assignFeedbackText(output, "Invalid placement plan");
      return;
    case CreativeEditorPlacementRejectionReason::ObjectRejected:
      assignFeedbackText(output, "Object could not be placed");
      return;
    case CreativeEditorPlacementRejectionReason::VoxelRejected:
      assignFeedbackText(output, "Cell could not be placed");
      return;
    case CreativeEditorPlacementRejectionReason::SemanticSourceOwned:
      assignFeedbackText(output, "Edit generated source");
      return;
    case CreativeEditorPlacementRejectionReason::ExternalReference:
      assignFeedbackText(output, "Referenced elsewhere");
      return;
    case CreativeEditorPlacementRejectionReason::CapacityReached:
      assignFeedbackText(output, "Stroke limit reached");
      return;
    case CreativeEditorPlacementRejectionReason::HistoryUnavailable:
      assignFeedbackText(output, "Document not editable");
      return;
  }
  assignFeedbackText(output, "Action rejected");
}

std::size_t appendBoxCornerEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const std::array<iggy3d::Vec3, 8U>& corners,
    iggy3d::RenderLineColor color,
    float thickness,
    cr::CreativeObjectId objectId,
    std::uint32_t segmentKind) {
  for (const iggy3d::Vec3 corner : corners) {
    if (!iggy3d::isFinite(corner)) {
      return 0U;
    }
  }
  const std::size_t begin = lines.size();
  for (const auto& edge : kBoxEdgeIndices) {
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = corners[edge[0]];
    line.end = corners[edge[1]];
    line.color = color;
    line.objectId = objectId;
    line.segmentKind = segmentKind;
    line.thickness = thickness;
    lines.push_back(line);
  }
  return lines.size() - begin;
}

std::size_t appendCreativeBoxCornerEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const std::array<cr::CreativeVec3, 8U>& corners,
    iggy3d::RenderLineColor color,
    float thickness,
    cr::CreativeObjectId objectId,
    std::uint32_t segmentKind) {
  std::array<iggy3d::Vec3, 8U> converted{};
  for (std::size_t index = 0U; index < corners.size(); ++index) {
    const cr::CreativeCoreVec3Conversion corner =
        cr::creativeVec3ToCoreChecked(corners[index]);
    if (!corner.converted) {
      return 0U;
    }
    converted[index] = corner.value;
  }
  return appendBoxCornerEdges(lines, converted, color, thickness, objectId,
                              segmentKind);
}

std::size_t appendPlacementTargetBoundary(
    const CreativeEditorPlacementVisualizationReceipt& visualization,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  if (visualization.attemptedCornerCount !=
      visualization.attemptedCorners.size()) {
    return 0U;
  }
  return appendCreativeBoxCornerEdges(
      lines, visualization.attemptedCorners,
      {1.0F, 0.12F, 0.10F, 1.0F}, thickness, cr::kInvalidObjectId,
      kCreativeEditorPlacementInvalidTargetSegmentKind);
}

std::size_t appendAabbBlocker(
    cr::CreativeBounds bounds,
    cr::CreativeObjectId objectId,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeCoreVec3Conversion minimum =
      cr::creativeVec3ToCoreChecked(bounds.min);
  const cr::CreativeCoreVec3Conversion maximum =
      cr::creativeVec3ToCoreChecked(bounds.max);
  if (!minimum.converted || !maximum.converted) {
    return 0U;
  }
  const std::size_t begin = lines.size();
  appendStandaloneWireframeBoxEdges(
      lines, minimum.value, maximum.value,
      {1.0F, 0.28F, 0.12F, 1.0F}, thickness);
  for (std::size_t index = begin; index < lines.size(); ++index) {
    lines[index].objectId = objectId;
    lines[index].segmentKind = kCreativeEditorPlacementBlockerSegmentKind;
  }
  return lines.size() - begin;
}

std::size_t appendAuthoredPlacementBlocker(
    const cr::CreativeDocument& document,
    cr::CreativeObjectId objectId,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeObject* object = document.findObject(objectId);
  if (object == nullptr) {
    return 0U;
  }
  if (const std::optional<iggy3d::OrientedBox> box =
          orientedVisualBoxForObject(*object)) {
    return appendBoxCornerEdges(
        lines, iggy3d::orientedBoxCorners(*box),
        {1.0F, 0.28F, 0.12F, 1.0F}, thickness, objectId,
        kCreativeEditorPlacementBlockerSegmentKind);
  }
  const VisualBounds visual = visualBoundsForObject(*object);
  return appendAabbBlocker(
      {{visual.min.x, visual.min.y, visual.min.z},
       {visual.max.x, visual.max.y, visual.max.z}},
      objectId, thickness, lines);
}

std::size_t appendTerrainPlacementBlocker(
    const cr::CreativeDocument& document,
    cr::CreativeTerrainCoord2 cell,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (!cr::isFiniteCreativeVec3(grid.origin) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    return 0U;
  }
  const cr::CreativeVec3 center{
      grid.origin.x +
          (static_cast<double>(cell.x) + 0.5) * grid.cellSizeMeters,
      grid.origin.y,
      grid.origin.z +
          (static_cast<double>(cell.z) + 0.5) * grid.cellSizeMeters};
  const cr::CreativeTerrainSurfacePose pose =
      cr::sampleCreativeTerrainSurfacePose(
          {&document.terrainField(), center, grid.origin,
           grid.cellSizeMeters});
  if (!pose.accepted || !pose.present) {
    return 0U;
  }
  const double halfHeight =
      std::clamp(grid.cellSizeMeters * 0.04, 0.025, 0.10);
  return appendAabbBlocker(
      {{center.x - grid.cellSizeMeters * 0.5,
        pose.position.y - halfHeight,
        center.z - grid.cellSizeMeters * 0.5},
       {center.x + grid.cellSizeMeters * 0.5,
        pose.position.y + halfHeight,
        center.z + grid.cellSizeMeters * 0.5}},
      cr::kInvalidObjectId, thickness, lines);
}

std::size_t appendPlacementBlocker(
    const cr::CreativeDocument& document,
    const cr::CreativePlacementClearanceResult& clearance,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  switch (clearance.status) {
    case cr::CreativePlacementClearanceStatus::OutsideWorldBounds:
      return appendAabbBlocker(document.worldBounds(),
                               cr::kInvalidObjectId, thickness, lines);
    case cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked:
      return appendAuthoredPlacementBlocker(
          document, clearance.blockingObjectId, thickness, lines);
    case cr::CreativePlacementClearanceStatus::VoxelBlocked:
      return appendAabbBlocker(
          cr::creativeVolumeCellBounds(
              clearance.blockingVoxelCell,
              document.gridSettings().cellSizeMeters,
              document.gridSettings().origin),
          cr::kInvalidObjectId, thickness, lines);
    case cr::CreativePlacementClearanceStatus::TerrainBlocked:
      return appendTerrainPlacementBlocker(
          document, clearance.blockingTerrainCell, thickness, lines);
    case cr::CreativePlacementClearanceStatus::NotEvaluated:
    case cr::CreativePlacementClearanceStatus::InvalidRequest:
    case cr::CreativePlacementClearanceStatus::TraversalLimitExceeded:
    case cr::CreativePlacementClearanceStatus::Ready:
      return 0U;
  }
  return 0U;
}

}  // namespace

void clearCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction) noexcept {
  interaction.placementFeedback = {};
}

void setCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    CreativeEditorPlacementFeedbackStatus status,
    std::uint64_t frameIndex,
    cr::CreativeObjectKind objectKind,
    cr::CreativeObjectId objectId) noexcept {
  interaction.placementFeedback = {};
  interaction.placementFeedback.status = status;
  interaction.placementFeedback.objectId = objectId;
  interaction.placementFeedback.objectKind = objectKind;
  interaction.placementFeedback.frameIndex = frameIndex;
  if (status == CreativeEditorPlacementFeedbackStatus::Rejected) {
    interaction.placementFeedback.rejectionReason =
        CreativeEditorPlacementRejectionReason::ActionRejected;
  }
}

void setCreativeEditorPlacementRejectionFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    cr::CreativeObjectKind objectKind,
    const cr::CreativePlacementClearanceResult& clearance,
    CreativeEditorPlacementRejectionReason reason) noexcept {
  setCreativeEditorPlacementFeedback(
      interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
      frameIndex, objectKind);
  interaction.placementFeedback.clearance = clearance;
  interaction.placementFeedback.rejectionReason = reason;
}

void setCreativeEditorVoxelPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    cr::CreativeObjectKind objectKind,
    cr::CreativeGridCoord3 voxelCell,
    cr::CreativeBounds voxelBounds) noexcept {
  setCreativeEditorPlacementFeedback(
      interaction, CreativeEditorPlacementFeedbackStatus::Placed, frameIndex,
      objectKind);
  interaction.placementFeedback.voxelPlaced = true;
  interaction.placementFeedback.voxelCell = voxelCell;
  interaction.placementFeedback.voxelBounds = voxelBounds;
}

void setCreativeEditorPlacementAdmissionRejectionFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    const CreativeBrushPlacementAdmission& admission) noexcept {
  setCreativeEditorPlacementRejectionFeedback(
      interaction, frameIndex, admission.plan.brush,
      admission.plan.clearance, rejectionReasonFor(admission.status));
}

void setCreativeEditorPlacementMutationFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    const CreativeBrushPlacementMutationReceipt& receipt) noexcept {
  if (receipt.accepted && receipt.changed && receipt.voxelCreated) {
    setCreativeEditorVoxelPlacementFeedback(
        interaction, frameIndex, receipt.objectKind, receipt.voxelCell,
        receipt.worldBounds);
    return;
  }
  if (receipt.accepted && receipt.changed && receipt.objectCreated) {
    setCreativeEditorPlacementFeedback(
        interaction, CreativeEditorPlacementFeedbackStatus::Placed,
        frameIndex, receipt.objectKind, receipt.objectId);
    return;
  }
  setCreativeEditorPlacementRejectionFeedback(
      interaction, frameIndex, receipt.objectKind, receipt.clearance,
      rejectionReasonFor(receipt.status));
}

CreativeEditorPlacementFeedbackViewModel
creativeEditorPlacementFeedbackViewModel(
    const CreativeEditorPlacementFeedback& feedback,
    std::uint64_t frameIndex,
    const cr::CreativeDocument* document) {
  CreativeEditorPlacementFeedbackViewModel model;
  model.status = feedback.status;
  model.visible = creativeEditorPlacementFeedbackVisible(feedback,
                                                          frameIndex);
  if (!model.visible) {
    return model;
  }
  if (feedback.status == CreativeEditorPlacementFeedbackStatus::Placed) {
    model.color = {0.25F, 1.0F, 0.35F, 1.0F};
    return model;
  }
  if (feedback.status != CreativeEditorPlacementFeedbackStatus::Rejected) {
    return model;
  }
  model.color = {1.0F, 0.28F, 0.16F, 1.0F};
  switch (feedback.clearance.status) {
    case cr::CreativePlacementClearanceStatus::OutsideWorldBounds:
      assignFeedbackText(model.label, "Outside build bounds");
      break;
    case cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked: {
      const cr::CreativeObject* blocker = document == nullptr
                                              ? nullptr
                                              : document->findObject(
                                                    feedback.clearance
                                                        .blockingObjectId);
      if (blocker == nullptr) {
        assignFeedbackText(model.label, "Blocked: object");
        break;
      }
      assignFeedbackText(model.label, "Blocked: ",
                         cr::toString(blocker->kind));
      break;
    }
    case cr::CreativePlacementClearanceStatus::VoxelBlocked: {
      const cr::CreativeObjectKind material =
          document == nullptr
              ? cr::CreativeObjectKind::Unknown
              : document->voxelField().materialAt(
                    feedback.clearance.blockingVoxelCell);
      if (material == cr::CreativeObjectKind::Unknown) {
        assignFeedbackText(model.label, "Blocked: voxel");
        break;
      }
      assignFeedbackText(model.label, "Blocked: ", cr::toString(material));
      break;
    }
    case cr::CreativePlacementClearanceStatus::TerrainBlocked:
      assignFeedbackText(model.label, "Blocked: terrain");
      break;
    case cr::CreativePlacementClearanceStatus::TraversalLimitExceeded:
      assignFeedbackText(model.label, "Placement too large");
      break;
    case cr::CreativePlacementClearanceStatus::InvalidRequest:
      assignFeedbackText(model.label, "Invalid placement");
      break;
    case cr::CreativePlacementClearanceStatus::NotEvaluated:
    case cr::CreativePlacementClearanceStatus::Ready:
      assignRejectionReasonText(model.label, feedback.rejectionReason);
      break;
  }
  return model;
}

void appendCreativeEditorPlacementClearanceWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  const cr::CreativeDocument& document = request.appState.facade.document();
  const float thickness =
      std::max(0.07F, request.gizmoThickness * 1.35F);
  if (output.placementVisualization.targetAvailable) {
    const CreativeEditorPlacementVisualizationReceipt& visualization =
        output.placementVisualization;
    if (!visualization.clearance.evaluated ||
        visualization.clearance.allowed) {
      return;
    }
    output.placementInvalidTargetEdgeCount = appendPlacementTargetBoundary(
        visualization, thickness, output.combinedWireLines);
    output.placementBlockerEdgeCount = appendPlacementBlocker(
        document, visualization.clearance, thickness,
        output.combinedWireLines);
    return;
  }

  const CreativeEditorPlacementFeedback& feedback =
      request.editor.interaction.placementFeedback;
  const CreativeEditorPlacementFeedbackViewModel view =
      creativeEditorPlacementFeedbackViewModel(
          feedback, request.editor.frameIndex, &document);
  if (!view.visible ||
      view.status != CreativeEditorPlacementFeedbackStatus::Rejected ||
      !feedback.clearance.evaluated || feedback.clearance.allowed) {
    return;
  }
  output.placementBlockerEdgeCount = appendPlacementBlocker(
      document, feedback.clearance, thickness, output.combinedWireLines);
}

}  // namespace iggy3d_creative_app
