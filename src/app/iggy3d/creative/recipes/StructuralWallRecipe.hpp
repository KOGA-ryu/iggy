#pragma once

#include "app/iggy3d/creative/document/Object.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeStructuralWallAxis : std::uint8_t {
  X,
  Z,
  Count,
};

enum class CreativeStructuralWallReferenceLine : std::uint8_t {
  // Authored endpoints remain fixed while thickness expands equally along
  // both sides of the stable wall normal.
  Centerline,
  Count,
};

enum class CreativeStructuralWallOpeningPose : std::uint8_t {
  Closed,
  OpenFromStartNegativeNormal,
  OpenFromStartPositiveNormal,
  OpenFromEndNegativeNormal,
  OpenFromEndPositiveNormal,
};

enum class CreativeStructuralWallRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidWall,
  UnsupportedOrientation,
  InvalidOpening,
  OverlappingOpenings,
  UnrepresentableGeometry,
  Ready,
};

struct CreativeStructuralWallFrame {
  CreativeStructuralWallAxis axis = CreativeStructuralWallAxis::Count;
  CreativeStructuralWallReferenceLine referenceLine =
      CreativeStructuralWallReferenceLine::Centerline;
  CreativeVec3 start;
  CreativeVec3 end;
  // Tangent follows authored start-to-end. Normal is the stable positive
  // transverse axis (+Z for X walls, +X for Z walls), independent of endpoint
  // order, so opening pose names remain deterministic after mirroring.
  CreativeVec3 tangent;
  CreativeVec3 normal;
  CreativeBounds bounds;
  double baseYMeters = 0.0;
  double lengthMeters = 0.0;
  double heightMeters = 0.0;
  double thicknessMeters = 0.0;
};

struct CreativeStructuralWallOpeningRequest {
  // Used only to make equal-position input ordering deterministic.
  std::string_view sortKey;
  // Offsets are measured from the authored wall start along its tangent.
  double centerOffsetMeters = 0.0;
  double widthMeters = 1.0;
  double cutoutBottomMeters = 0.0;
  double cutoutHeightMeters = 2.1;
  bool includeInsert = true;
  double insertBottomMeters = 0.0;
  double insertHeightMeters = 0.0;
  double insertWidthMeters = 0.0;
  double insertThicknessMeters = 0.0;
  CreativeStructuralWallOpeningPose pose =
      CreativeStructuralWallOpeningPose::Closed;
};

struct CreativeStructuralWallRecipeRequest {
  CreativeVec3 start;
  CreativeVec3 end;
  double heightMeters = 0.0;
  double thicknessMeters = 0.0;
  double minimumOpeningEdgeClearanceMeters = 0.0;
  double minimumOpeningSeparationMeters = 0.0;
  std::span<const CreativeStructuralWallOpeningRequest> openings;
};

struct CreativeStructuralWallOpeningPlan {
  std::size_t sourceIndex = 0U;
  double minimumOffsetMeters = 0.0;
  double maximumOffsetMeters = 0.0;
  CreativeBounds cutoutBounds;
  CreativeBounds sillBounds;
  CreativeBounds lintelBounds;
  CreativeBounds insertBounds;
  bool hasSill = false;
  bool hasLintel = false;
  bool hasInsert = false;
};

struct CreativeStructuralWallRecipeResult {
  bool accepted = false;
  CreativeStructuralWallRecipeStatus status =
      CreativeStructuralWallRecipeStatus::NotRequested;
  CreativeStructuralWallFrame frame;
  std::vector<CreativeBounds> fullHeightSpans;
  // Populated only when openings overlap along the wall axis but occupy
  // separate vertical bands. These non-overlapping solids are the canonical
  // wall geometry for that 2D cutout layout.
  std::vector<CreativeBounds> planarSolidPieces;
  std::vector<CreativeStructuralWallOpeningPlan> openings;
  std::size_t failedOpeningIndex = 0U;
  std::string_view reasonCode = "creative_structural_wall_not_requested";
};

[[nodiscard]] bool isCreativeStructuralWallOpeningPoseValid(
    CreativeStructuralWallOpeningPose pose) noexcept;

[[nodiscard]] CreativeStructuralWallRecipeResult planCreativeStructuralWall(
    const CreativeStructuralWallRecipeRequest& request);

}  // namespace iggy3d::creative
