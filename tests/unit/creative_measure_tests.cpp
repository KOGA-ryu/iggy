#include "app/iggy3d/creative/tools/Measure.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs, double epsilon = 1.0e-9) {
  return std::abs(lhs - rhs) <= epsilon;
}

cr::CreativeMeasurementPoint worldPoint(
    double x,
    double y,
    double z,
    cr::CreativeMeasurementSnapKind snapKind =
        cr::CreativeMeasurementSnapKind::None) {
  cr::CreativeMeasurementPoint point;
  point.x = x;
  point.y = y;
  point.z = z;
  point.snapKind = snapKind;
  return point;
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

bool twoPointModesShareExactWorldSpaceMetrics() {
  const std::array points{worldPoint(0.0, 0.0, 0.0),
                          worldPoint(3.0, 4.0, 12.0)};
  const cr::CreativeMeasurementPlan distance = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Distance,
       cr::CreativeMeasurementAxis::X, points, false});
  const cr::CreativeMeasurementPlan axisX = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::AxisProjected,
       cr::CreativeMeasurementAxis::X, points, false});
  const cr::CreativeMeasurementPlan axisY = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::AxisProjected,
       cr::CreativeMeasurementAxis::Y, points, false});
  const cr::CreativeMeasurementPlan axisZ = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::AxisProjected,
       cr::CreativeMeasurementAxis::Z, points, false});
  const cr::CreativeMeasurementPlan vertical = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Vertical,
       cr::CreativeMeasurementAxis::Y, points, false});

  return expect(distance.accepted && distance.pointCount == 2U &&
                    distance.segmentCount == 1U &&
                    near(distance.directDistanceMeters, 13.0) &&
                    near(distance.horizontalDistanceMeters, std::sqrt(153.0)) &&
                    near(distance.verticalDistanceMeters, 4.0) &&
                    near(distance.signedRiseMeters, 4.0),
                "distance plan reports exact 3D, horizontal, and vertical metrics") &&
         expect(axisX.accepted && near(axisX.axisDistanceMeters, 3.0) &&
                    axisY.accepted && near(axisY.axisDistanceMeters, 4.0) &&
                    axisZ.accepted && near(axisZ.axisDistanceMeters, 12.0),
                "axis plan projects onto the requested world axis") &&
         expect(vertical.accepted &&
                    near(vertical.verticalDistanceMeters, 4.0),
                "vertical mode reuses the exact signed world delta");
}

bool slopeReportsSignedAngleAndBoundedGrade() {
  const std::array rising{worldPoint(0.0, 0.0, 0.0),
                          worldPoint(3.0, 4.0, 0.0)};
  const std::array falling{worldPoint(0.0, 4.0, 0.0),
                           worldPoint(3.0, 0.0, 0.0)};
  const std::array vertical{worldPoint(0.0, 0.0, 0.0),
                            worldPoint(0.0, 5.0, 0.0)};
  const auto rise = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Slope, cr::CreativeMeasurementAxis::X,
       rising, false});
  const auto fall = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Slope, cr::CreativeMeasurementAxis::X,
       falling, false});
  const auto upright = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Slope, cr::CreativeMeasurementAxis::X,
       vertical, false});

  return expect(rise.accepted && near(rise.slopeDegrees, 53.13010235415598) &&
                    near(rise.gradePercent, 133.33333333333334) &&
                    rise.gradeFinite,
                "rising slope reports signed angle and percent grade") &&
         expect(fall.accepted && near(fall.slopeDegrees, -53.13010235415598) &&
                    near(fall.gradePercent, -133.33333333333334),
                "falling slope preserves its sign") &&
         expect(upright.accepted && near(upright.slopeDegrees, 90.0) &&
                    !upright.gradeFinite && near(upright.gradePercent, 0.0),
                "vertical slope reports angle without fabricating infinite grade");
}

bool pathsMeasureOpenClosedAndPlanarArea() {
  const std::array rectangle{worldPoint(0.0, 0.0, 0.0),
                             worldPoint(3.0, 0.0, 0.0),
                             worldPoint(3.0, 0.0, 4.0),
                             worldPoint(0.0, 0.0, 4.0)};
  const auto open = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Perimeter,
       cr::CreativeMeasurementAxis::X, rectangle, false});
  const auto closed = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Perimeter,
       cr::CreativeMeasurementAxis::X, rectangle, true});
  const auto area = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Area, cr::CreativeMeasurementAxis::X,
       rectangle, false});

  return expect(open.accepted && open.segmentCount == 3U &&
                    near(open.perimeterMeters, 10.0),
                "open perimeter omits the closing edge") &&
         expect(closed.accepted && closed.segmentCount == 4U &&
                    near(closed.perimeterMeters, 14.0),
                "closed perimeter includes the closing edge") &&
         expect(area.accepted && area.segmentCount == 4U &&
                    near(area.perimeterMeters, 14.0) &&
                    near(area.areaSquareMeters, 12.0) &&
                    near(std::abs(area.areaNormal.y), 1.0),
                "area uses the same closed path and reports its plane normal");
}

bool invalidAndOversizedPlansFailSpecifically() {
  const std::array onePoint{worldPoint(0.0, 0.0, 0.0)};
  const std::array duplicate{worldPoint(1.0, 1.0, 1.0),
                             worldPoint(1.0, 1.0, 1.0)};
  const std::array nonPlanar{worldPoint(0.0, 0.0, 0.0),
                             worldPoint(2.0, 0.0, 0.0),
                             worldPoint(2.0, 0.0, 2.0),
                             worldPoint(0.0, 1.0, 2.0)};
  std::array invalidPoint{worldPoint(0.0, 0.0, 0.0),
                          worldPoint(1.0, 0.0, 0.0)};
  invalidPoint[1].x = std::numeric_limits<double>::quiet_NaN();
  std::vector<cr::CreativeMeasurementPoint> oversized(
      cr::kCreativeMeasurementPointCapacity + 1U,
      worldPoint(1.0, 0.0, 0.0));

  const auto tooFew = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Distance,
       cr::CreativeMeasurementAxis::X, onePoint, false});
  const auto degenerate = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Distance,
       cr::CreativeMeasurementAxis::X, duplicate, false});
  const auto warped = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Area, cr::CreativeMeasurementAxis::X,
       nonPlanar, false});
  const auto invalid = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Distance,
       cr::CreativeMeasurementAxis::X, invalidPoint, false});
  const auto capacity = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Perimeter,
       cr::CreativeMeasurementAxis::X, oversized, false});
  const auto invalidMode = cr::planCreativeMeasurement(
      {cr::CreativeMeasurementMode::Count,
       cr::CreativeMeasurementAxis::X, duplicate, false});

  return expect(!tooFew.accepted &&
                    tooFew.status ==
                        cr::CreativeMeasurementPlanStatus::TooFewPoints,
                "too few measurement points reject specifically") &&
         expect(!degenerate.accepted &&
                    degenerate.status ==
                        cr::CreativeMeasurementPlanStatus::Degenerate,
                "degenerate two-point measurement rejects") &&
         expect(!warped.accepted &&
                    warped.status == cr::CreativeMeasurementPlanStatus::NonPlanar,
                "non-planar area rejects instead of reporting a false area") &&
         expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeMeasurementPlanStatus::InvalidPoint,
                "non-finite measurement point rejects") &&
         expect(!capacity.accepted &&
                    capacity.status ==
                        cr::CreativeMeasurementPlanStatus::CapacityExceeded,
                "measurement point capacity rejects atomically") &&
         expect(!invalidMode.accepted &&
                    invalidMode.status ==
                        cr::CreativeMeasurementPlanStatus::InvalidMode,
                "invalid measurement mode rejects");
}

bool measurementVocabularyIsExhaustive() {
  return expect(cr::toString(cr::CreativeMeasurementMode::Distance) ==
                    "Distance" &&
                    cr::toString(cr::CreativeMeasurementMode::AxisProjected) ==
                        "Axis projected" &&
                    cr::toString(cr::CreativeMeasurementMode::Vertical) ==
                        "Vertical" &&
                    cr::toString(cr::CreativeMeasurementMode::Slope) == "Slope" &&
                    cr::toString(cr::CreativeMeasurementMode::Perimeter) ==
                        "Perimeter" &&
                    cr::toString(cr::CreativeMeasurementMode::Area) == "Area",
                "measurement modes have stable user-facing labels") &&
         expect(cr::toString(cr::CreativeMeasurementSnapKind::Grid) == "Grid" &&
                    cr::toString(cr::CreativeMeasurementSnapKind::Vertex) ==
                        "Vertex" &&
                    cr::toString(cr::CreativeMeasurementPlanStatus::Ready) ==
                        "Ready",
                "measurement snap and status vocabulary is stable");
}

bool configuredPathLifecycleOwnsPreviewAndCompletion() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  const auto configured = cr::configureMeasurement(
      state, cr::CreativeMeasurementMode::Area,
      cr::CreativeMeasurementAxis::Z, true);
  const auto first = cr::appendMeasurementPoint(
      state, worldPoint(0.0, 0.0, 0.0, cr::CreativeMeasurementSnapKind::Grid));
  const auto second = cr::appendMeasurementPoint(
      state, worldPoint(3.0, 0.0, 0.0,
                        cr::CreativeMeasurementSnapKind::Surface));
  const auto third = cr::appendMeasurementPoint(
      state, worldPoint(3.0, 0.0, 4.0,
                        cr::CreativeMeasurementSnapKind::Vertex));
  const auto preview = cr::previewMeasurementPoint(
      state, worldPoint(0.0, 0.0, 4.0,
                        cr::CreativeMeasurementSnapKind::Level));
  const auto fourth = cr::appendMeasurementPoint(
      state, worldPoint(0.0, 0.0, 4.0,
                        cr::CreativeMeasurementSnapKind::Level));
  const auto completed = cr::appendMeasurementPoint(
      state, worldPoint(0.0, 0.0, 4.0,
                        cr::CreativeMeasurementSnapKind::Level));

  return expect(configured.accepted && configured.changed &&
                    state.mode == cr::CreativeMeasurementMode::Area &&
                    state.axis == cr::CreativeMeasurementAxis::Z &&
                    state.closePath,
                "measurement configuration is durable tool state") &&
         expect(first.accepted && second.accepted && third.accepted &&
                    preview.accepted && fourth.accepted,
                "path points and preview share one accepted lifecycle") &&
         expect(completed.accepted && completed.changed && !state.active &&
                    state.completed && state.hasMeasurement &&
                    state.pointCount == 4U && !state.hasPreviewPoint &&
                    state.plan.accepted && near(state.plan.areaSquareMeters, 12.0),
                "repeating the final controller point completes a valid path");
}

bool invalidEndpointsRecoverAndCancellationKeepsConfiguration() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  static_cast<void>(cr::configureMeasurement(
      state, cr::CreativeMeasurementMode::Slope,
      cr::CreativeMeasurementAxis::Y, false));
  const auto first = cr::appendMeasurementPoint(
      state, worldPoint(1.0, 2.0, 3.0));
  const auto duplicate = cr::appendMeasurementPoint(
      state, worldPoint(1.0, 2.0, 3.0));
  const auto preview = cr::previewMeasurementPoint(
      state, worldPoint(5.0, 6.0, 3.0));
  const auto cancelled = cr::cancelMeasurement(state);

  return expect(first.accepted && state.pointCount == 0U,
                "cancelled lifecycle setup was accepted") &&
         expect(duplicate.accepted && !duplicate.changed,
                "duplicate endpoint leaves an active measurement recoverable") &&
         expect(preview.accepted && preview.changed &&
                    preview.planStatusAfter ==
                        cr::CreativeMeasurementPlanStatus::Ready,
                "valid motion recovers the exact preview plan") &&
         expect(cancelled.accepted && cancelled.changed && !state.active &&
                    !state.hasMeasurement && state.pointCount == 0U &&
                    state.mode == cr::CreativeMeasurementMode::Slope &&
                    state.axis == cr::CreativeMeasurementAxis::Y,
                "cancel clears transient data but preserves tool configuration");
}

bool measurementLifecycleEnforcesItsFixedPointCapacity() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  static_cast<void>(cr::configureMeasurement(
      state, cr::CreativeMeasurementMode::Perimeter,
      cr::CreativeMeasurementAxis::X, false));
  bool accepted = true;
  for (std::size_t index = 0U; index < cr::kCreativeMeasurementPointCapacity;
       ++index) {
    const auto receipt = cr::appendMeasurementPoint(
        state, worldPoint(static_cast<double>(index), 0.0, 0.0));
    accepted = accepted && receipt.accepted;
  }
  const std::uint64_t samplesBefore = state.sampleCount;
  const auto overflow = cr::appendMeasurementPoint(
      state, worldPoint(static_cast<double>(state.pointCount), 0.0, 0.0));
  const auto previewOverflow = cr::previewMeasurementPoint(
      state, worldPoint(999.0, 0.0, 0.0));

  return expect(accepted && state.pointCount ==
                                   cr::kCreativeMeasurementPointCapacity,
                "measurement lifecycle accepts its documented capacity") &&
         expect(!overflow.accepted && !overflow.changed &&
                    overflow.message == "measurement_capacity_exceeded" &&
                    !previewOverflow.accepted && !previewOverflow.changed &&
                    state.pointCount == cr::kCreativeMeasurementPointCapacity &&
                    state.sampleCount == samplesBefore,
                "measurement lifecycle rejects overflow atomically");
}

bool pointerLifecycleUsesWorldDestinationWhenAvailable() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  cr::CreativeToolPointerPacket point;
  point.x = 90.0;
  point.y = 45.0;
  point.hasWorldDestination = true;
  point.worldDestination = {1.5, 2.5, 3.5};
  const auto began = cr::beginMeasurement(state, point);

  return expect(began.accepted && near(state.startPoint.x, 1.5) &&
                    near(state.startPoint.y, 2.5) &&
                    near(state.startPoint.z, 3.5),
                "measurement lifecycle consumes world space instead of screen coordinates");
}

bool readoutOwnsSharedValuesUnitsAndSnapProvenance() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  static_cast<void>(cr::configureMeasurement(
      state, cr::CreativeMeasurementMode::Slope,
      cr::CreativeMeasurementAxis::X, false));
  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(0.0, 0.0, 0.0,
                        cr::CreativeMeasurementSnapKind::Grid)));
  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(3.0, 4.0, 0.0,
                        cr::CreativeMeasurementSnapKind::Vertex)));
  const cr::CreativeMeasurementReadout slope =
      cr::buildCreativeMeasurementReadout(state);

  static_cast<void>(cr::configureMeasurement(
      state, cr::CreativeMeasurementMode::Area,
      cr::CreativeMeasurementAxis::Z, true));
  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(0.0, 0.0, 0.0)));
  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(3.0, 0.0, 0.0)));
  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(3.0, 0.0, 4.0,
                        cr::CreativeMeasurementSnapKind::Opening)));
  const cr::CreativeMeasurementReadout area =
      cr::buildCreativeMeasurementReadout(state);

  return expect(slope.visible && slope.valid && slope.completed &&
                    slope.primaryLabel == "Slope" &&
                    slope.primaryUnit == "deg" &&
                    near(slope.primaryValue, 53.13010235415598) &&
                    slope.hasSecondaryValue &&
                    slope.secondaryLabel == "Grade" &&
                    slope.secondaryUnit == "%" &&
                    near(slope.secondaryValue, 133.33333333333334) &&
                    slope.snapKind ==
                        cr::CreativeMeasurementSnapKind::Vertex,
                "slope readout owns values, units, and endpoint provenance") &&
         expect(area.visible && area.valid && !area.completed &&
                    area.primaryLabel == "Area" &&
                    area.primaryUnit == "m2" &&
                    near(area.primaryValue, 6.0) &&
                    area.hasSecondaryValue &&
                    area.secondaryLabel == "Perimeter" &&
                    area.secondaryUnit == "m" &&
                    near(area.secondaryValue, 12.0) &&
                    area.snapKind ==
                        cr::CreativeMeasurementSnapKind::Opening,
                "area readout shares area and perimeter truth while active");
}

bool geometryOwnsPreviewClosureAndCapacityWithoutDegenerateEdges() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  static_cast<void>(cr::configureMeasurement(
      state, cr::CreativeMeasurementMode::Area,
      cr::CreativeMeasurementAxis::Y, true));
  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(0.0, 0.0, 0.0,
                        cr::CreativeMeasurementSnapKind::Grid)));
  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(3.0, 0.0, 0.0,
                        cr::CreativeMeasurementSnapKind::Vertex)));
  static_cast<void>(cr::previewMeasurementPoint(
      state, worldPoint(3.0, 0.0, 4.0,
                        cr::CreativeMeasurementSnapKind::Opening)));
  const cr::CreativeMeasurementGeometry preview =
      cr::buildCreativeMeasurementGeometry(state);

  static_cast<void>(cr::appendMeasurementPoint(
      state, worldPoint(3.0, 0.0, 4.0,
                        cr::CreativeMeasurementSnapKind::Opening)));
  const cr::CreativeMeasurementGeometry committed =
      cr::buildCreativeMeasurementGeometry(state);

  cr::CreativeMeasurementState empty;
  const cr::CreativeMeasurementGeometry hidden =
      cr::buildCreativeMeasurementGeometry(empty);

  return expect(preview.visible && !preview.completed && preview.closed &&
                    preview.pointCount == 3U && preview.segmentCount == 3U,
                "geometry includes the active preview and canonical area closure") &&
         expect(near(preview.points[2].x, 3.0) &&
                    near(preview.points[2].z, 4.0) &&
                    preview.points[2].snapKind ==
                        cr::CreativeMeasurementSnapKind::Opening &&
                    near(preview.segments[2].start.z, 4.0) &&
                    near(preview.segments[2].end.x, 0.0),
                "geometry preserves exact endpoint provenance and closure order") &&
         expect(committed.visible && !committed.completed && committed.closed &&
                    committed.pointCount == 3U && committed.segmentCount == 3U,
                "committed path uses the same geometry projection") &&
         expect(!hidden.visible && hidden.pointCount == 0U &&
                    hidden.segmentCount == 0U,
                "empty measurement emits no render geometry");
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
                  nonMeasurementIntentIsNoOp() &&
                  twoPointModesShareExactWorldSpaceMetrics() &&
                  slopeReportsSignedAngleAndBoundedGrade() &&
                  pathsMeasureOpenClosedAndPlanarArea() &&
                  invalidAndOversizedPlansFailSpecifically() &&
                  measurementVocabularyIsExhaustive() &&
                  configuredPathLifecycleOwnsPreviewAndCompletion() &&
                  invalidEndpointsRecoverAndCancellationKeepsConfiguration() &&
                  measurementLifecycleEnforcesItsFixedPointCapacity() &&
                  pointerLifecycleUsesWorldDestinationWhenAvailable() &&
                  readoutOwnsSharedValuesUnitsAndSnapProvenance() &&
                  geometryOwnsPreviewClosureAndCapacityWithoutDegenerateEdges();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
