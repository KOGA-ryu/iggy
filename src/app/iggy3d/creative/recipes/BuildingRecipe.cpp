#include "app/iggy3d/creative/recipes/BuildingRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

enum class WallAxis : std::uint8_t {
  X,
  Z,
};

struct WallFrame {
  WallAxis axis = WallAxis::X;
  double startScalar = 0.0;
  double direction = 1.0;
  double constant = 0.0;
  double baseY = 0.0;
  double length = 0.0;
  double height = 0.0;
  double thickness = 0.0;
};

struct OrderedOpening {
  const CreativeBuildingOpeningSpec* opening = nullptr;
  std::size_t sourceIndex = 0U;
  double minimum = 0.0;
  double maximum = 0.0;
};

void setStatus(CreativeBuildingRecipeReceipt& receipt,
               CreativeBuildingRecipeStatus status,
               std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
}

[[nodiscard]] bool near(double lhs, double rhs) noexcept {
  return std::abs(lhs - rhs) <= kGeometryEpsilon;
}

[[nodiscard]] bool positiveFinite(double value) noexcept {
  return std::isfinite(value) && value > kGeometryEpsilon;
}

[[nodiscard]] bool nonNegativeFinite(double value) noexcept {
  return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool validBounds(const CreativeBounds& bounds) noexcept {
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  return metrics.valid && isPositiveCreativeVec3(metrics.size);
}

[[nodiscard]] bool allowedBoxKind(CreativeObjectKind kind) noexcept {
  return kind == CreativeObjectKind::Room ||
         kind == CreativeObjectKind::Floor ||
         kind == CreativeObjectKind::Ceiling ||
         kind == CreativeObjectKind::Roof;
}

[[nodiscard]] bool openingPoseIsOpen(
    CreativeBuildingOpeningPose pose) noexcept {
  return pose != CreativeBuildingOpeningPose::Closed;
}

[[nodiscard]] bool validOpeningKind(
    CreativeBuildingOpeningKind kind) noexcept {
  return kind == CreativeBuildingOpeningKind::Door ||
         kind == CreativeBuildingOpeningKind::Window;
}

[[nodiscard]] bool validOpeningPose(
    CreativeBuildingOpeningPose pose) noexcept {
  switch (pose) {
    case CreativeBuildingOpeningPose::Closed:
    case CreativeBuildingOpeningPose::OpenFromStartNegativeNormal:
    case CreativeBuildingOpeningPose::OpenFromStartPositiveNormal:
    case CreativeBuildingOpeningPose::OpenFromEndNegativeNormal:
    case CreativeBuildingOpeningPose::OpenFromEndPositiveNormal:
      return true;
  }
  return false;
}

[[nodiscard]] bool validRootMode(CreativeBuildingRootMode mode) noexcept {
  return mode == CreativeBuildingRootMode::None ||
         mode == CreativeBuildingRootMode::CreateRoom ||
         mode == CreativeBuildingRootMode::ExistingRoom;
}

[[nodiscard]] bool openingPoseUsesStart(
    CreativeBuildingOpeningPose pose) noexcept {
  return pose == CreativeBuildingOpeningPose::OpenFromStartNegativeNormal ||
         pose == CreativeBuildingOpeningPose::OpenFromStartPositiveNormal;
}

[[nodiscard]] double openingPoseNormalSign(
    CreativeBuildingOpeningPose pose) noexcept {
  return pose == CreativeBuildingOpeningPose::OpenFromStartNegativeNormal ||
                 pose == CreativeBuildingOpeningPose::OpenFromEndNegativeNormal
             ? -1.0
             : 1.0;
}

[[nodiscard]] std::optional<WallFrame> wallFrame(
    const CreativeBuildingWallSpec& wall) noexcept {
  if (!isFiniteCreativeVec3(wall.start) || !isFiniteCreativeVec3(wall.end) ||
      !near(wall.start.y, wall.end.y) ||
      !positiveFinite(wall.heightMeters) ||
      !positiveFinite(wall.thicknessMeters)) {
    return std::nullopt;
  }

  WallFrame frame;
  frame.baseY = wall.start.y;
  frame.height = wall.heightMeters;
  frame.thickness = wall.thicknessMeters;
  if (near(wall.start.z, wall.end.z) && !near(wall.start.x, wall.end.x)) {
    frame.axis = WallAxis::X;
    frame.startScalar = wall.start.x;
    frame.direction = wall.end.x > wall.start.x ? 1.0 : -1.0;
    frame.constant = wall.start.z;
    frame.length = std::abs(wall.end.x - wall.start.x);
    return frame;
  }
  if (near(wall.start.x, wall.end.x) && !near(wall.start.z, wall.end.z)) {
    frame.axis = WallAxis::Z;
    frame.startScalar = wall.start.z;
    frame.direction = wall.end.z > wall.start.z ? 1.0 : -1.0;
    frame.constant = wall.start.x;
    frame.length = std::abs(wall.end.z - wall.start.z);
    return frame;
  }
  return std::nullopt;
}

[[nodiscard]] CreativeBounds spanBounds(const WallFrame& frame,
                                         double minimumOffset,
                                         double maximumOffset,
                                         double bottom,
                                         double top,
                                         double thickness) noexcept {
  const double first = frame.startScalar + frame.direction * minimumOffset;
  const double second = frame.startScalar + frame.direction * maximumOffset;
  if (frame.axis == WallAxis::X) {
    return {{std::min(first, second), frame.baseY + bottom,
             frame.constant - thickness * 0.5},
            {std::max(first, second), frame.baseY + top,
             frame.constant + thickness * 0.5}};
  }
  return {{frame.constant - thickness * 0.5, frame.baseY + bottom,
           std::min(first, second)},
          {frame.constant + thickness * 0.5, frame.baseY + top,
           std::max(first, second)}};
}

[[nodiscard]] CreativeBounds openDoorBounds(
    const WallFrame& frame,
    const OrderedOpening& ordered,
    double insertBottom,
    double insertHeight,
    double insertWidth,
    double insertThickness,
    CreativeBuildingOpeningPose pose) noexcept {
  const double hingeOffset = openingPoseUsesStart(pose)
                                 ? ordered.minimum + insertThickness * 0.5
                                 : ordered.maximum - insertThickness * 0.5;
  const double hinge =
      frame.startScalar + frame.direction * hingeOffset;
  const double normalEnd =
      frame.constant + openingPoseNormalSign(pose) * insertWidth;
  if (frame.axis == WallAxis::X) {
    return {{hinge - insertThickness * 0.5,
             frame.baseY + insertBottom,
             std::min(frame.constant, normalEnd)},
            {hinge + insertThickness * 0.5,
             frame.baseY + insertBottom + insertHeight,
             std::max(frame.constant, normalEnd)}};
  }
  return {{std::min(frame.constant, normalEnd),
           frame.baseY + insertBottom,
           hinge - insertThickness * 0.5},
          {std::max(frame.constant, normalEnd),
           frame.baseY + insertBottom + insertHeight,
           hinge + insertThickness * 0.5}};
}

[[nodiscard]] CreativeDocumentCreateRequest makeBoxRequest(
    CreativeObjectKind kind,
    std::string name,
    CreativeBounds bounds,
    bool visible,
    const std::vector<std::string>& tags) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  const CreativeObjectDescriptor& descriptor = describeObject(kind);
  if (descriptor.hasTransform) {
    request.transform.position = measureCreativeBounds(bounds).center;
    request.hasTransformOverride = true;
  }
  request.visible = visible;
  request.hasVisibleOverride = true;
  request.tags = tags;
  return request;
}

void setRecipeParent(CreativeRecipeObjectPlan& object,
                     const CreativeBuildingRecipeRequest& request,
                     bool hasCreatedRoot) {
  if (hasCreatedRoot) {
    object.parentObjectIndex = 0U;
  } else if (request.rootMode == CreativeBuildingRootMode::ExistingRoom) {
    object.createRequest.parentId = request.existingRoomObjectId;
  }
}

void appendGeneratedObject(CreativeBuildingRecipeResult& result,
                           const CreativeBuildingRecipeRequest& request,
                           CreativeObjectKind kind,
                           std::string stableKey,
                           std::string name,
                           CreativeBounds bounds,
                           bool hasCreatedRoot) {
  CreativeRecipeObjectPlan object;
  object.createRequest = makeBoxRequest(kind, std::move(name), bounds,
                                        request.visible, request.tags);
  object.role = CreativeRecipeObjectRole::Generated;
  object.stableKey = std::move(stableKey);
  setRecipeParent(object, request, hasCreatedRoot);
  result.plan.objects.push_back(std::move(object));
  ++result.receipt.generatedObjectCount;
  if (kind == CreativeObjectKind::Wall) {
    ++result.receipt.wallObjectCount;
  } else if (kind == CreativeObjectKind::Door) {
    ++result.receipt.doorObjectCount;
  } else if (kind == CreativeObjectKind::Window) {
    ++result.receipt.windowObjectCount;
  } else {
    ++result.receipt.boxObjectCount;
  }
}

[[nodiscard]] std::string segmentName(const CreativeBuildingWallSpec& wall,
                                      std::size_t segmentIndex) {
  if (!wall.segmentNames.empty()) {
    return wall.segmentNames[segmentIndex];
  }
  if (wall.openings.empty()) {
    return wall.name;
  }
  return wall.name + " Segment " + std::to_string(segmentIndex + 1U);
}

[[nodiscard]] std::vector<OrderedOpening> orderedOpenings(
    const CreativeBuildingWallSpec& wall) {
  std::vector<OrderedOpening> ordered;
  ordered.reserve(wall.openings.size());
  for (std::size_t index = 0; index < wall.openings.size(); ++index) {
    const CreativeBuildingOpeningSpec& opening = wall.openings[index];
    ordered.push_back({&opening,
                       index,
                       opening.centerOffsetMeters - opening.widthMeters * 0.5,
                       opening.centerOffsetMeters + opening.widthMeters * 0.5});
  }
  std::sort(ordered.begin(), ordered.end(), [](const OrderedOpening& lhs,
                                                const OrderedOpening& rhs) {
    if (!near(lhs.minimum, rhs.minimum)) {
      return lhs.minimum < rhs.minimum;
    }
    return lhs.opening->stableKey < rhs.opening->stableKey;
  });
  return ordered;
}

[[nodiscard]] bool validateOpening(const OrderedOpening& ordered,
                                   const WallFrame& frame) noexcept {
  const CreativeBuildingOpeningSpec& opening = *ordered.opening;
  if (!validOpeningKind(opening.kind) || !validOpeningPose(opening.pose) ||
      opening.stableKey.empty() || opening.name.empty() ||
      !positiveFinite(opening.widthMeters) ||
      !nonNegativeFinite(opening.centerOffsetMeters) ||
      !nonNegativeFinite(opening.cutoutBottomMeters) ||
      !positiveFinite(opening.cutoutHeightMeters) ||
      ordered.minimum <= kGeometryEpsilon ||
      ordered.maximum >= frame.length - kGeometryEpsilon ||
      opening.cutoutBottomMeters + opening.cutoutHeightMeters >
          frame.height + kGeometryEpsilon) {
    return false;
  }
  if (opening.kind == CreativeBuildingOpeningKind::Window &&
      openingPoseIsOpen(opening.pose)) {
    return false;
  }
  if (!opening.includeInsert) {
    return true;
  }

  const double insertHeight = opening.insertHeightMeters > 0.0
                                  ? opening.insertHeightMeters
                                  : opening.cutoutHeightMeters;
  const double insertWidth = opening.insertWidthMeters > 0.0
                                 ? opening.insertWidthMeters
                                 : opening.widthMeters;
  const double insertThickness = opening.insertThicknessMeters > 0.0
                                     ? opening.insertThicknessMeters
                                     : frame.thickness;
  return nonNegativeFinite(opening.insertBottomMeters) &&
         positiveFinite(insertHeight) && positiveFinite(insertWidth) &&
         positiveFinite(insertThickness) &&
         insertWidth <= opening.widthMeters + kGeometryEpsilon &&
         opening.insertBottomMeters + insertHeight <=
             opening.cutoutBottomMeters + opening.cutoutHeightMeters +
                 kGeometryEpsilon &&
         opening.insertBottomMeters + kGeometryEpsilon >=
             opening.cutoutBottomMeters;
}

[[nodiscard]] bool appendWall(CreativeBuildingRecipeResult& result,
                              const CreativeBuildingRecipeRequest& request,
                              const CreativeBuildingWallSpec& wall,
                              const WallFrame& frame,
                              bool hasCreatedRoot) {
  std::vector<OrderedOpening> openings = orderedOpenings(wall);
  if (!wall.segmentNames.empty() &&
      wall.segmentNames.size() != openings.size() + 1U) {
    setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidWall,
              "creative_building_wall_segment_names_invalid");
    return false;
  }
  for (std::size_t index = 0; index < openings.size(); ++index) {
    result.receipt.failedOpeningIndex = openings[index].sourceIndex;
    if (!validateOpening(openings[index], frame)) {
      setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidOpening,
                "creative_building_opening_invalid");
      return false;
    }
    if (index > 0U && openings[index].minimum <=
                          openings[index - 1U].maximum + kGeometryEpsilon) {
      setStatus(result.receipt,
                CreativeBuildingRecipeStatus::OverlappingOpenings,
                "creative_building_openings_overlap");
      return false;
    }
  }

  double cursor = 0.0;
  std::size_t segmentIndex = 0U;
  for (const OrderedOpening& ordered : openings) {
    const CreativeBuildingOpeningSpec& opening = *ordered.opening;
    appendGeneratedObject(
        result, request, CreativeObjectKind::Wall,
        wall.stableKey + ".segment." + std::to_string(segmentIndex + 1U),
        segmentName(wall, segmentIndex),
        spanBounds(frame, cursor, ordered.minimum, 0.0, frame.height,
                   frame.thickness),
        hasCreatedRoot);
    ++segmentIndex;

    if (opening.cutoutBottomMeters > kGeometryEpsilon) {
      appendGeneratedObject(
          result, request, CreativeObjectKind::Wall,
          opening.stableKey + ".sill", opening.name + " Sill",
          spanBounds(frame, ordered.minimum, ordered.maximum, 0.0,
                     opening.cutoutBottomMeters, frame.thickness),
          hasCreatedRoot);
    }
    const double cutoutTop =
        opening.cutoutBottomMeters + opening.cutoutHeightMeters;
    if (cutoutTop < frame.height - kGeometryEpsilon) {
      appendGeneratedObject(
          result, request, CreativeObjectKind::Wall,
          opening.stableKey + ".lintel", opening.name + " Lintel",
          spanBounds(frame, ordered.minimum, ordered.maximum, cutoutTop,
                     frame.height, frame.thickness),
          hasCreatedRoot);
    }

    if (opening.includeInsert) {
      const double insertHeight = opening.insertHeightMeters > 0.0
                                      ? opening.insertHeightMeters
                                      : opening.cutoutHeightMeters;
      const double insertWidth = opening.insertWidthMeters > 0.0
                                     ? opening.insertWidthMeters
                                     : opening.widthMeters;
      const double insertThickness = opening.insertThicknessMeters > 0.0
                                         ? opening.insertThicknessMeters
                                         : frame.thickness;
      CreativeBounds insertBounds = spanBounds(
          frame, opening.centerOffsetMeters - insertWidth * 0.5,
          opening.centerOffsetMeters + insertWidth * 0.5,
          opening.insertBottomMeters,
          opening.insertBottomMeters + insertHeight, insertThickness);
      if (openingPoseIsOpen(opening.pose)) {
        insertBounds = openDoorBounds(frame, ordered,
                                      opening.insertBottomMeters,
                                      insertHeight, insertWidth,
                                      insertThickness, opening.pose);
      }
      const CreativeObjectKind kind =
          opening.kind == CreativeBuildingOpeningKind::Door
              ? CreativeObjectKind::Door
              : CreativeObjectKind::Window;
      appendGeneratedObject(result, request, kind,
                            opening.stableKey + ".insert", opening.name,
                            insertBounds, hasCreatedRoot);
    }
    cursor = ordered.maximum;
  }

  appendGeneratedObject(
      result, request, CreativeObjectKind::Wall,
      wall.stableKey + ".segment." + std::to_string(segmentIndex + 1U),
      segmentName(wall, segmentIndex),
      spanBounds(frame, cursor, frame.length, 0.0, frame.height,
                 frame.thickness),
      hasCreatedRoot);
  return true;
}

}  // namespace

std::string_view toString(CreativeBuildingRootMode mode) noexcept {
  switch (mode) {
    case CreativeBuildingRootMode::None:
      return "None";
    case CreativeBuildingRootMode::CreateRoom:
      return "CreateRoom";
    case CreativeBuildingRootMode::ExistingRoom:
      return "ExistingRoom";
  }
  return "Unknown";
}

std::string_view toString(CreativeBuildingOpeningKind kind) noexcept {
  switch (kind) {
    case CreativeBuildingOpeningKind::Door:
      return "Door";
    case CreativeBuildingOpeningKind::Window:
      return "Window";
  }
  return "Unknown";
}

std::string_view toString(CreativeBuildingOpeningPose pose) noexcept {
  switch (pose) {
    case CreativeBuildingOpeningPose::Closed:
      return "Closed";
    case CreativeBuildingOpeningPose::OpenFromStartNegativeNormal:
      return "OpenFromStartNegativeNormal";
    case CreativeBuildingOpeningPose::OpenFromStartPositiveNormal:
      return "OpenFromStartPositiveNormal";
    case CreativeBuildingOpeningPose::OpenFromEndNegativeNormal:
      return "OpenFromEndNegativeNormal";
    case CreativeBuildingOpeningPose::OpenFromEndPositiveNormal:
      return "OpenFromEndPositiveNormal";
  }
  return "Unknown";
}

std::string_view toString(CreativeBuildingRecipeStatus status) noexcept {
  switch (status) {
    case CreativeBuildingRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeBuildingRecipeStatus::InvalidRoot:
      return "InvalidRoot";
    case CreativeBuildingRecipeStatus::Empty:
      return "Empty";
    case CreativeBuildingRecipeStatus::InvalidBox:
      return "InvalidBox";
    case CreativeBuildingRecipeStatus::InvalidWall:
      return "InvalidWall";
    case CreativeBuildingRecipeStatus::UnsupportedWallOrientation:
      return "UnsupportedWallOrientation";
    case CreativeBuildingRecipeStatus::InvalidOpening:
      return "InvalidOpening";
    case CreativeBuildingRecipeStatus::OverlappingOpenings:
      return "OverlappingOpenings";
    case CreativeBuildingRecipeStatus::InvalidPlan:
      return "InvalidPlan";
    case CreativeBuildingRecipeStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeBuildingOpeningSpec makeCreativeBuildingDoorOpening(
    std::string stableKey,
    std::string name,
    double centerOffsetMeters,
    double widthMeters,
    double heightMeters) {
  CreativeBuildingOpeningSpec opening;
  opening.kind = CreativeBuildingOpeningKind::Door;
  opening.stableKey = std::move(stableKey);
  opening.name = std::move(name);
  opening.centerOffsetMeters = centerOffsetMeters;
  opening.widthMeters = widthMeters;
  opening.cutoutHeightMeters = heightMeters;
  return opening;
}

CreativeBuildingOpeningSpec makeCreativeBuildingWindowOpening(
    std::string stableKey,
    std::string name,
    double centerOffsetMeters,
    double widthMeters,
    double sillHeightMeters,
    double heightMeters) {
  CreativeBuildingOpeningSpec opening;
  opening.kind = CreativeBuildingOpeningKind::Window;
  opening.stableKey = std::move(stableKey);
  opening.name = std::move(name);
  opening.centerOffsetMeters = centerOffsetMeters;
  opening.widthMeters = widthMeters;
  opening.cutoutBottomMeters = sillHeightMeters;
  opening.cutoutHeightMeters = heightMeters;
  opening.insertBottomMeters = sillHeightMeters;
  return opening;
}

CreativeBuildingRecipeResult buildCreativeBuildingRecipe(
    const CreativeBuildingRecipeRequest& request) {
  CreativeBuildingRecipeResult result;
  result.receipt.requested = true;
  result.plan.kind = CreativeRecipeKind::Building;
  result.plan.instanceKey = request.stableKey;
  result.plan.instanceName = request.name;

  const bool createsRoot =
      request.rootMode == CreativeBuildingRootMode::CreateRoom;
  if (!validRootMode(request.rootMode) ||
      (createsRoot &&
       (request.existingRoomObjectId != kInvalidObjectId ||
        !validBounds(request.rootBounds))) ||
      (request.rootMode == CreativeBuildingRootMode::ExistingRoom &&
       request.existingRoomObjectId == kInvalidObjectId) ||
      (request.rootMode == CreativeBuildingRootMode::None &&
       request.existingRoomObjectId != kInvalidObjectId)) {
    setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidRoot,
              "creative_building_root_invalid");
    return result;
  }
  if (request.boxes.empty() && request.walls.empty()) {
    setStatus(result.receipt, CreativeBuildingRecipeStatus::Empty,
              "creative_building_recipe_empty");
    return result;
  }

  if (createsRoot) {
    CreativeRecipeObjectPlan root;
    root.createRequest = makeBoxRequest(
        CreativeObjectKind::Room,
        request.name.empty() ? std::string{"Building"} : request.name,
        request.rootBounds, request.visible, request.tags);
    root.role = CreativeRecipeObjectRole::Source;
    root.stableKey = "root";
    result.plan.objects.push_back(std::move(root));
    result.receipt.rootObjectCount = 1U;
  }

  for (std::size_t index = 0; index < request.boxes.size(); ++index) {
    result.receipt.failedBoxIndex = index;
    const CreativeBuildingBoxSpec& box = request.boxes[index];
    if (!allowedBoxKind(box.kind) || box.stableKey.empty() ||
        box.name.empty() ||
        !validBounds(box.bounds)) {
      setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidBox,
                "creative_building_box_invalid");
      result.plan.objects.clear();
      return result;
    }
    appendGeneratedObject(result, request, box.kind, box.stableKey, box.name,
                          box.bounds, createsRoot);
  }

  for (std::size_t index = 0; index < request.walls.size(); ++index) {
    result.receipt.failedWallIndex = index;
    const CreativeBuildingWallSpec& wall = request.walls[index];
    if (wall.stableKey.empty() || wall.name.empty()) {
      setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidWall,
                "creative_building_wall_invalid");
      result.plan.objects.clear();
      return result;
    }
    const std::optional<WallFrame> frame = wallFrame(wall);
    if (!frame.has_value()) {
      const bool finite = isFiniteCreativeVec3(wall.start) &&
                          isFiniteCreativeVec3(wall.end) &&
                          positiveFinite(wall.heightMeters) &&
                          positiveFinite(wall.thicknessMeters);
      setStatus(result.receipt,
                finite
                    ? CreativeBuildingRecipeStatus::UnsupportedWallOrientation
                    : CreativeBuildingRecipeStatus::InvalidWall,
                finite ? "creative_building_wall_orientation_unsupported"
                       : "creative_building_wall_invalid");
      result.plan.objects.clear();
      return result;
    }
    if (!appendWall(result, request, wall, *frame, createsRoot)) {
      result.plan.objects.clear();
      return result;
    }
  }

  const CreativeRecipeMaterializeResult validated =
      materializeCreativeRecipe(result.plan, 1U);
  if (!validated.receipt.accepted) {
    setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidPlan,
              validated.receipt.reasonCode);
    result.plan.objects.clear();
    return result;
  }

  result.receipt.accepted = true;
  result.receipt.failedBoxIndex = 0U;
  result.receipt.failedWallIndex = 0U;
  result.receipt.failedOpeningIndex = 0U;
  setStatus(result.receipt, CreativeBuildingRecipeStatus::Ready,
            "creative_building_recipe_ready");
  return result;
}

}  // namespace iggy3d::creative
