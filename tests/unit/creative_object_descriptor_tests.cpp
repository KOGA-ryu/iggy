#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

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

bool expectSpatialDescriptor(cr::CreativeObjectKind kind,
                             cr::CreativeSpatialProjectionProfile profile,
                             cr::CreativeSpatialOccupancyKind occupancyKind,
                             std::string_view message) {
  const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(kind);
  const bool ok =
      expect(descriptor.projectionProfile == profile, message) &&
      expect(descriptor.occupancyKind == occupancyKind, message);
  return ok;
}

bool expectShapeDescriptor(cr::CreativeObjectKind kind,
                           cr::CreativeObjectShapeKind shapeKind,
                           std::string_view message) {
  const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(kind);
  return expect(descriptor.shapeKind == shapeKind, message) &&
         expect(cr::shapeKindForObject(kind) == shapeKind, message);
}

bool expectHasDirtyFlag(cr::CreativeObjectDirtyFlags flags,
                        cr::CreativeObjectDirtyFlag flag,
                        std::string_view message) {
  return expect(cr::hasDirtyFlag(flags, flag), message);
}

bool expectLacksDirtyFlag(cr::CreativeObjectDirtyFlags flags,
                          cr::CreativeObjectDirtyFlag flag,
                          std::string_view message) {
  return expect(!cr::hasDirtyFlag(flags, flag), message);
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
         expect(cr::shapeKindForObject(descriptor.kind) ==
                    descriptor.shapeKind,
                "shapeKindForObject matches descriptor") &&
         expect(cr::dirtyFlagsForCreation(descriptor.kind) ==
                    descriptor.creationDirtyFlags,
                "creation dirty flags match descriptor") &&
         expect(descriptor.projectionProfile ==
                    cr::projectionProfileForObject(descriptor.kind),
                "projection profile helper reads descriptor") &&
         expect(descriptor.occupancyKind ==
                    cr::occupancyKindForObject(descriptor.kind),
                "occupancy helper reads descriptor") &&
         expect(descriptor.projectionProfile !=
                    cr::CreativeSpatialProjectionProfile::Unknown,
                "descriptor kind has explicit projection profile") &&
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

    if (descriptor.projectionProfile ==
        cr::CreativeSpatialProjectionProfile::PointProjection) {
      ok = expect(descriptor.hasTransform,
                  "point-projected descriptor carries transform") &&
           ok;
    }

    if (descriptor.kind != cr::CreativeObjectKind::Unknown) {
      ok = expect(descriptor.shapeKind != cr::CreativeObjectShapeKind::Unknown,
                  "valid descriptor kind has explicit shape kind") &&
           ok;
    }
  }

  return ok;
}

bool shapeKindStringsAreStable() {
  return expect(cr::toString(cr::CreativeObjectShapeKind::Unknown) ==
                    "Unknown",
                "unknown shape string") &&
         expect(cr::toString(cr::CreativeObjectShapeKind::Point) == "Point",
                "point shape string") &&
         expect(cr::toString(cr::CreativeObjectShapeKind::Line) == "Line",
                "line shape string") &&
         expect(cr::toString(cr::CreativeObjectShapeKind::BoxVolume) ==
                    "BoxVolume",
                "box volume shape string") &&
         expect(cr::toString(cr::CreativeObjectShapeKind::Surface) ==
                    "Surface",
                "surface shape string") &&
         expect(cr::toString(cr::CreativeObjectShapeKind::Path) == "Path",
                "path shape string") &&
         expect(cr::toString(cr::CreativeObjectShapeKind::MeshProxy) ==
                    "MeshProxy",
                "mesh proxy shape string");
}

bool spatialProjectionProfileStringsAreStable() {
  return expect(cr::toString(cr::CreativeSpatialProjectionProfile::Unknown) ==
                    "Unknown",
                "unknown projection string") &&
         expect(cr::toString(cr::CreativeSpatialProjectionProfile::NoProjection) ==
                    "NoProjection",
                "no projection string") &&
         expect(cr::toString(cr::CreativeSpatialProjectionProfile::PointProjection) ==
                    "PointProjection",
                "point projection string") &&
         expect(cr::toString(cr::CreativeSpatialProjectionProfile::BoxProjection) ==
                    "BoxProjection",
                "box projection string") &&
         expect(cr::toString(cr::CreativeSpatialProjectionProfile::VolumeProjection) ==
                    "VolumeProjection",
                "volume projection string") &&
         expect(cr::toString(cr::CreativeSpatialProjectionProfile::LineProjection) ==
                    "LineProjection",
                "line projection string") &&
         expect(cr::toString(cr::CreativeSpatialProjectionProfile::PathProjection) ==
                    "PathProjection",
                "path projection string") &&
         expect(cr::toString(cr::CreativeSpatialProjectionProfile::LinkProjection) ==
                    "LinkProjection",
                "link projection string");
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
         expect(descriptor.shapeKind == cr::CreativeObjectShapeKind::BoxVolume,
                "room shape box volume") &&
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
         expect(cr::descriptorAllowsMutation(cr::CreativeObjectKind::Room,
                                             cr::CreativeMutationKind::Move),
                "room allows corner-anchor move without a transform") &&
         expect(!cr::descriptorAllowsMutation(cr::CreativeObjectKind::Room,
                                              cr::CreativeMutationKind::Rotate),
                "room rejects rotate because it has no transform") &&
         expect(!cr::descriptorAllowsMutation(cr::CreativeObjectKind::Room,
                                              cr::CreativeMutationKind::SetTransform),
                "room rejects set transform because it has no transform") &&
         expect(cr::projectionProfileForObject(cr::CreativeObjectKind::Room) ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "room projects as box") &&
         expect(cr::occupancyKindForObject(cr::CreativeObjectKind::Room) ==
                    cr::CreativeSpatialOccupancyKind::Structural,
                "room occupancy structural") &&
         expect(descriptor.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "room descriptor projection box") &&
         expect(descriptor.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Structural,
                "room descriptor occupancy structural");
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
         expect(descriptor.shapeKind == cr::CreativeObjectShapeKind::Unknown,
                "unknown shape") &&
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
                "unknown occupancy unknown") &&
         expect(descriptor.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::NoProjection,
                "unknown descriptor projection no projection") &&
         expect(descriptor.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Unknown,
                "unknown descriptor occupancy unknown");
}

bool representativeDescriptorsPinShapeFacts() {
  return expectShapeDescriptor(cr::CreativeObjectKind::Room,
                               cr::CreativeObjectShapeKind::BoxVolume,
                               "room shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectShapeKind::Surface,
                               "wall shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::Floor,
                               cr::CreativeObjectShapeKind::Surface,
                               "floor shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::Door,
                               cr::CreativeObjectShapeKind::MeshProxy,
                               "door shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::SpawnPoint,
                               cr::CreativeObjectShapeKind::Point,
                               "spawn point shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::NavLink,
                               cr::CreativeObjectShapeKind::Line,
                               "nav link shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::PatrolRoute,
                               cr::CreativeObjectShapeKind::Path,
                               "patrol route shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::Crate,
                               cr::CreativeObjectShapeKind::MeshProxy,
                               "crate shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::TriggerZone,
                               cr::CreativeObjectShapeKind::BoxVolume,
                               "trigger zone shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::PointLight,
                               cr::CreativeObjectShapeKind::Point,
                               "point light shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::MeasurementLine,
                               cr::CreativeObjectShapeKind::Line,
                               "measurement line shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::ReferenceImage,
                               cr::CreativeObjectShapeKind::Surface,
                               "reference image shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::Note,
                               cr::CreativeObjectShapeKind::Point,
                               "note shape descriptor") &&
         expectShapeDescriptor(cr::CreativeObjectKind::EnemySpawn,
                               cr::CreativeObjectShapeKind::Point,
                               "enemy spawn shape descriptor");
}

bool shapeAndProjectionCanDifferByDesign() {
  const cr::CreativeObjectDescriptor& wall =
      cr::describeObject(cr::CreativeObjectKind::Wall);
  const cr::CreativeObjectDescriptor& door =
      cr::describeObject(cr::CreativeObjectKind::Door);
  const cr::CreativeObjectDescriptor& patrolRoute =
      cr::describeObject(cr::CreativeObjectKind::PatrolRoute);

  return expect(wall.shapeKind == cr::CreativeObjectShapeKind::Surface,
                "wall shape surface") &&
         expect(wall.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "wall projection remains box") &&
         expect(door.shapeKind == cr::CreativeObjectShapeKind::MeshProxy,
                "door shape mesh proxy") &&
         expect(door.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "door projection remains box") &&
         expect(patrolRoute.shapeKind == cr::CreativeObjectShapeKind::Path,
                "patrol route shape path") &&
         expect(patrolRoute.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::PathProjection,
                "patrol route projection path");
}

bool representativeDescriptorsPinSpatialFacts() {
  return expectSpatialDescriptor(
             cr::CreativeObjectKind::Unknown,
             cr::CreativeSpatialProjectionProfile::NoProjection,
             cr::CreativeSpatialOccupancyKind::Unknown,
             "unknown spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::Room,
             cr::CreativeSpatialProjectionProfile::BoxProjection,
             cr::CreativeSpatialOccupancyKind::Structural,
             "room spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::Note,
             cr::CreativeSpatialProjectionProfile::PointProjection,
             cr::CreativeSpatialOccupancyKind::Authoring,
             "note spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::Crate,
             cr::CreativeSpatialProjectionProfile::BoxProjection,
             cr::CreativeSpatialOccupancyKind::Structural,
             "crate spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::CameraRail,
             cr::CreativeSpatialProjectionProfile::LineProjection,
             cr::CreativeSpatialOccupancyKind::Camera,
             "camera rail spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::PatrolRoute,
             cr::CreativeSpatialProjectionProfile::PathProjection,
             cr::CreativeSpatialOccupancyKind::Gameplay,
             "patrol route spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::NavLink,
             cr::CreativeSpatialProjectionProfile::NoProjection,
             cr::CreativeSpatialOccupancyKind::Navigation,
             "nav link spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::JumpLink,
             cr::CreativeSpatialProjectionProfile::NoProjection,
             cr::CreativeSpatialOccupancyKind::Navigation,
             "jump link spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::Group,
             cr::CreativeSpatialProjectionProfile::NoProjection,
             cr::CreativeSpatialOccupancyKind::Authoring,
             "group spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::CutsceneMarker,
             cr::CreativeSpatialProjectionProfile::NoProjection,
             cr::CreativeSpatialOccupancyKind::Camera,
             "cutscene marker spatial descriptor");
}

bool mutationDirtyFlagsFollowDescriptorSpatialColumns() {
  const cr::CreativeObjectDirtyFlags noteRename =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Note,
                                cr::CreativeMutationKind::Rename);
  const cr::CreativeObjectDirtyFlags pointLightMove =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::PointLight,
                                cr::CreativeMutationKind::Move);
  const cr::CreativeObjectDirtyFlags cameraRailMove =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::CameraRail,
                                cr::CreativeMutationKind::Move);
  const cr::CreativeObjectDirtyFlags crateMove =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Crate,
                                cr::CreativeMutationKind::Move);
  const cr::CreativeObjectDirtyFlags navLinkTarget =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::NavLink,
                                cr::CreativeMutationKind::LinkTarget);
  const cr::CreativeObjectDirtyFlags roomVisible =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Room,
                                cr::CreativeMutationKind::SetVisible);

  return expectHasDirtyFlag(noteRename,
                            cr::CreativeObjectDirtyFlag::Identity,
                            "note rename identity") &&
         expectHasDirtyFlag(noteRename,
                            cr::CreativeObjectDirtyFlag::Preview,
                            "note rename preview") &&
         expectHasDirtyFlag(noteRename,
                            cr::CreativeObjectDirtyFlag::Serialization,
                            "note rename serialization") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Geometry,
                              "note rename no geometry") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Collision,
                              "note rename no collision") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Navigation,
                              "note rename no navigation") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Lighting,
                              "note rename no lighting") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Audio,
                              "note rename no audio") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Camera,
                              "note rename no camera") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Gameplay,
                              "note rename no gameplay") &&
         expectLacksDirtyFlag(noteRename,
                              cr::CreativeObjectDirtyFlag::Testing,
                              "note rename no testing") &&
         expectHasDirtyFlag(pointLightMove,
                            cr::CreativeObjectDirtyFlag::Transform,
                            "point light move transform") &&
         expectHasDirtyFlag(pointLightMove,
                            cr::CreativeObjectDirtyFlag::Preview,
                            "point light move preview") &&
         expectHasDirtyFlag(pointLightMove,
                            cr::CreativeObjectDirtyFlag::Lighting,
                            "point light move lighting") &&
         expectLacksDirtyFlag(pointLightMove,
                              cr::CreativeObjectDirtyFlag::Geometry,
                              "point light move no geometry") &&
         expectLacksDirtyFlag(pointLightMove,
                              cr::CreativeObjectDirtyFlag::Collision,
                              "point light move no collision") &&
         expectHasDirtyFlag(cameraRailMove,
                            cr::CreativeObjectDirtyFlag::Transform,
                            "camera rail move transform") &&
         expectHasDirtyFlag(cameraRailMove,
                            cr::CreativeObjectDirtyFlag::Bounds,
                            "camera rail move bounds") &&
         expectHasDirtyFlag(cameraRailMove,
                            cr::CreativeObjectDirtyFlag::Geometry,
                            "camera rail move geometry") &&
         expectHasDirtyFlag(cameraRailMove,
                            cr::CreativeObjectDirtyFlag::Preview,
                            "camera rail move preview") &&
         expectHasDirtyFlag(cameraRailMove,
                            cr::CreativeObjectDirtyFlag::Camera,
                            "camera rail move camera") &&
         expectHasDirtyFlag(crateMove,
                            cr::CreativeObjectDirtyFlag::Transform,
                            "crate move transform") &&
         expectHasDirtyFlag(crateMove,
                            cr::CreativeObjectDirtyFlag::Bounds,
                            "crate move bounds") &&
         expectHasDirtyFlag(crateMove,
                            cr::CreativeObjectDirtyFlag::Geometry,
                            "crate move geometry") &&
         expectHasDirtyFlag(crateMove,
                            cr::CreativeObjectDirtyFlag::Collision,
                            "crate move collision") &&
         expectHasDirtyFlag(crateMove,
                            cr::CreativeObjectDirtyFlag::Preview,
                            "crate move preview") &&
         expectHasDirtyFlag(navLinkTarget,
                            cr::CreativeObjectDirtyFlag::Navigation,
                            "nav link target navigation") &&
         expectLacksDirtyFlag(navLinkTarget,
                              cr::CreativeObjectDirtyFlag::Geometry,
                              "nav link target no geometry") &&
         expectLacksDirtyFlag(navLinkTarget,
                              cr::CreativeObjectDirtyFlag::Collision,
                              "nav link target no collision") &&
         expectLacksDirtyFlag(navLinkTarget,
                              cr::CreativeObjectDirtyFlag::Gameplay,
                              "nav link target no gameplay") &&
         expectHasDirtyFlag(roomVisible,
                            cr::CreativeObjectDirtyFlag::Identity,
                            "room visible identity") &&
         expectHasDirtyFlag(roomVisible,
                            cr::CreativeObjectDirtyFlag::Preview,
                            "room visible preview") &&
         expectHasDirtyFlag(roomVisible,
                            cr::CreativeObjectDirtyFlag::Serialization,
                            "room visible serialization") &&
         expectLacksDirtyFlag(roomVisible,
                              cr::CreativeObjectDirtyFlag::Geometry,
                              "room visible no geometry") &&
         expectLacksDirtyFlag(roomVisible,
                              cr::CreativeObjectDirtyFlag::Collision,
                              "room visible no collision");
}

}  // namespace

int main() {
  const bool ok = descriptorTableRowsAreStableAndUnique() &&
                  shapeKindStringsAreStable() &&
                  spatialProjectionProfileStringsAreStable() &&
                  roomDescriptorPinsShapeBearingProjectionContract() &&
                  unknownDescriptorRemainsInvalidAndNonProjectable() &&
                  representativeDescriptorsPinShapeFacts() &&
                  shapeAndProjectionCanDifferByDesign() &&
                  representativeDescriptorsPinSpatialFacts() &&
                  mutationDirtyFlagsFollowDescriptorSpatialColumns();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
