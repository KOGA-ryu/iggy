#include "app/iggy3d/creative/tools/Measure.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeToolPointerPacket pointer(double x, double y, cr::Id targetId = 0) {
  cr::CreativeToolPointerPacket packet;
  packet.x = x;
  packet.y = y;
  packet.target.value = targetId;
  return packet;
}

cr::CreativeToolIntent measurementIntent(cr::CreativeToolIntentKind kind,
                                         double x,
                                         double y,
                                         cr::Id targetId = 0) {
  cr::CreativeToolIntent intent;
  intent.kind = kind;
  intent.tool = cr::Tool::Measure;
  intent.pointer = pointer(x, y, targetId);
  return intent;
}

bool expectPoint(cr::CreativeMeasurementPoint point,
                 double x,
                 double y,
                 cr::Id targetId,
                 std::string_view label) {
  return expect(point.x == x, label) &&
         expect(point.y == y, label) &&
         expect(point.target.value == targetId, label);
}

bool defaultStateInactiveAndEmpty() {
  const cr::CreativeMeasurementState state =
      cr::makeDefaultCreativeMeasurementState();

  return expect(!state.active, "default inactive") &&
         expect(!state.hasMeasurement, "default empty") &&
         expect(state.sampleCount == 0U, "default sample count");
}

bool beginSetsActiveAndStoresPoints() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt receipt =
      cr::beginMeasurement(state, pointer(1.0, 2.0, 7));

  return expect(receipt.accepted, "begin accepted") &&
         expect(receipt.changed, "begin changed") &&
         expect(receipt.appliedChange ==
                    cr::CreativeMeasurementChangeKind::BeginMeasurement,
                "begin applied") &&
         expect(!receipt.activeBefore, "begin active before") &&
         expect(receipt.activeAfter, "begin active after") &&
         expect(!receipt.hasMeasurementBefore, "begin had none before") &&
         expect(receipt.hasMeasurementAfter, "begin has measurement after") &&
         expectPoint(state.startPoint, 1.0, 2.0, 7, "begin start") &&
         expectPoint(state.currentPoint, 1.0, 2.0, 7, "begin current") &&
         expect(state.sampleCount == 1U, "begin sample count");
}

bool beginWhileActiveRestarts() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt first =
      cr::beginMeasurement(state, pointer(1.0, 2.0, 7));
  const cr::CreativeMeasurementReceipt second =
      cr::beginMeasurement(state, pointer(5.0, 6.0, 9));

  return expect(first.changed, "restart setup begin") &&
         expect(second.accepted, "restart accepted") &&
         expect(second.changed, "restart changed") &&
         expect(second.activeBefore, "restart active before") &&
         expect(second.activeAfter, "restart active after") &&
         expectPoint(second.startPointBefore, 1.0, 2.0, 7,
                     "restart start before") &&
         expectPoint(state.startPoint, 5.0, 6.0, 9, "restart start") &&
         expectPoint(state.currentPoint, 5.0, 6.0, 9, "restart current") &&
         expect(state.sampleCount == 1U, "restart sample count reset");
}

bool updateWhileActiveChangesCurrentOnly() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt begin =
      cr::beginMeasurement(state, pointer(1.0, 2.0, 7));
  const cr::CreativeMeasurementReceipt update =
      cr::updateMeasurement(state, pointer(3.0, 4.0, 9));

  return expect(begin.changed, "update setup begin") &&
         expect(update.accepted, "update accepted") &&
         expect(update.changed, "update changed") &&
         expect(update.appliedChange ==
                    cr::CreativeMeasurementChangeKind::UpdateMeasurement,
                "update applied") &&
         expectPoint(state.startPoint, 1.0, 2.0, 7,
                     "update keeps start") &&
         expectPoint(state.currentPoint, 3.0, 4.0, 9,
                     "update changes current") &&
         expect(state.sampleCount == 2U, "update sample count");
}

bool updateWhileInactiveIsNoOp() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt receipt =
      cr::updateMeasurement(state, pointer(3.0, 4.0, 9));

  return expect(!receipt.accepted, "inactive update not accepted") &&
         expect(!receipt.changed, "inactive update unchanged") &&
         expect(receipt.message == "measurement_not_active",
                "inactive update message") &&
         expect(!state.active, "inactive update state inactive") &&
         expect(!state.hasMeasurement, "inactive update state empty");
}

bool endWhileActiveCompletesMeasurement() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt begin =
      cr::beginMeasurement(state, pointer(1.0, 2.0, 7));
  const cr::CreativeMeasurementReceipt end =
      cr::endMeasurement(state, pointer(5.0, 6.0, 9));

  return expect(begin.changed, "end setup begin") &&
         expect(end.accepted, "end accepted") &&
         expect(end.changed, "end changed") &&
         expect(end.appliedChange ==
                    cr::CreativeMeasurementChangeKind::EndMeasurement,
                "end applied") &&
         expect(end.activeBefore, "end active before") &&
         expect(!end.activeAfter, "end active after") &&
         expect(end.hasMeasurementAfter, "end keeps measurement") &&
         expect(!state.active, "end state inactive") &&
         expect(state.hasMeasurement, "end state has measurement") &&
         expectPoint(state.startPoint, 1.0, 2.0, 7, "end start") &&
         expectPoint(state.currentPoint, 5.0, 6.0, 9, "end current") &&
         expect(state.sampleCount == 2U, "end sample count");
}

bool endWhileInactiveIsNoOp() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt receipt =
      cr::endMeasurement(state, pointer(5.0, 6.0, 9));

  return expect(!receipt.accepted, "inactive end not accepted") &&
         expect(!receipt.changed, "inactive end unchanged") &&
         expect(receipt.message == "measurement_not_active",
                "inactive end message") &&
         expect(!state.active, "inactive end state inactive") &&
         expect(!state.hasMeasurement, "inactive end state empty");
}

bool cancelWhileActiveClearsMeasurement() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt begin =
      cr::beginMeasurement(state, pointer(1.0, 2.0, 7));
  const cr::CreativeMeasurementReceipt cancel = cr::cancelMeasurement(state);

  return expect(begin.changed, "cancel setup begin") &&
         expect(cancel.accepted, "cancel accepted") &&
         expect(cancel.changed, "cancel changed") &&
         expect(cancel.appliedChange ==
                    cr::CreativeMeasurementChangeKind::CancelMeasurement,
                "cancel applied") &&
         expect(cancel.activeBefore, "cancel active before") &&
         expect(!cancel.activeAfter, "cancel active after") &&
         expect(!cancel.hasMeasurementAfter, "cancel clears measurement") &&
         expect(!state.active, "cancel state inactive") &&
         expect(!state.hasMeasurement, "cancel state empty") &&
         expect(state.sampleCount == 0U, "cancel sample count");
}

bool cancelWhileInactiveIsNoOp() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt receipt = cr::cancelMeasurement(state);

  return expect(!receipt.accepted, "inactive cancel not accepted") &&
         expect(!receipt.changed, "inactive cancel unchanged") &&
         expect(receipt.message == "measurement_not_active",
                "inactive cancel message") &&
         expect(!state.active, "inactive cancel state inactive") &&
         expect(!state.hasMeasurement, "inactive cancel state empty");
}

bool applyMeasurementToolIntentRoutesMeasurementKinds() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt begin =
      cr::applyMeasurementToolIntent(
          state,
          measurementIntent(cr::CreativeToolIntentKind::BeginMeasurement,
                            1.0,
                            2.0,
                            7));
  const cr::CreativeMeasurementReceipt update =
      cr::applyMeasurementToolIntent(
          state,
          measurementIntent(cr::CreativeToolIntentKind::UpdateMeasurement,
                            3.0,
                            4.0,
                            8));
  const cr::CreativeMeasurementReceipt end =
      cr::applyMeasurementToolIntent(
          state,
          measurementIntent(cr::CreativeToolIntentKind::EndMeasurement,
                            5.0,
                            6.0,
                            9));
  const cr::CreativeMeasurementReceipt restart =
      cr::applyMeasurementToolIntent(
          state,
          measurementIntent(cr::CreativeToolIntentKind::BeginMeasurement,
                            7.0,
                            8.0,
                            10));
  cr::CreativeToolIntent cancelIntent;
  cancelIntent.kind = cr::CreativeToolIntentKind::CancelToolAction;
  cancelIntent.tool = cr::Tool::Measure;
  const cr::CreativeMeasurementReceipt cancel =
      cr::applyMeasurementToolIntent(state, cancelIntent);

  return expect(begin.appliedChange ==
                    cr::CreativeMeasurementChangeKind::BeginMeasurement,
                "apply begin") &&
         expect(update.appliedChange ==
                    cr::CreativeMeasurementChangeKind::UpdateMeasurement,
                "apply update") &&
         expect(end.appliedChange ==
                    cr::CreativeMeasurementChangeKind::EndMeasurement,
                "apply end") &&
         expect(restart.appliedChange ==
                    cr::CreativeMeasurementChangeKind::BeginMeasurement,
                "apply restart") &&
         expect(cancel.appliedChange ==
                    cr::CreativeMeasurementChangeKind::CancelMeasurement,
                "apply cancel") &&
         expect(!state.active, "apply final inactive") &&
         expect(!state.hasMeasurement, "apply cancel cleared");
}

bool nonMeasurementIntentIsNoOp() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const cr::CreativeMeasurementReceipt begin =
      cr::beginMeasurement(state, pointer(1.0, 2.0, 7));

  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::PreviewPointer;
  intent.tool = cr::Tool::Measure;
  intent.pointer = pointer(3.0, 4.0, 9);

  const cr::CreativeMeasurementReceipt receipt =
      cr::applyMeasurementToolIntent(state, intent);

  return expect(begin.changed, "non-measurement setup begin") &&
         expect(!receipt.accepted, "non-measurement not accepted") &&
         expect(!receipt.changed, "non-measurement unchanged") &&
         expect(receipt.appliedChange == cr::CreativeMeasurementChangeKind::None,
                "non-measurement no applied change") &&
         expect(state.active, "non-measurement active preserved") &&
         expect(state.hasMeasurement, "non-measurement data preserved") &&
         expectPoint(state.currentPoint, 1.0, 2.0, 7,
                     "non-measurement current preserved") &&
         expect(receipt.message == "non_measurement_intent",
                "non-measurement message");
}

}  // namespace

int main() {
  const bool ok = defaultStateInactiveAndEmpty() &&
                  beginSetsActiveAndStoresPoints() &&
                  beginWhileActiveRestarts() &&
                  updateWhileActiveChangesCurrentOnly() &&
                  updateWhileInactiveIsNoOp() &&
                  endWhileActiveCompletesMeasurement() &&
                  endWhileInactiveIsNoOp() &&
                  cancelWhileActiveClearsMeasurement() &&
                  cancelWhileInactiveIsNoOp() &&
                  applyMeasurementToolIntentRoutesMeasurementKinds() &&
                  nonMeasurementIntentIsNoOp();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
