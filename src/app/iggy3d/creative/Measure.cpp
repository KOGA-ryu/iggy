#include "app/iggy3d/creative/Measure.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeMeasurementPoint pointFromPointer(
    const CreativeToolPointerPacket& pointer) noexcept {
  return CreativeMeasurementPoint{pointer.x, pointer.y, pointer.target};
}

[[nodiscard]] bool samePoint(CreativeMeasurementPoint lhs,
                             CreativeMeasurementPoint rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y &&
         lhs.target.value == rhs.target.value;
}

[[nodiscard]] CreativeMeasurementReceipt makeReceipt(
    const CreativeMeasurementState& state,
    CreativeMeasurementChangeKind requestedChange) noexcept {
  CreativeMeasurementReceipt receipt;
  receipt.requestedChange = requestedChange;
  receipt.activeBefore = state.active;
  receipt.activeAfter = state.active;
  receipt.hasMeasurementBefore = state.hasMeasurement;
  receipt.hasMeasurementAfter = state.hasMeasurement;
  receipt.startPointBefore = state.startPoint;
  receipt.startPointAfter = state.startPoint;
  receipt.currentPointBefore = state.currentPoint;
  receipt.currentPointAfter = state.currentPoint;
  receipt.sampleCountBefore = state.sampleCount;
  receipt.sampleCountAfter = state.sampleCount;
  return receipt;
}

void refreshAfter(CreativeMeasurementReceipt& receipt,
                  const CreativeMeasurementState& state) noexcept {
  receipt.activeAfter = state.active;
  receipt.hasMeasurementAfter = state.hasMeasurement;
  receipt.startPointAfter = state.startPoint;
  receipt.currentPointAfter = state.currentPoint;
  receipt.sampleCountAfter = state.sampleCount;
}

}  // namespace

CreativeMeasurementState makeDefaultCreativeMeasurementState() noexcept {
  return {};
}

CreativeMeasurementReceipt clearMeasurement(
    CreativeMeasurementState& state) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::ClearMeasurement);
  receipt.accepted = true;

  if (!state.active && !state.hasMeasurement) {
    receipt.message = "measurement_already_empty";
    return receipt;
  }

  state = {};
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::ClearMeasurement;
  receipt.message = "measurement_cleared";
  return receipt;
}

CreativeMeasurementReceipt beginMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::BeginMeasurement);
  receipt.accepted = true;

  const CreativeMeasurementPoint point = pointFromPointer(pointer);
  state.active = true;
  state.hasMeasurement = true;
  state.startPoint = point;
  state.currentPoint = point;
  state.sampleCount = 1;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::BeginMeasurement;
  receipt.message = "measurement_began";
  return receipt;
}

CreativeMeasurementReceipt updateMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::UpdateMeasurement);

  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }

  receipt.accepted = true;
  const CreativeMeasurementPoint point = pointFromPointer(pointer);
  if (samePoint(state.currentPoint, point)) {
    receipt.message = "measurement_update_unchanged";
    return receipt;
  }

  state.currentPoint = point;
  ++state.sampleCount;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::UpdateMeasurement;
  receipt.message = "measurement_updated";
  return receipt;
}

CreativeMeasurementReceipt endMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::EndMeasurement);

  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }

  receipt.accepted = true;
  const CreativeMeasurementPoint point = pointFromPointer(pointer);
  if (!samePoint(state.currentPoint, point)) {
    state.currentPoint = point;
    ++state.sampleCount;
  }
  state.active = false;
  state.hasMeasurement = true;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::EndMeasurement;
  receipt.message = "measurement_ended";
  return receipt;
}

CreativeMeasurementReceipt cancelMeasurement(
    CreativeMeasurementState& state) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::CancelMeasurement);

  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }

  receipt.accepted = true;
  state = {};
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::CancelMeasurement;
  receipt.message = "measurement_cancelled";
  return receipt;
}

CreativeMeasurementReceipt applyMeasurementToolIntent(
    CreativeMeasurementState& state,
    const CreativeToolIntent& intent) noexcept {
  switch (intent.kind) {
    case CreativeToolIntentKind::BeginMeasurement:
      return beginMeasurement(state, intent.pointer);
    case CreativeToolIntentKind::UpdateMeasurement:
      return updateMeasurement(state, intent.pointer);
    case CreativeToolIntentKind::EndMeasurement:
      return endMeasurement(state, intent.pointer);
    case CreativeToolIntentKind::CancelToolAction:
      return cancelMeasurement(state);
    case CreativeToolIntentKind::NoIntent:
    case CreativeToolIntentKind::SelectObjectCandidate:
    case CreativeToolIntentKind::InspectObjectCandidate:
    case CreativeToolIntentKind::PreviewPointer:
      break;
  }

  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::None);
  receipt.message = "non_measurement_intent";
  return receipt;
}

}  // namespace iggy3d::creative
