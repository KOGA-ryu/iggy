#include "app/iggy3d/creative/play/RuntimeDoors.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/play/PlayPreparation.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/ai/SegmentOcclusion.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs, double epsilon = 0.0001) {
  return std::fabs(lhs - rhs) <= epsilon;
}

cr::CreativeDocument baseDocument(cr::CreativeDocumentId id) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Door Runtime");
  static_cast<void>(document.assignId(id));

  cr::CreativeDocumentCreateRequest floor;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.name = "Floor";
  floor.bounds = {{-4.0, 0.0, -4.0}, {4.0, 0.25, 4.0}};
  floor.hasBoundsOverride = true;
  static_cast<void>(document.createObject(floor));

  cr::CreativeDocumentCreateRequest spawn;
  spawn.kind = cr::CreativeObjectKind::SpawnPoint;
  spawn.name = "Spawn";
  spawn.transform.position = {0.0, 0.25, -2.0};
  spawn.hasTransformOverride = true;
  static_cast<void>(document.createObject(spawn));
  return document;
}

cr::CreativeDocumentCreateReceipt createDoor(
    cr::CreativeDocument& document,
    cr::CreativeDoorSettings settings,
    cr::CreativeBounds bounds = {{-0.45, 0.25, -0.04},
                                 {0.45, 2.35, 0.04}},
    cr::CreativeVec3 hinge = {-0.45, 0.25, 0.0}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Door;
  request.name = "Runtime Door";
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  request.transform.position = hinge;
  request.hasTransformOverride = true;
  request.door = settings;
  request.hasDoorSettingsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt createDoorPart(
    cr::CreativeDocument& document,
    cr::CreativeObjectId parent,
    std::string socket,
    std::string name,
    cr::CreativeBounds bounds,
    cr::CreativeVec3 pivot) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Prop;
  request.name = std::move(name);
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  request.transform.position = pivot;
  request.hasTransformOverride = true;
  request.parentId = parent;
  request.attachmentSocket = std::move(socket);
  return document.createObject(request);
}

bool addDoubleDoorChildren(cr::CreativeDocument& document,
                           cr::CreativeObjectId doorId) {
  const cr::CreativeVec3 primaryPivot{-0.89, 0.25, 0.0};
  const cr::CreativeVec3 secondaryPivot{0.89, 0.25, 0.0};
  const auto part = [&](std::string socket, std::string name,
                        cr::CreativeBounds bounds,
                        cr::CreativeVec3 pivot) {
    return createDoorPart(document, doorId, std::move(socket), std::move(name),
                          bounds, pivot)
        .accepted;
  };
  return part("door_primary_lower_hinge", "Primary Lower Hinge",
              {{-0.91, 0.62, -0.055}, {-0.87, 0.72, 0.055}},
              primaryPivot) &&
         part("door_primary_upper_hinge", "Primary Upper Hinge",
              {{-0.91, 1.88, -0.055}, {-0.87, 1.98, 0.055}},
              primaryPivot) &&
         part("door_primary_handle", "Primary Handle",
              {{-0.18, 1.15, -0.08}, {-0.12, 1.27, 0.08}}, primaryPivot) &&
         part("door_secondary_leaf", "Secondary Leaf",
              {{0.01, 0.25, -0.04}, {0.89, 2.35, 0.04}},
              secondaryPivot) &&
         part("door_secondary_lower_hinge", "Secondary Lower Hinge",
              {{0.87, 0.62, -0.055}, {0.91, 0.72, 0.055}},
              secondaryPivot) &&
         part("door_secondary_upper_hinge", "Secondary Upper Hinge",
              {{0.87, 1.88, -0.055}, {0.91, 1.98, 0.055}},
              secondaryPivot) &&
         part("door_secondary_handle", "Secondary Handle",
              {{0.12, 1.15, -0.08}, {0.18, 1.27, 0.08}}, secondaryPivot);
}

std::optional<cr::CreativeRuntimeSandbox> activate(
    cr::CreativeDocument& document) {
  static iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativePlayPreparationResult prepared = cr::prepareCreativePlay(
      {&document, &catalog, "runtime_door_test", 1.0F});
  if (!prepared.accepted || !prepared.payload.has_value()) {
    return std::nullopt;
  }
  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult result =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  return std::move(result.sandbox);
}

cr::CreativeRuntimeInteractableState* findDoor(
    cr::CreativeRuntimeSandbox& sandbox,
    cr::CreativeObjectId objectId) {
  const auto found = std::find_if(
      sandbox.interactables.begin(), sandbox.interactables.end(),
      [objectId](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == objectId;
      });
  return found == sandbox.interactables.end() ? nullptr : &*found;
}

const iggy3d::RoomStaticMeshAsset* findMesh(
    const iggy3d::RoomAsset& room,
    std::string_view id) {
  const auto found = std::find_if(
      room.staticMeshes.begin(), room.staticMeshes.end(),
      [id](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.id == id;
      });
  return found == room.staticMeshes.end() ? nullptr : &*found;
}

iggy3d::SegmentOcclusionVerdict doorwayOcclusion(
    const cr::CreativeRuntimeSandbox& sandbox) {
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &sandbox.collisionSurfaces;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(request);
  return bake.ok
             ? iggy3d::segmentOcclusion(bake.colliders,
                                        {0.0F, 1.2F, -1.25F},
                                        {0.0F, 1.2F, 1.25F}, 0.0F)
             : iggy3d::SegmentOcclusionVerdict::Unknown;
}

bool movePlayer(cr::CreativeRuntimeSandbox& sandbox,
                iggy3d::Vec3 position) {
  iggy3d::SessionState& state =
      sandbox.session.mutableStateForOwnedSystems();
  const iggy3d::EntityState* current = state.world.findById({1U});
  if (current == nullptr) {
    return false;
  }
  iggy3d::Transform3 transform = current->transform;
  transform.position = position;
  return state.world.updateTransform({1U}, transform).status ==
         iggy3d::WorldStatus::Ok;
}

bool pureKernelBuildsBoundedSingleAndDoubleDoors() {
  cr::CreativeDocument document = baseDocument(201U);
  cr::CreativeDoorSettings settings;
  settings.leafArrangement = cr::CreativeDoorLeafArrangement::Double;
  settings.hingeSide = cr::CreativeDoorHingeSide::MinimumEdge;
  settings.swingSide = cr::CreativeDoorSwingSide::PositiveNormal;
  const cr::CreativeDocumentCreateReceipt door = createDoor(
      document, settings,
      {{-0.89, 0.25, -0.04}, {-0.01, 2.35, 0.04}},
      {-0.89, 0.25, 0.0});
  if (!door.accepted || !addDoubleDoorChildren(document, door.objectId)) {
    return expect(false, "double door fixture creates");
  }
  const cr::CreativeObject* root = document.findObject(door.objectId);
  if (root == nullptr) {
    return expect(false, "double door root exists");
  }
  const cr::CreativeRuntimeDoorBuildResult built =
      cr::buildCreativeRuntimeDoorDefinition(*root, document.objects());
  cr::CreativeDoorSettings singleSettings;
  cr::CreativeObject invalidDouble = *root;
  std::vector<cr::CreativeObject> rootOnly{invalidDouble};
  const cr::CreativeRuntimeDoorBuildResult missingSecondary =
      cr::buildCreativeRuntimeDoorDefinition(invalidDouble, rootOnly);

  return expect(built.ok && built.definition.leafCount == 2U,
                "double door owns two leaves") &&
         expect(built.definition.leaves[0].roomMeshIdCount == 4U &&
                    built.definition.leaves[1].roomMeshIdCount == 4U,
                "each leaf owns one panel and three hardware parts") &&
         expect(built.definition.leaves[0].openAngleRadians < 0.0F &&
                    built.definition.leaves[1].openAngleRadians > 0.0F,
                "opposed hinges produce mirrored swing angles") &&
         expect(cr::creativeRuntimeDoorLeafIndexForMesh(
                    built.definition,
                    "creative_object_" + std::to_string(door.objectId)) == 0U,
                "semantic root mesh belongs to primary leaf") &&
         expect(!missingSecondary.ok &&
                    missingSecondary.status ==
                        cr::CreativeRuntimeDoorBuildStatus::InvalidHierarchy,
                "double door without secondary leaf fails closed") &&
         expect(singleSettings.transitionSeconds > 0.0,
                "default transition remains positive");
}

bool runtimeAnimationPublishesCollisionLosReasoningAndSound() {
  cr::CreativeDocument document = baseDocument(202U);
  cr::CreativeDoorSettings settings;
  settings.transitionSeconds = 0.2;
  const cr::CreativeDocumentCreateReceipt created = createDoor(document, settings);
  std::optional<cr::CreativeRuntimeSandbox> activated = activate(document);
  if (!created.accepted || !activated.has_value()) {
    return expect(false, "single runtime door activates");
  }
  cr::CreativeRuntimeSandbox& sandbox = *activated;
  cr::CreativeRuntimeInteractableState* door =
      findDoor(sandbox, created.objectId);
  if (door == nullptr) {
    return expect(false, "single runtime door state exists");
  }
  const std::string meshId = door->definition.roomMeshId;
  const iggy3d::Aabb3 targetWorldBounds{
      door->definition.transform.position + door->definition.localBounds.min,
      door->definition.transform.position + door->definition.localBounds.max};
  const iggy3d::Aabb3& closedBounds =
      door->definition.door.leaves[0].closedAssemblyBounds;
  const iggy3d::RoomStaticMeshAsset* closedMesh = findMesh(sandbox.room, meshId);
  if (closedMesh == nullptr) {
    return expect(false, "closed door mesh exists");
  }
  const float closedYaw = closedMesh->rotationEulerRadians.y;
  const std::size_t meshCount = sandbox.room.staticMeshes.size();
  const std::size_t surfaceCount = sandbox.room.spatialSurfaces.size();
  const std::size_t soundCount =
      sandbox.session.state().transient.soundEvents.size();
  const std::array<iggy3d::Vec3, 2U> waypoints{
      iggy3d::Vec3{0.0F, 0.25F, -1.25F},
      iggy3d::Vec3{0.0F, 0.25F, 1.25F}};
  const iggy3d::ReasoningGraph closedReasoning =
      iggy3d::buildReasoningGraph(sandbox.room, waypoints);
  const cr::CreativeRuntimeInteractionEffectReceipt opening =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, door->entity);
  const bool openingTargetActive = door->targetActive;
  const cr::CreativeRuntimeDoorUpdateReceipt half =
      cr::updateCreativeRuntimeDoors(sandbox, 10U);
  const iggy3d::RoomStaticMeshAsset* halfMesh = findMesh(sandbox.room, meshId);
  const float halfYaw =
      halfMesh == nullptr ? closedYaw : halfMesh->rotationEulerRadians.y;
  const cr::CreativeRuntimeDoorUpdateReceipt opened =
      cr::updateCreativeRuntimeDoors(sandbox, 10U);
  const iggy3d::RoomStaticMeshAsset* openMesh = findMesh(sandbox.room, meshId);
  const bool openMeshPresent = openMesh != nullptr;
  const float openYaw = openMeshPresent ? openMesh->rotationEulerRadians.y
                                       : closedYaw;
  const double openedFraction = door->door.openFraction;
  const iggy3d::ReasoningGraph openReasoning =
      iggy3d::buildReasoningGraph(sandbox.room, waypoints);

  if (!movePlayer(sandbox, {0.0F, 0.25F, 0.0F})) {
    return expect(false, "player moves into doorway");
  }
  const cr::CreativeRuntimeInteractionEffectReceipt occupiedClose =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, door->entity);
  const bool stayedOpen = door->targetActive;
  if (!movePlayer(sandbox, {2.0F, 0.25F, -2.0F})) {
    return expect(false, "player clears doorway");
  }
  const cr::CreativeRuntimeInteractionEffectReceipt closing =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, door->entity);
  const cr::CreativeRuntimeDoorUpdateReceipt closingHalf =
      cr::updateCreativeRuntimeDoors(sandbox, 10U);
  const cr::CreativeRuntimeDoorUpdateReceipt closed =
      cr::updateCreativeRuntimeDoors(sandbox, 10U);

  return expect(door->targetMeshes.size() == 1U &&
                    door->targetSurfaces.size() == 2U,
                "single door captures one moving mesh and collision pair") &&
         expect(near(targetWorldBounds.min.x,
                     closedBounds.min.x -
                         cr::kCreativeRuntimeDoorTargetPaddingMeters) &&
                    near(targetWorldBounds.min.y,
                         closedBounds.min.y -
                             cr::kCreativeRuntimeDoorTargetPaddingMeters) &&
                    near(targetWorldBounds.min.z,
                         closedBounds.min.z -
                             cr::kCreativeRuntimeDoorTargetPaddingMeters) &&
                    near(targetWorldBounds.max.x,
                         closedBounds.max.x +
                             cr::kCreativeRuntimeDoorTargetPaddingMeters) &&
                    near(targetWorldBounds.max.y,
                         closedBounds.max.y +
                             cr::kCreativeRuntimeDoorTargetPaddingMeters) &&
                    near(targetWorldBounds.max.z,
                         closedBounds.max.z +
                             cr::kCreativeRuntimeDoorTargetPaddingMeters),
                "interaction proxy tightly covers the closed leaf assembly") &&
         expect(doorwayOcclusion(sandbox) ==
                    iggy3d::SegmentOcclusionVerdict::Blocked,
                "closed door blocks LOS after the round trip") &&
         expect(opening.accepted && opening.changed &&
                    opening.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorOpening &&
                    opening.geometryRevision == 0U && openingTargetActive,
                "interaction requests open without deleting geometry") &&
         expect(sandbox.session.state().transient.soundEvents.size() ==
                    soundCount + 2U,
                "open and accepted close each queue exactly one sound") &&
         expect(half.accepted && half.changed &&
                    near(door->definition.door.settings.transitionSeconds, 0.2) &&
                    half.geometryRevision == 1U &&
                    !near(halfYaw, closedYaw),
                "first fixed tick publishes an interpolated hinge pose") &&
         expect(opened.accepted && opened.completedDoorCount == 1U &&
                    openMeshPresent && near(openedFraction, 1.0) &&
                    !near(openYaw, closedYaw),
                "second fixed tick reaches the authored open pose") &&
         expect(openReasoning.edges.size() > closedReasoning.edges.size(),
                "open door restores the cross-door reasoning edge") &&
         expect(occupiedClose.accepted && !occupiedClose.changed &&
                    occupiedClose.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::TargetOccupied &&
                    stayedOpen,
                "occupied doorway rejects closing nonfatally") &&
         expect(closing.accepted && closing.changed && !door->targetActive &&
                    closingHalf.changed && closed.changed &&
                    closed.completedDoorCount == 1U,
                "clear doorway closes through the same fixed-tick path") &&
         expect(meshCount == sandbox.room.staticMeshes.size() &&
                    surfaceCount == sandbox.room.spatialSurfaces.size() &&
                    sandbox.geometryRevision == 4U,
                "animation preserves geometry cardinality and revisions per tick");
}

bool initialOpenAndLockedVariantsAreHonored() {
  cr::CreativeDocument openDocument = baseDocument(203U);
  cr::CreativeDoorSettings openSettings;
  openSettings.initialState = cr::CreativeDoorInitialState::Open;
  openSettings.transitionSeconds = 0.2;
  const cr::CreativeDocumentCreateReceipt openDoor =
      createDoor(openDocument, openSettings);
  const std::uint64_t openRevision = openDocument.revision();
  std::optional<cr::CreativeRuntimeSandbox> opened = activate(openDocument);
  if (!openDoor.accepted || !opened.has_value()) {
    return expect(false, "initial-open door activates");
  }
  cr::CreativeRuntimeInteractableState* openState =
      findDoor(*opened, openDoor.objectId);

  cr::CreativeDocument lockedDocument = baseDocument(204U);
  cr::CreativeDoorSettings lockedSettings;
  lockedSettings.gameplayLocked = true;
  const cr::CreativeDocumentCreateReceipt lockedDoor =
      createDoor(lockedDocument, lockedSettings);
  std::optional<cr::CreativeRuntimeSandbox> locked = activate(lockedDocument);
  if (!lockedDoor.accepted || !locked.has_value()) {
    return expect(false, "locked door activates");
  }
  cr::CreativeRuntimeInteractableState* lockedState =
      findDoor(*locked, lockedDoor.objectId);
  if (openState == nullptr || lockedState == nullptr) {
    return expect(false, "variant runtime states exist");
  }
  const std::size_t soundCount =
      locked->session.state().transient.soundEvents.size();
  const cr::CreativeRuntimeInteractionEffectReceipt rejected =
      cr::applyCreativeRuntimeInteractionEffect(*locked, lockedState->entity);

  return expect(openState->targetActive &&
                    near(openState->door.openFraction, 1.0) &&
                    opened->geometryRevision == 0U &&
                    doorwayOcclusion(*opened) ==
                        iggy3d::SegmentOcclusionVerdict::Clear,
                "initial-open pose publishes before activation without revision") &&
         expect(openDocument.revision() == openRevision,
                "runtime initial pose does not dirty authored state") &&
         expect(rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorLocked &&
                    !lockedState->targetActive &&
                    locked->session.state().transient.soundEvents.size() ==
                        soundCount,
                "locked door reports feedback without motion or sound") &&
         expect(cr::toString(rejected.status) == "door_locked",
                "locked status has stable diagnostic text");
}

bool groupedDoubleDoorMovesBothLeaves() {
  cr::CreativeDocument document = baseDocument(205U);
  cr::CreativeDoorSettings settings;
  settings.leafArrangement = cr::CreativeDoorLeafArrangement::Double;
  settings.transitionSeconds = 0.1;
  const cr::CreativeDocumentCreateReceipt created = createDoor(
      document, settings,
      {{-0.89, 0.25, -0.04}, {-0.01, 2.35, 0.04}},
      {-0.89, 0.25, 0.0});
  if (!created.accepted || !addDoubleDoorChildren(document, created.objectId)) {
    return expect(false, "runtime double door fixture creates");
  }
  std::optional<cr::CreativeRuntimeSandbox> activated = activate(document);
  if (!activated.has_value()) {
    return expect(false, "runtime double door activates");
  }
  cr::CreativeRuntimeInteractableState* door =
      findDoor(*activated, created.objectId);
  if (door == nullptr) {
    return expect(false, "runtime double door state exists");
  }
  const std::string primaryId = door->definition.door.leaves[0].roomMeshIds[0];
  const std::string secondaryId = door->definition.door.leaves[1].roomMeshIds[0];
  const iggy3d::RoomStaticMeshAsset* primaryClosed =
      findMesh(activated->room, primaryId);
  const iggy3d::RoomStaticMeshAsset* secondaryClosed =
      findMesh(activated->room, secondaryId);
  if (primaryClosed == nullptr || secondaryClosed == nullptr) {
    return expect(false, "double door leaf meshes exist");
  }
  const float primaryYaw = primaryClosed->rotationEulerRadians.y;
  const float secondaryYaw = secondaryClosed->rotationEulerRadians.y;
  const auto opening =
      cr::applyCreativeRuntimeInteractionEffect(*activated, door->entity);
  const auto advanced = cr::updateCreativeRuntimeDoors(*activated, 10U);
  const iggy3d::RoomStaticMeshAsset* primaryOpen =
      findMesh(activated->room, primaryId);
  const iggy3d::RoomStaticMeshAsset* secondaryOpen =
      findMesh(activated->room, secondaryId);

  return expect(door->targetMeshes.size() == 8U &&
                    door->targetSurfaces.size() == 16U,
                "double door captures both leaves and all hardware") &&
         expect(opening.accepted && advanced.accepted && advanced.changed &&
                    advanced.completedDoorCount == 1U,
                "double door completes as one semantic runtime target") &&
         expect(primaryOpen != nullptr && secondaryOpen != nullptr &&
                    primaryOpen->rotationEulerRadians.y < primaryYaw &&
                    secondaryOpen->rotationEulerRadians.y > secondaryYaw,
                "double door leaf groups rotate around opposed hinges");
}

}  // namespace

int main() {
  const bool ok = pureKernelBuildsBoundedSingleAndDoubleDoors() &&
                  runtimeAnimationPublishesCollisionLosReasoningAndSound() &&
                  initialOpenAndLockedVariantsAreHonored() &&
                  groupedDoubleDoorMovesBothLeaves();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
