#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeMeasurementChangeKind : std::uint8_t {
  None,
  BeginMeasurement,
  UpdateMeasurement,
  EndMeasurement,
  CancelMeasurement,
  ClearMeasurement,
};

struct CreativeMeasurementPoint {
  double x = 0.0;
  double y = 0.0;
  TargetRef target;
};

struct CreativeMeasurementState {
  bool active = false;
  bool hasMeasurement = false;
  CreativeMeasurementPoint startPoint;
  CreativeMeasurementPoint currentPoint;
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
  bool changed = false;
  bool accepted = false;
  std::string_view message = "no_measurement_change";
};

[[nodiscard]] CreativeMeasurementState makeDefaultCreativeMeasurementState() noexcept;
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
