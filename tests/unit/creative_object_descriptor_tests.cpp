#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/tools/StructuralPlacement.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
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

bool near(double actual, double expected, double epsilon = 1.0e-9) {
  return std::fabs(actual - expected) <= epsilon;
}

bool nearVec3(cr::CreativeVec3 actual,
              cr::CreativeVec3 expected,
              double epsilon = 1.0e-9) {
  return near(actual.x, expected.x, epsilon) &&
         near(actual.y, expected.y, epsilon) &&
         near(actual.z, expected.z, epsilon);
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

bool kindIsInCreativeObjectInventory(cr::CreativeObjectKind kind) {
  for (const cr::CreativeObjectKind knownKind : cr::allCreativeObjectKinds()) {
    if (knownKind == kind) {
      return true;
    }
  }
  return false;
}

std::uint64_t descriptorCountForKind(
    std::span<const cr::CreativeObjectDescriptor> descriptors,
    cr::CreativeObjectKind kind) {
  std::uint64_t count = 0;
  for (const cr::CreativeObjectDescriptor& descriptor : descriptors) {
    count += descriptor.kind == kind ? 1U : 0U;
  }
  return count;
}

bool descriptorLookupIsTotalForEveryEnumKind() {
  bool ok = true;
  const std::span<const cr::CreativeObjectKind> knownKinds =
      cr::allCreativeObjectKinds();
  const std::size_t expectedKindCount =
      static_cast<std::size_t>(cr::CreativeObjectKind::Count);

  ok = expect(knownKinds.size() == expectedKindCount,
              "known object inventory covers every enum value before Count") &&
       ok;

  for (std::size_t index = 0; index < expectedKindCount; ++index) {
    const cr::CreativeObjectKind kind =
        static_cast<cr::CreativeObjectKind>(index);
    const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(kind);

    ok = expect(index < knownKinds.size() && knownKinds[index] == kind,
                "known object inventory follows enum order") &&
         expect(descriptor.kind == kind,
                "describeObject returns descriptor for requested enum kind") &&
         ok;
  }

  return ok;
}

bool descriptorTableRowsAreStableAndUnique() {
  const std::span<const cr::CreativeObjectDescriptor> descriptors =
      cr::allObjectDescriptors();
  const std::span<const cr::CreativeObjectKind> knownKinds =
      cr::allCreativeObjectKinds();
  bool ok = expect(descriptors.size() == knownKinds.size(),
                   "descriptor table has one row for every known object kind") &&
            expect(descriptors.front().kind == cr::CreativeObjectKind::Unknown,
                   "unknown descriptor is first");

  for (const cr::CreativeObjectKind kind : knownKinds) {
    const cr::CreativeObjectDescriptor& described =
        cr::describeObject(kind);
    ok = expect(descriptorCountForKind(descriptors, kind) == 1U,
                "known object kind has exactly one descriptor row") &&
         expect(described.kind == kind,
                "describeObject returns in-range known kind") &&
         expect(described.name == cr::toString(kind),
                "known object kind string matches descriptor name") &&
         ok;
  }

  for (std::size_t i = 0; i < descriptors.size(); ++i) {
    const cr::CreativeObjectDescriptor& descriptor = descriptors[i];
    const cr::CreativeObjectDescriptor& described =
        cr::describeObject(descriptor.kind);

    ok = expect(kindIsInCreativeObjectInventory(descriptor.kind),
                "descriptor kind is in known object inventory") &&
         expect(described.kind == descriptor.kind,
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

bool serializedObjectKindIdsAreStableAndUnique() {
  bool ok = true;
  for (const cr::CreativeObjectKind kind : cr::allCreativeObjectKinds()) {
    const std::string_view serializedId = cr::serializedObjectKindId(kind);
    cr::CreativeObjectKind parsed = cr::CreativeObjectKind::Unknown;
    if (kind == cr::CreativeObjectKind::Unknown) {
      ok = expect(serializedId == "Unknown", "unknown serialized id") &&
           expect(!cr::parseSerializedObjectKindId(serializedId, parsed),
                  "unknown serialized id does not parse as authored kind") &&
           ok;
      continue;
    }

    ok = expect(!serializedId.empty(), "serialized kind id is non-empty") &&
         expect(serializedId != "Unknown",
                "authored kind serialized id is explicit") &&
         expect(serializedId == cr::toString(kind),
                "current serialized kind id preserves legacy save string") &&
         expect(cr::parseSerializedObjectKindId(serializedId, parsed),
                "serialized kind id parses") &&
         expect(parsed == kind, "serialized kind id round-trips") &&
         ok;

    for (const cr::CreativeObjectKind other : cr::allCreativeObjectKinds()) {
      if (other == kind || other == cr::CreativeObjectKind::Unknown) {
        continue;
      }
      ok = expect(cr::serializedObjectKindId(other) != serializedId,
                  "serialized kind ids are unique") &&
           ok;
    }
  }

  cr::CreativeObjectKind parsed = cr::CreativeObjectKind::Unknown;
  const cr::CreativeObjectDescriptor& movingPlatform =
      cr::describeObject(cr::CreativeObjectKind::MovingPlatform);
  ok = expect(cr::serializedObjectKindId(movingPlatform.kind) ==
                  "MovingPlatform",
              "moving platform serialized id is compact legacy token") &&
       expect(movingPlatform.displayName == "Moving Platform",
              "moving platform display name remains human readable") &&
       expect(!cr::parseSerializedObjectKindId(movingPlatform.displayName,
                                               parsed),
              "display labels are not serialized kind ids") &&
       expect(!cr::parseSerializedObjectKindId("", parsed),
              "empty serialized kind id rejected") &&
       expect(!cr::parseSerializedObjectKindId("DefinitelyNotAKind", parsed),
              "invalid serialized kind id rejected") &&
       ok;
  return ok;
}

bool legacyCategoryPredicatesFollowDescriptorTruth() {
  bool ok = true;
  for (const cr::CreativeObjectKind kind : cr::allCreativeObjectKinds()) {
    ok = expect(cr::isStructuralObject(kind) ==
                    cr::objectUsesCategory(
                        kind, cr::CreativeObjectCategory::Structural),
                "structural predicate follows descriptor category") &&
         expect(cr::isTerrainOrVolumeObject(kind) ==
                    cr::objectUsesCategory(
                        kind, cr::CreativeObjectCategory::TerrainOrVolume),
                "terrain or volume predicate follows descriptor category") &&
         expect(cr::isNavigationOrMovementObject(kind) ==
                    cr::objectUsesCategory(
                        kind,
                        cr::CreativeObjectCategory::NavigationOrMovement),
                "navigation or movement predicate follows descriptor category") &&
         expect(cr::isLogicObject(kind) ==
                    cr::objectUsesCategory(kind,
                                           cr::CreativeObjectCategory::Logic),
                "logic predicate follows descriptor category") &&
         expect(cr::isTestingObject(kind) ==
                    cr::objectUsesCategory(kind,
                                           cr::CreativeObjectCategory::Testing),
                "testing predicate follows descriptor category") &&
         expect(cr::isVisualDressingObject(kind) ==
                    cr::objectUsesCategory(
                        kind, cr::CreativeObjectCategory::VisualDressing),
                "visual dressing predicate follows descriptor category") &&
         expect(cr::isLightSoundOrCameraObject(kind) ==
                    cr::objectUsesCategory(
                        kind, cr::CreativeObjectCategory::LightSoundOrCamera),
                "light sound camera predicate follows descriptor category") &&
         expect(cr::isAuthoringMetaObject(kind) ==
                    cr::objectUsesCategory(
                        kind, cr::CreativeObjectCategory::AuthoringMeta),
                "authoring meta predicate follows descriptor category") &&
         expect(cr::isGameplayObject(kind) ==
                    cr::objectUsesCategory(
                        kind, cr::CreativeObjectCategory::Gameplay),
                "gameplay predicate follows descriptor category") &&
         ok;
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

bool runtimeAnchorSemanticStringsAreStable() {
  return expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::None).empty(),
                "none anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Spawn) ==
                    "spawn",
                "spawn anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Exit) ==
                    "exit",
                "exit anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Npc) == "npc",
                "npc anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Monster) ==
                    "monster",
                "monster anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Pickup) ==
                    "pickup",
                "pickup anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Light) ==
                    "light",
                "light anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Audio) ==
                    "audio",
                "audio anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Camera) ==
                    "camera",
                "camera anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::Cover) ==
                    "cover",
                "cover anchor semantic string") &&
         expect(cr::toString(cr::CreativeRuntimeAnchorSemantic::PatrolPost) ==
                    "patrol_post",
                "patrol post anchor semantic string");
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
  const cr::CreativeObjectDescriptor& sentinelDescriptor =
      cr::describeObject(cr::CreativeObjectKind::Count);
  const cr::CreativeObjectDescriptor& invalidDescriptor =
      cr::describeObject(static_cast<cr::CreativeObjectKind>(999999U));

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
                "unknown descriptor occupancy unknown") &&
         expect(sentinelDescriptor.kind == cr::CreativeObjectKind::Unknown,
                "count sentinel describes as unknown") &&
         expect(invalidDescriptor.kind == cr::CreativeObjectKind::Unknown,
                "invalid cast describes as unknown") &&
         expect(cr::toString(cr::CreativeObjectKind::Count) == "Unknown",
                "count sentinel string unknown") &&
         expect(cr::toString(static_cast<cr::CreativeObjectKind>(999999U)) ==
                    "Unknown",
                "invalid cast string unknown");
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

bool descriptorOwnsCanonicalStructuralDimensions() {
  const cr::CreativeVec3 wallSize =
      cr::defaultCreativeObjectSize(cr::CreativeObjectKind::Wall);
  const cr::CreativeVec3 floorSize =
      cr::defaultCreativeObjectSize(cr::CreativeObjectKind::Floor);
  const cr::CreativeVec3 ceilingSize =
      cr::defaultCreativeObjectSize(cr::CreativeObjectKind::Ceiling);
  const cr::CreativeVec3 roofSize =
      cr::defaultCreativeObjectSize(cr::CreativeObjectKind::Roof);
  const cr::CreativeWallGeometryDefaults wall =
      cr::defaultCreativeWallGeometry();
  return expect(sameVec3(wallSize, {4.0, 3.0, 0.25}),
                "wall descriptor owns canonical dimensions") &&
         expect(sameVec3(floorSize, {4.0, 0.05, 4.0}),
                "floor descriptor owns canonical dimensions") &&
         expect(sameVec3(ceilingSize, {4.0, 0.25, 4.0}),
                "ceiling descriptor owns canonical dimensions") &&
         expect(sameVec3(roofSize, {5.0, 1.0, 5.0}),
                "roof descriptor owns canonical dimensions") &&
         expect(wall.lengthMeters == 4.0 && wall.heightMeters == 3.0 &&
                    wall.thicknessMeters == 0.25,
                "wall semantic dimensions derive from descriptor axes") &&
         expect(cr::defaultCreativeStructuralLayerThicknessMeters(
                    cr::CreativeObjectKind::Floor) == 0.05 &&
                    cr::defaultCreativeStructuralLayerThicknessMeters(
                        cr::CreativeObjectKind::Ceiling) == 0.25 &&
                    cr::defaultCreativeStructuralLayerThicknessMeters(
                        cr::CreativeObjectKind::Roof) == 1.0 &&
                    cr::defaultCreativeStructuralLayerThicknessMeters(
                        cr::CreativeObjectKind::Wall) == 0.0,
                "horizontal layer thickness is descriptor-owned");
}

bool descriptorOwnsGeneratedTraversalGeometry() {
  const cr::CreativeObjectDescriptor& stair =
      cr::describeObject(cr::CreativeObjectKind::Stair);
  const cr::CreativeObjectDescriptor& ramp =
      cr::describeObject(cr::CreativeObjectKind::Ramp);
  const cr::CreativeObjectDescriptor& platform =
      cr::describeObject(cr::CreativeObjectKind::Platform);
  const cr::CreativeObjectDescriptor& movingPlatform =
      cr::describeObject(cr::CreativeObjectKind::MovingPlatform);
  const cr::CreativeObjectDescriptor& column =
      cr::describeObject(cr::CreativeObjectKind::Column);
  const cr::CreativeObjectDescriptor& pillar =
      cr::describeObject(cr::CreativeObjectKind::Pillar);
  const cr::CreativeObjectDescriptor& beam =
      cr::describeObject(cr::CreativeObjectKind::Beam);
  const cr::CreativeObjectDescriptor& fence =
      cr::describeObject(cr::CreativeObjectKind::Fence);
  const cr::CreativeObjectDescriptor& railing =
      cr::describeObject(cr::CreativeObjectKind::Railing);
  const cr::CreativeObjectDescriptor& bridge =
      cr::describeObject(cr::CreativeObjectKind::Bridge);
  const cr::CreativeObjectDescriptor& arch =
      cr::describeObject(cr::CreativeObjectKind::Arch);
  const cr::CreativeObjectDescriptor& ladder =
      cr::describeObject(cr::CreativeObjectKind::Ladder);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  return expect(sameVec3(cr::defaultCreativeObjectSize(stair.kind),
                         {2.0, 1.0, 3.0}),
                "stair descriptor owns canonical dimensions") &&
         expect(sameVec3(cr::defaultCreativeObjectSize(ramp.kind),
                         {2.0, 1.0, 4.0}),
                "ramp descriptor owns canonical dimensions") &&
         expect(sameVec3(cr::defaultCreativeObjectSize(platform.kind),
                         {3.0, 0.35, 3.0}) &&
                    sameVec3(cr::defaultCreativeObjectSize(movingPlatform.kind),
                             {3.0, 0.35, 3.0}),
                "static and moving platform share canonical dimensions") &&
         expect(stair.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::StairSteps &&
                    stair.generatedGeometry.maximumStepRiseMeters == 0.25 &&
                    ramp.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::RampWedge &&
                    platform.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::WalkableSlab &&
                    movingPlatform.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::WalkableSlab,
                "descriptor rows own generated traversal profiles") &&
         expect(column.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::SolidPrism &&
                    pillar.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::SolidPrism &&
                    beam.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::SolidPrism &&
                    fence.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::SolidPrism &&
                    railing.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::SolidPrism &&
                    bridge.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::WalkableSlab &&
                    arch.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::OpenFrame &&
                    ladder.generatedGeometry.profile ==
                        cr::CreativeGeneratedGeometryProfile::DescriptorDefault,
                "descriptor rows own generated structural profiles") &&
         expect(cr::creativeGeneratedGeometrySegmentCount(
                    stair, cr::defaultCreativeObjectSize(stair.kind)) == 4U &&
                    cr::creativeGeneratedGeometrySegmentCount(
                        stair, {2.0, 2.0, 3.0}) == 8U,
                "stair segment count follows resolved rise") &&
         expect(cr::creativeGeneratedGeometrySegmentCount(
                    stair, {2.0, 100.0, 3.0}) ==
                    cr::kMaximumCreativeGeneratedGeometrySegmentCount,
                "stair segment count is bounded") &&
         expect(cr::creativeGeneratedGeometrySegmentCount(
                    ramp, {2.0, 1.0, 4.0}) == 0U &&
                    cr::creativeGeneratedGeometrySegmentCount(
                        stair, {2.0, nan, 3.0}) == 0U &&
                    cr::creativeGeneratedGeometrySegmentCount(
                        stair, {2.0, 0.0, 3.0}) == 0U,
                "wrong-profile and invalid segment requests fail closed");
}

bool structuralSpanPlacementIsDescriptorDrivenAndBounded() {
  constexpr std::array spanKinds{
      cr::CreativeObjectKind::Beam,
      cr::CreativeObjectKind::Fence,
      cr::CreativeObjectKind::Railing,
      cr::CreativeObjectKind::Bridge,
  };
  constexpr std::array immediateKinds{
      cr::CreativeObjectKind::Column,
      cr::CreativeObjectKind::Pillar,
      cr::CreativeObjectKind::Arch,
      cr::CreativeObjectKind::Ladder,
  };
  bool ok = true;
  std::size_t spanPolicyCount = 0U;
  for (const cr::CreativeObjectDescriptor& descriptor :
       cr::allObjectDescriptors()) {
    const bool spanPolicy =
        descriptor.placementPolicy.gesturePolicy ==
        cr::CreativePlacementGesturePolicy::HorizontalSpan;
    spanPolicyCount += spanPolicy ? 1U : 0U;
    if (spanPolicy) {
      ok = expect(cr::descriptorSupportsCreativeStructuralSpan(descriptor),
                  "span policy rows satisfy the structural planner contract") &&
           ok;
    }
  }
  for (cr::CreativeObjectKind kind : spanKinds) {
    ok = expect(cr::describeObject(kind).placementPolicy.gesturePolicy ==
                    cr::CreativePlacementGesturePolicy::HorizontalSpan,
                "beam-like descriptors opt into two-anchor spans") &&
         ok;
  }
  for (cr::CreativeObjectKind kind : immediateKinds) {
    ok = expect(cr::describeObject(kind).placementPolicy.gesturePolicy ==
                    cr::CreativePlacementGesturePolicy::Immediate,
                "column arch and ladder descriptors remain immediate") &&
         ok;
  }

  const cr::CreativeStructuralSpanPlan beam =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {0.0, 2.0, 0.0},
           {5.0, 2.0, 0.0}});
  const cr::CreativeStructuralSpanPlan reversed =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {5.0, 2.0, 0.0},
           {0.0, 2.0, 0.0}});
  const cr::CreativeTransformedBounds beamWorld =
      cr::resolveCreativeTransformedBounds(beam.authoredBounds,
                                           beam.transform);
  ok = expect(spanPolicyCount == spanKinds.size(),
              "only the four beam-like descriptors own span gestures") &&
       expect(beam.accepted &&
                  beam.status == cr::CreativeStructuralSpanStatus::Ready &&
                  beam.spanAxis == cr::CreativeStructuralSpanAxis::LocalX &&
                  near(beam.spanLengthMeters, 5.0) &&
                  nearVec3(beam.transform.position, {2.5, 2.175, 0.0}) &&
                  nearVec3(beamWorld.size, {5.0, 0.35, 0.35}),
              "beam span preserves thickness and bottom-anchors at support") &&
       expect(reversed.accepted &&
                  sameVec3(reversed.transform.position,
                           beam.transform.position) &&
                  sameVec3(reversed.transform.rotationEulerRadians,
                           beam.transform.rotationEulerRadians) &&
                  cr::creativeBoundsExactlyEqual(reversed.authoredBounds,
                                                 beam.authoredBounds),
              "reversing equal-height endpoints produces identical geometry") &&
       ok;

  const cr::CreativeStructuralSpanPlan diagonal =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {0.0, 0.0, 0.0},
           {3.0, 0.0, 4.0}});
  const cr::CreativeStructuralSpanPlan bridge =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Bridge, {0.0, 1.0, 0.0},
           {5.0, 1.0, 0.0}});
  const cr::CreativeTransformedBounds bridgeWorld =
      cr::resolveCreativeTransformedBounds(bridge.authoredBounds,
                                           bridge.transform);
  const cr::CreativeBoundsMetrics bridgeWorldMetrics =
      cr::measureCreativeBounds(bridgeWorld.worldBounds);
  ok = expect(diagonal.accepted && near(diagonal.spanLengthMeters, 5.0) &&
                  near(diagonal.transform.rotationEulerRadians.y,
                       std::atan2(-4.0, 3.0)),
              "diagonal span uses finite horizontal length and yaw") &&
       expect(bridge.accepted &&
                  bridge.spanAxis == cr::CreativeStructuralSpanAxis::LocalZ &&
                  bridgeWorldMetrics.valid &&
                  nearVec3(bridgeWorld.size, {3.0, 0.35, 5.0}) &&
                  nearVec3(bridgeWorldMetrics.size, {5.0, 0.35, 3.0}),
              "bridge extends its descriptor-owned long axis") &&
       ok;

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeStructuralSpanPlan degenerate =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {}, {0.001, 0.0, 0.0}});
  const cr::CreativeStructuralSpanPlan nonFinite =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {}, {nan, 0.0, 0.0}});
  const cr::CreativeStructuralSpanPlan tooLong =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {}, {5000.0, 0.0, 0.0}});
  const cr::CreativeStructuralSpanPlan unsupported =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Column, {}, {5.0, 0.0, 0.0}});
  cr::CreativeTransform scaledBeamTransform = beam.transform;
  scaledBeamTransform.scale = {2.0, 1.5, 0.5};
  const cr::CreativeStructuralSpanInstance scaledBeam =
      cr::resolveCreativeStructuralSpan(
          cr::CreativeObjectKind::Beam, scaledBeamTransform,
          beam.authoredBounds);
  const cr::CreativeVec3 movedBeamTarget{
      scaledBeam.endpoints[1].x + 2.0, scaledBeam.endpoints[1].y + 99.0,
      scaledBeam.endpoints[1].z + 3.0};
  const cr::CreativeStructuralSpanEditPlan scaledBeamEdit =
      cr::planCreativeStructuralSpanEdit(
          {cr::CreativeObjectKind::Beam, scaledBeamTransform,
           beam.authoredBounds,
           cr::CreativeStructuralSpanEndpoint::Second, movedBeamTarget});
  const cr::CreativeBoundsMetrics sourceBeamBounds =
      cr::measureCreativeBounds(beam.authoredBounds);
  const cr::CreativeBoundsMetrics editedBeamBounds =
      cr::measureCreativeBounds(scaledBeamEdit.authoredBounds);
  const cr::CreativeStructuralSpanInstance editedBeam =
      cr::resolveCreativeStructuralSpan(
          cr::CreativeObjectKind::Beam, scaledBeamEdit.transform,
          scaledBeamEdit.authoredBounds);
  ok = expect(scaledBeam.accepted && scaledBeamEdit.accepted &&
                  scaledBeamEdit.changed && editedBeam.accepted &&
                  sameVec3(scaledBeamEdit.transform.scale,
                           scaledBeamTransform.scale) &&
                  near(editedBeamBounds.size.y, sourceBeamBounds.size.y) &&
                  near(editedBeamBounds.size.z, sourceBeamBounds.size.z) &&
                  nearVec3(editedBeam.endpoints[0],
                           scaledBeam.endpoints[0]) &&
                  near(editedBeam.endpoints[1].x, movedBeamTarget.x) &&
                  near(editedBeam.endpoints[1].z, movedBeamTarget.z) &&
                  near(editedBeam.endpoints[1].y,
                       scaledBeam.endpoints[1].y),
              "endpoint editing preserves scale cross-section fixed end and "
              "bottom elevation") &&
       ok;

  const cr::CreativeStructuralSpanInstance bridgeInstance =
      cr::resolveCreativeStructuralSpan(
          cr::CreativeObjectKind::Bridge, bridge.transform,
          bridge.authoredBounds);
  const cr::CreativeStructuralSpanEditPlan bridgeEdit =
      cr::planCreativeStructuralSpanEdit(
          {cr::CreativeObjectKind::Bridge, bridge.transform,
           bridge.authoredBounds,
           cr::CreativeStructuralSpanEndpoint::First,
           {bridgeInstance.endpoints[0].x - 1.0, -50.0,
            bridgeInstance.endpoints[0].z + 2.0}});
  cr::CreativeTransform tilted = beam.transform;
  tilted.rotationEulerRadians.x = 0.25;
  const cr::CreativeStructuralSpanInstance tiltedInstance =
      cr::resolveCreativeStructuralSpan(
          cr::CreativeObjectKind::Beam, tilted, beam.authoredBounds);
  const cr::CreativeStructuralSpanEditPlan invalidEndpoint =
      cr::planCreativeStructuralSpanEdit(
          {cr::CreativeObjectKind::Beam, beam.transform,
           beam.authoredBounds,
           static_cast<cr::CreativeStructuralSpanEndpoint>(255U),
           {8.0, 0.0, 0.0}});
  ok = expect(bridgeInstance.accepted && bridgeEdit.accepted &&
                  bridgeEdit.changed &&
                  bridgeEdit.spanAxis ==
                      cr::CreativeStructuralSpanAxis::LocalZ,
              "bridge endpoint editing retains its descriptor-owned local Z "
              "span axis") &&
       expect(!tiltedInstance.accepted &&
                  tiltedInstance.status ==
                      cr::CreativeStructuralSpanStatus::UnsupportedTransform,
              "non-planar structural transforms fail closed") &&
       expect(!invalidEndpoint.accepted &&
                  invalidEndpoint.status ==
                      cr::CreativeStructuralSpanStatus::InvalidEndpoint,
              "invalid endpoint ids fail closed") &&
       ok;
  return expect(!degenerate.accepted &&
                    degenerate.status ==
                        cr::CreativeStructuralSpanStatus::DegenerateSpan,
                "sub-centimeter spans fail closed") &&
         expect(!nonFinite.accepted &&
                    nonFinite.status ==
                        cr::CreativeStructuralSpanStatus::InvalidAnchor,
                "non-finite endpoints fail closed") &&
         expect(!tooLong.accepted &&
                    tooLong.status ==
                        cr::CreativeStructuralSpanStatus::SpanTooLong,
                "structural spans enforce a finite world bound") &&
         expect(!unsupported.accepted &&
                    unsupported.status ==
                        cr::CreativeStructuralSpanStatus::UnsupportedDescriptor,
                "immediate descriptors cannot enter the span planner") &&
         ok;
}

bool representativeDescriptorsPinRuntimeAnchorSemantics() {
  return expect(cr::describeObject(cr::CreativeObjectKind::SpawnPoint)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Spawn,
                "spawn point runtime anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::ExitPoint)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Exit,
                "exit point runtime anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::EnemySpawn)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Monster,
                "enemy spawn runtime anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::NpcSpawn)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Npc,
                "npc spawn runtime anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::LootPoint)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Pickup,
                "loot point runtime anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::PointLight)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Light,
                "point light metadata anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::SoundEmitter)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Audio,
                "sound emitter metadata anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::CameraMarker)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Camera,
                "camera marker metadata anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::CoverPoint)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::Cover,
                "cover point runtime anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::PatrolNode)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::PatrolPost,
                "patrol node runtime anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::EntrancePoint)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::None,
                "entrance point has no session anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::QuestMarker)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::None,
                "quest marker does not inherit gameplay anchor semantic") &&
         expect(cr::describeObject(cr::CreativeObjectKind::Socket)
                    .runtimeAnchorSemantic ==
                    cr::CreativeRuntimeAnchorSemantic::None,
                "socket has no runtime anchor semantic");
}

bool representativeDescriptorsPinCapabilityFacts() {
  const cr::CreativeObjectDescriptor& wall =
      cr::describeObject(cr::CreativeObjectKind::Wall);
  const cr::CreativeObjectDescriptor& crate =
      cr::describeObject(cr::CreativeObjectKind::Crate);
  const cr::CreativeObjectDescriptor& pointLight =
      cr::describeObject(cr::CreativeObjectKind::PointLight);
  const cr::CreativeObjectDescriptor& note =
      cr::describeObject(cr::CreativeObjectKind::Note);
  const cr::CreativeObjectDescriptor& navLink =
      cr::describeObject(cr::CreativeObjectKind::NavLink);
  const cr::CreativeObjectDescriptor& patrolRoute =
      cr::describeObject(cr::CreativeObjectKind::PatrolRoute);
  const cr::CreativeObjectDescriptor& group =
      cr::describeObject(cr::CreativeObjectKind::Group);
  const cr::CreativeObjectDescriptor& prefab =
      cr::describeObject(cr::CreativeObjectKind::PrefabInstance);
  const cr::CreativeObjectDescriptor& testLane =
      cr::describeObject(cr::CreativeObjectKind::TestLane);

  return expect(wall.hasTransform, "wall has transform") &&
         expect(wall.hasBounds, "wall has bounds") &&
         expect(wall.canHaveParent, "wall can have parent") &&
         expect(!wall.canOwnChildren, "wall cannot own children") &&
         expect(wall.isRuntimeMeaningful, "wall runtime meaningful") &&
         expect(!wall.isEditorOnly, "wall not editor-only") &&
         expect(crate.hasTransform, "crate has transform") &&
         expect(crate.hasBounds, "crate has bounds") &&
         expect(crate.canHaveParent, "crate can join transform groups") &&
         expect(!crate.canOwnChildren, "crate cannot own children") &&
         expect(!crate.isRuntimeMeaningful, "crate not runtime meaningful") &&
         expect(!crate.isEditorOnly, "crate not editor-only") &&
         expect(pointLight.hasTransform, "point light has transform") &&
         expect(!pointLight.hasBounds, "point light has no bounds") &&
         expect(pointLight.canHaveParent,
                "point light can join transform groups") &&
         expect(!pointLight.canOwnChildren, "point light cannot own children") &&
         expect(!pointLight.isRuntimeMeaningful,
                "point light not runtime meaningful") &&
         expect(!pointLight.isEditorOnly, "point light not editor-only") &&
         expect(note.hasTransform, "note has transform") &&
         expect(!note.hasBounds, "note has no bounds") &&
         expect(note.canHaveParent, "note can join transform groups") &&
         expect(!note.canOwnChildren, "note cannot own children") &&
         expect(!note.isRuntimeMeaningful, "note not runtime meaningful") &&
         expect(note.isEditorOnly, "note editor-only") &&
         expect(!navLink.hasTransform, "nav link has no transform") &&
         expect(!navLink.hasBounds, "nav link has no bounds") &&
         expect(!navLink.canHaveParent, "nav link cannot have parent") &&
         expect(!navLink.canOwnChildren, "nav link cannot own children") &&
         expect(navLink.isRuntimeMeaningful, "nav link runtime meaningful") &&
         expect(!navLink.isEditorOnly, "nav link not editor-only") &&
         expect(!patrolRoute.hasTransform, "patrol route has no transform") &&
         expect(!patrolRoute.hasBounds, "patrol route has no bounds") &&
         expect(!patrolRoute.canHaveParent,
                "patrol route cannot have parent") &&
         expect(patrolRoute.canOwnChildren,
                "patrol route can own children") &&
         expect(patrolRoute.isRuntimeMeaningful,
                "patrol route runtime meaningful") &&
         expect(!patrolRoute.isEditorOnly, "patrol route not editor-only") &&
         expect(group.hasTransform, "group has transform") &&
         expect(!group.hasBounds, "group has no bounds") &&
         expect(group.canHaveParent, "group can have parent") &&
         expect(group.canOwnChildren, "group can own children") &&
         expect(!group.isRuntimeMeaningful, "group not runtime meaningful") &&
         expect(!group.isEditorOnly, "group not editor-only") &&
         expect(prefab.hasTransform, "prefab has transform") &&
         expect(prefab.hasBounds, "prefab has bounds") &&
         expect(prefab.canHaveParent, "prefab can have parent") &&
         expect(prefab.canOwnChildren, "prefab can own children") &&
         expect(prefab.isRuntimeMeaningful, "prefab runtime meaningful") &&
         expect(!prefab.isEditorOnly, "prefab not editor-only") &&
         expect(testLane.hasTransform, "test lane has transform") &&
         expect(testLane.hasBounds, "test lane has bounds") &&
         expect(testLane.canHaveParent,
                "test lane can join transform groups") &&
         expect(testLane.canOwnChildren, "test lane can own children") &&
         expect(!testLane.isRuntimeMeaningful,
                "test lane not runtime meaningful") &&
         expect(!testLane.isEditorOnly, "test lane not editor-only");
}

bool representativeDescriptorsPinAuthoringBrushPaletteVisibility() {
  const cr::CreativeObjectDescriptor& room =
      cr::describeObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDescriptor& crate =
      cr::describeObject(cr::CreativeObjectKind::Crate);
  const cr::CreativeObjectDescriptor& wall =
      cr::describeObject(cr::CreativeObjectKind::Wall);
  const cr::CreativeObjectDescriptor& beam =
      cr::describeObject(cr::CreativeObjectKind::Beam);
  const cr::CreativeObjectDescriptor& pointLight =
      cr::describeObject(cr::CreativeObjectKind::PointLight);
  const cr::CreativeObjectDescriptor& patrolRoute =
      cr::describeObject(cr::CreativeObjectKind::PatrolRoute);
  const cr::CreativeObjectDescriptor& triggerZone =
      cr::describeObject(cr::CreativeObjectKind::TriggerZone);
  const cr::CreativeObjectDescriptor& testLane =
      cr::describeObject(cr::CreativeObjectKind::TestLane);
  const cr::CreativeObjectDescriptor& fallShaft =
      cr::describeObject(cr::CreativeObjectKind::FallShaft);
  const cr::CreativeObjectDescriptor& timingGate =
      cr::describeObject(cr::CreativeObjectKind::TimingGate);
  const cr::CreativeObjectDescriptor& note =
      cr::describeObject(cr::CreativeObjectKind::Note);
  const cr::CreativeObjectDescriptor& measurementLine =
      cr::describeObject(cr::CreativeObjectKind::MeasurementLine);
  bool helpersFollowDescriptorIntent = true;
  for (const cr::CreativeObjectDescriptor& descriptor :
       cr::allObjectDescriptors()) {
    helpersFollowDescriptorIntent =
        expect(cr::descriptorShowsInAuthoringBrushPalette(descriptor) ==
                   (descriptor.authoringPaletteVisibility ==
                    cr::CreativeAuthoringPaletteVisibility::Brush),
               "authoring palette helper reads descriptor intent") &&
        helpersFollowDescriptorIntent;
  }

  return expect(helpersFollowDescriptorIntent,
                "authoring palette helpers follow descriptor intent") &&
         expect(room.authoringPaletteVisibility ==
                    cr::CreativeAuthoringPaletteVisibility::Hidden,
                "room descriptor explicitly hidden from brush palette") &&
         expect(!cr::descriptorShowsInAuthoringBrushPalette(room),
                "room metadata hidden from brush palette") &&
         expect(!cr::objectShowsInAuthoringBrushPalette(
                    cr::CreativeObjectKind::Room),
                "room object hidden from brush palette") &&
         expect(crate.authoringPaletteVisibility ==
                    cr::CreativeAuthoringPaletteVisibility::Brush,
                "crate descriptor explicitly visible in brush palette") &&
         expect(cr::descriptorShowsInAuthoringBrushPalette(crate),
                "crate visible in brush palette") &&
         expect(cr::objectShowsInAuthoringBrushPalette(
                    cr::CreativeObjectKind::Crate),
                "crate object visible in brush palette") &&
         expect(cr::descriptorShowsInAuthoringBrushPalette(wall),
                "wall visible in brush palette") &&
         expect(cr::descriptorShowsInAuthoringBrushPalette(beam),
                "beam visible in brush palette") &&
         expect(cr::descriptorShowsInAuthoringBrushPalette(pointLight),
                "point light visible in brush palette") &&
         expect(cr::descriptorShowsInAuthoringBrushPalette(patrolRoute),
                "patrol route visible in brush palette") &&
         expect(cr::descriptorShowsInAuthoringBrushPalette(triggerZone),
                "trigger zone visible in brush palette") &&
         expect(testLane.authoringPaletteVisibility ==
                    cr::CreativeAuthoringPaletteVisibility::Hidden,
                "test lane descriptor explicitly hidden from brush palette") &&
         expect(!cr::descriptorShowsInAuthoringBrushPalette(testLane),
                "test lane box volume hidden from brush palette") &&
         expect(!cr::descriptorShowsInAuthoringBrushPalette(fallShaft),
                "fall shaft box volume hidden from brush palette") &&
         expect(!cr::descriptorShowsInAuthoringBrushPalette(timingGate),
                "timing gate box volume hidden from brush palette") &&
         expect(note.authoringPaletteVisibility ==
                    cr::CreativeAuthoringPaletteVisibility::Hidden,
                "editor-only note explicitly hidden from brush palette") &&
         expect(!cr::descriptorShowsInAuthoringBrushPalette(note),
                "note editor-only descriptor hidden from brush palette") &&
         expect(!cr::descriptorShowsInAuthoringBrushPalette(measurementLine),
                "measurement line editor-only descriptor hidden from brush palette");
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

bool boxVolumeTestingDescriptorsAreNonRuntimeHelpers() {
  const cr::CreativeObjectDescriptor& testLane =
      cr::describeObject(cr::CreativeObjectKind::TestLane);
  const cr::CreativeObjectDescriptor& fallShaft =
      cr::describeObject(cr::CreativeObjectKind::FallShaft);
  const cr::CreativeObjectDescriptor& timingGate =
      cr::describeObject(cr::CreativeObjectKind::TimingGate);

  return expect(testLane.shapeKind == cr::CreativeObjectShapeKind::BoxVolume,
                "test lane box volume") &&
         expect(testLane.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "test lane box projection") &&
         expect(testLane.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Testing,
                "test lane testing occupancy") &&
         expect(!testLane.isRuntimeMeaningful,
                "test lane not runtime meaningful") &&
         expect(!testLane.isEditorOnly, "test lane not editor-only") &&
         expect(fallShaft.shapeKind == cr::CreativeObjectShapeKind::BoxVolume,
                "fall shaft box volume") &&
         expect(fallShaft.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "fall shaft box projection") &&
         expect(fallShaft.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Testing,
                "fall shaft testing occupancy") &&
         expect(!fallShaft.isRuntimeMeaningful,
                "fall shaft not runtime meaningful") &&
         expect(!fallShaft.isEditorOnly, "fall shaft not editor-only") &&
         expect(timingGate.shapeKind == cr::CreativeObjectShapeKind::BoxVolume,
                "timing gate box volume") &&
         expect(timingGate.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "timing gate box projection") &&
         expect(timingGate.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Testing,
                "timing gate testing occupancy") &&
         expect(!timingGate.isRuntimeMeaningful,
                "timing gate not runtime meaningful") &&
         expect(!timingGate.isEditorOnly, "timing gate not editor-only");
}

bool volumeProjectionDescriptorsAreSemanticVolumes() {
  const cr::CreativeObjectDescriptor& trigger =
      cr::describeObject(cr::CreativeObjectKind::TriggerZone);
  const cr::CreativeObjectDescriptor& alert =
      cr::describeObject(cr::CreativeObjectKind::AlertZone);
  const cr::CreativeObjectDescriptor& boundary =
      cr::describeObject(cr::CreativeObjectKind::BoundaryVolume);

  return expect(trigger.shapeKind == cr::CreativeObjectShapeKind::BoxVolume,
                "trigger zone box volume") &&
         expect(trigger.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::VolumeProjection,
                "trigger zone volume projection") &&
         expect(trigger.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Trigger,
                "trigger zone trigger occupancy") &&
         expect(alert.shapeKind == cr::CreativeObjectShapeKind::BoxVolume,
                "alert zone box volume") &&
         expect(alert.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::VolumeProjection,
                "alert zone volume projection") &&
         expect(alert.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Gameplay,
                "alert zone gameplay occupancy") &&
         expect(alert.isRuntimeMeaningful,
                "alert zone runtime meaningful metadata") &&
         expect(boundary.shapeKind == cr::CreativeObjectShapeKind::BoxVolume,
                "boundary volume box volume") &&
         expect(boundary.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::VolumeProjection,
                "boundary volume projection") &&
         expect(boundary.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Collision,
                "boundary collision occupancy");
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
             cr::CreativeSpatialProjectionProfile::LinkProjection,
             cr::CreativeSpatialOccupancyKind::Navigation,
             "nav link spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::JumpLink,
             cr::CreativeSpatialProjectionProfile::LinkProjection,
             cr::CreativeSpatialOccupancyKind::Navigation,
             "jump link spatial descriptor") &&
         expectSpatialDescriptor(
             cr::CreativeObjectKind::ClimbLink,
             cr::CreativeSpatialProjectionProfile::LinkProjection,
             cr::CreativeSpatialOccupancyKind::Navigation,
             "climb link spatial descriptor") &&
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

bool placementPoliciesAreDescriptorOwnedAndFailClosed() {
  bool ok = true;
  std::size_t enabledCount = 0U;
  std::size_t voxelBackedCount = 0U;
  for (const cr::CreativeObjectDescriptor& descriptor :
       cr::allObjectDescriptors()) {
    const cr::CreativeObjectPlacementPolicy& policy =
        descriptor.placementPolicy;
    if (!policy.enabled) {
      continue;
    }
    ++enabledCount;
    const bool voxelBacked =
        policy.storagePolicy ==
        cr::CreativePlacementStoragePolicy::VoxelCell;
    voxelBackedCount += voxelBacked ? 1U : 0U;
    const cr::CreativePlacementOccupancyPolicy expectedOccupancy =
        voxelBacked
            ? cr::CreativePlacementOccupancyPolicy::RejectOccupied
            : cr::CreativePlacementOccupancyPolicy::AllowOverlap;
    ok = expect(policy.allowedFaces == cr::kCreativePlacementAllFaces,
                "current placeable descriptors explicitly allow every face") &&
         expect(policy.targetPolicy ==
                    cr::CreativePlacementTargetPolicy::AdjacentCell,
                "current placeable descriptors use adjacent-cell targets") &&
         expect(policy.occupancyPolicy == expectedOccupancy,
                "descriptor storage owns its occupancy behavior") &&
         ok;
  }

  cr::CreativeObjectPlacementPolicy topOnly;
  topOnly.enabled = true;
  topOnly.allowedFaces =
      cr::creativePlacementFaceBit(cr::CreativePlacementFace::PositiveY);
  constexpr std::array cardinalOrientationKinds{
      cr::CreativeObjectKind::Door,
      cr::CreativeObjectKind::Window,
      cr::CreativeObjectKind::Arch,
      cr::CreativeObjectKind::Fence,
      cr::CreativeObjectKind::Railing,
      cr::CreativeObjectKind::Ladder,
      cr::CreativeObjectKind::WallRunSurface,
      cr::CreativeObjectKind::Sign,
      cr::CreativeObjectKind::Banner,
  };
  for (const cr::CreativeObjectKind kind : cardinalOrientationKinds) {
    ok = expect(cr::describeObject(kind).placementPolicy.orientationPolicy ==
                    cr::CreativePlacementOrientationPolicy::
                        CardinalFaceOrPlacerFacing,
                "wall-like descriptor opts into cardinal orientation") &&
         ok;
  }
  const double nan = std::numeric_limits<double>::quiet_NaN();
  return expect(enabledCount > 0U, "descriptor table owns placeable rows") &&
         expect(voxelBackedCount == 4U,
                "exactly four structural materials use voxel storage") &&
         expect(cr::describeObject(cr::CreativeObjectKind::Wall)
                            .placementPolicy.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::VoxelCell &&
                    cr::describeObject(cr::CreativeObjectKind::Floor)
                            .placementPolicy.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::VoxelCell &&
                    cr::describeObject(cr::CreativeObjectKind::Ceiling)
                            .placementPolicy.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::VoxelCell &&
                    cr::describeObject(cr::CreativeObjectKind::Roof)
                            .placementPolicy.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::VoxelCell,
                "wall floor ceiling and roof are voxel-backed") &&
         expect(cr::describeObject(cr::CreativeObjectKind::Crate)
                            .placementPolicy.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::AuthoredObject &&
                    cr::describeObject(cr::CreativeObjectKind::Door)
                            .placementPolicy.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::AuthoredObject &&
                    cr::describeObject(cr::CreativeObjectKind::PatrolRoute)
                            .placementPolicy.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::AuthoredObject,
                "props attachments and paths remain authored objects") &&
         expect(!cr::describeObject(cr::CreativeObjectKind::Unknown)
                     .placementPolicy.enabled,
                "unknown descriptor cannot be placed") &&
         expect(cr::creativePlacementPolicyAllowsFace(topOnly,
                                                       {0.1, 0.9, 0.2}),
                "dominant positive Y normal admits top-only policy") &&
         expect(!cr::creativePlacementPolicyAllowsFace(topOnly,
                                                        {0.0, -1.0, 0.0}),
                "negative Y is rejected by top-only policy") &&
         expect(!cr::creativePlacementPolicyAllowsFace(topOnly, {}),
                "degenerate face normal fails closed") &&
         expect(!cr::creativePlacementPolicyAllowsFace(topOnly,
                                                        {nan, 0.0, 0.0}),
                "non-finite face normal fails closed") &&
         expect(cr::describeObject(cr::CreativeObjectKind::Floor)
                    .placementPolicy.orientationPolicy ==
                    cr::CreativePlacementOrientationPolicy::DescriptorDefault &&
                    cr::describeObject(cr::CreativeObjectKind::Crate)
                            .placementPolicy.orientationPolicy ==
                        cr::CreativePlacementOrientationPolicy::
                            DescriptorDefault,
                "floor and prop descriptors retain default orientation") &&
         expect(cr::describeObject(cr::CreativeObjectKind::Door)
                        .placementPolicy.localForwardFace ==
                    cr::CreativePlacementFace::PositiveZ &&
                    cr::describeObject(cr::CreativeObjectKind::WallRunSurface)
                            .placementPolicy.localForwardFace ==
                        cr::CreativePlacementFace::PositiveX,
                "descriptor data owns each oriented brush local forward axis") &&
         ok;
}

}  // namespace

int main() {
  const bool ok = descriptorLookupIsTotalForEveryEnumKind() &&
                  descriptorTableRowsAreStableAndUnique() &&
                  serializedObjectKindIdsAreStableAndUnique() &&
                  legacyCategoryPredicatesFollowDescriptorTruth() &&
                  shapeKindStringsAreStable() &&
                  runtimeAnchorSemanticStringsAreStable() &&
                  spatialProjectionProfileStringsAreStable() &&
                  roomDescriptorPinsShapeBearingProjectionContract() &&
                  unknownDescriptorRemainsInvalidAndNonProjectable() &&
                  representativeDescriptorsPinShapeFacts() &&
                  descriptorOwnsCanonicalStructuralDimensions() &&
                  descriptorOwnsGeneratedTraversalGeometry() &&
                  structuralSpanPlacementIsDescriptorDrivenAndBounded() &&
                  representativeDescriptorsPinRuntimeAnchorSemantics() &&
                  representativeDescriptorsPinCapabilityFacts() &&
                  representativeDescriptorsPinAuthoringBrushPaletteVisibility() &&
                  shapeAndProjectionCanDifferByDesign() &&
                  boxVolumeTestingDescriptorsAreNonRuntimeHelpers() &&
                  volumeProjectionDescriptorsAreSemanticVolumes() &&
                  representativeDescriptorsPinSpatialFacts() &&
                  mutationDirtyFlagsFollowDescriptorSpatialColumns() &&
                  placementPoliciesAreDescriptorOwnedAndFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
