#include "app/iggy3d/creative/document/Document.hpp"

namespace iggy3d::creative {

CreativeMeasurementAnnotationMutationReceipt
CreativeDocument::applyMeasurementAnnotationMutation(
    const CreativeMeasurementAnnotationMutationRequest& request) {
  CreativeMeasurementAnnotationMutationReceipt receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.annotationId = request.annotationId;
  receipt.annotationCountBefore = measurementAnnotationStore_.annotations.size();
  receipt.annotationCountAfter = receipt.annotationCountBefore;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  if (!valid_) {
    receipt.status =
        CreativeMeasurementAnnotationMutationStatus::InvalidDocument;
    receipt.reasonCode = "creative_measurement_annotation_document_invalid";
    return receipt;
  }

  receipt = applyCreativeMeasurementAnnotationMutation(
      measurementAnnotationStore_, request);
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  if (!receipt.changed) {
    return receipt;
  }

  markObjectMutationChanged(CreativeObjectDirtyFlag::Preview |
                            CreativeObjectDirtyFlag::Serialization);
  receipt.revisionAfter = revision_;
  return receipt;
}

}  // namespace iggy3d::creative
