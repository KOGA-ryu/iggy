#include "app/iggy3d/creative/tools/Pattern.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <utility>

namespace iggy3d::creative {
namespace {

template <typename Enum>
[[nodiscard]] bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(count);
}

[[nodiscard]] CreativeVec3 directionUnit(
    CreativeLinearArrayDirection direction) noexcept {
  switch (direction) {
    case CreativeLinearArrayDirection::PositiveX: return {1.0, 0.0, 0.0};
    case CreativeLinearArrayDirection::NegativeX: return {-1.0, 0.0, 0.0};
    case CreativeLinearArrayDirection::PositiveY: return {0.0, 1.0, 0.0};
    case CreativeLinearArrayDirection::NegativeY: return {0.0, -1.0, 0.0};
    case CreativeLinearArrayDirection::PositiveZ: return {0.0, 0.0, 1.0};
    case CreativeLinearArrayDirection::NegativeZ: return {0.0, 0.0, -1.0};
    case CreativeLinearArrayDirection::Count: break;
  }
  return {};
}

[[nodiscard]] CreativeLinearArrayStatus statusForClipboardFailure(
    CreativeClipboardStatus status) noexcept {
  switch (status) {
    case CreativeClipboardStatus::EmptySelection:
      return CreativeLinearArrayStatus::EmptySelection;
    case CreativeClipboardStatus::MissingObject:
      return CreativeLinearArrayStatus::MissingObject;
    case CreativeClipboardStatus::ObjectIdExhausted:
      return CreativeLinearArrayStatus::ObjectIdExhausted;
    case CreativeClipboardStatus::NotRequested:
    case CreativeClipboardStatus::InvalidClipboard:
    case CreativeClipboardStatus::InvalidRequest:
    case CreativeClipboardStatus::CreateRejected:
    case CreativeClipboardStatus::RemoveRejected:
    case CreativeClipboardStatus::Copied:
    case CreativeClipboardStatus::Cut:
    case CreativeClipboardStatus::Pasted:
      return CreativeLinearArrayStatus::PasteRejected;
  }
  return CreativeLinearArrayStatus::PasteRejected;
}

[[nodiscard]] CreativeRadialArrayStatus radialStatusForClipboardFailure(
    CreativeClipboardStatus status) noexcept {
  switch (status) {
    case CreativeClipboardStatus::EmptySelection:
      return CreativeRadialArrayStatus::EmptySelection;
    case CreativeClipboardStatus::MissingObject:
      return CreativeRadialArrayStatus::MissingObject;
    case CreativeClipboardStatus::ObjectIdExhausted:
      return CreativeRadialArrayStatus::ObjectIdExhausted;
    case CreativeClipboardStatus::NotRequested:
    case CreativeClipboardStatus::InvalidClipboard:
    case CreativeClipboardStatus::InvalidRequest:
    case CreativeClipboardStatus::CreateRejected:
    case CreativeClipboardStatus::RemoveRejected:
    case CreativeClipboardStatus::Copied:
    case CreativeClipboardStatus::Cut:
    case CreativeClipboardStatus::Pasted:
      return CreativeRadialArrayStatus::PasteRejected;
  }
  return CreativeRadialArrayStatus::PasteRejected;
}

}  // namespace

std::string_view toString(CreativeLinearArrayDirection direction) noexcept {
  switch (direction) {
    case CreativeLinearArrayDirection::PositiveX: return "+X";
    case CreativeLinearArrayDirection::NegativeX: return "-X";
    case CreativeLinearArrayDirection::PositiveY: return "+Y";
    case CreativeLinearArrayDirection::NegativeY: return "-Y";
    case CreativeLinearArrayDirection::PositiveZ: return "+Z";
    case CreativeLinearArrayDirection::NegativeZ: return "-Z";
    case CreativeLinearArrayDirection::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeLinearArrayCopyCount count) noexcept {
  switch (count) {
    case CreativeLinearArrayCopyCount::One: return "1 NEW";
    case CreativeLinearArrayCopyCount::Two: return "2 NEW";
    case CreativeLinearArrayCopyCount::Four: return "4 NEW";
    case CreativeLinearArrayCopyCount::Eight: return "8 NEW";
    case CreativeLinearArrayCopyCount::Sixteen: return "16 NEW";
    case CreativeLinearArrayCopyCount::ThirtyTwo: return "32 NEW";
    case CreativeLinearArrayCopyCount::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeLinearArraySpacing spacing) noexcept {
  switch (spacing) {
    case CreativeLinearArraySpacing::OneCell: return "1 CELL";
    case CreativeLinearArraySpacing::TwoCells: return "2 CELLS";
    case CreativeLinearArraySpacing::FourCells: return "4 CELLS";
    case CreativeLinearArraySpacing::EightCells: return "8 CELLS";
    case CreativeLinearArraySpacing::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeLinearArrayStatus status) noexcept {
  switch (status) {
    case CreativeLinearArrayStatus::NotRequested: return "NotRequested";
    case CreativeLinearArrayStatus::EmptySelection: return "EmptySelection";
    case CreativeLinearArrayStatus::InvalidRequest: return "InvalidRequest";
    case CreativeLinearArrayStatus::OperationLimitExceeded:
      return "OperationLimitExceeded";
    case CreativeLinearArrayStatus::MissingObject: return "MissingObject";
    case CreativeLinearArrayStatus::ObjectIdExhausted:
      return "ObjectIdExhausted";
    case CreativeLinearArrayStatus::PasteRejected: return "PasteRejected";
    case CreativeLinearArrayStatus::Planned: return "Planned";
    case CreativeLinearArrayStatus::Applied: return "Applied";
  }
  return "Unknown";
}

std::string_view toString(CreativeRadialArrayInstanceCount count) noexcept {
  switch (count) {
    case CreativeRadialArrayInstanceCount::Two: return "2 TOTAL";
    case CreativeRadialArrayInstanceCount::Four: return "4 TOTAL";
    case CreativeRadialArrayInstanceCount::Eight: return "8 TOTAL";
    case CreativeRadialArrayInstanceCount::Sixteen: return "16 TOTAL";
    case CreativeRadialArrayInstanceCount::ThirtyTwo: return "32 TOTAL";
    case CreativeRadialArrayInstanceCount::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRadialArraySweep sweep) noexcept {
  switch (sweep) {
    case CreativeRadialArraySweep::Degrees90: return "90 DEG";
    case CreativeRadialArraySweep::Degrees180: return "180 DEG";
    case CreativeRadialArraySweep::Degrees360: return "360 DEG";
    case CreativeRadialArraySweep::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRadialArrayStatus status) noexcept {
  switch (status) {
    case CreativeRadialArrayStatus::NotRequested: return "NotRequested";
    case CreativeRadialArrayStatus::EmptySelection: return "EmptySelection";
    case CreativeRadialArrayStatus::InvalidRequest: return "InvalidRequest";
    case CreativeRadialArrayStatus::DegenerateRadius:
      return "DegenerateRadius";
    case CreativeRadialArrayStatus::OperationLimitExceeded:
      return "OperationLimitExceeded";
    case CreativeRadialArrayStatus::MissingObject: return "MissingObject";
    case CreativeRadialArrayStatus::ObjectIdExhausted:
      return "ObjectIdExhausted";
    case CreativeRadialArrayStatus::PasteRejected: return "PasteRejected";
    case CreativeRadialArrayStatus::Planned: return "Planned";
    case CreativeRadialArrayStatus::Applied: return "Applied";
  }
  return "Unknown";
}

std::uint32_t creativeLinearArrayCopyCountValue(
    CreativeLinearArrayCopyCount count) noexcept {
  constexpr std::array<std::uint32_t, 6> values{1U, 2U, 4U, 8U, 16U, 32U};
  const std::size_t index = static_cast<std::size_t>(count);
  return index < values.size() ? values[index] : 0U;
}

std::uint32_t creativeLinearArraySpacingCells(
    CreativeLinearArraySpacing spacing) noexcept {
  constexpr std::array<std::uint32_t, 4> values{1U, 2U, 4U, 8U};
  const std::size_t index = static_cast<std::size_t>(spacing);
  return index < values.size() ? values[index] : 0U;
}

std::uint32_t creativeRadialArrayInstanceCountValue(
    CreativeRadialArrayInstanceCount count) noexcept {
  constexpr std::array<std::uint32_t, 5> values{2U, 4U, 8U, 16U, 32U};
  const std::size_t index = static_cast<std::size_t>(count);
  return index < values.size() ? values[index] : 0U;
}

double creativeRadialArraySweepDegrees(
    CreativeRadialArraySweep sweep) noexcept {
  constexpr std::array values{90.0, 180.0, 360.0};
  const std::size_t index = static_cast<std::size_t>(sweep);
  return index < values.size() ? values[index] : 0.0;
}

CreativeLinearArrayPlanReceipt planCreativeLinearArray(
    const CreativeLinearArrayPlanRequest& request) noexcept {
  CreativeLinearArrayPlanReceipt receipt;
  receipt.requested = true;
  receipt.sourceObjectCount = request.sourceObjectCount;
  if (request.sourceObjectCount == 0U) {
    receipt.status = CreativeLinearArrayStatus::EmptySelection;
    receipt.reasonCode = "creative_linear_array_selection_empty";
    return receipt;
  }
  if (!validEnum(request.direction, CreativeLinearArrayDirection::Count) ||
      !validEnum(request.copyCount, CreativeLinearArrayCopyCount::Count) ||
      !validEnum(request.spacing, CreativeLinearArraySpacing::Count) ||
      !std::isfinite(request.cellSize) || request.cellSize <= 0.0 ||
      request.maxGeneratedObjects == 0U ||
      request.maxGeneratedObjects >
          kCreativeLinearArrayGeneratedObjectCapacity) {
    receipt.status = CreativeLinearArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_linear_array_request_invalid";
    return receipt;
  }

  const std::uint64_t copyCount =
      creativeLinearArrayCopyCountValue(request.copyCount);
  const std::uint64_t spacingCells =
      creativeLinearArraySpacingCells(request.spacing);
  receipt.copyCount = copyCount;
  if (copyCount == 0U || spacingCells == 0U ||
      request.sourceObjectCount >
          std::numeric_limits<std::uint64_t>::max() / copyCount) {
    receipt.status = CreativeLinearArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_linear_array_size_overflow";
    return receipt;
  }
  receipt.generatedObjectCount = request.sourceObjectCount * copyCount;
  if (receipt.generatedObjectCount > request.maxGeneratedObjects) {
    receipt.status = CreativeLinearArrayStatus::OperationLimitExceeded;
    receipt.reasonCode = "creative_linear_array_limit_exceeded";
    return receipt;
  }

  const CreativeVec3 unit = directionUnit(request.direction);
  for (std::uint64_t ordinal = 1U; ordinal <= copyCount; ++ordinal) {
    const double distance = static_cast<double>(ordinal) *
                            static_cast<double>(spacingCells) *
                            request.cellSize;
    if (!std::isfinite(distance)) {
      receipt.status = CreativeLinearArrayStatus::InvalidRequest;
      receipt.reasonCode = "creative_linear_array_distance_invalid";
      receipt.instanceCount = 0U;
      return receipt;
    }
    CreativeLinearArrayInstance& instance =
        receipt.instances[receipt.instanceCount++];
    instance.ordinal = static_cast<std::uint32_t>(ordinal);
    instance.offset = {unit.x * distance, unit.y * distance,
                       unit.z * distance};
  }

  receipt.accepted = true;
  receipt.status = CreativeLinearArrayStatus::Planned;
  receipt.reasonCode = "creative_linear_array_planned";
  return receipt;
}

CreativeRadialArrayPlanReceipt planCreativeRadialArray(
    const CreativeRadialArrayPlanRequest& request) noexcept {
  CreativeRadialArrayPlanReceipt receipt;
  receipt.requested = true;
  receipt.sourceObjectCount = request.sourceObjectCount;
  receipt.pivot = request.pivot;
  receipt.axis = request.axis;
  if (request.sourceObjectCount == 0U) {
    receipt.status = CreativeRadialArrayStatus::EmptySelection;
    receipt.reasonCode = "creative_radial_array_selection_empty";
    return receipt;
  }
  if (!isFiniteCreativeVec3(request.pivot) ||
      !isValidCreativeAxis3(request.axis) ||
      !validEnum(request.instanceCount,
                 CreativeRadialArrayInstanceCount::Count) ||
      !validEnum(request.sweep, CreativeRadialArraySweep::Count) ||
      request.maxGeneratedObjects == 0U ||
      request.maxGeneratedObjects >
          kCreativeRadialArrayGeneratedObjectCapacity) {
    receipt.status = CreativeRadialArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_radial_array_request_invalid";
    return receipt;
  }

  const std::uint64_t totalInstanceCount =
      creativeRadialArrayInstanceCountValue(request.instanceCount);
  const std::uint64_t generatedCopyCount = totalInstanceCount - 1U;
  receipt.totalInstanceCount = totalInstanceCount;
  receipt.generatedCopyCount = generatedCopyCount;
  if (totalInstanceCount < 2U ||
      generatedCopyCount > kCreativeRadialArrayInstanceCapacity ||
      request.sourceObjectCount >
          std::numeric_limits<std::uint64_t>::max() / generatedCopyCount) {
    receipt.status = CreativeRadialArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_radial_array_size_overflow";
    return receipt;
  }
  receipt.generatedObjectCount =
      request.sourceObjectCount * generatedCopyCount;
  if (receipt.generatedObjectCount > request.maxGeneratedObjects) {
    receipt.status = CreativeRadialArrayStatus::OperationLimitExceeded;
    receipt.reasonCode = "creative_radial_array_limit_exceeded";
    return receipt;
  }

  const double sweepRadians = creativeRadialArraySweepDegrees(request.sweep) *
                              std::numbers::pi / 180.0;
  const bool closedRing =
      request.sweep == CreativeRadialArraySweep::Degrees360;
  const double divisor = static_cast<double>(
      closedRing ? totalInstanceCount : generatedCopyCount);
  for (std::uint64_t ordinal = 1U; ordinal <= generatedCopyCount; ++ordinal) {
    const double angle = sweepRadians * static_cast<double>(ordinal) / divisor;
    if (!std::isfinite(angle)) {
      receipt.status = CreativeRadialArrayStatus::InvalidRequest;
      receipt.reasonCode = "creative_radial_array_angle_invalid";
      receipt.instanceCount = 0U;
      return receipt;
    }
    CreativeRadialArrayInstance& instance =
        receipt.instances[receipt.instanceCount++];
    instance.ordinal = static_cast<std::uint32_t>(ordinal);
    instance.angleRadians = angle;
  }

  receipt.accepted = true;
  receipt.status = CreativeRadialArrayStatus::Planned;
  receipt.reasonCode = "creative_radial_array_planned";
  return receipt;
}

CreativeLinearArrayReceipt createCreativeLinearArrayAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeLinearArrayRequest& request) {
  CreativeLinearArrayReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (objectIds.empty()) {
    receipt.status = CreativeLinearArrayStatus::EmptySelection;
    receipt.message = "creative_linear_array_selection_empty";
    return receipt;
  }

  CreativeClipboard clipboard;
  receipt.copyReceipt =
      copyDocumentObjectsToClipboard(document, objectIds, clipboard);
  if (!receipt.copyReceipt.accepted) {
    receipt.failedObjectId = receipt.copyReceipt.failedObjectId;
    receipt.status = statusForClipboardFailure(receipt.copyReceipt.status);
    receipt.message = receipt.copyReceipt.reasonCode;
    return receipt;
  }
  receipt.sourceObjectCount = receipt.copyReceipt.copiedObjectCount;

  CreativeLinearArrayPlanRequest planRequest;
  planRequest.sourceObjectCount = receipt.sourceObjectCount;
  planRequest.direction = request.direction;
  planRequest.copyCount = request.copyCount;
  planRequest.spacing = request.spacing;
  planRequest.cellSize = request.cellSize;
  planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
  receipt.plan = planCreativeLinearArray(planRequest);
  receipt.generatedObjectCount = receipt.plan.generatedObjectCount;
  if (!receipt.plan.accepted) {
    receipt.status = receipt.plan.status;
    receipt.message = receipt.plan.reasonCode;
    return receipt;
  }

  std::array<CreativeClipboardPasteRequest,
             kCreativeLinearArrayInstanceCapacity>
      pasteRequests{};
  for (std::size_t index = 0; index < receipt.plan.instanceCount; ++index) {
    pasteRequests[index].offset = receipt.plan.instances[index].offset;
    pasteRequests[index].appendCopySuffix = true;
    pasteRequests[index].externalParentPolicy =
        CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  }
  receipt.pasteReceipt = pasteCreativeClipboardBatchAtomically(
      document, clipboard,
      std::span<const CreativeClipboardPasteRequest>{pasteRequests.data(),
                                                      receipt.plan.instanceCount});
  if (!receipt.pasteReceipt.accepted) {
    receipt.failedObjectId = receipt.pasteReceipt.failedObjectId;
    receipt.status = statusForClipboardFailure(receipt.pasteReceipt.status);
    receipt.message = receipt.pasteReceipt.reasonCode;
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeLinearArrayStatus::Applied;
  receipt.generatedObjectCount = receipt.pasteReceipt.pastedObjectCount;
  receipt.revisionAfter = document.revision();
  receipt.finalCopyObjectCount =
      static_cast<std::size_t>(receipt.sourceObjectCount);
  receipt.finalCopyFirstObjectIndex =
      receipt.pasteReceipt.pastedObjectIds.size() -
      receipt.finalCopyObjectCount;
  receipt.message = "creative_linear_array_applied";
  return receipt;
}

CreativeRadialArrayReceipt createCreativeRadialArrayAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeRadialArrayRequest& request) {
  CreativeRadialArrayReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (objectIds.empty()) {
    receipt.status = CreativeRadialArrayStatus::EmptySelection;
    receipt.message = "creative_radial_array_selection_empty";
    return receipt;
  }

  CreativeClipboard clipboard;
  receipt.copyReceipt =
      copyDocumentObjectsToClipboard(document, objectIds, clipboard);
  if (!receipt.copyReceipt.accepted) {
    receipt.failedObjectId = receipt.copyReceipt.failedObjectId;
    receipt.status = radialStatusForClipboardFailure(receipt.copyReceipt.status);
    receipt.message = receipt.copyReceipt.reasonCode;
    return receipt;
  }
  receipt.sourceObjectCount = receipt.copyReceipt.copiedObjectCount;

  CreativeRadialArrayPlanRequest planRequest;
  planRequest.sourceObjectCount = receipt.sourceObjectCount;
  planRequest.pivot = request.pivot;
  planRequest.axis = request.axis;
  planRequest.instanceCount = request.instanceCount;
  planRequest.sweep = request.sweep;
  planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
  receipt.plan = planCreativeRadialArray(planRequest);
  receipt.generatedObjectCount = receipt.plan.generatedObjectCount;
  if (!receipt.plan.accepted) {
    receipt.status = receipt.plan.status;
    receipt.message = receipt.plan.reasonCode;
    return receipt;
  }

  const double radiusSquared = creativeSquaredDistanceFromAxis(
      clipboard.placementAnchor, request.pivot, request.axis);
  if (!std::isfinite(radiusSquared) || radiusSquared <= 1.0e-12) {
    receipt.status = CreativeRadialArrayStatus::DegenerateRadius;
    receipt.message = "creative_radial_array_radius_degenerate";
    return receipt;
  }

  std::array<CreativeClipboardPasteRequest,
             kCreativeRadialArrayInstanceCapacity>
      pasteRequests{};
  for (std::size_t index = 0; index < receipt.plan.instanceCount; ++index) {
    CreativeClipboardPasteRequest& paste = pasteRequests[index];
    paste.offset = {};
    paste.hasTransformAnchor = true;
    paste.transformAnchor = request.pivot;
    paste.hasAxisAngleRotation = true;
    paste.rotationAxis = request.axis;
    paste.rotationRadians = receipt.plan.instances[index].angleRadians;
    paste.appendCopySuffix = true;
    paste.externalParentPolicy =
        CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  }
  receipt.pasteReceipt = pasteCreativeClipboardBatchAtomically(
      document, clipboard,
      std::span<const CreativeClipboardPasteRequest>{
          pasteRequests.data(), receipt.plan.instanceCount});
  if (!receipt.pasteReceipt.accepted) {
    receipt.failedObjectId = receipt.pasteReceipt.failedObjectId;
    receipt.status =
        radialStatusForClipboardFailure(receipt.pasteReceipt.status);
    receipt.message = receipt.pasteReceipt.reasonCode;
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeRadialArrayStatus::Applied;
  receipt.generatedObjectCount = receipt.pasteReceipt.pastedObjectCount;
  receipt.revisionAfter = document.revision();
  receipt.finalCopyObjectCount =
      static_cast<std::size_t>(receipt.sourceObjectCount);
  receipt.finalCopyFirstObjectIndex =
      receipt.pasteReceipt.pastedObjectIds.size() -
      receipt.finalCopyObjectCount;
  receipt.message = "creative_radial_array_applied";
  return receipt;
}

}  // namespace iggy3d::creative
