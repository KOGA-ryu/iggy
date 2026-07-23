#pragma once

#include "app/iggy3d/creative/world/WorldLayoutPlanProjection.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutPlanHitTestStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidPrimitive,
  Miss,
  Hit,
};

enum class CreativeWorldLayoutPlanRegionMode : std::uint8_t {
  Window,
  Crossing,
  Count,
};

struct CreativeWorldLayoutPlanHitTestResult {
  bool hit = false;
  CreativeWorldLayoutPlanHitTestStatus status =
      CreativeWorldLayoutPlanHitTestStatus::NotRequested;
  double distanceCells = 0.0;
  std::string_view reasonCode =
      "creative_world_layout_plan_hit_test_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutPlanHitTestStatus status) noexcept;

// Constant-time, allocation-free geometry test for one bounded plan primitive.
// Polygon interiors and circle interiors are selectable. Segment width expands
// its selectable footprint; the caller supplies additional zoom-derived
// tolerance in grid-cell units. Invalid input fails closed.
[[nodiscard]] CreativeWorldLayoutPlanHitTestResult
hitTestCreativeWorldLayoutPlanPrimitive(
    const CreativeWorldLayoutPlanPrimitive& primitive,
    CreativeWorldLayoutPlanPoint point, double toleranceCells) noexcept;

// CAD-style bounded region test. Window mode requires the complete primitive
// footprint to be enclosed; Crossing mode accepts any geometric intersection.
// The two modes intentionally have different semantics so drag direction can
// communicate selection intent without another modal tool.
[[nodiscard]] CreativeWorldLayoutPlanHitTestResult
selectCreativeWorldLayoutPlanPrimitiveInRegion(
    const CreativeWorldLayoutPlanPrimitive& primitive,
    CreativeWorldLayoutPlanPoint first,
    CreativeWorldLayoutPlanPoint second,
    CreativeWorldLayoutPlanRegionMode mode,
    double toleranceCells = 0.0) noexcept;

}  // namespace iggy3d::creative
