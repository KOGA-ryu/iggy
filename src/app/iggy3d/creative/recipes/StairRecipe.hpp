#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

inline constexpr double kCreativeStairMaximumRiserHeightMeters =
    kCreativeGeneratedStairMaximumRiseMeters;
inline constexpr double kCreativeStairMinimumTreadDepthMeters =
    kCreativeGeneratedStairMinimumRunMeters;
inline constexpr double kCreativeStairMinimumHeadroomMeters = 1.84;
inline constexpr std::size_t kCreativeStairSocketCapacity = 4U;

enum class CreativeStairRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidBounds,
  InvalidTransform,
  InvalidDimensions,
  InvalidHeadroom,
  StepCapacityExceeded,
  Ready,
};

enum class CreativeStairSocketKind : std::uint8_t {
  RailLeft,
  RailRight,
  StringerLeft,
  StringerRight,
  Count,
};

struct CreativeStairRecipeRequest {
  CreativeBounds authoredBounds;
  CreativeTransform transform;
  double maximumRiserHeightMeters{kCreativeStairMaximumRiserHeightMeters};
  double minimumTreadDepthMeters{kCreativeStairMinimumTreadDepthMeters};
  double landingDepthMeters{1.0};
  double availableHeadroomMeters{3.0};
  double minimumHeadroomMeters{kCreativeStairMinimumHeadroomMeters};
};

struct CreativeStairLandingPlan {
  CreativeVec3 centerMeters;
  CreativeVec3 sizeMeters;
  CreativeVec3 rotationEulerRadians;
};

// Socket positions are in the target object's authored local frame. The
// attachment kernel applies target scale, rotation, and translation once.
struct CreativeStairSocketPlan {
  CreativeStairSocketKind kind{CreativeStairSocketKind::Count};
  CreativeVec3 localPosition;
  CreativeVec3 forward{0.0, 0.0, 1.0};
  CreativeVec3 up{0.0, 1.0, 0.0};
};

struct CreativeStairRecipeResult {
  bool accepted{false};
  CreativeStairRecipeStatus status{CreativeStairRecipeStatus::NotRequested};
  std::uint16_t stepCount{0U};
  double widthMeters{0.0};
  double riseMeters{0.0};
  double runMeters{0.0};
  double riserHeightMeters{0.0};
  double treadDepthMeters{0.0};
  double headroomMeters{0.0};
  CreativeStairLandingPlan lowerLanding;
  CreativeStairLandingPlan upperLanding;
  CreativeVec3 lowTreadCenterMeters;
  CreativeVec3 highTreadCenterMeters;
  std::array<CreativeStairSocketPlan, kCreativeStairSocketCapacity> sockets{};
  std::size_t socketCount{0U};
  std::string_view reasonCode{"creative_stair_not_requested"};
};

static_assert(std::is_trivially_copyable_v<CreativeStairRecipeRequest>);
static_assert(std::is_trivially_copyable_v<CreativeStairLandingPlan>);
static_assert(std::is_trivially_copyable_v<CreativeStairSocketPlan>);
static_assert(std::is_trivially_copyable_v<CreativeStairRecipeResult>);

[[nodiscard]] std::string_view creativeStairSocketName(
    CreativeStairSocketKind kind) noexcept;
[[nodiscard]] std::string_view creativeStairSocketCompatibility(
    CreativeStairSocketKind kind) noexcept;

[[nodiscard]] CreativeStairRecipeResult planCreativeStair(
    const CreativeStairRecipeRequest& request) noexcept;

}  // namespace iggy3d::creative
