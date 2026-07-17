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

[[nodiscard]] bool validBounds(const CreativeBounds& bounds) noexcept {
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  return metrics.valid && isPositiveCreativeVec3(metrics.size);
}

void setGeometryStatus(CreativeRectangularRoomGeometryPlan& plan,
                       CreativeRectangularRoomGeometryStatus status,
                       std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool allowedBoxKind(CreativeObjectKind kind) noexcept {
  return kind == CreativeObjectKind::Room ||
         kind == CreativeObjectKind::Floor ||
         kind == CreativeObjectKind::Ceiling ||
         kind == CreativeObjectKind::Roof ||
         kind == CreativeObjectKind::GableRoof ||
         kind == CreativeObjectKind::Stair || kind == CreativeObjectKind::Ramp;
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

[[nodiscard]] bool validRootMode(CreativeBuildingRootMode mode) noexcept {
  return mode == CreativeBuildingRootMode::None ||
         mode == CreativeBuildingRootMode::CreateRoom ||
         mode == CreativeBuildingRootMode::ExistingRoom;
}

[[nodiscard]] CreativeDocumentCreateRequest
makeBoxRequest(CreativeObjectKind kind, std::string name, CreativeBounds bounds,
               bool visible, const std::vector<std::string>& tags,
               CreativeVec3 scale = {1.0, 1.0, 1.0},
               CreativeVec3 rotationEulerRadians = {}) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  const CreativeObjectDescriptor& descriptor = describeObject(kind);
  if (descriptor.hasTransform) {
    request.transform.position = measureCreativeBounds(bounds).center;
    request.transform.scale = scale;
    request.transform.rotationEulerRadians = rotationEulerRadians;
    request.hasTransformOverride = true;
  }
  request.visible = visible;
  request.hasVisibleOverride = true;
  request.tags = tags;
  return request;
}

[[nodiscard]] std::vector<std::string> mergedTags(
    const std::vector<std::string>& common,
    const std::vector<std::string>& specific) {
  std::vector<std::string> output = common;
  output.reserve(common.size() + specific.size());
  for (const std::string& tag : specific) {
    if (std::find(output.begin(), output.end(), tag) == output.end()) {
      output.push_back(tag);
    }
  }
  return output;
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
                           CreativeObjectKind kind, std::string stableKey,
                           std::string name, CreativeBounds bounds,
                           bool hasCreatedRoot,
                           const std::vector<std::string>& specificTags = {},
                           CreativeVec3 scale = {1.0, 1.0, 1.0},
                           CreativeVec3 rotationEulerRadians = {}) {
  CreativeRecipeObjectPlan object;
  const std::vector<std::string> tags =
      mergedTags(request.tags, specificTags);
  object.createRequest =
      makeBoxRequest(kind, std::move(name), bounds, request.visible, tags,
                     scale, rotationEulerRadians);
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

void setWallKernelFailure(CreativeBuildingRecipeReceipt& receipt,
                          const CreativeStructuralWallRecipeResult& geometry) {
  receipt.failedOpeningIndex = geometry.failedOpeningIndex;
  switch (geometry.status) {
    case CreativeStructuralWallRecipeStatus::UnsupportedOrientation:
      setStatus(receipt,
                CreativeBuildingRecipeStatus::UnsupportedWallOrientation,
                "creative_building_wall_orientation_unsupported");
      return;
    case CreativeStructuralWallRecipeStatus::InvalidOpening:
      setStatus(receipt, CreativeBuildingRecipeStatus::InvalidOpening,
                "creative_building_opening_invalid");
      return;
    case CreativeStructuralWallRecipeStatus::OverlappingOpenings:
      setStatus(receipt, CreativeBuildingRecipeStatus::OverlappingOpenings,
                "creative_building_openings_overlap");
      return;
    case CreativeStructuralWallRecipeStatus::NotRequested:
    case CreativeStructuralWallRecipeStatus::InvalidWall:
    case CreativeStructuralWallRecipeStatus::UnrepresentableGeometry:
    case CreativeStructuralWallRecipeStatus::Ready:
      setStatus(receipt, CreativeBuildingRecipeStatus::InvalidWall,
                "creative_building_wall_invalid");
      return;
  }
}

[[nodiscard]] bool appendWall(CreativeBuildingRecipeResult& result,
                              const CreativeBuildingRecipeRequest& request,
                              const CreativeBuildingWallSpec& wall,
                              bool hasCreatedRoot) {
  if (!wall.segmentNames.empty() &&
      wall.segmentNames.size() != wall.openings.size() + 1U) {
    setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidWall,
              "creative_building_wall_segment_names_invalid");
    return false;
  }

  std::vector<CreativeStructuralWallOpeningRequest> openingRequests;
  openingRequests.reserve(wall.openings.size());
  for (std::size_t index = 0U; index < wall.openings.size(); ++index) {
    result.receipt.failedOpeningIndex = index;
    const CreativeBuildingOpeningSpec& opening = wall.openings[index];
    if (!validOpeningKind(opening.kind) ||
        !isCreativeStructuralWallOpeningPoseValid(opening.pose) ||
        opening.stableKey.empty() || opening.name.empty() ||
        (opening.kind == CreativeBuildingOpeningKind::Window &&
         openingPoseIsOpen(opening.pose))) {
      setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidOpening,
                "creative_building_opening_invalid");
      return false;
    }
    openingRequests.push_back(
        {opening.stableKey, opening.centerOffsetMeters, opening.widthMeters,
         opening.cutoutBottomMeters, opening.cutoutHeightMeters,
         opening.includeInsert, opening.insertBottomMeters,
         opening.insertHeightMeters, opening.insertWidthMeters,
         opening.insertThicknessMeters, opening.pose});
  }
  const CreativeStructuralWallRecipeResult geometry =
      planCreativeStructuralWall({wall.start, wall.end, wall.heightMeters,
                                  wall.thicknessMeters, 0.0, 0.0,
                                  openingRequests});
  if (!geometry.accepted) {
    setWallKernelFailure(result.receipt, geometry);
    return false;
  }

  std::size_t segmentIndex = 0U;
  for (const CreativeStructuralWallOpeningPlan& openingPlan :
       geometry.openings) {
    const CreativeBuildingOpeningSpec& opening =
        wall.openings[openingPlan.sourceIndex];
    appendGeneratedObject(
        result, request, CreativeObjectKind::Wall,
        wall.stableKey + ".segment." + std::to_string(segmentIndex + 1U),
        segmentName(wall, segmentIndex), geometry.fullHeightSpans[segmentIndex],
        hasCreatedRoot, wall.tags);
    ++segmentIndex;

    if (openingPlan.hasSill) {
      appendGeneratedObject(result, request, CreativeObjectKind::Wall,
                            opening.stableKey + ".sill", opening.name + " Sill",
                            openingPlan.sillBounds, hasCreatedRoot,
                            opening.tags);
    }
    if (openingPlan.hasLintel) {
      appendGeneratedObject(result, request, CreativeObjectKind::Wall,
                            opening.stableKey + ".lintel",
                            opening.name + " Lintel", openingPlan.lintelBounds,
                            hasCreatedRoot, opening.tags);
    }

    if (openingPlan.hasInsert) {
      const CreativeObjectKind kind =
          opening.kind == CreativeBuildingOpeningKind::Door
              ? CreativeObjectKind::Door
              : CreativeObjectKind::Window;
      appendGeneratedObject(
          result, request, kind, opening.stableKey + ".insert", opening.name,
          openingPlan.insertBounds, hasCreatedRoot, opening.tags);
    }
  }

  appendGeneratedObject(
      result, request, CreativeObjectKind::Wall,
      wall.stableKey + ".segment." + std::to_string(segmentIndex + 1U),
      segmentName(wall, segmentIndex), geometry.fullHeightSpans.back(),
      hasCreatedRoot, wall.tags);
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

std::string_view toString(
    CreativeRectangularRoomGeometryStatus status) noexcept {
  switch (status) {
    case CreativeRectangularRoomGeometryStatus::NotRequested:
      return "NotRequested";
    case CreativeRectangularRoomGeometryStatus::InvalidCorner:
      return "InvalidCorner";
    case CreativeRectangularRoomGeometryStatus::UnevenFloorPlane:
      return "UnevenFloorPlane";
    case CreativeRectangularRoomGeometryStatus::InvalidDimension:
      return "InvalidDimension";
    case CreativeRectangularRoomGeometryStatus::DegenerateFootprint:
      return "DegenerateFootprint";
    case CreativeRectangularRoomGeometryStatus::WallConsumesFootprint:
      return "WallConsumesFootprint";
    case CreativeRectangularRoomGeometryStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeRectangularRoomGeometryPlan planCreativeRectangularRoomGeometry(
    const CreativeRectangularRoomGeometryRequest& request) noexcept {
  CreativeRectangularRoomGeometryPlan plan;
  if (!isFiniteCreativeVec3(request.firstFloorCorner) ||
      !isFiniteCreativeVec3(request.oppositeFloorCorner)) {
    setGeometryStatus(plan,
                      CreativeRectangularRoomGeometryStatus::InvalidCorner,
                      "creative_rectangular_room_corner_invalid");
    return plan;
  }
  if (!near(request.firstFloorCorner.y,
            request.oppositeFloorCorner.y)) {
    setGeometryStatus(
        plan, CreativeRectangularRoomGeometryStatus::UnevenFloorPlane,
        "creative_rectangular_room_floor_plane_uneven");
    return plan;
  }
  if (!positiveFinite(request.wallHeightMeters) ||
      !positiveFinite(request.wallThicknessMeters) ||
      !positiveFinite(request.floorThicknessMeters)) {
    setGeometryStatus(plan,
                      CreativeRectangularRoomGeometryStatus::InvalidDimension,
                      "creative_rectangular_room_dimension_invalid");
    return plan;
  }

  const double minimumX =
      std::min(request.firstFloorCorner.x, request.oppositeFloorCorner.x);
  const double maximumX =
      std::max(request.firstFloorCorner.x, request.oppositeFloorCorner.x);
  const double minimumZ =
      std::min(request.firstFloorCorner.z, request.oppositeFloorCorner.z);
  const double maximumZ =
      std::max(request.firstFloorCorner.z, request.oppositeFloorCorner.z);
  const double width = maximumX - minimumX;
  const double depth = maximumZ - minimumZ;
  if (!positiveFinite(width) || !positiveFinite(depth)) {
    setGeometryStatus(
        plan, CreativeRectangularRoomGeometryStatus::DegenerateFootprint,
        "creative_rectangular_room_footprint_degenerate");
    return plan;
  }
  if (width <= request.wallThicknessMeters * 2.0 + kGeometryEpsilon ||
      depth <= request.wallThicknessMeters * 2.0 + kGeometryEpsilon) {
    setGeometryStatus(
        plan, CreativeRectangularRoomGeometryStatus::WallConsumesFootprint,
        "creative_rectangular_room_wall_consumes_footprint");
    return plan;
  }

  const double floorTop = request.firstFloorCorner.y;
  const double halfWall = request.wallThicknessMeters * 0.5;
  plan.floorBounds = {{minimumX, floorTop - request.floorThicknessMeters,
                       minimumZ},
                      {maximumX, floorTop, maximumZ}};
  plan.rootBounds = {{minimumX - halfWall,
                      floorTop - request.floorThicknessMeters,
                      minimumZ - halfWall},
                     {maximumX + halfWall,
                      floorTop + request.wallHeightMeters,
                      maximumZ + halfWall}};
  plan.wallStarts = {{{minimumX, floorTop, minimumZ},
                      {maximumX, floorTop, minimumZ},
                      {maximumX, floorTop, maximumZ},
                      {minimumX, floorTop, maximumZ}}};
  plan.wallEnds = {{{maximumX, floorTop, minimumZ},
                    {maximumX, floorTop, maximumZ},
                    {minimumX, floorTop, maximumZ},
                    {minimumX, floorTop, minimumZ}}};
  for (std::size_t index = 0U; index < plan.wallBounds.size(); ++index) {
    const CreativeStructuralWallRecipeResult wall =
        planCreativeStructuralWall({plan.wallStarts[index],
                                    plan.wallEnds[index],
                                    request.wallHeightMeters,
                                    request.wallThicknessMeters,
                                    0.0,
                                    0.0,
                                    {}});
    if (!wall.accepted) {
      setGeometryStatus(
          plan, CreativeRectangularRoomGeometryStatus::InvalidDimension,
          "creative_rectangular_room_wall_geometry_unrepresentable");
      return plan;
    }
    plan.wallBounds[index] = wall.frame.bounds;
  }
  plan.accepted = true;
  setGeometryStatus(plan, CreativeRectangularRoomGeometryStatus::Ready,
                    "creative_rectangular_room_geometry_ready");
  return plan;
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
        box.name.empty() || !validBounds(box.bounds) ||
        !isPositiveCreativeVec3(box.scale) ||
        !isFiniteCreativeVec3(box.rotationEulerRadians)) {
      setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidBox,
                "creative_building_box_invalid");
      result.plan.objects.clear();
      return result;
    }
    appendGeneratedObject(result, request, box.kind, box.stableKey, box.name,
                          box.bounds, createsRoot, box.tags, box.scale,
                          box.rotationEulerRadians);
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
    if (!appendWall(result, request, wall, createsRoot)) {
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

CreativeBuildingRecipeResult buildCreativeRectangularRoomRecipe(
    const CreativeRectangularRoomRecipeRequest& request) {
  const CreativeRectangularRoomGeometryPlan geometry =
      planCreativeRectangularRoomGeometry(request.geometry);
  if (!geometry.accepted) {
    CreativeBuildingRecipeResult result;
    result.receipt.requested = true;
    result.receipt.status =
        geometry.status ==
                CreativeRectangularRoomGeometryStatus::WallConsumesFootprint
            ? CreativeBuildingRecipeStatus::InvalidWall
            : CreativeBuildingRecipeStatus::InvalidRoot;
    result.receipt.reasonCode = std::string(geometry.reasonCode);
    result.plan.kind = CreativeRecipeKind::Building;
    result.plan.instanceKey = request.stableKey;
    result.plan.instanceName = request.name;
    return result;
  }

  CreativeBuildingRecipeRequest building;
  building.stableKey = request.stableKey;
  building.name = request.name;
  // The recipe instance and one history transaction own the shell as a unit.
  // Avoid a visible Room box that would fill the usable interior.
  building.rootMode = CreativeBuildingRootMode::None;
  building.visible = request.visible;
  building.tags = request.tags;
  building.boxes.push_back({CreativeObjectKind::Floor,
                            "floor",
                            request.name + " Floor",
                            geometry.floorBounds});
  constexpr std::array<std::string_view, 4U> kWallKeys{
      "wall.north", "wall.east", "wall.south", "wall.west"};
  constexpr std::array<std::string_view, 4U> kWallNames{
      " North Wall", " East Wall", " South Wall", " West Wall"};
  building.walls.reserve(kWallKeys.size());
  for (std::size_t index = 0U; index < kWallKeys.size(); ++index) {
    CreativeBuildingWallSpec wall;
    wall.stableKey = std::string(kWallKeys[index]);
    wall.name = request.name + std::string(kWallNames[index]);
    wall.start = geometry.wallStarts[index];
    wall.end = geometry.wallEnds[index];
    wall.heightMeters = request.geometry.wallHeightMeters;
    wall.thicknessMeters = request.geometry.wallThicknessMeters;
    building.walls.push_back(std::move(wall));
  }
  return buildCreativeBuildingRecipe(building);
}

}  // namespace iggy3d::creative
