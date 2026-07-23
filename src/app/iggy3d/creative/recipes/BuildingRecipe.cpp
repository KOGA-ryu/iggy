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

[[nodiscard]] bool boundsNear(const CreativeBounds& lhs,
                              const CreativeBounds& rhs) noexcept {
  const auto nearComponent = [](double first, double second) {
    return std::abs(first - second) <= 1.0e-8;
  };
  return nearComponent(lhs.min.x, rhs.min.x) &&
         nearComponent(lhs.min.y, rhs.min.y) &&
         nearComponent(lhs.min.z, rhs.min.z) &&
         nearComponent(lhs.max.x, rhs.max.x) &&
         nearComponent(lhs.max.y, rhs.max.y) &&
         nearComponent(lhs.max.z, rhs.max.z);
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
         kind == CreativeObjectKind::HipRoof ||
         kind == CreativeObjectKind::Window ||
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

[[nodiscard]] bool validOpeningFacing(
    CreativeBuildingOpeningFacing facing) noexcept {
  return facing < CreativeBuildingOpeningFacing::Count;
}

[[nodiscard]] double openingPoseNormalSign(
    CreativeBuildingOpeningPose pose) noexcept {
  return pose == CreativeBuildingOpeningPose::OpenFromStartNegativeNormal ||
                 pose == CreativeBuildingOpeningPose::OpenFromEndNegativeNormal
             ? -1.0
             : 1.0;
}

[[nodiscard]] bool validOpeningAsset(
    const CreativeBuildingOpeningSpec& opening) noexcept {
  if (!opening.hasInsertAssetSourceBounds) {
    return opening.insertAssetId.empty();
  }
  const CreativeBoundsMetrics source =
      measureCreativeBounds(opening.insertAssetSourceBoundsMeters);
  return !opening.insertAssetId.empty() && source.valid &&
         isPositiveCreativeVec3(source.size);
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

void appendGeneratedRequest(CreativeBuildingRecipeResult& result,
                            const CreativeBuildingRecipeRequest& request,
                            CreativeObjectKind kind, std::string stableKey,
                            CreativeDocumentCreateRequest createRequest,
                            bool hasCreatedRoot,
                            std::optional<std::size_t> parentObjectIndex =
                                std::nullopt,
                            bool doorPart = false,
                            bool windowPart = false) {
  CreativeRecipeObjectPlan object;
  object.createRequest = std::move(createRequest);
  object.role = CreativeRecipeObjectRole::Generated;
  object.stableKey = std::move(stableKey);
  if (parentObjectIndex.has_value()) {
    object.parentObjectIndex = parentObjectIndex;
  } else {
    setRecipeParent(object, request, hasCreatedRoot);
  }
  result.plan.objects.push_back(std::move(object));
  ++result.receipt.generatedObjectCount;
  if (doorPart) {
    ++result.receipt.doorPartObjectCount;
  }
  if (windowPart) {
    ++result.receipt.windowPartObjectCount;
  }
  if (kind == CreativeObjectKind::Wall && !doorPart && !windowPart) {
    ++result.receipt.wallObjectCount;
  } else if (kind == CreativeObjectKind::Door) {
    ++result.receipt.doorObjectCount;
  } else if (kind == CreativeObjectKind::Window) {
    ++result.receipt.windowObjectCount;
  } else if (!doorPart && !windowPart) {
    ++result.receipt.boxObjectCount;
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
  const std::vector<std::string> tags =
      mergedTags(request.tags, specificTags);
  appendGeneratedRequest(
      result, request, kind, std::move(stableKey),
      makeBoxRequest(kind, std::move(name), bounds, request.visible, tags,
                     scale, rotationEulerRadians),
      hasCreatedRoot);
}

[[nodiscard]] std::string_view doorPartStableSuffix(
    CreativeDoorPartKind kind) noexcept {
  switch (kind) {
    case CreativeDoorPartKind::MinimumJamb:
      return "frame.minimum_jamb";
    case CreativeDoorPartKind::MaximumJamb:
      return "frame.maximum_jamb";
    case CreativeDoorPartKind::Header:
      return "frame.header";
    case CreativeDoorPartKind::PrimaryLeaf:
      return "insert";
    case CreativeDoorPartKind::SecondaryLeaf:
      return "leaf.secondary";
    case CreativeDoorPartKind::PrimaryLowerHinge:
      return "hardware.primary.hinge.lower";
    case CreativeDoorPartKind::PrimaryUpperHinge:
      return "hardware.primary.hinge.upper";
    case CreativeDoorPartKind::SecondaryLowerHinge:
      return "hardware.secondary.hinge.lower";
    case CreativeDoorPartKind::SecondaryUpperHinge:
      return "hardware.secondary.hinge.upper";
    case CreativeDoorPartKind::PrimaryHandle:
      return "hardware.primary.handle";
    case CreativeDoorPartKind::SecondaryHandle:
      return "hardware.secondary.handle";
    case CreativeDoorPartKind::Count:
      break;
  }
  return "invalid";
}

[[nodiscard]] std::string_view doorPartNameSuffix(
    CreativeDoorPartKind kind) noexcept {
  switch (kind) {
    case CreativeDoorPartKind::MinimumJamb:
      return "Minimum Jamb";
    case CreativeDoorPartKind::MaximumJamb:
      return "Maximum Jamb";
    case CreativeDoorPartKind::Header:
      return "Frame Header";
    case CreativeDoorPartKind::PrimaryLeaf:
      return "";
    case CreativeDoorPartKind::SecondaryLeaf:
      return "Secondary Leaf";
    case CreativeDoorPartKind::PrimaryLowerHinge:
      return "Primary Lower Hinge";
    case CreativeDoorPartKind::PrimaryUpperHinge:
      return "Primary Upper Hinge";
    case CreativeDoorPartKind::SecondaryLowerHinge:
      return "Secondary Lower Hinge";
    case CreativeDoorPartKind::SecondaryUpperHinge:
      return "Secondary Upper Hinge";
    case CreativeDoorPartKind::PrimaryHandle:
      return "Primary Handle";
    case CreativeDoorPartKind::SecondaryHandle:
      return "Secondary Handle";
    case CreativeDoorPartKind::Count:
      break;
  }
  return "Invalid";
}

[[nodiscard]] std::string_view doorPartAttachmentSocket(
    CreativeDoorPartKind kind) noexcept {
  switch (kind) {
    case CreativeDoorPartKind::SecondaryLeaf:
      return "door_secondary_leaf";
    case CreativeDoorPartKind::PrimaryLowerHinge:
      return "door_primary_lower_hinge";
    case CreativeDoorPartKind::PrimaryUpperHinge:
      return "door_primary_upper_hinge";
    case CreativeDoorPartKind::SecondaryLowerHinge:
      return "door_secondary_lower_hinge";
    case CreativeDoorPartKind::SecondaryUpperHinge:
      return "door_secondary_upper_hinge";
    case CreativeDoorPartKind::PrimaryHandle:
      return "door_primary_handle";
    case CreativeDoorPartKind::SecondaryHandle:
      return "door_secondary_handle";
    case CreativeDoorPartKind::MinimumJamb:
    case CreativeDoorPartKind::MaximumJamb:
    case CreativeDoorPartKind::Header:
    case CreativeDoorPartKind::PrimaryLeaf:
    case CreativeDoorPartKind::Count:
      break;
  }
  return {};
}

[[nodiscard]] bool appendGeneratedDoor(
    CreativeBuildingRecipeResult& result,
    const CreativeBuildingRecipeRequest& request,
    const CreativeBuildingOpeningSpec& opening,
    const CreativeStructuralWallFrame& frame,
    const CreativeStructuralWallOpeningPlan& openingPlan,
    bool hasCreatedRoot) {
  const CreativeDoorRecipeResult door =
      planCreativeDoor({frame, openingPlan.cutoutBounds, opening.door});
  if (!door.accepted) {
    result.receipt.failedOpeningIndex = openingPlan.sourceIndex;
    setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidOpening,
              door.reasonCode);
    return false;
  }

  std::optional<std::size_t> primaryDoorIndex;
  for (std::size_t index = 0U; index < door.partCount; ++index) {
    const CreativeDoorPartPlan& part = door.parts[index];
    CreativeDocumentCreateRequest createRequest = makeBoxRequest(
        part.objectKind,
        part.kind == CreativeDoorPartKind::PrimaryLeaf
            ? opening.name
            : opening.name + " " +
                  std::string(doorPartNameSuffix(part.kind)),
        part.closedBounds, request.visible,
        mergedTags(request.tags, opening.tags));
    createRequest.transform = part.closedTransform;
    createRequest.hasTransformOverride = true;
    const bool isLeaf = part.kind == CreativeDoorPartKind::PrimaryLeaf ||
                        part.kind == CreativeDoorPartKind::SecondaryLeaf;
    if (isLeaf && opening.hasInsertAssetSourceBounds) {
      const CreativeBuildingOpeningAssetFitPlan fit =
          planCreativeBuildingOpeningAssetFit(
              {opening.insertAssetSourceBoundsMeters, part.closedBounds, frame,
               CreativeBuildingOpeningPose::Closed, opening.facing});
      if (!fit.accepted) {
        result.receipt.failedOpeningIndex = openingPlan.sourceIndex;
        setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidOpening,
                  fit.reasonCode);
        return false;
      }
      createRequest.assetId = opening.insertAssetId;
      createRequest.bounds = fit.authoredBoundsMeters;
      createRequest.transform = fit.transform;
    }
    if (part.kind == CreativeDoorPartKind::PrimaryLeaf) {
      createRequest.hasDoorSettingsOverride = true;
      createRequest.door = opening.door;
    }
    const bool isMovingChild =
        part.leafIndex != kCreativeDoorNoLeaf &&
        part.kind != CreativeDoorPartKind::PrimaryLeaf;
    if (isMovingChild) {
      if (!primaryDoorIndex.has_value()) {
        setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidPlan,
                  "creative_door_primary_leaf_missing");
        return false;
      }
      createRequest.attachmentSocket =
          std::string(doorPartAttachmentSocket(part.kind));
    }
    appendGeneratedRequest(
        result, request, part.objectKind,
        opening.stableKey + "." +
            std::string(doorPartStableSuffix(part.kind)),
        std::move(createRequest), hasCreatedRoot,
        isMovingChild ? primaryDoorIndex : std::nullopt, true);
    if (part.kind == CreativeDoorPartKind::PrimaryLeaf) {
      primaryDoorIndex = result.plan.objects.size() - 1U;
    }
  }
  return primaryDoorIndex.has_value();
}

[[nodiscard]] bool appendGeneratedOpeningInsert(
    CreativeBuildingRecipeResult& result,
    const CreativeBuildingRecipeRequest& request,
    const CreativeBuildingOpeningSpec& opening,
    const CreativeStructuralWallFrame& frame,
    const CreativeStructuralWallOpeningPlan& openingPlan,
    bool hasCreatedRoot) {
  if (opening.kind == CreativeBuildingOpeningKind::Door) {
    return appendGeneratedDoor(result, request, opening, frame, openingPlan,
                               hasCreatedRoot);
  }

  const CreativeWindowRecipeResult window = planCreativeWindow(
      {frame, openingPlan.cutoutBounds, opening.window,
       !opening.hasInsertAssetSourceBounds});
  if (!window.accepted ||
      window.primaryInsertPartIndex >= window.partCount) {
    result.receipt.failedOpeningIndex = openingPlan.sourceIndex;
    setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidOpening,
              window.reasonCode);
    return false;
  }

  std::optional<std::size_t> primaryWindowIndex;
  for (std::size_t index = 0U; index < window.partCount; ++index) {
    const CreativeWindowPartPlan& part = window.parts[index];
    std::string_view stableSuffix;
    std::string_view nameSuffix;
    switch (part.kind) {
      case CreativeWindowPartKind::MinimumJamb:
        stableSuffix = "frame.minimum_jamb";
        nameSuffix = "Minimum Jamb";
        break;
      case CreativeWindowPartKind::MaximumJamb:
        stableSuffix = "frame.maximum_jamb";
        nameSuffix = "Maximum Jamb";
        break;
      case CreativeWindowPartKind::FrameSill:
        stableSuffix = "frame.sill";
        nameSuffix = "Frame Sill";
        break;
      case CreativeWindowPartKind::FrameHeader:
        stableSuffix = "frame.header";
        nameSuffix = "Frame Header";
        break;
      case CreativeWindowPartKind::PrimaryInsert:
        stableSuffix = "insert";
        break;
      case CreativeWindowPartKind::SecondaryShutter:
        stableSuffix = "shutter.secondary";
        nameSuffix = "Secondary Shutter";
        break;
      case CreativeWindowPartKind::Count:
        setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidPlan,
                  "creative_window_part_kind_invalid");
        return false;
    }

    CreativeDocumentCreateRequest createRequest = makeBoxRequest(
        part.objectKind,
        nameSuffix.empty()
            ? opening.name
            : opening.name + " " + std::string(nameSuffix),
        part.bounds, request.visible,
        mergedTags(request.tags, opening.tags));
    if (part.kind == CreativeWindowPartKind::PrimaryInsert) {
      createRequest.hasWindowSettingsOverride = true;
      createRequest.window = opening.window;
      if (opening.hasInsertAssetSourceBounds) {
        const CreativeBuildingOpeningAssetFitPlan fit =
            planCreativeBuildingOpeningAssetFit(
                {opening.insertAssetSourceBoundsMeters, part.bounds, frame,
                 CreativeBuildingOpeningPose::Closed, opening.facing});
        if (!fit.accepted) {
          result.receipt.failedOpeningIndex = openingPlan.sourceIndex;
          setStatus(result.receipt,
                    CreativeBuildingRecipeStatus::InvalidOpening,
                    fit.reasonCode);
          return false;
        }
        createRequest.assetId = opening.insertAssetId;
        createRequest.bounds = fit.authoredBoundsMeters;
        createRequest.transform = fit.transform;
      }
    }
    const bool secondary =
        part.kind == CreativeWindowPartKind::SecondaryShutter;
    if (secondary) {
      if (!primaryWindowIndex.has_value()) {
        setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidPlan,
                  "creative_window_primary_insert_missing");
        return false;
      }
      createRequest.attachmentSocket = "window_secondary_shutter";
    }
    appendGeneratedRequest(
        result, request, part.objectKind,
        opening.stableKey + "." + std::string(stableSuffix),
        std::move(createRequest), hasCreatedRoot,
        secondary ? primaryWindowIndex : std::nullopt, false, true);
    if (part.kind == CreativeWindowPartKind::PrimaryInsert) {
      primaryWindowIndex = result.plan.objects.size() - 1U;
    }
  }
  return primaryWindowIndex.has_value();
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
        !validOpeningFacing(opening.facing) ||
        (opening.kind == CreativeBuildingOpeningKind::Door &&
         !isValidCreativeDoorSettings(opening.door)) ||
        (opening.kind == CreativeBuildingOpeningKind::Window &&
         !isValidCreativeWindowSettings(opening.window)) ||
        opening.stableKey.empty() || opening.name.empty() ||
        !validOpeningAsset(opening) ||
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
         opening.insertThicknessMeters,
         opening.kind == CreativeBuildingOpeningKind::Door
             ? CreativeBuildingOpeningPose::Closed
             : opening.pose});
  }
  const CreativeStructuralWallRecipeResult geometry =
      planCreativeStructuralWall({wall.start, wall.end, wall.heightMeters,
                                  wall.thicknessMeters, 0.0, 0.0,
                                  openingRequests});
  if (!geometry.accepted) {
    setWallKernelFailure(result.receipt, geometry);
    return false;
  }

  if (!geometry.planarSolidPieces.empty()) {
    if (!wall.segmentNames.empty()) {
      setStatus(result.receipt, CreativeBuildingRecipeStatus::InvalidWall,
                "creative_building_wall_segment_names_invalid");
      return false;
    }
    for (std::size_t pieceIndex = 0U;
         pieceIndex < geometry.planarSolidPieces.size(); ++pieceIndex) {
      appendGeneratedObject(
          result, request, CreativeObjectKind::Wall,
          wall.stableKey + ".piece." + std::to_string(pieceIndex + 1U),
          wall.name + " Piece " + std::to_string(pieceIndex + 1U),
          geometry.planarSolidPieces[pieceIndex], hasCreatedRoot, wall.tags);
    }
    for (const CreativeStructuralWallOpeningPlan& openingPlan :
         geometry.openings) {
      const CreativeBuildingOpeningSpec& opening =
          wall.openings[openingPlan.sourceIndex];
      if (openingPlan.hasInsert &&
          !appendGeneratedOpeningInsert(result, request, opening,
                                        geometry.frame, openingPlan,
                                        hasCreatedRoot)) {
        return false;
      }
    }
    return true;
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
      if (!appendGeneratedOpeningInsert(result, request, opening,
                                        geometry.frame, openingPlan,
                                        hasCreatedRoot)) {
        return false;
      }
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
    CreativeBuildingOpeningAssetFitStatus status) noexcept {
  switch (status) {
    case CreativeBuildingOpeningAssetFitStatus::NotRequested:
      return "NotRequested";
    case CreativeBuildingOpeningAssetFitStatus::InvalidSourceBounds:
      return "InvalidSourceBounds";
    case CreativeBuildingOpeningAssetFitStatus::InvalidTargetBounds:
      return "InvalidTargetBounds";
    case CreativeBuildingOpeningAssetFitStatus::InvalidWallFrame:
      return "InvalidWallFrame";
    case CreativeBuildingOpeningAssetFitStatus::InvalidPose:
      return "InvalidPose";
    case CreativeBuildingOpeningAssetFitStatus::InvalidFacing:
      return "InvalidFacing";
    case CreativeBuildingOpeningAssetFitStatus::UnrepresentableTransform:
      return "UnrepresentableTransform";
    case CreativeBuildingOpeningAssetFitStatus::Ready:
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

CreativeBuildingOpeningAssetFitPlan planCreativeBuildingOpeningAssetFit(
    const CreativeBuildingOpeningAssetFitRequest& request) noexcept {
  CreativeBuildingOpeningAssetFitPlan plan;
  const CreativeBoundsMetrics source =
      measureCreativeBounds(request.sourceBoundsMeters);
  if (!source.valid || !isPositiveCreativeVec3(source.size)) {
    plan.status =
        CreativeBuildingOpeningAssetFitStatus::InvalidSourceBounds;
    plan.reasonCode = "creative_building_opening_asset_source_invalid";
    return plan;
  }
  const CreativeBoundsMetrics target =
      measureCreativeBounds(request.targetBoundsMeters);
  if (!target.valid || !isPositiveCreativeVec3(target.size)) {
    plan.status =
        CreativeBuildingOpeningAssetFitStatus::InvalidTargetBounds;
    plan.reasonCode = "creative_building_opening_asset_target_invalid";
    return plan;
  }
  if (request.wallFrame.axis >= CreativeStructuralWallAxis::Count ||
      !isFiniteCreativeVec3(request.wallFrame.tangent) ||
      !isFiniteCreativeVec3(request.wallFrame.normal)) {
    plan.status = CreativeBuildingOpeningAssetFitStatus::InvalidWallFrame;
    plan.reasonCode = "creative_building_opening_asset_wall_frame_invalid";
    return plan;
  }
  if (!isCreativeStructuralWallOpeningPoseValid(request.pose)) {
    plan.status = CreativeBuildingOpeningAssetFitStatus::InvalidPose;
    plan.reasonCode = "creative_building_opening_asset_pose_invalid";
    return plan;
  }
  if (!validOpeningFacing(request.facing)) {
    plan.status = CreativeBuildingOpeningAssetFitStatus::InvalidFacing;
    plan.reasonCode = "creative_building_opening_asset_facing_invalid";
    return plan;
  }

  const bool open = openingPoseIsOpen(request.pose);
  CreativeVec3 targetDirection =
      open ? request.wallFrame.normal : request.wallFrame.tangent;
  const double facingSign =
      request.facing == CreativeBuildingOpeningFacing::PositiveNormal ? 1.0
                                                                      : -1.0;
  targetDirection.x *= facingSign;
  targetDirection.z *= facingSign;
  if (open) {
    const double normalSign = openingPoseNormalSign(request.pose);
    targetDirection.x *= normalSign;
    targetDirection.z *= normalSign;
  }
  const double directionLength =
      std::hypot(targetDirection.x, targetDirection.z);
  if (!std::isfinite(directionLength) || directionLength <= kGeometryEpsilon ||
      std::abs(targetDirection.y) > kGeometryEpsilon) {
    plan.status = CreativeBuildingOpeningAssetFitStatus::InvalidWallFrame;
    plan.reasonCode = "creative_building_opening_asset_wall_frame_invalid";
    return plan;
  }
  targetDirection.x /= directionLength;
  targetDirection.z /= directionLength;

  const bool wallRunsAlongX =
      request.wallFrame.axis == CreativeStructuralWallAxis::X;
  const double targetPrimary =
      open ? (wallRunsAlongX ? target.size.z : target.size.x)
           : (wallRunsAlongX ? target.size.x : target.size.z);
  const double targetSecondary =
      open ? (wallRunsAlongX ? target.size.x : target.size.z)
           : (wallRunsAlongX ? target.size.z : target.size.x);
  const bool localXIsPrimary = source.size.x >= source.size.z;
  if (localXIsPrimary) {
    plan.transform.scale = {targetPrimary / source.size.x,
                            target.size.y / source.size.y,
                            targetSecondary / source.size.z};
    plan.transform.rotationEulerRadians.y =
        std::atan2(-targetDirection.z, targetDirection.x);
  } else {
    plan.transform.scale = {targetSecondary / source.size.x,
                            target.size.y / source.size.y,
                            targetPrimary / source.size.z};
    plan.transform.rotationEulerRadians.y =
        std::atan2(targetDirection.x, targetDirection.z);
  }
  if (!isPositiveCreativeVec3(plan.transform.scale) ||
      !isFiniteCreativeVec3(plan.transform.rotationEulerRadians)) {
    plan.status =
        CreativeBuildingOpeningAssetFitStatus::UnrepresentableTransform;
    plan.reasonCode = "creative_building_opening_asset_transform_invalid";
    return plan;
  }

  const CreativeVec3 scaledSourceCenter{
      source.center.x * plan.transform.scale.x,
      source.center.y * plan.transform.scale.y,
      source.center.z * plan.transform.scale.z};
  const CreativeVec3 rotatedSourceCenter = rotateCreativeVectorEulerXyz(
      scaledSourceCenter, plan.transform.rotationEulerRadians);
  plan.transform.position = {
      target.center.x - rotatedSourceCenter.x,
      target.center.y - rotatedSourceCenter.y,
      target.center.z - rotatedSourceCenter.z};
  plan.authoredBoundsMeters = {
      {plan.transform.position.x + request.sourceBoundsMeters.min.x,
       plan.transform.position.y + request.sourceBoundsMeters.min.y,
       plan.transform.position.z + request.sourceBoundsMeters.min.z},
      {plan.transform.position.x + request.sourceBoundsMeters.max.x,
       plan.transform.position.y + request.sourceBoundsMeters.max.y,
       plan.transform.position.z + request.sourceBoundsMeters.max.z}};
  const CreativeTransformedBounds resolved = resolveCreativeTransformedBounds(
      plan.authoredBoundsMeters, plan.transform);
  if (!resolved.valid ||
      !boundsNear(resolved.worldBounds, request.targetBoundsMeters)) {
    plan = {};
    plan.status =
        CreativeBuildingOpeningAssetFitStatus::UnrepresentableTransform;
    plan.reasonCode = "creative_building_opening_asset_transform_invalid";
    return plan;
  }
  plan.status = CreativeBuildingOpeningAssetFitStatus::Ready;
  plan.accepted = true;
  plan.reasonCode = "creative_building_opening_asset_fit_ready";
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
