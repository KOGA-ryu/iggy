#include "app/iggy3d/creative/DocumentSnap.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool expectNear(double actual,
                double expected,
                std::string_view message,
                double epsilon = 0.000001) {
  return expect(std::fabs(actual - expected) <= epsilon, message);
}

bool expectPoint(cr::CreativeDocumentSnapPoint3 point,
                 double x,
                 double y,
                 double z,
                 std::string_view message) {
  return expectNear(point.x, x, message) &&
         expectNear(point.y, y, message) &&
         expectNear(point.z, z, message);
}

bool expectBounds(cr::CreativeDocumentSnapBounds3 bounds,
                  cr::CreativeDocumentSnapPoint3 min,
                  cr::CreativeDocumentSnapPoint3 max,
                  std::string_view message) {
  return expectPoint(bounds.min, min.x, min.y, min.z, message) &&
         expectPoint(bounds.max, max.x, max.y, max.z, message);
}

bool defaultSettingsAreUnitGridOnXyz() {
  const cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  const cr::CreativeDocumentSnapReceipt receipt =
      cr::snapCreativeDocumentPoint({1.2, 2.7, 3.4}, settings);

  return expect(cr::isValidCreativeDocumentSnapSettings(settings),
                "default settings valid") &&
         expect(settings.mode == cr::CreativeDocumentSnapMode::Grid,
                "default mode grid") &&
         expect(settings.axes == cr::kCreativeDocumentSnapAxisXYZ,
                "default axes xyz") &&
         expect(settings.stepX == 1.0, "default step x") &&
         expect(settings.stepY == 1.0, "default step y") &&
         expect(settings.stepZ == 1.0, "default step z") &&
         expect(receipt.requested, "default requested") &&
         expect(receipt.accepted, "default accepted") &&
         expect(receipt.settingsValid, "default receipt valid") &&
         expect(receipt.snapped, "default snapped") &&
         expect(receipt.changed, "default changed") &&
         expectPoint(receipt.originalPoint, 1.2, 2.7, 3.4,
                     "default original") &&
         expectPoint(receipt.snappedPoint, 1.0, 3.0, 3.0,
                     "default snapped point") &&
         expect(receipt.status == "document_snap_snapped",
                "default status");
}

bool disabledModePassesThroughPointAndBounds() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.mode = cr::CreativeDocumentSnapMode::Disabled;
  settings.stepX = -1.0;
  settings.stepY = 0.0;
  settings.stepZ = std::numeric_limits<double>::infinity();
  const cr::CreativeDocumentSnapBounds3 bounds{{1.2, 2.7, 3.4},
                                               {4.6, 5.1, 6.9}};
  const cr::CreativeDocumentSnapReceipt pointReceipt =
      cr::snapCreativeDocumentPoint({1.2, 2.7, 3.4}, settings);
  const cr::CreativeDocumentSnapReceipt boundsReceipt =
      cr::snapCreativeDocumentBounds(bounds, settings);

  return expect(cr::isValidCreativeDocumentSnapSettings(settings),
                "disabled invalid steps still valid") &&
         expect(pointReceipt.accepted, "disabled point accepted") &&
         expect(pointReceipt.settingsValid, "disabled point valid") &&
         expect(!pointReceipt.snapped, "disabled point not snapped") &&
         expect(!pointReceipt.changed, "disabled point unchanged") &&
         expectPoint(pointReceipt.snappedPoint, 1.2, 2.7, 3.4,
                     "disabled point output") &&
         expect(pointReceipt.status == "document_snap_disabled",
                "disabled point status") &&
         expect(boundsReceipt.accepted, "disabled bounds accepted") &&
         expect(!boundsReceipt.snapped, "disabled bounds not snapped") &&
         expect(!boundsReceipt.changed, "disabled bounds unchanged") &&
         expectBounds(boundsReceipt.snappedBounds, bounds.min, bounds.max,
                      "disabled bounds output") &&
         expect(boundsReceipt.status == "document_snap_disabled",
                "disabled bounds status");
}

bool invalidGridStepsRejectAndReturnOriginal() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.stepY = 0.0;
  const cr::CreativeDocumentSnapReceipt receipt =
      cr::snapCreativeDocumentPoint({1.2, 2.7, 3.4}, settings);

  cr::CreativeDocumentSnapSettings xOnly =
      cr::makeDefaultCreativeDocumentSnapSettings();
  xOnly.axes = cr::kCreativeDocumentSnapAxisX;
  xOnly.stepY = 0.0;
  xOnly.stepZ = std::numeric_limits<double>::infinity();

  return expect(!cr::isValidCreativeDocumentSnapSettings(settings),
                "invalid settings rejected") &&
         expect(!receipt.accepted, "invalid not accepted") &&
         expect(!receipt.settingsValid, "invalid valid flag false") &&
         expect(!receipt.snapped, "invalid not snapped") &&
         expect(!receipt.changed, "invalid unchanged") &&
         expectPoint(receipt.snappedPoint, 1.2, 2.7, 3.4,
                     "invalid original output") &&
         expect(receipt.status == "invalid_document_snap_settings",
                "invalid status") &&
         expect(cr::isValidCreativeDocumentSnapSettings(xOnly),
                "x-only ignores disabled axis steps");
}

bool originRoundFormulaAppliesOnAllAxes() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.stepX = 2.0;
  settings.stepY = 3.0;
  settings.stepZ = 0.5;
  settings.originX = 1.0;
  settings.originY = 1.0;
  settings.originZ = 0.25;
  const cr::CreativeDocumentSnapReceipt receipt =
      cr::snapCreativeDocumentPoint({2.2, 5.0, 1.0}, settings);

  return expect(receipt.accepted, "origin accepted") &&
         expect(receipt.changed, "origin changed") &&
         expectPoint(receipt.snappedPoint, 3.0, 4.0, 1.25,
                     "origin output") &&
         expectNear(cr::snapCreativeDocumentScalar(2.2, 2.0, 1.0), 3.0,
                    "scalar formula x") &&
         expectNear(cr::snapCreativeDocumentScalar(5.0, 3.0, 1.0), 4.0,
                    "scalar formula y") &&
         expectNear(cr::snapCreativeDocumentScalar(1.0, 0.5, 0.25), 1.25,
                    "scalar formula z");
}

bool axisMasksSnapOnlyEnabledAxes() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  const cr::CreativeDocumentSnapPoint3 point{1.2, 2.7, 3.4};

  settings.axes = cr::kCreativeDocumentSnapAxisX;
  const cr::CreativeDocumentSnapReceipt xReceipt =
      cr::snapCreativeDocumentPoint(point, settings);
  settings.axes = cr::kCreativeDocumentSnapAxisY;
  const cr::CreativeDocumentSnapReceipt yReceipt =
      cr::snapCreativeDocumentPoint(point, settings);
  settings.axes = cr::kCreativeDocumentSnapAxisZ;
  const cr::CreativeDocumentSnapReceipt zReceipt =
      cr::snapCreativeDocumentPoint(point, settings);
  settings.axes = cr::kCreativeDocumentSnapAxisXYZ;
  const cr::CreativeDocumentSnapReceipt xyzReceipt =
      cr::snapCreativeDocumentPoint(point, settings);
  settings.axes = cr::kCreativeDocumentSnapAxisNone;
  settings.stepX = 0.0;
  settings.stepY = 0.0;
  settings.stepZ = 0.0;
  const cr::CreativeDocumentSnapReceipt noneReceipt =
      cr::snapCreativeDocumentPoint(point, settings);

  return expectPoint(xReceipt.snappedPoint, 1.0, 2.7, 3.4,
                     "x-only output") &&
         expect(xReceipt.snapped, "x-only snapped") &&
         expectPoint(yReceipt.snappedPoint, 1.2, 3.0, 3.4,
                     "y-only output") &&
         expect(yReceipt.snapped, "y-only snapped") &&
         expectPoint(zReceipt.snappedPoint, 1.2, 2.7, 3.0,
                     "z-only output") &&
         expect(zReceipt.snapped, "z-only snapped") &&
         expectPoint(xyzReceipt.snappedPoint, 1.0, 3.0, 3.0,
                     "xyz output") &&
         expect(noneReceipt.accepted, "none accepted") &&
         expect(noneReceipt.settingsValid, "none valid") &&
         expect(!noneReceipt.snapped, "none not snapped") &&
         expect(!noneReceipt.changed, "none unchanged") &&
         expectPoint(noneReceipt.snappedPoint, 1.2, 2.7, 3.4,
                     "none output") &&
         expect(noneReceipt.status == "document_snap_axes_disabled",
                "none status");
}

bool negativeCoordinatesRoundCorrectly() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.stepX = 0.5;
  settings.stepY = 1.0;
  settings.stepZ = 2.0;
  const cr::CreativeDocumentSnapReceipt receipt =
      cr::snapCreativeDocumentPoint({-1.26, -2.6, -3.1}, settings);

  return expect(receipt.accepted, "negative accepted") &&
         expect(receipt.changed, "negative changed") &&
         expectPoint(receipt.snappedPoint, -1.5, -3.0, -4.0,
                     "negative output");
}

bool alreadySnappedReportsUnchanged() {
  const cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  const cr::CreativeDocumentSnapReceipt receipt =
      cr::snapCreativeDocumentPoint({1.0, 2.0, 3.0}, settings);

  return expect(receipt.accepted, "already accepted") &&
         expect(receipt.snapped, "already snapped") &&
         expect(!receipt.changed, "already unchanged") &&
         expectPoint(receipt.snappedPoint, 1.0, 2.0, 3.0,
                     "already output") &&
         expect(receipt.status == "document_snap_already_snapped",
                "already status");
}

bool boundsSnapNormalizesPerAxisOrder() {
  cr::CreativeDocumentSnapSettings settings =
      cr::makeDefaultCreativeDocumentSnapSettings();
  settings.stepX = 10.0;
  settings.stepY = 10.0;
  settings.stepZ = 1.0;
  const cr::CreativeDocumentSnapBounds3 bounds{{5.0, 14.0, 0.2},
                                               {4.0, 6.0, 0.8}};
  const cr::CreativeDocumentSnapReceipt receipt =
      cr::snapCreativeDocumentBounds(bounds, settings);

  return expect(receipt.accepted, "bounds accepted") &&
         expect(receipt.snapped, "bounds snapped") &&
         expect(receipt.changed, "bounds changed") &&
         expectBounds(receipt.originalBounds, bounds.min, bounds.max,
                      "bounds original") &&
         expectBounds(receipt.snappedBounds,
                      cr::CreativeDocumentSnapPoint3{0.0, 10.0, 0.0},
                      cr::CreativeDocumentSnapPoint3{10.0, 10.0, 1.0},
                      "bounds normalized output") &&
         expect(receipt.snappedBounds.min.x <= receipt.snappedBounds.max.x,
                "bounds x ordered") &&
         expect(receipt.snappedBounds.min.y <= receipt.snappedBounds.max.y,
                "bounds y ordered") &&
         expect(receipt.snappedBounds.min.z <= receipt.snappedBounds.max.z,
                "bounds z ordered");
}

}  // namespace

int main() {
  const bool ok = defaultSettingsAreUnitGridOnXyz() &&
                  disabledModePassesThroughPointAndBounds() &&
                  invalidGridStepsRejectAndReturnOriginal() &&
                  originRoundFormulaAppliesOnAllAxes() &&
                  axisMasksSnapOnlyEnabledAxes() &&
                  negativeCoordinatesRoundCorrectly() &&
                  alreadySnappedReportsUnchanged() &&
                  boundsSnapNormalizesPerAxisOrder();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
