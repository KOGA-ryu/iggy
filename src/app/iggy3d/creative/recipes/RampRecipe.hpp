#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "runtime/movement/MovementParams.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

inline constexpr double kCreativeRampMinimumHeadroomMeters = 1.84;
inline constexpr double kCreativeRampMaximumWalkableSlopeDegrees =
    static_cast<double>(kDefaultMaxWalkableSlopeDegrees);
inline constexpr std::size_t kCreativeRampSideEdgeCount = 2U;
inline constexpr std::size_t kCreativeRampSocketCapacity = 2U;

enum class CreativeRampRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidBounds,
  InvalidTransform,
  InvalidDimensions,
  InvalidMaterial,
  InvalidHeadroom,
  InvalidSlope,
  Ready,
};

enum class CreativeRampSocketKind : std::uint8_t {
  EdgeLeft,
  EdgeRight,
  Count,
};

struct CreativeRampRecipeRequest {
  CreativeBounds authoredBounds;
  CreativeTransform transform;
  double landingDepthMeters{1.0};
  double availableHeadroomMeters{3.0};
  double minimumHeadroomMeters{kCreativeRampMinimumHeadroomMeters};
  double maximumWalkableSlopeDegrees{
      kCreativeRampMaximumWalkableSlopeDegrees};
  CreativeStructuralMaterial material{CreativeStructuralMaterial::Blockout};
};

struct CreativeRampLandingPlan {
  CreativeVec3 centerMeters;
  CreativeVec3 sizeMeters;
  CreativeVec3 rotationEulerRadians;
};

struct CreativeRampSideEdgePlan {
  CreativeVec3 lowMeters;
  CreativeVec3 highMeters;
};

// Socket positions are in the target object's authored local frame. The
// attachment kernel applies target scale, rotation, and translation once.
struct CreativeRampSocketPlan {
  CreativeRampSocketKind kind{CreativeRampSocketKind::Count};
  CreativeVec3 localPosition;
  CreativeVec3 forward{0.0, 0.0, 1.0};
  CreativeVec3 up{0.0, 1.0, 0.0};
};

struct CreativeRampRecipeResult {
  bool accepted{false};
  bool walkable{false};
  CreativeRampRecipeStatus status{CreativeRampRecipeStatus::NotRequested};
  double widthMeters{0.0};
  double riseMeters{0.0};
  double runMeters{0.0};
  double surfaceLengthMeters{0.0};
  double slopeAngleDegrees{0.0};
  double maximumWalkableSlopeDegrees{0.0};
  double headroomMeters{0.0};
  CreativeStructuralMaterial material{CreativeStructuralMaterial::Blockout};
  CreativeVec3 surfaceNormal;
  CreativeRampLandingPlan lowerLanding;
  CreativeRampLandingPlan upperLanding;
  CreativeVec3 lowSurfaceCenterMeters;
  CreativeVec3 highSurfaceCenterMeters;
  std::array<CreativeRampSideEdgePlan, kCreativeRampSideEdgeCount> sideEdges{};
  std::array<CreativeRampSocketPlan, kCreativeRampSocketCapacity> sockets{};
  std::size_t socketCount{0U};
  std::string_view reasonCode{"creative_ramp_not_requested"};
};

static_assert(std::is_trivially_copyable_v<CreativeRampRecipeRequest>);
static_assert(std::is_trivially_copyable_v<CreativeRampLandingPlan>);
static_assert(std::is_trivially_copyable_v<CreativeRampSideEdgePlan>);
static_assert(std::is_trivially_copyable_v<CreativeRampSocketPlan>);
static_assert(std::is_trivially_copyable_v<CreativeRampRecipeResult>);

[[nodiscard]] std::string_view creativeRampSocketName(
    CreativeRampSocketKind kind) noexcept;
[[nodiscard]] std::string_view creativeRampSocketCompatibility(
    CreativeRampSocketKind kind) noexcept;

[[nodiscard]] CreativeRampRecipeResult planCreativeRamp(
    const CreativeRampRecipeRequest& request) noexcept;

}  // namespace iggy3d::creative
