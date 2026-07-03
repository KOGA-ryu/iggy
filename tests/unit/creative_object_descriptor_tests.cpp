#include "app/iggy3d/creative/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/SpatialProjection.hpp"

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

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool descriptorTableRowsAreStableAndUnique() {
  const std::span<const cr::CreativeObjectDescriptor> descriptors =
      cr::allObjectDescriptors();
  bool ok = expect(descriptors.size() == 108U,
                   "current descriptor table has every known object kind") &&
            expect(descriptors.front().kind == cr::CreativeObjectKind::Unknown,
                   "unknown descriptor is first");

  for (std::size_t i = 0; i < descriptors.size(); ++i) {
    const cr::CreativeObjectDescriptor& descriptor = descriptors[i];
    const cr::CreativeObjectDescriptor& described =
        cr::describeObject(descriptor.kind);

    ok = expect(described.kind == descriptor.kind,
                "describeObject round-trips descriptor kind") &&
         expect(cr::categoryOf(descriptor.kind) == descriptor.category,
                "categoryOf matches descriptor") &&
         expect(cr::profileOf(descriptor.kind) == descriptor.profile,
                "profileOf matches descriptor") &&
         expect(cr::dirtyFlagsForCreation(descriptor.kind) ==
                    descriptor.creationDirtyFlags,
                "creation dirty flags match descriptor") &&
         expect(descriptor.name == cr::toString(descriptor.kind),
                "descriptor stable name matches object kind string") &&
         expect(!descriptor.displayName.empty(),
                "descriptor display name is stable") &&
         expect(!descriptor.purpose.empty(), "descriptor purpose is stable") &&
         ok;

    for (std::size_t j = i + 1; j < descriptors.size(); ++j) {
      ok = expect(descriptors[j].kind != descriptor.kind,
                  "descriptor kinds are unique") &&
           ok;
    }

    const cr::CreativeSpatialProjectionProfile projectionProfile =
        cr::projectionProfileForObject(descriptor.kind);
    ok = expect(projectionProfile != cr::CreativeSpatialProjectionProfile::Unknown,
                "descriptor kind has explicit projection profile") &&
         ok;

    if (projectionProfile ==
        cr::CreativeSpatialProjectionProfile::PointProjection) {
      ok = expect(descriptor.hasTransform,
                  "point-projected descriptor carries transform") &&
           ok;
    }
  }

  return ok;
}

bool roomDescriptorPinsShapeBearingProjectionContract() {
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::Room);

  return expect(descriptor.kind == cr::CreativeObjectKind::Room,
                "room descriptor kind") &&
         expect(descriptor.category == cr::CreativeObjectCategory::Structural,
                "room category") &&
         expect(descriptor.profile == cr::CreativeObjectProfile::RoomContainer,
                "room profile") &&
         expect(descriptor.name == "Room", "room stable name") &&
         expect(descriptor.displayName == "Room", "room display name") &&
         expect(!descriptor.hasTransform, "room has no transform by default") &&
         expect(descriptor.hasBounds, "room is shape-bearing") &&
         expect(!descriptor.canHaveParent, "room cannot have parent") &&
         expect(descriptor.canOwnChildren, "room can own children") &&
         expect(descriptor.canBeHidden, "room can be hidden") &&
         expect(descriptor.canBeLocked, "room can be locked") &&
         expect(descriptor.canBeTagged, "room can be tagged") &&
         expect(descriptor.isRuntimeMeaningful, "room runtime meaningful") &&
         expect(!descriptor.isEditorOnly, "room is not editor-only") &&
         expect(descriptor.defaults.visible, "room default visible") &&
         expect(!descriptor.defaults.locked, "room default unlocked") &&
         expect(sameVec3(descriptor.defaults.bounds.min, {0.0, 0.0, 0.0}),
                "room default bounds min") &&
         expect(sameVec3(descriptor.defaults.bounds.max, {10.0, 4.0, 10.0}),
                "room default bounds max") &&
         expect(cr::hasDirtyFlag(descriptor.creationDirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Identity),
                "room creation identity dirty") &&
         expect(cr::hasDirtyFlag(descriptor.creationDirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Bounds),
                "room creation bounds dirty") &&
         expect(cr::hasDirtyFlag(descriptor.creationDirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Geometry),
                "room creation geometry dirty") &&
         expect(cr::hasDirtyFlag(descriptor.creationDirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Collision),
                "room creation collision dirty") &&
         expect(cr::hasDirtyFlag(descriptor.creationDirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Preview),
                "room creation preview dirty") &&
         expect(cr::hasDirtyFlag(descriptor.creationDirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Serialization),
                "room creation serialization dirty") &&
         expect(cr::descriptorAllowsMutation(cr::CreativeObjectKind::Room,
                                             cr::CreativeMutationKind::SetVisible),
                "room allows visibility mutation") &&
         expect(cr::descriptorAllowsMutation(cr::CreativeObjectKind::Room,
                                             cr::CreativeMutationKind::SetBounds),
                "room allows bounds mutation") &&
         expect(!cr::descriptorAllowsMutation(cr::CreativeObjectKind::Room,
                                              cr::CreativeMutationKind::Move),
                "room rejects move because it has no transform") &&
         expect(cr::projectionProfileForObject(cr::CreativeObjectKind::Room) ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "room projects as box") &&
         expect(cr::occupancyKindForObject(cr::CreativeObjectKind::Room) ==
                    cr::CreativeSpatialOccupancyKind::Structural,
                "room occupancy structural");
}

bool unknownDescriptorRemainsInvalidAndNonProjectable() {
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::Unknown);

  return expect(descriptor.kind == cr::CreativeObjectKind::Unknown,
                "unknown descriptor kind") &&
         expect(descriptor.category == cr::CreativeObjectCategory::Unknown,
                "unknown category") &&
         expect(descriptor.profile == cr::CreativeObjectProfile::Unknown,
                "unknown profile") &&
         expect(descriptor.creationDirtyFlags == 0U,
                "unknown has no creation dirty flags") &&
         expect(!descriptor.hasTransform, "unknown has no transform") &&
         expect(!descriptor.hasBounds, "unknown has no bounds") &&
         expect(!descriptor.canHaveParent, "unknown cannot have parent") &&
         expect(!descriptor.canOwnChildren, "unknown cannot own children") &&
         expect(!descriptor.isRuntimeMeaningful,
                "unknown not runtime meaningful") &&
         expect(!descriptor.isEditorOnly, "unknown not editor-only") &&
         expect(!cr::descriptorAllowsMutation(cr::CreativeObjectKind::Unknown,
                                              cr::CreativeMutationKind::SetVisible),
                "unknown rejects visibility mutation") &&
         expect(!cr::descriptorAllowsMutation(cr::CreativeObjectKind::Unknown,
                                              cr::CreativeMutationKind::Rename),
                "unknown rejects rename mutation") &&
         expect(cr::projectionProfileForObject(cr::CreativeObjectKind::Unknown) ==
                    cr::CreativeSpatialProjectionProfile::NoProjection,
                "unknown has no projection") &&
         expect(cr::occupancyKindForObject(cr::CreativeObjectKind::Unknown) ==
                    cr::CreativeSpatialOccupancyKind::Unknown,
                "unknown occupancy unknown");
}

}  // namespace

int main() {
  const bool ok = descriptorTableRowsAreStableAndUnique() &&
                  roomDescriptorPinsShapeBearingProjectionContract() &&
                  unknownDescriptorRemainsInvalidAndNonProjectable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
