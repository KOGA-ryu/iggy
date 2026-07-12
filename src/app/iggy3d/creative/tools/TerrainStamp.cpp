#include "app/iggy3d/creative/tools/TerrainStamp.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace iggy3d::creative {
namespace {

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

[[nodiscard]] constexpr bool coordLess(CreativeTerrainCoord2 lhs,
                                       CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] constexpr bool controlLess(
    const CreativeTerrainControlPoint& lhs,
    const CreativeTerrainControlPoint& rhs) noexcept {
  return coordLess(lhs.coord, rhs.coord);
}

[[nodiscard]] constexpr bool editLess(const CreativeTerrainControlEdit& lhs,
                                      const CreativeTerrainControlEdit& rhs) {
  return coordLess(lhs.control.coord, rhs.control.coord);
}

[[nodiscard]] bool validControls(
    std::span<const CreativeTerrainControlPoint> controls) noexcept {
  if (controls.size() > kCreativeTerrainControlCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < controls.size(); ++index) {
    if (!isValidCreativeTerrainControlPoint(controls[index]) ||
        (index > 0U &&
         !coordLess(controls[index - 1U].coord, controls[index].coord))) {
      return false;
    }
  }
  return true;
}

void mixSignature(std::uint64_t& signature, std::uint64_t value) noexcept {
  signature ^= value;
  signature *= kFnvPrime;
}

[[nodiscard]] std::uint64_t stampSignature(
    const CreativeTerrainStamp& stamp) noexcept {
  std::uint64_t signature = kFnvOffset;
  mixSignature(signature, stamp.widthCells);
  mixSignature(signature, stamp.depthCells);
  mixSignature(signature, stamp.controlCount);
  for (const CreativeTerrainControlPoint& control : stamp.items()) {
    mixSignature(signature, static_cast<std::uint32_t>(control.coord.x));
    mixSignature(signature, static_cast<std::uint32_t>(control.coord.z));
    mixSignature(signature, control.heightCells);
    mixSignature(signature, control.radiusCells);
  }
  return signature;
}

[[nodiscard]] bool validStampMode(CreativeTerrainStampMode mode) noexcept {
  return static_cast<std::size_t>(mode) <
         static_cast<std::size_t>(CreativeTerrainStampMode::Count);
}

[[nodiscard]] const CreativeTerrainControlPoint* findControl(
    std::span<const CreativeTerrainControlPoint> controls,
    CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      controls.begin(), controls.end(), coord,
      [](const CreativeTerrainControlPoint& control,
         CreativeTerrainCoord2 candidate) {
        return coordLess(control.coord, candidate);
      });
  return found != controls.end() && found->coord == coord ? &*found : nullptr;
}

[[nodiscard]] bool insideBounds(CreativeTerrainCoord2 coord,
                                CreativeTerrainCoord2 minimum,
                                CreativeTerrainCoord2 maximum) noexcept {
  return coord.x >= minimum.x && coord.x <= maximum.x &&
         coord.z >= minimum.z && coord.z <= maximum.z;
}

[[nodiscard]] bool checkedCoord(std::int64_t x,
                                std::int64_t z,
                                CreativeTerrainCoord2& output) noexcept {
  constexpr std::int64_t minimum = std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t maximum = std::numeric_limits<std::int32_t>::max();
  if (x < minimum || x > maximum || z < minimum || z > maximum) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

struct TransformedLocalCoord {
  std::int64_t x = 0;
  std::int64_t z = 0;
  std::uint32_t widthCells = 0U;
  std::uint32_t depthCells = 0U;
};

[[nodiscard]] TransformedLocalCoord transformLocal(
    CreativeTerrainCoord2 local,
    std::uint32_t widthCells,
    std::uint32_t depthCells,
    std::uint8_t quarterTurns,
    bool mirrorX,
    bool mirrorZ) noexcept {
  std::int64_t x = local.x;
  std::int64_t z = local.z;
  const std::int64_t width = widthCells;
  const std::int64_t depth = depthCells;
  if (mirrorX) {
    x = width - 1 - x;
  }
  if (mirrorZ) {
    z = depth - 1 - z;
  }
  switch (quarterTurns) {
    case 0U: return {x, z, widthCells, depthCells};
    case 1U:
      return {depth - 1 - z, x, depthCells, widthCells};
    case 2U:
      return {width - 1 - x, depth - 1 - z, widthCells, depthCells};
    case 3U:
      return {z, width - 1 - x, depthCells, widthCells};
    default: break;
  }
  return {};
}

[[nodiscard]] bool appendEdit(CreativeTerrainStampPlan& plan,
                              CreativeTerrainControlEdit edit) noexcept {
  if (plan.editCount >= plan.edits.size()) {
    return false;
  }
  plan.edits[plan.editCount++] = edit;
  return true;
}

}  // namespace

std::string_view toString(CreativeTerrainStampMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainStampMode::Merge: return "MERGE";
    case CreativeTerrainStampMode::Replace: return "REPLACE";
    case CreativeTerrainStampMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainStampCopyStatus status) noexcept {
  switch (status) {
    case CreativeTerrainStampCopyStatus::NotRequested: return "NotRequested";
    case CreativeTerrainStampCopyStatus::InvalidSource: return "InvalidSource";
    case CreativeTerrainStampCopyStatus::InvalidBounds: return "InvalidBounds";
    case CreativeTerrainStampCopyStatus::EmptyRegion: return "EmptyRegion";
    case CreativeTerrainStampCopyStatus::Copied: return "Copied";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainStampPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainStampPlanStatus::NotRequested: return "NotRequested";
    case CreativeTerrainStampPlanStatus::InvalidStamp: return "InvalidStamp";
    case CreativeTerrainStampPlanStatus::InvalidDestination:
      return "InvalidDestination";
    case CreativeTerrainStampPlanStatus::InvalidRequest: return "InvalidRequest";
    case CreativeTerrainStampPlanStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeTerrainStampPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainStampPlanStatus::NoChange: return "NoChange";
    case CreativeTerrainStampPlanStatus::Ready: return "Ready";
  }
  return "Unknown";
}

bool isValidCreativeTerrainStamp(const CreativeTerrainStamp& stamp) noexcept {
  if (stamp.controlCount == 0U ||
      stamp.controlCount > kCreativeTerrainControlCapacity ||
      stamp.widthCells == 0U || stamp.depthCells == 0U ||
      stamp.widthCells >
          static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
      stamp.depthCells >
          static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
      !validControls(stamp.items())) {
    return false;
  }
  for (const CreativeTerrainControlPoint& control : stamp.items()) {
    if (control.coord.x < 0 || control.coord.z < 0 ||
        static_cast<std::uint32_t>(control.coord.x) >= stamp.widthCells ||
        static_cast<std::uint32_t>(control.coord.z) >= stamp.depthCells) {
      return false;
    }
  }
  return stamp.contentSignature == stampSignature(stamp);
}

bool creativeTerrainStampEmpty(const CreativeTerrainStamp& stamp) noexcept {
  return stamp.controlCount == 0U;
}

void clearCreativeTerrainStamp(CreativeTerrainStamp& stamp) noexcept {
  stamp = {};
}

CreativeTerrainStampCopyReceipt copyCreativeTerrainRegionToStamp(
    std::uint64_t sourceDocumentId,
    std::uint64_t sourceRevision,
    std::span<const CreativeTerrainControlPoint> controls,
    CreativeTerrainCoord2 minimumCoord,
    CreativeTerrainCoord2 maximumCoord,
    CreativeTerrainStamp& outStamp) noexcept {
  CreativeTerrainStampCopyReceipt receipt;
  receipt.requested = true;
  receipt.minimumCoord = minimumCoord;
  receipt.maximumCoord = maximumCoord;
  if (!validControls(controls)) {
    receipt.status = CreativeTerrainStampCopyStatus::InvalidSource;
    receipt.reasonCode = "creative_terrain_stamp_source_invalid";
    return receipt;
  }
  const std::int64_t width = static_cast<std::int64_t>(maximumCoord.x) -
                                 minimumCoord.x +
                             1;
  const std::int64_t depth = static_cast<std::int64_t>(maximumCoord.z) -
                                 minimumCoord.z +
                             1;
  constexpr std::int64_t maximumExtent =
      std::numeric_limits<std::int32_t>::max();
  if (width <= 0 || depth <= 0 || width > maximumExtent ||
      depth > maximumExtent) {
    receipt.status = CreativeTerrainStampCopyStatus::InvalidBounds;
    receipt.reasonCode = "creative_terrain_stamp_bounds_invalid";
    return receipt;
  }

  CreativeTerrainStamp staged;
  staged.sourceDocumentId = sourceDocumentId;
  staged.sourceRevision = sourceRevision;
  staged.sourceMinimum = minimumCoord;
  staged.widthCells = static_cast<std::uint32_t>(width);
  staged.depthCells = static_cast<std::uint32_t>(depth);
  for (const CreativeTerrainControlPoint& control : controls) {
    if (!insideBounds(control.coord, minimumCoord, maximumCoord)) {
      continue;
    }
    CreativeTerrainControlPoint local = control;
    local.coord = {static_cast<std::int32_t>(
                       static_cast<std::int64_t>(control.coord.x) -
                       minimumCoord.x),
                   static_cast<std::int32_t>(
                       static_cast<std::int64_t>(control.coord.z) -
                       minimumCoord.z)};
    staged.controls[staged.controlCount++] = local;
  }
  if (staged.controlCount == 0U) {
    receipt.status = CreativeTerrainStampCopyStatus::EmptyRegion;
    receipt.reasonCode = "creative_terrain_stamp_region_empty";
    return receipt;
  }
  staged.contentSignature = stampSignature(staged);
  receipt.accepted = true;
  receipt.status = CreativeTerrainStampCopyStatus::Copied;
  receipt.copiedControlCount = staged.controlCount;
  receipt.reasonCode = "creative_terrain_stamp_copied";
  outStamp = staged;
  return receipt;
}

CreativeTerrainStampPlan buildCreativeTerrainStampPlan(
    const CreativeTerrainStampRequest& request) noexcept {
  CreativeTerrainStampPlan plan;
  plan.requested = true;
  plan.targetMinimum = request.targetMinimum;
  plan.mode = request.mode;
  if (request.stamp == nullptr ||
      !isValidCreativeTerrainStamp(*request.stamp)) {
    plan.status = CreativeTerrainStampPlanStatus::InvalidStamp;
    plan.reasonCode = "creative_terrain_stamp_invalid";
    return plan;
  }
  if (!validControls(request.destinationControls)) {
    plan.status = CreativeTerrainStampPlanStatus::InvalidDestination;
    plan.reasonCode = "creative_terrain_stamp_destination_invalid";
    return plan;
  }
  if (request.quarterTurns > 3U || !validStampMode(request.mode)) {
    plan.status = CreativeTerrainStampPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_stamp_request_invalid";
    return plan;
  }

  const CreativeTerrainStamp& stamp = *request.stamp;
  for (const CreativeTerrainControlPoint& source : stamp.items()) {
    const TransformedLocalCoord transformed = transformLocal(
        source.coord, stamp.widthCells, stamp.depthCells, request.quarterTurns,
        request.mirrorX, request.mirrorZ);
    plan.transformedWidthCells = transformed.widthCells;
    plan.transformedDepthCells = transformed.depthCells;
    CreativeTerrainControlPoint finalControl = source;
    if (!checkedCoord(static_cast<std::int64_t>(request.targetMinimum.x) +
                          transformed.x,
                      static_cast<std::int64_t>(request.targetMinimum.z) +
                          transformed.z,
                      finalControl.coord) ||
        !isValidCreativeTerrainControlPoint(finalControl)) {
      plan.finalControlCount = 0U;
      plan.status = CreativeTerrainStampPlanStatus::CoordinateOverflow;
      plan.reasonCode = "creative_terrain_stamp_coordinate_overflow";
      return plan;
    }
    plan.finalControls[plan.finalControlCount++] = finalControl;
  }
  std::sort(plan.finalControls.begin(),
            plan.finalControls.begin() + plan.finalControlCount, controlLess);

  CreativeTerrainCoord2 targetExclusiveMaximum{};
  if (!checkedCoord(static_cast<std::int64_t>(request.targetMinimum.x) +
                        plan.transformedWidthCells,
                    static_cast<std::int64_t>(request.targetMinimum.z) +
                        plan.transformedDepthCells,
                    targetExclusiveMaximum)) {
    plan.finalControlCount = 0U;
    plan.status = CreativeTerrainStampPlanStatus::CoordinateOverflow;
    plan.reasonCode = "creative_terrain_stamp_bounds_overflow";
    return plan;
  }
  plan.targetMaximum = {targetExclusiveMaximum.x - 1,
                        targetExclusiveMaximum.z - 1};

  for (const CreativeTerrainControlPoint& destination :
       request.destinationControls) {
    const CreativeTerrainControlPoint* replacement =
        findControl(plan.controls(), destination.coord);
    if (replacement != nullptr) {
      if (!(*replacement == destination)) {
        if (!appendEdit(plan,
                        {CreativeTerrainEditKind::Upsert, *replacement})) {
          break;
        }
        ++plan.updatedControlCount;
      }
    } else if (request.mode == CreativeTerrainStampMode::Replace &&
               insideBounds(destination.coord, plan.targetMinimum,
                            plan.targetMaximum)) {
      if (!appendEdit(
              plan, {CreativeTerrainEditKind::Remove, destination})) {
        break;
      }
      ++plan.removedControlCount;
    }
  }
  for (const CreativeTerrainControlPoint& source : plan.controls()) {
    if (findControl(request.destinationControls, source.coord) != nullptr) {
      continue;
    }
    if (!appendEdit(plan, {CreativeTerrainEditKind::Upsert, source})) {
      break;
    }
    ++plan.insertedControlCount;
  }
  if (plan.editCount > kCreativeTerrainStampEditCapacity) {
    plan.editCount = 0U;
    plan.status = CreativeTerrainStampPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_stamp_edit_capacity_exceeded";
    return plan;
  }

  const std::size_t finalDestinationCount =
      request.destinationControls.size() - plan.removedControlCount +
      plan.insertedControlCount;
  if (finalDestinationCount > kCreativeTerrainControlCapacity) {
    plan.editCount = 0U;
    plan.insertedControlCount = 0U;
    plan.updatedControlCount = 0U;
    plan.removedControlCount = 0U;
    plan.status = CreativeTerrainStampPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_stamp_control_capacity_exceeded";
    return plan;
  }
  std::sort(plan.edits.begin(), plan.edits.begin() + plan.editCount, editLess);
  plan.accepted = true;
  if (plan.editCount == 0U) {
    plan.status = CreativeTerrainStampPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_stamp_no_change";
    return plan;
  }
  plan.status = CreativeTerrainStampPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_stamp_ready";
  return plan;
}

}  // namespace iggy3d::creative
