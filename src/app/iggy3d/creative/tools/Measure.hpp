#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/tools/MeasurementTypes.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeMeasurementChangeKind : std::uint8_t {
  None,
  BeginMeasurement,
  UpdateMeasurement,
  EndMeasurement,
  ConfigureMeasurement,
  AppendMeasurementPoint,
  CompleteMeasurement,
  CancelMeasurement,
  ClearMeasurement,
};

enum class CreativeMeasurementPlanStatus : std::uint8_t {
  NotRequested,
  InvalidMode,
  InvalidAxis,
  TooFewPoints,
  CapacityExceeded,
  InvalidPoint,
  Degenerate,
  NonPlanar,
  Ready,
};

inline constexpr std::size_t kCreativeMeasurementPointCapacity = 256U;

struct CreativeMeasurementPoint {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  TargetRef target;
  CreativeMeasurementSnapKind snapKind = CreativeMeasurementSnapKind::None;
};

struct CreativeMeasurementPlanRequest {
  CreativeMeasurementMode mode = CreativeMeasurementMode::Distance;
  CreativeMeasurementAxis axis = CreativeMeasurementAxis::X;
  std::span<const CreativeMeasurementPoint> points;
  bool closePath = false;
};

struct CreativeMeasurementPlan {
  bool requested = false;
  bool accepted = false;
  CreativeMeasurementPlanStatus status =
      CreativeMeasurementPlanStatus::NotRequested;
  CreativeMeasurementMode mode = CreativeMeasurementMode::Distance;
  CreativeMeasurementAxis axis = CreativeMeasurementAxis::X;
  std::size_t pointCount = 0U;
  std::size_t segmentCount = 0U;
  CreativeMeasurementPoint firstPoint;
  CreativeMeasurementPoint lastPoint;
  double deltaX = 0.0;
  double deltaY = 0.0;
  double deltaZ = 0.0;
  double directDistanceMeters = 0.0;
  double horizontalDistanceMeters = 0.0;
  double verticalDistanceMeters = 0.0;
  double signedRiseMeters = 0.0;
  double axisDistanceMeters = 0.0;
  double slopeDegrees = 0.0;
  double gradePercent = 0.0;
  bool gradeFinite = true;
  double perimeterMeters = 0.0;
  double areaSquareMeters = 0.0;
  CreativeVec3 areaNormal;
  std::string_view reasonCode = "creative_measurement_not_requested";
};

struct CreativeMeasurementState {
  CreativeMeasurementMode mode = CreativeMeasurementMode::Distance;
  CreativeMeasurementAxis axis = CreativeMeasurementAxis::X;
  bool closePath = false;
  bool active = false;
  bool hasMeasurement = false;
  bool completed = false;
  CreativeMeasurementPoint startPoint;
  CreativeMeasurementPoint currentPoint;
  std::array<CreativeMeasurementPoint, kCreativeMeasurementPointCapacity + 1U>
      points{};
  std::size_t pointCount = 0U;
  bool hasPreviewPoint = false;
  CreativeMeasurementPlan plan;
  std::uint64_t sampleCount = 0;
};

struct CreativeMeasurementReceipt {
  CreativeMeasurementChangeKind requestedChange =
      CreativeMeasurementChangeKind::None;
  CreativeMeasurementChangeKind appliedChange =
      CreativeMeasurementChangeKind::None;
  bool activeBefore = false;
  bool activeAfter = false;
  bool hasMeasurementBefore = false;
  bool hasMeasurementAfter = false;
  CreativeMeasurementPoint startPointBefore;
  CreativeMeasurementPoint startPointAfter;
  CreativeMeasurementPoint currentPointBefore;
  CreativeMeasurementPoint currentPointAfter;
  std::uint64_t sampleCountBefore = 0;
  std::uint64_t sampleCountAfter = 0;
  std::size_t pointCountBefore = 0U;
  std::size_t pointCountAfter = 0U;
  CreativeMeasurementPlanStatus planStatusBefore =
      CreativeMeasurementPlanStatus::NotRequested;
  CreativeMeasurementPlanStatus planStatusAfter =
      CreativeMeasurementPlanStatus::NotRequested;
  bool changed = false;
  bool accepted = false;
  std::string_view message = "no_measurement_change";
};

struct CreativeMeasurementReadout {
  bool visible = false;
  bool valid = false;
  bool completed = false;
  CreativeMeasurementMode mode = CreativeMeasurementMode::Distance;
  CreativeMeasurementAxis axis = CreativeMeasurementAxis::X;
  CreativeMeasurementSnapKind snapKind = CreativeMeasurementSnapKind::None;
  std::size_t pointCount = 0U;
  std::size_t segmentCount = 0U;
  double primaryValue = 0.0;
  double secondaryValue = 0.0;
  bool hasSecondaryValue = false;
  std::string_view primaryLabel = "Distance";
  std::string_view primaryUnit = "m";
  std::string_view secondaryLabel;
  std::string_view secondaryUnit;
};

struct CreativeMeasurementSegment {
  CreativeMeasurementPoint start;
  CreativeMeasurementPoint end;
};

struct CreativeMeasurementGeometry {
  bool visible = false;
  bool completed = false;
  bool closed = false;
  std::array<CreativeMeasurementPoint, kCreativeMeasurementPointCapacity>
      points{};
  std::size_t pointCount = 0U;
  std::array<CreativeMeasurementSegment, kCreativeMeasurementPointCapacity>
      segments{};
  std::size_t segmentCount = 0U;
};

[[nodiscard]] CreativeMeasurementState makeDefaultCreativeMeasurementState() noexcept;
[[nodiscard]] CreativeMeasurementPlan planCreativeMeasurement(
    const CreativeMeasurementPlanRequest& request) noexcept;
[[nodiscard]] CreativeMeasurementReadout buildCreativeMeasurementReadout(
    const CreativeMeasurementState& state) noexcept;
[[nodiscard]] CreativeMeasurementGeometry buildCreativeMeasurementGeometry(
    const CreativeMeasurementState& state) noexcept;
[[nodiscard]] std::string_view toString(CreativeMeasurementMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMeasurementAxis axis) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMeasurementSnapKind snapKind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMeasurementPlanStatus status) noexcept;
[[nodiscard]] CreativeMeasurementReceipt configureMeasurement(
    CreativeMeasurementState& state,
    CreativeMeasurementMode mode,
    CreativeMeasurementAxis axis,
    bool closePath) noexcept;
[[nodiscard]] CreativeMeasurementReceipt appendMeasurementPoint(
    CreativeMeasurementState& state,
    CreativeMeasurementPoint point) noexcept;
[[nodiscard]] CreativeMeasurementReceipt previewMeasurementPoint(
    CreativeMeasurementState& state,
    CreativeMeasurementPoint point) noexcept;
[[nodiscard]] CreativeMeasurementReceipt completeMeasurement(
    CreativeMeasurementState& state) noexcept;
[[nodiscard]] CreativeMeasurementReceipt clearMeasurement(
    CreativeMeasurementState& state) noexcept;
[[nodiscard]] CreativeMeasurementReceipt beginMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept;
[[nodiscard]] CreativeMeasurementReceipt updateMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept;
[[nodiscard]] CreativeMeasurementReceipt endMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept;
[[nodiscard]] CreativeMeasurementReceipt cancelMeasurement(
    CreativeMeasurementState& state) noexcept;
[[nodiscard]] CreativeMeasurementReceipt applyMeasurementToolIntent(
    CreativeMeasurementState& state,
    const CreativeToolIntent& intent) noexcept;

}  // namespace iggy3d::creative
