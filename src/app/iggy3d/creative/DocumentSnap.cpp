#include "app/iggy3d/creative/DocumentSnap.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool hasAxis(CreativeDocumentSnapAxisMask axes,
                           CreativeDocumentSnapAxisMask axis) noexcept {
  return (axes & axis) != 0;
}

[[nodiscard]] bool hasOnlyKnownAxes(
    CreativeDocumentSnapAxisMask axes) noexcept {
  return (axes & ~kCreativeDocumentSnapAxisXYZ) == 0;
}

[[nodiscard]] bool validStep(double step) noexcept {
  return std::isfinite(step) && step > 0.0;
}

[[nodiscard]] bool samePoint(CreativeDocumentSnapPoint3 lhs,
                             CreativeDocumentSnapPoint3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool sameBounds(CreativeDocumentSnapBounds3 lhs,
                              CreativeDocumentSnapBounds3 rhs) noexcept {
  return samePoint(lhs.min, rhs.min) && samePoint(lhs.max, rhs.max);
}

void setReceiptStatus(CreativeDocumentSnapReceipt& receipt,
                      std::string_view status) noexcept {
  receipt.message = status;
  receipt.status = status;
  receipt.reasonCode = status;
}

void prepareReceipt(CreativeDocumentSnapReceipt& receipt,
                    CreativeDocumentSnapSettings settings) noexcept {
  receipt.requested = true;
  receipt.mode = settings.mode;
  receipt.axes = settings.axes;
  receipt.settingsValid = isValidCreativeDocumentSnapSettings(settings);
  if (!receipt.settingsValid) {
    setReceiptStatus(receipt, "invalid_document_snap_settings");
    return;
  }

  receipt.accepted = true;
  if (settings.mode == CreativeDocumentSnapMode::Disabled) {
    setReceiptStatus(receipt, "document_snap_disabled");
    return;
  }

  if (settings.axes == kCreativeDocumentSnapAxisNone) {
    setReceiptStatus(receipt, "document_snap_axes_disabled");
    return;
  }

  receipt.snapped = true;
}

[[nodiscard]] CreativeDocumentSnapPoint3 snapPointValues(
    CreativeDocumentSnapPoint3 point,
    CreativeDocumentSnapSettings settings) noexcept {
  CreativeDocumentSnapPoint3 snapped = point;
  if (hasAxis(settings.axes, kCreativeDocumentSnapAxisX)) {
    snapped.x =
        snapCreativeDocumentScalar(point.x, settings.stepX, settings.originX);
  }
  if (hasAxis(settings.axes, kCreativeDocumentSnapAxisY)) {
    snapped.y =
        snapCreativeDocumentScalar(point.y, settings.stepY, settings.originY);
  }
  if (hasAxis(settings.axes, kCreativeDocumentSnapAxisZ)) {
    snapped.z =
        snapCreativeDocumentScalar(point.z, settings.stepZ, settings.originZ);
  }
  return snapped;
}

[[nodiscard]] CreativeDocumentSnapBounds3 normalizeBounds(
    CreativeDocumentSnapBounds3 bounds) noexcept {
  if (bounds.min.x > bounds.max.x) {
    std::swap(bounds.min.x, bounds.max.x);
  }
  if (bounds.min.y > bounds.max.y) {
    std::swap(bounds.min.y, bounds.max.y);
  }
  if (bounds.min.z > bounds.max.z) {
    std::swap(bounds.min.z, bounds.max.z);
  }
  return bounds;
}

}  // namespace

CreativeDocumentSnapSettings
makeDefaultCreativeDocumentSnapSettings() noexcept {
  return {};
}

bool isValidCreativeDocumentSnapSettings(
    CreativeDocumentSnapSettings settings) noexcept {
  if (settings.mode == CreativeDocumentSnapMode::Disabled) {
    return true;
  }

  if (!hasOnlyKnownAxes(settings.axes)) {
    return false;
  }

  if (settings.axes == kCreativeDocumentSnapAxisNone) {
    return true;
  }

  if (hasAxis(settings.axes, kCreativeDocumentSnapAxisX) &&
      !validStep(settings.stepX)) {
    return false;
  }
  if (hasAxis(settings.axes, kCreativeDocumentSnapAxisY) &&
      !validStep(settings.stepY)) {
    return false;
  }
  if (hasAxis(settings.axes, kCreativeDocumentSnapAxisZ) &&
      !validStep(settings.stepZ)) {
    return false;
  }

  return true;
}

double snapCreativeDocumentScalar(double value,
                                  double step,
                                  double origin) noexcept {
  if (!validStep(step)) {
    return value;
  }

  return origin + std::round((value - origin) / step) * step;
}

CreativeDocumentSnapReceipt snapCreativeDocumentPoint(
    CreativeDocumentSnapPoint3 point,
    CreativeDocumentSnapSettings settings) noexcept {
  CreativeDocumentSnapReceipt receipt;
  receipt.originalPoint = point;
  receipt.snappedPoint = point;
  prepareReceipt(receipt, settings);

  if (!receipt.accepted || !receipt.snapped) {
    return receipt;
  }

  receipt.snappedPoint = snapPointValues(point, settings);
  receipt.changed = !samePoint(receipt.originalPoint, receipt.snappedPoint);
  setReceiptStatus(receipt,
                   receipt.changed ? "document_snap_snapped"
                                   : "document_snap_already_snapped");
  return receipt;
}

CreativeDocumentSnapReceipt snapCreativeDocumentBounds(
    CreativeDocumentSnapBounds3 bounds,
    CreativeDocumentSnapSettings settings) noexcept {
  CreativeDocumentSnapReceipt receipt;
  receipt.originalBounds = bounds;
  receipt.snappedBounds = bounds;
  prepareReceipt(receipt, settings);

  if (!receipt.accepted || !receipt.snapped) {
    return receipt;
  }

  receipt.snappedBounds.min = snapPointValues(bounds.min, settings);
  receipt.snappedBounds.max = snapPointValues(bounds.max, settings);
  receipt.snappedBounds = normalizeBounds(receipt.snappedBounds);
  receipt.changed = !sameBounds(receipt.originalBounds, receipt.snappedBounds);
  setReceiptStatus(receipt,
                   receipt.changed ? "document_snap_snapped"
                                   : "document_snap_already_snapped");
  return receipt;
}

}  // namespace iggy3d::creative
