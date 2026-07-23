#include "app/iggy3d/creative/Facade.hpp"

namespace iggy3d::creative {

CreativeFacadeMeasurementAnnotationSaveReceipt
Facade::saveMeasurementAnnotation(std::string_view name) {
  CreativeFacadeMeasurementAnnotationSaveReceipt receipt;
  receipt.requested = true;
  receipt.build =
      buildCreativeMeasurementAnnotation(measurementState_, name);
  if (!receipt.build.accepted) {
    receipt.reasonCode = receipt.build.reasonCode;
    return receipt;
  }

  receipt.mutation = document_.applyMeasurementAnnotationMutation(
      {CreativeMeasurementAnnotationMutationKind::Add,
       kInvalidCreativeMeasurementAnnotationId, receipt.build.annotation});
  receipt.annotationId = receipt.mutation.annotationId;
  receipt.accepted = receipt.mutation.accepted;
  receipt.changed = receipt.mutation.changed;
  receipt.reasonCode = receipt.mutation.reasonCode;
  if (!receipt.changed) {
    return receipt;
  }

  receipt.clear = iggy3d::creative::clearMeasurement(measurementState_);
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeMeasurementAnnotationMutationReceipt
Facade::removeMeasurementAnnotation(
    CreativeMeasurementAnnotationId annotationId) {
  return document_.applyMeasurementAnnotationMutation(
      {CreativeMeasurementAnnotationMutationKind::Remove, annotationId, {}});
}

}  // namespace iggy3d::creative
