#include "app/iggy3d/creative/spatial/Snap.hpp"

#include "core/math/Snap.hpp"

#include <cmath>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool hasAxis(CreativeSnapAxisMask axes,
                           CreativeSnapAxisMask axis) noexcept {
  return (axes & axis) != 0;
}

[[nodiscard]] bool samePoint(CreativeSnapPoint2 lhs,
                             CreativeSnapPoint2 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

[[nodiscard]] bool validStep(double step) noexcept {
  return std::isfinite(step) && step > 0.0;
}

}  // namespace

CreativeSnapSettings makeDefaultCreativeSnapSettings() noexcept {
  return {};
}

bool isValidSnapSettings(CreativeSnapSettings settings) noexcept {
  if (settings.mode == CreativeSnapMode::Disabled) {
    return true;
  }

  if (settings.axes == kCreativeSnapAxisNone) {
    return true;
  }

  if (hasAxis(settings.axes, kCreativeSnapAxisX) && !validStep(settings.stepX)) {
    return false;
  }
  if (hasAxis(settings.axes, kCreativeSnapAxisY) && !validStep(settings.stepY)) {
    return false;
  }

  return true;
}

double snapScalar(double value, double step, double origin) noexcept {
  return iggy3d::snapScalarToGrid(value, step, origin);
}

CreativeSnapReceipt snapPoint(CreativeSnapPoint2 point,
                              CreativeSnapSettings settings) noexcept {
  CreativeSnapReceipt receipt;
  receipt.inputPoint = point;
  receipt.outputPoint = point;
  receipt.settingsValid = isValidSnapSettings(settings);

  if (!receipt.settingsValid) {
    receipt.message = "invalid_snap_settings";
    return receipt;
  }

  receipt.accepted = true;
  if (settings.mode == CreativeSnapMode::Disabled) {
    receipt.message = "snap_disabled";
    return receipt;
  }

  if (settings.axes == kCreativeSnapAxisNone) {
    receipt.message = "snap_axes_disabled";
    return receipt;
  }

  if (hasAxis(settings.axes, kCreativeSnapAxisX)) {
    receipt.outputPoint.x = snapScalar(point.x, settings.stepX, settings.originX);
  }
  if (hasAxis(settings.axes, kCreativeSnapAxisY)) {
    receipt.outputPoint.y = snapScalar(point.y, settings.stepY, settings.originY);
  }

  receipt.snapped = true;
  receipt.changed = !samePoint(receipt.inputPoint, receipt.outputPoint);
  receipt.message = receipt.changed ? "snapped" : "already_snapped";
  return receipt;
}

CreativeToolPointerPacket snapPointerPacket(
    const CreativeToolPointerPacket& pointer,
    CreativeSnapSettings settings) noexcept {
  CreativeToolPointerPacket snapped = pointer;
  const CreativeSnapReceipt receipt = snapPoint({pointer.x, pointer.y},
                                                settings);
  snapped.x = receipt.outputPoint.x;
  snapped.y = receipt.outputPoint.y;
  return snapped;
}

}  // namespace iggy3d::creative
