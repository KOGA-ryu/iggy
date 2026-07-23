#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/Measure.hpp"
#include "app/iggy3d/creative/tools/MeasurementAnnotation.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeMeasurementState completedDistance() {
  cr::CreativeMeasurementState state;
  state.mode = cr::CreativeMeasurementMode::Distance;
  state.axis = cr::CreativeMeasurementAxis::X;
  state.hasMeasurement = true;
  state.completed = true;
  state.pointCount = 2U;
  state.points[0U] = {1.0, 2.0, 3.0, {41U},
                      cr::CreativeMeasurementSnapKind::Vertex};
  state.points[1U] = {4.0, 6.0, 3.0, {42U},
                      cr::CreativeMeasurementSnapKind::Opening};
  state.startPoint = state.points[0U];
  state.currentPoint = state.points[1U];
  state.plan = cr::planCreativeMeasurement(
      {state.mode, state.axis,
       std::span<const cr::CreativeMeasurementPoint>{state.points.data(),
                                                      state.pointCount},
       state.closePath});
  return state;
}

bool buildCreatesDetachedDurableSnapshot() {
  const cr::CreativeMeasurementState state = completedDistance();
  const cr::CreativeMeasurementAnnotationBuildResult result =
      cr::buildCreativeMeasurementAnnotation(state, "Door clearance");
  if (!expect(result.accepted, "annotation build accepted") ||
      !expect(result.annotation.id ==
                  cr::kInvalidCreativeMeasurementAnnotationId,
              "annotation build leaves id assignment to store") ||
      !expect(result.annotation.name == "Door clearance",
              "annotation build name") ||
      !expect(result.annotation.pointCount == 2U,
              "annotation build point count") ||
      !expect(result.annotation.points[0U].snapKind ==
                  cr::CreativeMeasurementSnapKind::Vertex,
              "annotation preserves snap provenance")) {
    return false;
  }

  cr::CreativeMeasurementAnnotation durable = result.annotation;
  durable.id = 1U;
  const cr::CreativeMeasurementGeometry geometry =
      cr::buildCreativeMeasurementGeometry(durable);
  return expect(cr::validateCreativeMeasurementAnnotation(durable),
                "durable annotation validates") &&
         expect(geometry.visible && geometry.completed,
                "durable geometry visible and complete") &&
         expect(geometry.pointCount == 2U && geometry.segmentCount == 1U,
                "durable geometry topology") &&
         expect(std::abs(geometry.segments[0U].end.y - 6.0) < 1.0e-9,
                "durable geometry coordinates");
}

bool buildRejectsIncompleteInvalidAndUnnamedMeasurements() {
  cr::CreativeMeasurementState incomplete = completedDistance();
  incomplete.completed = false;
  cr::CreativeMeasurementState invalid = completedDistance();
  invalid.points[1U].x = std::numeric_limits<double>::quiet_NaN();

  return expect(
             cr::buildCreativeMeasurementAnnotation(incomplete, "Incomplete")
                     .status ==
                 cr::CreativeMeasurementAnnotationBuildStatus::
                     IncompleteMeasurement,
             "incomplete annotation rejected") &&
         expect(cr::buildCreativeMeasurementAnnotation(completedDistance(), "")
                    .status ==
                    cr::CreativeMeasurementAnnotationBuildStatus::InvalidName,
                "empty annotation name rejected") &&
         expect(cr::buildCreativeMeasurementAnnotation(invalid, "Invalid")
                    .status == cr::CreativeMeasurementAnnotationBuildStatus::
                                   InvalidMeasurement,
                "invalid annotation point rejected");
}

bool storeAssignsStableIdsAndRemovesById() {
  cr::CreativeMeasurementAnnotationStore store;
  const auto built = cr::buildCreativeMeasurementAnnotation(
      completedDistance(), "Door clearance");
  const cr::CreativeMeasurementAnnotationMutationReceipt added =
      cr::applyCreativeMeasurementAnnotationMutation(
          store, {cr::CreativeMeasurementAnnotationMutationKind::Add,
                  cr::kInvalidCreativeMeasurementAnnotationId,
                  built.annotation});
  if (!expect(added.accepted && added.changed, "store add accepted") ||
      !expect(added.annotationId == 1U, "store assigns first stable id") ||
      !expect(store.nextAnnotationId == 2U,
              "store advances next stable id") ||
      !expect(cr::findCreativeMeasurementAnnotation(store, 1U) != nullptr,
              "store lookup by stable id")) {
    return false;
  }

  const cr::CreativeMeasurementAnnotationMutationReceipt removed =
      cr::applyCreativeMeasurementAnnotationMutation(
          store, {cr::CreativeMeasurementAnnotationMutationKind::Remove, 1U,
                  {}});
  return expect(removed.accepted && removed.changed,
                "store remove accepted") &&
         expect(store.annotations.empty(), "store remove erases annotation") &&
         expect(store.nextAnnotationId == 2U,
                "store never reuses removed id") &&
         expect(cr::validateCreativeMeasurementAnnotationStore(store),
                "store remains valid after removal");
}

bool documentMutationOwnsRevisionAndNarrowDirtyFlags() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Annotations");
  const auto built = cr::buildCreativeMeasurementAnnotation(
      completedDistance(), "Door clearance");
  const cr::CreativeMeasurementAnnotationMutationReceipt added =
      document.applyMeasurementAnnotationMutation(
          {cr::CreativeMeasurementAnnotationMutationKind::Add,
           cr::kInvalidCreativeMeasurementAnnotationId, built.annotation});

  return expect(added.accepted && added.changed,
                "document annotation accepted") &&
         expect(added.revisionBefore == 0U && added.revisionAfter == 1U,
                "document annotation advances one revision") &&
         expect(document.measurementAnnotationStore().annotations.size() == 1U,
                "document owns annotation store") &&
         expect(cr::hasDirtyFlag(document.dirtyFlags(),
                                 cr::CreativeObjectDirtyFlag::Preview),
                "annotation dirties preview") &&
         expect(cr::hasDirtyFlag(document.dirtyFlags(),
                                 cr::CreativeObjectDirtyFlag::Serialization),
                "annotation dirties serialization") &&
         expect(!cr::hasDirtyFlag(document.dirtyFlags(),
                                  cr::CreativeObjectDirtyFlag::Geometry),
                "annotation does not dirty room geometry") &&
         expect(!cr::hasDirtyFlag(document.dirtyFlags(),
                                  cr::CreativeObjectDirtyFlag::Collision),
                "annotation does not dirty collision");
}

}  // namespace

int main() {
  const bool ok = buildCreatesDetachedDurableSnapshot() &&
                  buildRejectsIncompleteInvalidAndUnnamedMeasurements() &&
                  storeAssignsStableIdsAndRemovesById() &&
                  documentMutationOwnsRevisionAndNarrowDirtyFlags();
  if (ok) {
    std::cout << "creative_measurement_annotation_tests passed\n";
    return 0;
  }
  return 1;
}
