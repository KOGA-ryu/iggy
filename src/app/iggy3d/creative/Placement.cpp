#include "app/iggy3d/creative/Placement.hpp"

#include "app/iggy3d/creative/ObjectDescriptor.hpp"

namespace iggy3d::creative {

CreativePlacedCreateRequest buildPlacedCreateRequest(
    const CreativeDocument& document, CreativeObjectKind kind) {
  CreativePlacedCreateRequest placed;
  placed.createRequest.kind = kind;
  placed.existingObjectCount = document.objectCount();
  placed.stepX = document.documentSnapSettings().stepX;
  placed.offsetX =
      static_cast<double>(placed.existingObjectCount) * placed.stepX;

  const CreativeObjectDescriptor& descriptor = describeObject(kind);
  if (descriptor.kind == CreativeObjectKind::Unknown ||
      placed.offsetX == 0.0) {
    return placed;
  }

  if (descriptor.hasBounds) {
    CreativeBounds bounds = descriptor.defaults.bounds;
    bounds.min.x += placed.offsetX;
    bounds.max.x += placed.offsetX;
    placed.createRequest.bounds = bounds;
    placed.createRequest.hasBoundsOverride = true;
    placed.boundsOffsetApplied = true;
  }

  if (descriptor.hasTransform) {
    CreativeTransform transform = descriptor.defaults.transform;
    transform.position.x += placed.offsetX;
    placed.createRequest.transform = transform;
    placed.createRequest.hasTransformOverride = true;
    placed.transformOffsetApplied = true;
  }

  placed.offsetApplied =
      placed.boundsOffsetApplied || placed.transformOffsetApplied;
  return placed;
}

}  // namespace iggy3d::creative
