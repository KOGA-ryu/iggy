#include "app/iggy3d/creative/tools/MeasurementAnnotation.hpp"

#include "app/iggy3d/creative/tools/Measure.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

static_assert(kCreativeMeasurementAnnotationPointCapacity ==
              kCreativeMeasurementPointCapacity);

[[nodiscard]] bool validName(std::string_view name) noexcept {
  if (name.empty() || name.size() > kCreativeMeasurementAnnotationNameCapacity) {
    return false;
  }
  bool hasVisibleCharacter = false;
  for (const char rawValue : name) {
    const auto value = static_cast<unsigned char>(rawValue);
    if (value < 0x20U || value == 0x7fU) {
      return false;
    }
    hasVisibleCharacter = hasVisibleCharacter || value != 0x20U;
  }
  return hasVisibleCharacter;
}

[[nodiscard]] bool finitePoint(
    const CreativeMeasurementAnnotationPoint& point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.y) &&
         std::isfinite(point.z) &&
         point.snapKind < CreativeMeasurementSnapKind::Count;
}

[[nodiscard]] CreativeMeasurementPoint toMeasurementPoint(
    const CreativeMeasurementAnnotationPoint& point) noexcept {
  CreativeMeasurementPoint result;
  result.x = point.x;
  result.y = point.y;
  result.z = point.z;
  result.snapKind = point.snapKind;
  return result;
}

[[nodiscard]] bool validContent(
    const CreativeMeasurementAnnotation& annotation) noexcept {
  if (!validName(annotation.name) ||
      annotation.mode >= CreativeMeasurementMode::Count ||
      annotation.axis >= CreativeMeasurementAxis::Count ||
      annotation.pointCount > kCreativeMeasurementPointCapacity) {
    return false;
  }

  std::array<CreativeMeasurementPoint, kCreativeMeasurementPointCapacity>
      points{};
  for (std::size_t index = 0U; index < annotation.pointCount; ++index) {
    if (!finitePoint(annotation.points[index])) {
      return false;
    }
    points[index] = toMeasurementPoint(annotation.points[index]);
  }
  return planCreativeMeasurement(
             {annotation.mode, annotation.axis,
              std::span<const CreativeMeasurementPoint>{points.data(),
                                                        annotation.pointCount},
              annotation.closePath})
      .accepted;
}

void setStatus(CreativeMeasurementAnnotationMutationReceipt& receipt,
               CreativeMeasurementAnnotationMutationStatus status,
               std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

}  // namespace

CreativeMeasurementAnnotationBuildResult buildCreativeMeasurementAnnotation(
    const CreativeMeasurementState& state,
    std::string_view name) {
  CreativeMeasurementAnnotationBuildResult result;
  result.requested = true;
  if (!state.completed || state.active || state.hasPreviewPoint) {
    result.status =
        CreativeMeasurementAnnotationBuildStatus::IncompleteMeasurement;
    result.reasonCode = "creative_measurement_annotation_incomplete";
    return result;
  }
  if (!validName(name)) {
    result.status = CreativeMeasurementAnnotationBuildStatus::InvalidName;
    result.reasonCode = "creative_measurement_annotation_name_invalid";
    return result;
  }
  if (!state.plan.accepted ||
      state.pointCount > kCreativeMeasurementPointCapacity) {
    result.status = CreativeMeasurementAnnotationBuildStatus::InvalidMeasurement;
    result.reasonCode = "creative_measurement_annotation_measurement_invalid";
    return result;
  }

  CreativeMeasurementAnnotation annotation;
  annotation.name = std::string{name};
  annotation.mode = state.mode;
  annotation.axis = state.axis;
  annotation.closePath = state.closePath;
  annotation.pointCount = state.pointCount;
  for (std::size_t index = 0U; index < state.pointCount; ++index) {
    annotation.points[index] = {state.points[index].x, state.points[index].y,
                                state.points[index].z,
                                state.points[index].snapKind};
  }
  if (!validContent(annotation)) {
    result.status = CreativeMeasurementAnnotationBuildStatus::InvalidMeasurement;
    result.reasonCode = "creative_measurement_annotation_measurement_invalid";
    return result;
  }

  result.accepted = true;
  result.status = CreativeMeasurementAnnotationBuildStatus::Ready;
  result.annotation = std::move(annotation);
  result.reasonCode = "creative_measurement_annotation_ready";
  return result;
}

bool validateCreativeMeasurementAnnotation(
    const CreativeMeasurementAnnotation& annotation) noexcept {
  return annotation.id != kInvalidCreativeMeasurementAnnotationId &&
         validContent(annotation);
}

bool validateCreativeMeasurementAnnotationStore(
    const CreativeMeasurementAnnotationStore& store) noexcept {
  if (store.version != kCreativeMeasurementAnnotationStoreVersion ||
      store.nextAnnotationId == kInvalidCreativeMeasurementAnnotationId ||
      store.annotations.size() > kCreativeMeasurementAnnotationCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < store.annotations.size(); ++index) {
    const CreativeMeasurementAnnotation& annotation = store.annotations[index];
    if (!validateCreativeMeasurementAnnotation(annotation) ||
        annotation.id >= store.nextAnnotationId) {
      return false;
    }
    for (std::size_t previous = 0U; previous < index; ++previous) {
      if (store.annotations[previous].id == annotation.id) {
        return false;
      }
    }
  }
  return true;
}

const CreativeMeasurementAnnotation* findCreativeMeasurementAnnotation(
    const CreativeMeasurementAnnotationStore& store,
    CreativeMeasurementAnnotationId annotationId) noexcept {
  const auto found = std::find_if(
      store.annotations.begin(), store.annotations.end(),
      [annotationId](const CreativeMeasurementAnnotation& annotation) {
        return annotation.id == annotationId;
      });
  return found == store.annotations.end() ? nullptr : &*found;
}

CreativeMeasurementGeometry buildCreativeMeasurementGeometry(
    const CreativeMeasurementAnnotation& annotation) noexcept {
  if (!validateCreativeMeasurementAnnotation(annotation)) {
    return {};
  }
  CreativeMeasurementState state;
  state.mode = annotation.mode;
  state.axis = annotation.axis;
  state.closePath = annotation.closePath;
  state.hasMeasurement = true;
  state.completed = true;
  state.pointCount = annotation.pointCount;
  for (std::size_t index = 0U; index < annotation.pointCount; ++index) {
    state.points[index] = toMeasurementPoint(annotation.points[index]);
  }
  state.startPoint = state.points.front();
  state.currentPoint = state.points[annotation.pointCount - 1U];
  state.plan = planCreativeMeasurement(
      {state.mode, state.axis,
       std::span<const CreativeMeasurementPoint>{state.points.data(),
                                                 state.pointCount},
       state.closePath});
  return buildCreativeMeasurementGeometry(state);
}

CreativeMeasurementAnnotationMutationReceipt
applyCreativeMeasurementAnnotationMutation(
    CreativeMeasurementAnnotationStore& store,
    const CreativeMeasurementAnnotationMutationRequest& request) {
  CreativeMeasurementAnnotationMutationReceipt receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.annotationId = request.annotationId;
  receipt.annotationCountBefore = store.annotations.size();
  receipt.annotationCountAfter = receipt.annotationCountBefore;
  if (!validateCreativeMeasurementAnnotationStore(store)) {
    setStatus(receipt, CreativeMeasurementAnnotationMutationStatus::InvalidStore,
              "creative_measurement_annotation_store_invalid");
    return receipt;
  }
  if (request.kind >= CreativeMeasurementAnnotationMutationKind::Count) {
    setStatus(receipt,
              CreativeMeasurementAnnotationMutationStatus::InvalidRequest,
              "creative_measurement_annotation_mutation_kind_invalid");
    return receipt;
  }

  CreativeMeasurementAnnotationStore staged = store;
  switch (request.kind) {
    case CreativeMeasurementAnnotationMutationKind::Add: {
      if (request.annotation.id != kInvalidCreativeMeasurementAnnotationId ||
          !validContent(request.annotation)) {
        setStatus(receipt,
                  CreativeMeasurementAnnotationMutationStatus::InvalidRequest,
                  "creative_measurement_annotation_add_invalid");
        return receipt;
      }
      if (staged.annotations.size() >=
          kCreativeMeasurementAnnotationCapacity) {
        setStatus(
            receipt,
            CreativeMeasurementAnnotationMutationStatus::CapacityExceeded,
            "creative_measurement_annotation_capacity_exceeded");
        return receipt;
      }
      if (staged.nextAnnotationId ==
          std::numeric_limits<CreativeMeasurementAnnotationId>::max()) {
        setStatus(receipt,
                  CreativeMeasurementAnnotationMutationStatus::IdExhausted,
                  "creative_measurement_annotation_id_exhausted");
        return receipt;
      }
      CreativeMeasurementAnnotation added = request.annotation;
      added.id = staged.nextAnnotationId++;
      receipt.annotationId = added.id;
      staged.annotations.push_back(std::move(added));
      break;
    }
    case CreativeMeasurementAnnotationMutationKind::Remove: {
      if (request.annotationId == kInvalidCreativeMeasurementAnnotationId) {
        setStatus(receipt,
                  CreativeMeasurementAnnotationMutationStatus::InvalidRequest,
                  "creative_measurement_annotation_id_invalid");
        return receipt;
      }
      const auto found = std::find_if(
          staged.annotations.begin(), staged.annotations.end(),
          [&request](const CreativeMeasurementAnnotation& annotation) {
            return annotation.id == request.annotationId;
          });
      if (found == staged.annotations.end()) {
        setStatus(receipt, CreativeMeasurementAnnotationMutationStatus::NotFound,
                  "creative_measurement_annotation_not_found");
        return receipt;
      }
      staged.annotations.erase(found);
      break;
    }
    case CreativeMeasurementAnnotationMutationKind::Count:
      break;
  }

  if (!validateCreativeMeasurementAnnotationStore(staged)) {
    setStatus(receipt,
              CreativeMeasurementAnnotationMutationStatus::InvalidRequest,
              "creative_measurement_annotation_mutation_invalid");
    return receipt;
  }
  store = std::move(staged);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.annotationCountAfter = store.annotations.size();
  setStatus(receipt, CreativeMeasurementAnnotationMutationStatus::Applied,
            request.kind == CreativeMeasurementAnnotationMutationKind::Remove
                ? "creative_measurement_annotation_removed"
                : "creative_measurement_annotation_added");
  return receipt;
}

}  // namespace iggy3d::creative
