#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeStructuralRoofStyle : std::uint8_t {
  Flat,
  Gable,
  Count,
};

enum class CreativeStructuralRoofRidgeAxis : std::uint8_t {
  X,
  Z,
  Count,
};

inline constexpr double kDefaultCreativeStructuralRoofPitchDegrees = 30.0;
inline constexpr double kMinimumCreativeStructuralRoofPitchDegrees = 5.0;
inline constexpr double kMaximumCreativeStructuralRoofPitchDegrees = 75.0;

enum class CreativeStructuralRoofRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidFootprint,
  InvalidSupportPlane,
  InvalidLayerCount,
  InvalidStyle,
  InvalidRidgeAxis,
  InvalidPitch,
  InvalidOverhang,
  UnrepresentableGeometry,
  Ready,
};

struct CreativeStructuralRoofRecipeRequest {
  CreativeStructuralRoofStyle style = CreativeStructuralRoofStyle::Flat;
  CreativeStructuralRoofRidgeAxis ridgeAxis =
      CreativeStructuralRoofRidgeAxis::X;
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
  double supportPlaneMeters = 0.0;
  std::uint16_t layerCount = 1U;
  double pitchDegrees = kDefaultCreativeStructuralRoofPitchDegrees;
  double overhangMeters = 0.0;
};

struct CreativeStructuralRoofPart {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  CreativeBounds bounds;
  CreativeVec3 rotationEulerRadians;
};

inline constexpr std::size_t kCreativeStructuralRoofPartCapacity = 3U;

struct CreativeStructuralRoofRecipeResult {
  bool accepted = false;
  CreativeStructuralRoofRecipeStatus status =
      CreativeStructuralRoofRecipeStatus::NotRequested;
  std::array<CreativeStructuralRoofPart,
             kCreativeStructuralRoofPartCapacity>
      parts{};
  std::uint8_t partCount = 0U;
  CreativeBounds worldBounds;
  CreativeVec3 ridgeStart;
  CreativeVec3 ridgeEnd;
  double riseMeters = 0.0;
  double baseThicknessMeters = 0.0;
  std::string_view reasonCode = "creative_structural_roof_not_requested";
};

[[nodiscard]] bool validCreativeStructuralRoofSettings(
    CreativeStructuralRoofStyle style,
    CreativeStructuralRoofRidgeAxis ridgeAxis,
    double pitchDegrees,
    double overhangMeters) noexcept;

// Produces the exact generated parts for both preview and document creation.
// Gable roofs are a base slab plus two mirrored slope wedges. The wedge kind
// reuses the descriptor-owned ramp geometry/collision kernel without exposing
// a generic roof brush.
[[nodiscard]] CreativeStructuralRoofRecipeResult planCreativeStructuralRoof(
    const CreativeStructuralRoofRecipeRequest& request) noexcept;

}  // namespace iggy3d::creative
