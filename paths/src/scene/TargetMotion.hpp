#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "app/iggy3d/creative/play/RuntimeMovingPlatforms.hpp"

namespace paths {

inline constexpr std::uint32_t kGalleryTickRate = 60;
inline constexpr std::uint64_t kMotionPreviewWindowTicks = 60 * kGalleryTickRate;

enum class RouteKind : std::uint8_t {
  Stationary, Horizontal, Vertical, DiagonalRebound, Circle, Ellipse,
  FigureEight, Sine, Zigzag, Box, StopAndGo, BreathingSpiral, SeededRoam,
  Waypoints, Count,
};
struct RouteDescriptor { RouteKind kind; std::string_view id, label; };
[[nodiscard]] std::span<const RouteDescriptor> targetRouteDescriptors();

struct RouteSpec {
  RouteKind kind = RouteKind::Waypoints;
  iggy3d::Vec3 extent{0.65F,0.45F,0.2F};
  iggy3d::creative::CreativeMovingPlatformSettings settings;
  std::array<iggy3d::creative::CreativePathPoint,32> points{};
  std::size_t pointCount = 0;
  double dwellSeconds = 0.4;
  std::uint32_t seed = 1;
};
struct PreparedRoute {
  RouteSpec spec;
  iggy3d::Vec3 origin{};
  iggy3d::creative::CreativeRuntimeMovingPlatformDefinition waypoint;
  bool usesWaypoints = false, ready = false;
};
struct MotionState {
  iggy3d::Vec3 position{};
  std::uint64_t tick = 0;
  iggy3d::creative::CreativeRuntimeMovingPlatformState waypoint;
};
struct MotionPreview { MotionState anchor; };
struct MotionResult { bool accepted; std::string_view reason; };

// Prepare into a candidate; a rejection never replaces the previous route.
[[nodiscard]] MotionResult prepareRoute(const RouteSpec&, iggy3d::Vec3 origin,
                                       PreparedRoute& output);
[[nodiscard]] MotionState initialMotion(const PreparedRoute&);
// Exactly one fixed tick, without allocation, random draws, or route rebuilding.
[[nodiscard]] MotionResult advanceMotion(const PreparedRoute&, MotionState&,
                                        std::uint64_t nextTick);
// The checkpoint belongs to preview. Waypoints replay the live kernel, with a
// bounded 60-second window; curves can evaluate that same tick directly.
[[nodiscard]] MotionResult seekMotion(const PreparedRoute&, std::uint64_t tick,
                                     const MotionPreview&, MotionState& output);

}  // namespace paths
