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

}  // namespace iggy3d::creative
