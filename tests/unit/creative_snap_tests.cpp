#include "app/iggy3d/creative/Snap.hpp"

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

bool expectPoint(cr::CreativeSnapPoint2 point,
                 double x,
                 double y,
                 std::string_view message) {
  return expect(point.x == x, message) && expect(point.y == y, message);
}

bool defaultSettingsValidAndSnapToUnitGrid() {
  const cr::CreativeSnapSettings settings =
      cr::makeDefaultCreativeSnapSettings();
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({1.2, 2.7}, settings);

  return expect(cr::isValidSnapSettings(settings), "default settings valid") &&
         expect(settings.mode == cr::CreativeSnapMode::Grid,
                "default mode grid") &&
         expect(settings.axes == cr::kCreativeSnapAxisXY,
                "default axes xy") &&
         expect(settings.stepX == 1.0, "default step x") &&
         expect(settings.stepY == 1.0, "default step y") &&
         expect(receipt.accepted, "default snap accepted") &&
         expect(receipt.settingsValid, "default receipt valid") &&
         expect(receipt.snapped, "default receipt snapped") &&
         expect(receipt.changed, "default receipt changed") &&
         expectPoint(receipt.outputPoint, 1.0, 3.0, "default output");
}

bool disabledModeReturnsOriginalPoint() {
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.mode = cr::CreativeSnapMode::Disabled;
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({1.2, 2.7}, settings);

  return expect(receipt.accepted, "disabled accepted") &&
         expect(receipt.settingsValid, "disabled valid") &&
         expect(!receipt.snapped, "disabled not snapped") &&
         expect(!receipt.changed, "disabled unchanged") &&
         expectPoint(receipt.outputPoint, 1.2, 2.7, "disabled output") &&
         expect(receipt.message == "snap_disabled", "disabled message");
}

bool invalidStepRejectsSnap() {
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.stepX = 0.0;
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({1.2, 2.7}, settings);

  return expect(!cr::isValidSnapSettings(settings), "invalid settings") &&
         expect(!receipt.accepted, "invalid not accepted") &&
         expect(!receipt.settingsValid, "invalid receipt valid flag") &&
         expect(!receipt.snapped, "invalid not snapped") &&
         expect(!receipt.changed, "invalid unchanged") &&
         expectPoint(receipt.outputPoint, 1.2, 2.7, "invalid output") &&
         expect(receipt.message == "invalid_snap_settings",
                "invalid message");
}

bool nearestRoundingOnBothAxes() {
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.stepX = 0.5;
  settings.stepY = 2.0;
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({1.26, 2.9}, settings);

  return expect(receipt.accepted, "nearest accepted") &&
         expect(receipt.changed, "nearest changed") &&
         expectPoint(receipt.outputPoint, 1.5, 2.0, "nearest output") &&
         expect(cr::snapScalar(1.26, 0.5, 0.0) == 1.5,
                "nearest scalar");
}

bool nonZeroOriginRounding() {
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.stepX = 2.0;
  settings.stepY = 3.0;
  settings.originX = 1.0;
  settings.originY = 1.0;
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({2.2, 5.0}, settings);

  return expect(receipt.accepted, "origin accepted") &&
         expect(receipt.changed, "origin changed") &&
         expectPoint(receipt.outputPoint, 3.0, 4.0, "origin output");
}

bool xOnlySnappingPreservesY() {
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.axes = cr::kCreativeSnapAxisX;
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({1.2, 2.7}, settings);

  return expect(receipt.accepted, "x-only accepted") &&
         expect(receipt.snapped, "x-only snapped") &&
         expectPoint(receipt.outputPoint, 1.0, 2.7, "x-only output");
}

bool yOnlySnappingPreservesX() {
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.axes = cr::kCreativeSnapAxisY;
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({1.2, 2.7}, settings);

  return expect(receipt.accepted, "y-only accepted") &&
         expect(receipt.snapped, "y-only snapped") &&
         expectPoint(receipt.outputPoint, 1.2, 3.0, "y-only output");
}

bool alreadySnappedPointReportsUnchanged() {
  const cr::CreativeSnapSettings settings =
      cr::makeDefaultCreativeSnapSettings();
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({1.0, 3.0}, settings);

  return expect(receipt.accepted, "already accepted") &&
         expect(receipt.snapped, "already snapped") &&
         expect(!receipt.changed, "already unchanged") &&
         expectPoint(receipt.outputPoint, 1.0, 3.0, "already output") &&
         expect(receipt.message == "already_snapped", "already message");
}

bool negativeCoordinatesRoundCorrectly() {
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.stepX = 0.5;
  settings.stepY = 1.0;
  const cr::CreativeSnapReceipt receipt = cr::snapPoint({-1.26, -2.6},
                                                        settings);

  return expect(receipt.accepted, "negative accepted") &&
         expect(receipt.changed, "negative changed") &&
         expectPoint(receipt.outputPoint, -1.5, -3.0, "negative output");
}

bool pointerPacketHelperPreservesMetadata() {
  cr::CreativeToolPointerPacket pointer;
  pointer.x = 1.2;
  pointer.y = 2.7;
  pointer.button = cr::CreativeToolPointerButton::Secondary;
  pointer.modifiers = cr::kCreativeToolModifierShift |
                      cr::kCreativeToolModifierAlt;
  pointer.target.value = 42;

  const cr::CreativeToolPointerPacket snapped =
      cr::snapPointerPacket(pointer, cr::makeDefaultCreativeSnapSettings());

  return expect(snapped.x == 1.0, "pointer x") &&
         expect(snapped.y == 3.0, "pointer y") &&
         expect(snapped.button == cr::CreativeToolPointerButton::Secondary,
                "pointer button") &&
         expect(snapped.modifiers == pointer.modifiers,
                "pointer modifiers") &&
         expect(snapped.target.value == 42U, "pointer target");
}

}  // namespace

int main() {
  const bool ok = defaultSettingsValidAndSnapToUnitGrid() &&
                  disabledModeReturnsOriginalPoint() &&
                  invalidStepRejectsSnap() &&
                  nearestRoundingOnBothAxes() &&
                  nonZeroOriginRounding() &&
                  xOnlySnappingPreservesY() &&
                  yOnlySnappingPreservesX() &&
                  alreadySnappedPointReportsUnchanged() &&
                  negativeCoordinatesRoundCorrectly() &&
                  pointerPacketHelperPreservesMetadata();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
