#include "app/iggy3d/creative/document/TerrainField.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool controlLess(const CreativeTerrainControlPoint& lhs,
                               const CreativeTerrainControlPoint& rhs) noexcept {
  return coordLess(lhs.coord, rhs.coord);
}

[[nodiscard]] bool editLess(const CreativeTerrainControlEdit& lhs,
                            const CreativeTerrainControlEdit& rhs) noexcept {
  return coordLess(lhs.control.coord, rhs.control.coord);
}

[[nodiscard]] bool coordinateAllowsMaximumRadius(
    CreativeTerrainCoord2 coord) noexcept {
  constexpr std::int32_t margin = kCreativeTerrainMaximumRadiusCells;
  constexpr std::int32_t minimum =
      std::numeric_limits<std::int32_t>::min() + margin;
  constexpr std::int32_t maximum =
      std::numeric_limits<std::int32_t>::max() - margin;
  return coord.x >= minimum && coord.x <= maximum && coord.z >= minimum &&
         coord.z <= maximum;
}

using ControlIterator = std::vector<CreativeTerrainControlPoint>::iterator;
using ConstControlIterator =
    std::vector<CreativeTerrainControlPoint>::const_iterator;

[[nodiscard]] ControlIterator findControl(
    std::vector<CreativeTerrainControlPoint>& controls,
    CreativeTerrainCoord2 coord) noexcept {
  const ControlIterator found = std::lower_bound(
      controls.begin(), controls.end(), coord,
      [](const CreativeTerrainControlPoint& control, CreativeTerrainCoord2 value) {
        return coordLess(control.coord, value);
      });
  return found != controls.end() && found->coord == coord ? found
                                                          : controls.end();
}

[[nodiscard]] ConstControlIterator findControl(
    const std::vector<CreativeTerrainControlPoint>& controls,
    CreativeTerrainCoord2 coord) noexcept {
  const ConstControlIterator found = std::lower_bound(
      controls.begin(), controls.end(), coord,
      [](const CreativeTerrainControlPoint& control, CreativeTerrainCoord2 value) {
        return coordLess(control.coord, value);
      });
  return found != controls.end() && found->coord == coord ? found
                                                          : controls.end();
}

struct TerrainContribution {
  CreativeTerrainCoord2 coord{};
  std::uint32_t weight = 0;
  std::uint64_t weightedHeight = 0;
};

[[nodiscard]] bool contributionLess(const TerrainContribution& lhs,
                                    const TerrainContribution& rhs) noexcept {
  return coordLess(lhs.coord, rhs.coord);
}

[[nodiscard]] const CreativeTerrainColumn* findTerrainColumn(
    std::span<const CreativeTerrainColumn> columns,
    std::int64_t x,
    std::int64_t z) noexcept {
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return nullptr;
  }
  const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                    static_cast<std::int32_t>(z)};
  const auto found = std::lower_bound(
      columns.begin(), columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 value) {
        return coordLess(column.coord, value);
      });
  return found != columns.end() && found->coord == coord ? &*found : nullptr;
}

[[nodiscard]] double terrainCornerHeightCells(
    std::span<const CreativeTerrainColumn> columns,
    std::int64_t cornerX,
    std::int64_t cornerZ) noexcept {
  constexpr std::array<std::array<std::int32_t, 2U>, 4U> offsets{{
      {-1, -1},
      {0, -1},
      {-1, 0},
      {0, 0},
  }};
  std::uint64_t heightSum = 0U;
  std::uint32_t count = 0U;
  for (const auto& offset : offsets) {
    const CreativeTerrainColumn* column = findTerrainColumn(
        columns, cornerX + offset[0], cornerZ + offset[1]);
    if (column == nullptr) {
      continue;
    }
    heightSum += column->heightCells;
    ++count;
  }
  return count == 0U ? 0.0
                     : static_cast<double>(heightSum) /
                           static_cast<double>(count);
}

[[nodiscard]] std::uint32_t terrainContributionWeight(
    const CreativeTerrainControlPoint& control,
    CreativeTerrainCoord2 coord) noexcept {
  const std::int64_t dx = static_cast<std::int64_t>(coord.x) - control.coord.x;
  const std::int64_t dz = static_cast<std::int64_t>(coord.z) - control.coord.z;
  const std::int64_t radius = control.radiusCells;
  const std::int64_t radiusSquared = radius * radius;
  const std::int64_t distanceSquared = dx * dx + dz * dz;
  if (distanceSquared > radiusSquared) {
    return 0U;
  }
  const std::uint32_t base = static_cast<std::uint32_t>(
      radiusSquared - distanceSquared + 1);
  return base * base;
}

[[nodiscard]] CreativeTerrainHeightSample sampleTerrainHeightUnchecked(
    std::span<const CreativeTerrainControlPoint> controls,
    CreativeTerrainCoord2 coord) noexcept {
  CreativeTerrainHeightSample sample;
  sample.coord = coord;
  std::uint64_t weightedHeight = 0U;
  for (const CreativeTerrainControlPoint& control : controls) {
    const std::uint32_t weight = terrainContributionWeight(control, coord);
    if (weight == 0U) {
      continue;
    }
    sample.totalWeight += weight;
    weightedHeight += static_cast<std::uint64_t>(weight) * control.heightCells;
    ++sample.contributingControlCount;
  }
  if (sample.totalWeight == 0U) {
    return sample;
  }
  const std::uint64_t roundedHeight =
      (weightedHeight + sample.totalWeight / 2U) / sample.totalWeight;
  sample.heightCells = static_cast<std::uint16_t>(
      std::clamp<std::uint64_t>(roundedHeight,
                                kCreativeTerrainMinimumHeightCells,
                                kCreativeTerrainMaximumHeightCells));
  sample.present = true;
  return sample;
}

[[nodiscard]] bool tryTerrainCell(double worldX,
                                  double worldZ,
                                  CreativeVec3 gridOrigin,
                                  double cellSize,
                                  CreativeTerrainCoord2& output) noexcept {
  const double x = std::floor((worldX - gridOrigin.x) / cellSize);
  const double z = std::floor((worldZ - gridOrigin.z) / cellSize);
  constexpr double minimum =
      static_cast<double>(std::numeric_limits<std::int32_t>::min());
  constexpr double maximum =
      static_cast<double>(std::numeric_limits<std::int32_t>::max());
  if (!std::isfinite(x) || !std::isfinite(z) || x < minimum || x > maximum ||
      z < minimum || z > maximum) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

struct TerrainAabbHit {
  bool hit = false;
  bool startInside = false;
  double distance = 0.0;
  CreativeVec3 normal{};
};

[[nodiscard]] TerrainAabbHit intersectTerrainColumn(
    CreativeVec3 origin,
    CreativeVec3 direction,
    CreativeVec3 minimum,
    CreativeVec3 maximum,
    double maxDistance) noexcept {
  TerrainAabbHit result;
  result.startInside = origin.x > minimum.x && origin.x < maximum.x &&
                       origin.y > minimum.y && origin.y < maximum.y &&
                       origin.z > minimum.z && origin.z < maximum.z;
  double nearDistance = 0.0;
  double farDistance = maxDistance;
  CreativeVec3 nearNormal{};
  constexpr double epsilon = 1.0e-12;
  const auto applyAxis = [&](double axisOrigin, double axisDirection,
                             double axisMinimum, double axisMaximum,
                             CreativeVec3 minimumNormal,
                             CreativeVec3 maximumNormal) {
    if (std::fabs(axisDirection) <= epsilon) {
      return axisOrigin >= axisMinimum && axisOrigin <= axisMaximum;
    }
    double first = (axisMinimum - axisOrigin) / axisDirection;
    double second = (axisMaximum - axisOrigin) / axisDirection;
    CreativeVec3 firstNormal = minimumNormal;
    CreativeVec3 secondNormal = maximumNormal;
    if (first > second) {
      std::swap(first, second);
      std::swap(firstNormal, secondNormal);
    }
    if (first >= nearDistance) {
      nearDistance = first;
      nearNormal = firstNormal;
    }
    farDistance = std::min(farDistance, second);
    return nearDistance <= farDistance;
  };
  if (!applyAxis(origin.x, direction.x, minimum.x, maximum.x,
                 {-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}) ||
      !applyAxis(origin.y, direction.y, minimum.y, maximum.y,
                 {0.0, -1.0, 0.0}, {0.0, 1.0, 0.0}) ||
      !applyAxis(origin.z, direction.z, minimum.z, maximum.z,
                 {0.0, 0.0, -1.0}, {0.0, 0.0, 1.0}) ||
      farDistance < 0.0 || nearDistance > maxDistance) {
    return result;
  }
  result.hit = true;
  result.distance = result.startInside ? 0.0 : std::max(0.0, nearDistance);
  if (result.startInside) {
    const double absX = std::fabs(direction.x);
    const double absY = std::fabs(direction.y);
    const double absZ = std::fabs(direction.z);
    result.normal = absX >= absY && absX >= absZ
                        ? CreativeVec3{direction.x > 0.0 ? -1.0 : 1.0, 0.0,
                                       0.0}
                    : absY >= absZ
                        ? CreativeVec3{0.0, direction.y > 0.0 ? -1.0 : 1.0,
                                       0.0}
                        : CreativeVec3{0.0, 0.0,
                                       direction.z > 0.0 ? -1.0 : 1.0};
  } else {
    result.normal = nearNormal;
  }
  return result;
}

[[nodiscard]] std::int32_t floorDivByVoxelChunk(
    std::int32_t value) noexcept {
  std::int32_t quotient = value / kCreativeVoxelChunkEdge;
  if (value % kCreativeVoxelChunkEdge < 0) {
    --quotient;
  }
  return quotient;
}

void appendRowCuboids(std::span<const CreativeTerrainColumn> row,
                      std::vector<CreativeVoxelCuboid>& output) {
  if (row.empty()) {
    return;
  }
  std::size_t begin = 0U;
  while (begin < row.size()) {
    std::size_t end = begin + 1U;
    while (end < row.size() &&
           row[end].heightCells == row[begin].heightCells &&
           row[end].coord.x == row[end - 1U].coord.x + 1) {
      ++end;
    }
    const CreativeTerrainColumn& first = row[begin];
    CreativeVoxelCuboid cuboid;
    cuboid.chunk = {floorDivByVoxelChunk(first.coord.x), 0,
                    floorDivByVoxelChunk(first.coord.z)};
    cuboid.minCell = {first.coord.x, 0, first.coord.z};
    cuboid.maxCellExclusive = {row[end - 1U].coord.x + 1,
                               first.heightCells, first.coord.z + 1};
    cuboid.material = CreativeObjectKind::TerrainPatch;
    output.push_back(cuboid);
    begin = end;
  }
}

}  // namespace

bool isValidCreativeTerrainControlPoint(
    CreativeTerrainControlPoint control) noexcept {
  return coordinateAllowsMaximumRadius(control.coord) &&
         control.heightCells >= kCreativeTerrainMinimumHeightCells &&
         control.heightCells <= kCreativeTerrainMaximumHeightCells &&
         control.radiusCells >= kCreativeTerrainMinimumRadiusCells &&
         control.radiusCells <= kCreativeTerrainMaximumRadiusCells;
}

std::string_view toString(CreativeTerrainMutationStatus status) noexcept {
  switch (status) {
    case CreativeTerrainMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainMutationStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainMutationStatus::InvalidEdit:
      return "InvalidEdit";
    case CreativeTerrainMutationStatus::DuplicateCoordinate:
      return "DuplicateCoordinate";
    case CreativeTerrainMutationStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainMutationStatus::NoChange:
      return "NoChange";
    case CreativeTerrainMutationStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainSurfacePlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainSurfacePlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainSurfacePlanStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainSurfacePlanStatus::Empty:
      return "Empty";
    case CreativeTerrainSurfacePlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainRaycastStatus status) noexcept {
  switch (status) {
    case CreativeTerrainRaycastStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainRaycastStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainRaycastStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainRaycastStatus::TraversalLimitExceeded:
      return "TraversalLimitExceeded";
    case CreativeTerrainRaycastStatus::Miss:
      return "Miss";
    case CreativeTerrainRaycastStatus::Hit:
      return "Hit";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainRenderPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainRenderPlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainRenderPlanStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainRenderPlanStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainRenderPlanStatus::SurfacePlanFailed:
      return "SurfacePlanFailed";
    case CreativeTerrainRenderPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainRenderPlanStatus::ArithmeticOverflow:
      return "ArithmeticOverflow";
    case CreativeTerrainRenderPlanStatus::Empty:
      return "Empty";
    case CreativeTerrainRenderPlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeTerrainMutationPreviewStatus status) noexcept {
  switch (status) {
    case CreativeTerrainMutationPreviewStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainMutationPreviewStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainMutationPreviewStatus::MutationRejected:
      return "MutationRejected";
    case CreativeTerrainMutationPreviewStatus::RenderRejected:
      return "RenderRejected";
    case CreativeTerrainMutationPreviewStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

bool CreativeTerrainField::isValid() const noexcept {
  return valid_;
}

bool CreativeTerrainField::validateInvariants() const noexcept {
  if (!valid_ || controls_.size() > kCreativeTerrainControlCapacity) {
    return false;
  }
  for (std::size_t index = 0; index < controls_.size(); ++index) {
    if (!isValidCreativeTerrainControlPoint(controls_[index]) ||
        (index > 0U &&
         !coordLess(controls_[index - 1U].coord, controls_[index].coord))) {
      return false;
    }
  }
  return true;
}

std::uint64_t CreativeTerrainField::revision() const noexcept {
  return revision_;
}

std::uint64_t CreativeTerrainField::controlCount() const noexcept {
  return controls_.size();
}

std::span<const CreativeTerrainControlPoint> CreativeTerrainField::controls()
    const noexcept {
  return controls_;
}

const CreativeTerrainControlPoint* CreativeTerrainField::controlAt(
    CreativeTerrainCoord2 coord) const noexcept {
  const ConstControlIterator found = findControl(controls_, coord);
  return found == controls_.end() ? nullptr : &*found;
}

CreativeTerrainMutationReceipt CreativeTerrainField::apply(
    std::span<const CreativeTerrainControlEdit> edits) {
  CreativeTerrainMutationReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  receipt.attemptedEditCount = edits.size();
  receipt.controlCountBefore = controls_.size();
  receipt.controlCountAfter = controls_.size();
  if (!validateInvariants()) {
    receipt.status = CreativeTerrainMutationStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_field_invalid";
    return receipt;
  }

  std::vector<CreativeTerrainControlEdit> ordered{edits.begin(), edits.end()};
  for (const CreativeTerrainControlEdit& edit : ordered) {
    if (edit.kind >= CreativeTerrainEditKind::Count ||
        !coordinateAllowsMaximumRadius(edit.control.coord) ||
        (edit.kind == CreativeTerrainEditKind::Upsert &&
         !isValidCreativeTerrainControlPoint(edit.control))) {
      receipt.status = CreativeTerrainMutationStatus::InvalidEdit;
      receipt.reasonCode = "creative_terrain_edit_invalid";
      return receipt;
    }
  }
  std::sort(ordered.begin(), ordered.end(), editLess);
  for (std::size_t index = 1U; index < ordered.size(); ++index) {
    if (ordered[index - 1U].control.coord == ordered[index].control.coord) {
      receipt.status = CreativeTerrainMutationStatus::DuplicateCoordinate;
      receipt.reasonCode = "creative_terrain_duplicate_coordinate";
      return receipt;
    }
  }

  std::vector<CreativeTerrainControlPoint> staged = controls_;
  for (const CreativeTerrainControlEdit& edit : ordered) {
    ControlIterator found = findControl(staged, edit.control.coord);
    if (edit.kind == CreativeTerrainEditKind::Remove) {
      if (found != staged.end()) {
        staged.erase(found);
        ++receipt.changedControlCount;
      }
      continue;
    }
    if (found == staged.end()) {
      const ControlIterator insertion = std::lower_bound(
          staged.begin(), staged.end(), edit.control, controlLess);
      staged.insert(insertion, edit.control);
      ++receipt.changedControlCount;
    } else if (!(*found == edit.control)) {
      *found = edit.control;
      ++receipt.changedControlCount;
    }
  }
  if (staged.size() > kCreativeTerrainControlCapacity) {
    receipt.changedControlCount = 0U;
    receipt.status = CreativeTerrainMutationStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_control_capacity_exceeded";
    return receipt;
  }
  if (receipt.changedControlCount == 0U) {
    receipt.accepted = true;
    receipt.status = CreativeTerrainMutationStatus::NoChange;
    receipt.reasonCode = "creative_terrain_no_change";
    return receipt;
  }

  controls_ = std::move(staged);
  ++revision_;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeTerrainMutationStatus::Applied;
  receipt.revisionAfter = revision_;
  receipt.controlCountAfter = controls_.size();
  receipt.reasonCode = "creative_terrain_applied";
  return receipt;
}

void CreativeTerrainField::clear() noexcept {
  controls_.clear();
  revision_ = 0;
  valid_ = true;
}

CreativeTerrainHeightSample sampleCreativeTerrainHeight(
    const CreativeTerrainField& field,
    CreativeTerrainCoord2 coord) noexcept {
  if (!field.validateInvariants()) {
    CreativeTerrainHeightSample invalid;
    invalid.coord = coord;
    return invalid;
  }
  return sampleTerrainHeightUnchecked(field.controls(), coord);
}

CreativeTerrainRaycastReceipt raycastCreativeTerrainField(
    const CreativeTerrainField& field,
    const CreativeTerrainRaycastRequest& request) noexcept {
  CreativeTerrainRaycastReceipt receipt;
  receipt.requested = true;
  if (!field.validateInvariants()) {
    receipt.status = CreativeTerrainRaycastStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_raycast_field_invalid";
    return receipt;
  }
  const double directionLength =
      std::sqrt(request.rayDirection.x * request.rayDirection.x +
                request.rayDirection.y * request.rayDirection.y +
                request.rayDirection.z * request.rayDirection.z);
  if (!isFiniteCreativeVec3(request.rayOrigin) ||
      !isFiniteCreativeVec3(request.rayDirection) ||
      !isFiniteCreativeVec3(request.gridOrigin) ||
      !std::isfinite(request.cellSize) || request.cellSize <= 0.0 ||
      !std::isfinite(request.maxDistance) || request.maxDistance < 0.0 ||
      !std::isfinite(directionLength) || directionLength <= 1.0e-12 ||
      request.maxVisitedCells == 0U) {
    receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
    receipt.reasonCode = "creative_terrain_raycast_request_invalid";
    return receipt;
  }
  receipt.accepted = true;
  if (field.controls().empty()) {
    receipt.status = CreativeTerrainRaycastStatus::Miss;
    receipt.reasonCode = "creative_terrain_raycast_miss";
    return receipt;
  }

  const CreativeVec3 direction{request.rayDirection.x / directionLength,
                               request.rayDirection.y / directionLength,
                               request.rayDirection.z / directionLength};
  CreativeTerrainCoord2 cell{};
  if (!tryTerrainCell(request.rayOrigin.x, request.rayOrigin.z,
                      request.gridOrigin, request.cellSize, cell)) {
    receipt.accepted = false;
    receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
    receipt.reasonCode = "creative_terrain_raycast_origin_out_of_range";
    return receipt;
  }

  const std::int32_t stepX = direction.x > 0.0 ? 1 : direction.x < 0.0 ? -1 : 0;
  const std::int32_t stepZ = direction.z > 0.0 ? 1 : direction.z < 0.0 ? -1 : 0;
  const double infinity = std::numeric_limits<double>::infinity();
  const auto firstBoundaryDistance = [&](std::int32_t coordinate,
                                         std::int32_t step,
                                         double axisOrigin,
                                         double axisDirection,
                                         double gridAxisOrigin) {
    if (step == 0) {
      return infinity;
    }
    const double boundary =
        gridAxisOrigin +
        (static_cast<double>(coordinate) + (step > 0 ? 1.0 : 0.0)) *
            request.cellSize;
    return std::max(0.0, (boundary - axisOrigin) / axisDirection);
  };
  double nextX = firstBoundaryDistance(cell.x, stepX, request.rayOrigin.x,
                                       direction.x, request.gridOrigin.x);
  double nextZ = firstBoundaryDistance(cell.z, stepZ, request.rayOrigin.z,
                                       direction.z, request.gridOrigin.z);
  const double deltaX =
      stepX == 0 ? infinity : request.cellSize / std::fabs(direction.x);
  const double deltaZ =
      stepZ == 0 ? infinity : request.cellSize / std::fabs(direction.z);

  while (receipt.visitedCellCount < request.maxVisitedCells) {
    ++receipt.visitedCellCount;
    const CreativeTerrainHeightSample sample =
        sampleTerrainHeightUnchecked(field.controls(), cell);
    if (sample.present) {
      const CreativeVec3 minimum{
          request.gridOrigin.x + static_cast<double>(cell.x) * request.cellSize,
          request.gridOrigin.y,
          request.gridOrigin.z + static_cast<double>(cell.z) * request.cellSize};
      const CreativeVec3 maximum{
          minimum.x + request.cellSize,
          minimum.y + static_cast<double>(sample.heightCells) * request.cellSize,
          minimum.z + request.cellSize};
      const TerrainAabbHit hit = intersectTerrainColumn(
          request.rayOrigin, direction, minimum, maximum, request.maxDistance);
      if (hit.hit) {
        receipt.hit = true;
        receipt.startInside = hit.startInside;
        receipt.status = CreativeTerrainRaycastStatus::Hit;
        receipt.cell = cell;
        receipt.heightCells = sample.heightCells;
        receipt.distance = hit.distance;
        receipt.hitPoint = {request.rayOrigin.x + direction.x * hit.distance,
                            request.rayOrigin.y + direction.y * hit.distance,
                            request.rayOrigin.z + direction.z * hit.distance};
        receipt.faceNormal = hit.normal;
        receipt.reasonCode = "creative_terrain_raycast_hit";
        return receipt;
      }
    }

    const double nextDistance = std::min(nextX, nextZ);
    if (!std::isfinite(nextDistance) || nextDistance > request.maxDistance) {
      receipt.status = CreativeTerrainRaycastStatus::Miss;
      receipt.reasonCode = "creative_terrain_raycast_miss";
      return receipt;
    }
    if (nextX < nextZ) {
      if ((stepX > 0 && cell.x == std::numeric_limits<std::int32_t>::max()) ||
          (stepX < 0 && cell.x == std::numeric_limits<std::int32_t>::min())) {
        receipt.accepted = false;
        receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
        receipt.reasonCode = "creative_terrain_raycast_cell_overflow";
        return receipt;
      }
      cell.x += stepX;
      nextX += deltaX;
    } else if (nextZ < nextX) {
      if ((stepZ > 0 && cell.z == std::numeric_limits<std::int32_t>::max()) ||
          (stepZ < 0 && cell.z == std::numeric_limits<std::int32_t>::min())) {
        receipt.accepted = false;
        receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
        receipt.reasonCode = "creative_terrain_raycast_cell_overflow";
        return receipt;
      }
      cell.z += stepZ;
      nextZ += deltaZ;
    } else {
      if ((stepX > 0 && cell.x == std::numeric_limits<std::int32_t>::max()) ||
          (stepX < 0 && cell.x == std::numeric_limits<std::int32_t>::min()) ||
          (stepZ > 0 && cell.z == std::numeric_limits<std::int32_t>::max()) ||
          (stepZ < 0 && cell.z == std::numeric_limits<std::int32_t>::min())) {
        receipt.accepted = false;
        receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
        receipt.reasonCode = "creative_terrain_raycast_cell_overflow";
        return receipt;
      }
      cell.x += stepX;
      cell.z += stepZ;
      nextX += deltaX;
      nextZ += deltaZ;
    }
  }

  receipt.accepted = false;
  receipt.status = CreativeTerrainRaycastStatus::TraversalLimitExceeded;
  receipt.reasonCode = "creative_terrain_raycast_traversal_limit";
  return receipt;
}

CreativeTerrainSurfacePlan buildCreativeTerrainSurfacePlan(
    const CreativeTerrainField& field) {
  CreativeTerrainSurfacePlan plan;
  plan.requested = true;
  plan.sourceRevision = field.revision();
  if (!field.validateInvariants()) {
    plan.status = CreativeTerrainSurfacePlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_surface_field_invalid";
    return plan;
  }
  if (field.controls().empty()) {
    plan.accepted = true;
    plan.status = CreativeTerrainSurfacePlanStatus::Empty;
    plan.reasonCode = "creative_terrain_surface_empty";
    return plan;
  }

  constexpr std::size_t maximumContributionsPerControl =
      (kCreativeTerrainMaximumRadiusCells * 2U + 1U) *
      (kCreativeTerrainMaximumRadiusCells * 2U + 1U);
  std::vector<TerrainContribution> contributions;
  contributions.reserve(field.controls().size() *
                        maximumContributionsPerControl);
  for (const CreativeTerrainControlPoint& control : field.controls()) {
    const std::int32_t radius = control.radiusCells;
    const std::int32_t radiusSquared = radius * radius;
    for (std::int32_t dz = -radius; dz <= radius; ++dz) {
      for (std::int32_t dx = -radius; dx <= radius; ++dx) {
        const std::int32_t distanceSquared = dx * dx + dz * dz;
        if (distanceSquared > radiusSquared) {
          continue;
        }
        const std::uint32_t weight = terrainContributionWeight(
            control, {control.coord.x + dx, control.coord.z + dz});
        contributions.push_back(
            {{control.coord.x + dx, control.coord.z + dz}, weight,
             static_cast<std::uint64_t>(weight) * control.heightCells});
      }
    }
  }
  plan.contributionCount = contributions.size();
  std::sort(contributions.begin(), contributions.end(), contributionLess);

  plan.columns.reserve(contributions.size());
  for (std::size_t begin = 0U; begin < contributions.size();) {
    std::size_t end = begin + 1U;
    std::uint64_t totalWeight = contributions[begin].weight;
    std::uint64_t weightedHeight = contributions[begin].weightedHeight;
    while (end < contributions.size() &&
           contributions[end].coord == contributions[begin].coord) {
      totalWeight += contributions[end].weight;
      weightedHeight += contributions[end].weightedHeight;
      ++end;
    }
    const std::uint64_t roundedHeight =
        (weightedHeight + totalWeight / 2U) / totalWeight;
    plan.columns.push_back(
        {contributions[begin].coord,
         static_cast<std::uint16_t>(std::clamp<std::uint64_t>(
             roundedHeight, kCreativeTerrainMinimumHeightCells,
             kCreativeTerrainMaximumHeightCells))});
    begin = end;
  }

  for (std::size_t begin = 0U; begin < plan.columns.size();) {
    std::size_t end = begin + 1U;
    while (end < plan.columns.size() &&
           plan.columns[end].coord.z == plan.columns[begin].coord.z) {
      ++end;
    }
    appendRowCuboids(
        std::span<const CreativeTerrainColumn>{plan.columns.data() + begin,
                                               end - begin},
        plan.cuboids);
    begin = end;
  }

  plan.accepted = true;
  plan.status = CreativeTerrainSurfacePlanStatus::Ready;
  plan.reasonCode = "creative_terrain_surface_ready";
  return plan;
}

CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainField& field,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  return buildCreativeTerrainRenderPlan(
      buildCreativeTerrainSurfacePlan(field), gridOrigin, cellSize,
      maxPatchCount);
}

static CreativeTerrainRenderPlan buildCreativeTerrainRenderPlanWithMaterials(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainMaterialField* materials,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  CreativeTerrainRenderPlan plan;
  plan.requested = true;
  plan.sourceRevision = surface.sourceRevision;
  plan.sourceMaterialRevision = materials == nullptr ? 0U : materials->revision();
  if (surface.status == CreativeTerrainSurfacePlanStatus::InvalidField) {
    plan.status = CreativeTerrainRenderPlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_render_field_invalid";
    return plan;
  }
  if (!isFiniteCreativeVec3(gridOrigin) || !std::isfinite(cellSize) ||
      cellSize <= 0.0 || maxPatchCount == 0U) {
    plan.status = CreativeTerrainRenderPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_render_request_invalid";
    return plan;
  }

  if (!surface.accepted) {
    plan.status = CreativeTerrainRenderPlanStatus::SurfacePlanFailed;
    plan.reasonCode = "creative_terrain_render_surface_plan_failed";
    return plan;
  }
  plan.sourceColumnCount = surface.columns.size();
  if (surface.columns.empty()) {
    plan.accepted = true;
    plan.status = CreativeTerrainRenderPlanStatus::Empty;
    plan.reasonCode = "creative_terrain_render_empty";
    return plan;
  }
  if (surface.columns.size() > maxPatchCount) {
    plan.status = CreativeTerrainRenderPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_render_patch_capacity_exceeded";
    return plan;
  }

  plan.patches.reserve(surface.columns.size());
  const std::span<const CreativeTerrainColumn> columns = surface.columns;
  for (const CreativeTerrainColumn& column : columns) {
    const std::int64_t x = column.coord.x;
    const std::int64_t z = column.coord.z;
    const std::array<std::array<std::int64_t, 2U>, 4U> corners{{
        {x, z},
        {x + 1, z},
        {x + 1, z + 1},
        {x, z + 1},
    }};
    CreativeTerrainSurfacePatch patch;
    patch.coord = column.coord;
    patch.material = materials == nullptr
                         ? CreativeTerrainMaterial::Grass
                         : materials->materialAt(column.coord);
    patch.center = {
        gridOrigin.x + (static_cast<double>(x) + 0.5) * cellSize,
        gridOrigin.y + static_cast<double>(column.heightCells) * cellSize,
        gridOrigin.z + (static_cast<double>(z) + 0.5) * cellSize,
    };
    if (!isFiniteCreativeVec3(patch.center)) {
      plan.patches.clear();
      plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
      plan.reasonCode = "creative_terrain_render_arithmetic_overflow";
      return plan;
    }
    for (std::size_t index = 0U; index < corners.size(); ++index) {
      const std::int64_t cornerX = corners[index][0];
      const std::int64_t cornerZ = corners[index][1];
      patch.corners[index] = {
          gridOrigin.x + static_cast<double>(cornerX) * cellSize,
          gridOrigin.y +
              terrainCornerHeightCells(columns, cornerX, cornerZ) * cellSize,
          gridOrigin.z + static_cast<double>(cornerZ) * cellSize,
      };
      if (!isFiniteCreativeVec3(patch.corners[index])) {
        plan.patches.clear();
        plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
        plan.reasonCode = "creative_terrain_render_arithmetic_overflow";
        return plan;
      }
    }
    plan.patches.push_back(patch);
  }

  plan.accepted = true;
  plan.status = CreativeTerrainRenderPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_render_ready";
  return plan;
}

CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainSurfacePlan& surface,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  return buildCreativeTerrainRenderPlanWithMaterials(
      surface, nullptr, gridOrigin, cellSize, maxPatchCount);
}

CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainMaterialField& materials,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  return buildCreativeTerrainRenderPlanWithMaterials(
      surface, &materials, gridOrigin, cellSize, maxPatchCount);
}

CreativeTerrainMutationPreviewReceipt buildCreativeTerrainMutationPreview(
    const CreativeTerrainField& field,
    std::span<const CreativeTerrainControlEdit> edits,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  CreativeTerrainMutationPreviewReceipt receipt;
  receipt.requested = true;
  if (!field.validateInvariants()) {
    receipt.status = CreativeTerrainMutationPreviewStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_preview_field_invalid";
    return receipt;
  }

  CreativeTerrainField previewField = field;
  if (!edits.empty()) {
    receipt.mutation = previewField.apply(edits);
    if (!receipt.mutation.accepted) {
      receipt.status =
          CreativeTerrainMutationPreviewStatus::MutationRejected;
      receipt.reasonCode = "creative_terrain_preview_mutation_rejected";
      return receipt;
    }
  }
  receipt.render = buildCreativeTerrainRenderPlan(
      previewField, gridOrigin, cellSize, maxPatchCount);
  if (!receipt.render.accepted) {
    receipt.status = CreativeTerrainMutationPreviewStatus::RenderRejected;
    receipt.reasonCode = "creative_terrain_preview_render_rejected";
    return receipt;
  }
  receipt.accepted = true;
  receipt.status = CreativeTerrainMutationPreviewStatus::Ready;
  receipt.reasonCode = "creative_terrain_preview_ready";
  return receipt;
}

}  // namespace iggy3d::creative
