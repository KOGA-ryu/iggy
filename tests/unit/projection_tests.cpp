#include "content/PackageLoader.hpp"
#include "content/assets/RoomAsset.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/physics/PhysicsAabbCollisionBatch.hpp"
#include "runtime/physics/PhysicsDebugSnapshot.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::SessionCreateRequest createRequestFromPackage() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{"fixtures/demos/first_room/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  return create;
}

iggy3d::Session makeSession() {
  return iggy3d::Session::create(createRequestFromPackage()).value;
}

iggy3d::RoomAsset floorWallProjectionRoom() {
  iggy3d::RoomAsset room;
  room.id = "ascii_floor_wall_room";
  room.version = 7;
  room.sourceFile = "inline_ascii_room";
  room.sourceSubset = "floor_wall_slice";

  iggy3d::RoomStaticMeshAsset floor;
  floor.id = "floor_r1_c1";
  floor.role = "floor";
  floor.materialId = "debug_floor";
  floor.positionMeters = {1.0F, 0.0F, 2.0F};
  floor.sizeMeters = {1.0F, 0.1F, 1.0F};
  room.staticMeshes.push_back(floor);

  iggy3d::RoomStaticMeshAsset wall;
  wall.id = "wall_r0_c1";
  wall.role = "wall";
  wall.materialId = "debug_wall";
  wall.positionMeters = {1.0F, 0.5F, 0.0F};
  wall.sizeMeters = {1.0F, 1.0F, 0.1F};
  room.staticMeshes.push_back(wall);

  iggy3d::RoomStaticMeshAsset prop;
  prop.id = "crate_r1_c2";
  prop.role = "prop";
  prop.materialId = "wood_crate_proxy";
  prop.positionMeters = {2.0F, 0.4F, 2.0F};
  prop.sizeMeters = {0.8F, 0.8F, 0.8F};
  room.staticMeshes.push_back(prop);

  iggy3d::RoomStaticMeshAsset ledge;
  ledge.id = "ledge_r1_c3";
  ledge.role = "ledge";
  ledge.materialId = "movement_clamber_ledge_proxy";
  ledge.positionMeters = {3.0F, 0.85F, 2.0F};
  ledge.sizeMeters = {2.0F, 1.7F, 1.0F};
  room.staticMeshes.push_back(ledge);

  iggy3d::RoomStaticMeshAsset door;
  door.id = "door_panel";
  door.role = "opening";
  door.materialId = "debug_door";
  door.positionMeters = {2.0F, 0.5F, 0.0F};
  door.sizeMeters = {1.0F, 1.0F, 0.1F};
  room.staticMeshes.push_back(door);

  iggy3d::RoomAnchorAsset key;
  key.id = "marker_key";
  key.runtimeStableName = "gold_key";
  key.positionMeters = {2.0F, 0.0F, 2.0F};
  room.anchors.push_back(key);

  iggy3d::RoomAnchorAsset dummy;
  dummy.id = "marker_dummy";
  dummy.runtimeStableName = "training_dummy";
  dummy.positionMeters = {3.0F, 0.0F, 2.0F};
  room.anchors.push_back(dummy);

  return room;
}

iggy3d::CommandRecord submittedInteract() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {2};
  return command;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord submittedRetry(iggy3d::CommandId sourceCommandId) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Retry;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.retrySourceCommandId = sourceCommandId;
  return command;
}

iggy3d::CommandRecord submittedControl(iggy3d::CommandKind kind) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::CommandRecord submittedWait() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

bool runQueuedCommand(iggy3d::Session& session) {
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 8, true, true});
  return run.status == iggy3d::SessionRunnerStatus::Advanced && run.ticksAdvanced == 1U;
}

iggy3d::Session makePickedUpSession() {
  iggy3d::Session session = makeSession();
  (void)session.submitCommand(submittedInteract());
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedRetry(1));
  (void)runQueuedCommand(session);
  return session;
}

iggy3d::Session makeCompletedSession() {
  iggy3d::Session session = makePickedUpSession();
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));
  (void)session.submitCommand(submittedWait());
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  (void)session.finalizeDemoIfComplete();
  return session;
}

const iggy3d::SceneItem* findSceneItem(const iggy3d::SceneProjectionResult& projection,
                                       std::string_view stableName) {
  for (const iggy3d::SceneItem& item : projection.items) {
    if (item.stableName == stableName) {
      return &item;
    }
  }
  return nullptr;
}

bool hasDebugKind(const iggy3d::DebugProjectionResult& projection,
                  iggy3d::DebugProjectionKind kind) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == kind) {
      return true;
    }
  }
  return false;
}

bool hasOutOfRangeRejection(const iggy3d::DebugProjectionResult& projection) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == iggy3d::DebugProjectionKind::CommandRejected &&
        item.commandId == 1U && item.sequence == 1U &&
        item.rejection == iggy3d::CommandRejectionReason::OutOfRange &&
        item.actor == iggy3d::EntityId{1} && item.target == iggy3d::EntityId{2}) {
      return true;
    }
  }
  return false;
}

const iggy3d::DebugProjectionItem* findNpcProjectionItem(
    const iggy3d::DebugProjectionResult& projection,
    iggy3d::EntityId actor) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == iggy3d::DebugProjectionKind::NpcBehavior &&
        item.actor == actor) {
      return &item;
    }
  }
  return nullptr;
}

const iggy3d::DebugProjectionItem* nthDebugKind(
    const iggy3d::DebugProjectionResult& projection,
    iggy3d::DebugProjectionKind kind,
    std::size_t targetIndex) {
  std::size_t index = 0U;
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind != kind) {
      continue;
    }
    if (index == targetIndex) {
      return &item;
    }
    ++index;
  }
  return nullptr;
}

std::size_t countDebugKind(const iggy3d::DebugProjectionResult& projection,
                           iggy3d::DebugProjectionKind kind) {
  std::size_t count = 0U;
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == kind) {
      ++count;
    }
  }
  return count;
}

bool nearlyEqualAabb(iggy3d::Aabb3 lhs, iggy3d::Aabb3 rhs) {
  return iggy3d::nearlyEqual(lhs.min, rhs.min) &&
         iggy3d::nearlyEqual(lhs.max, rhs.max);
}

iggy3d::RuntimeDebugSnapshot okRuntimeDebugSnapshot() {
  iggy3d::RuntimeDebugSnapshot snapshot;
  snapshot.status = iggy3d::RuntimeDebugSnapshotStatus::Ok;
  snapshot.sourceTick = 3;
  snapshot.actor = {1};
  snapshot.playerPositionAvailable = true;
  snapshot.position = {1.0F, 0.0F, 2.0F};
  snapshot.grounded = true;
  snapshot.groundContact = true;
  snapshot.groundWalkable = true;
  snapshot.movementPolicyBand = "walkable";
  snapshot.slopeTravelDirection = "flat";
  snapshot.groundNormal = iggy3d::vec3UnitY();
  return snapshot;
}

iggy3d::NpcBehaviorDebugActorRow npcRow(iggy3d::EntityId actor,
                                        std::string_view stableName,
                                        std::string_view profileId,
                                        bool profileResolved,
                                        std::string_view profileStatus,
                                        iggy3d::NpcEngagementPolicy policy,
                                        iggy3d::AiBehaviorKind behavior,
                                        iggy3d::AiIntentKind intent) {
  iggy3d::NpcBehaviorDebugActorRow row;
  row.actor = actor;
  row.stableName = std::string(stableName);
  row.active = true;
  row.isNpc = true;
  row.hasAiState = true;
  row.hasCombatant = true;
  row.behaviorProfileId = std::string(profileId);
  row.profileResolved = profileResolved;
  row.profileStatus = std::string(profileStatus);
  row.engagementPolicy = policy;
  row.behavior = behavior;
  row.lastIntent = intent;
  return row;
}

iggy3d::NpcBehaviorDebugSnapshot npcDebugSnapshot() {
  iggy3d::NpcBehaviorDebugSnapshot snapshot;
  snapshot.status = iggy3d::NpcBehaviorDebugSnapshotStatus::Ok;
  snapshot.reasonCode = "npc_behavior_debug_ok";
  snapshot.sourceTick = 11;
  snapshot.npcWorldCount = 3;
  snapshot.aiActorCount = 3;
  snapshot.resolvedProfileCount = 2;
  snapshot.failedProfileCount = 1;
  snapshot.hostileCount = 1;
  snapshot.passiveCount = 1;
  snapshot.attackingCount = 1;
  snapshot.waitingCount = 1;

  iggy3d::NpcBehaviorDebugActorRow hostile =
      npcRow({2},
             "training_dummy",
             "default",
             true,
             "profile_resolved",
             iggy3d::NpcEngagementPolicy::Hostile,
             iggy3d::AiBehaviorKind::Attacking,
             iggy3d::AiIntentKind::AttackTarget);
  hostile.target = {1};
  hostile.targetStableName = "player";
  hostile.targetResolved = true;
  hostile.targetActive = true;
  hostile.targetDistanceMeters = 1.25F;
  hostile.cooldownTicksRemaining = 2;
  snapshot.actors.push_back(hostile);

  iggy3d::NpcBehaviorDebugActorRow passive =
      npcRow({3},
             "observer",
             "passive",
             true,
             "profile_resolved",
             iggy3d::NpcEngagementPolicy::Passive,
             iggy3d::AiBehaviorKind::Alert,
             iggy3d::AiIntentKind::Wait);
  passive.target = {1};
  passive.targetStableName = "player";
  passive.targetResolved = true;
  passive.targetActive = true;
  passive.targetDistanceMeters = 2.5F;
  snapshot.actors.push_back(passive);

  iggy3d::NpcBehaviorDebugActorRow ghost =
      npcRow({4},
             "ghost",
             "ghost_profile",
             false,
             "profile_missing",
             iggy3d::NpcEngagementPolicy::Hostile,
             iggy3d::AiBehaviorKind::Idle,
             iggy3d::AiIntentKind::None);
  snapshot.actors.push_back(ghost);
  return snapshot;
}

iggy3d::PhysicsDebugSnapshot physicsDebugSnapshot() {
  iggy3d::PhysicsDebugSnapshot snapshot;
  snapshot.ok = true;
  snapshot.status = iggy3d::PhysicsDebugSnapshotStatus::Ready;
  snapshot.reasonCode = "physics_debug_snapshot_ready";
  snapshot.statsOk = true;
  snapshot.sourcePacketCount = 2U;
  snapshot.failedPacketCount = 0U;
  snapshot.broadphaseColliderCount = 7U;
  snapshot.broadphaseOccupiedCellCount = 3U;
  snapshot.broadphaseCellEntryCount = 12U;
  snapshot.broadphaseMaxBucketSize = 4U;
  snapshot.broadphaseCandidatePairCount = 8U;
  snapshot.broadphaseTestedPairCount = 5U;
  snapshot.broadphaseDuplicatePairRejectedCount = 1U;
  snapshot.broadphaseOverlappingPairCount = 2U;
  snapshot.bindingCount = 6U;
  snapshot.colliderCount = 7U;
  snapshot.broadphasePairCount = 2U;
  snapshot.contactCount = 2U;
  snapshot.sensorContactCount = 1U;
  snapshot.solvePlanCount = 2U;
  snapshot.accumulatedPlanCount = 1U;
  snapshot.skippedNoOpPlanCount = 1U;
  snapshot.positionCorrectionAppliedCount = 1U;
  snapshot.velocityImpulseAppliedCount = 2U;
  snapshot.frictionImpulseAppliedCount = 1U;
  snapshot.maxPenetrationMeters = 0.125F;
  snapshot.totalNormalImpulseMagnitude = 3.5F;
  snapshot.maxNormalImpulseMagnitude = 2.25F;
  snapshot.totalFrictionImpulseMagnitude = 1.0F;
  snapshot.maxFrictionImpulseMagnitude = 0.75F;
  snapshot.bodyCount = 6U;
  snapshot.appliedPositionCount = 1U;
  snapshot.appliedVelocityCount = 1U;
  snapshot.kinematicIterationCount = 3U;
  snapshot.kinematicHitCount = 1U;
  snapshot.kinematicSweepTestedColliderCount = 9U;
  snapshot.kinematicGroundTestedColliderCount = 4U;
  snapshot.playerBakedSurfaceCount = 8U;
  snapshot.playerBakedColliderCount = 5U;
  snapshot.playerSkippedSurfaceCount = 2U;
  snapshot.playerHitCount = 1U;
  snapshot.playerIterationCount = 2U;
  snapshot.hasContacts = true;
  snapshot.hasSensorContacts = true;
  snapshot.hasSolverActivity = true;
  snapshot.hasAppliedDeltas = true;
  snapshot.hasKinematicHits = true;
  snapshot.hasPlayerHits = true;
  return snapshot;
}

iggy3d::PhysicsDebugSnapshot physicsWarningSnapshot() {
  iggy3d::PhysicsDebugSnapshot snapshot = physicsDebugSnapshot();
  snapshot.ok = false;
  snapshot.status = iggy3d::PhysicsDebugSnapshotStatus::StatsFailed;
  snapshot.reasonCode = "physics_debug_snapshot_stats_failed";
  snapshot.statsOk = false;
  snapshot.upstreamReasonCode = "physics_frame_stats_nonfinite_scalar";
  snapshot.failedPacketCount = 1U;
  snapshot.hasFailedPackets = true;
  snapshot.hasBroadphasePressure = true;
  snapshot.hasPenetrationWarning = true;
  snapshot.hasImpulseWarning = true;
  snapshot.hasWarnings = true;
  return snapshot;
}

iggy3d::PhysicsAabbCollider physicsCollider(iggy3d::PhysicsBodyId bodyId,
                                            iggy3d::Vec3 center,
                                            iggy3d::Vec3 halfExtents,
                                            bool sensor) {
  iggy3d::PhysicsAabbCollider collider;
  collider.bodyId = bodyId;
  collider.worldCenterMeters = center;
  collider.halfExtentsMeters = halfExtents;
  collider.bounds = iggy3d::aabbFromCenterExtents(center, halfExtents);
  collider.sensor = sensor;
  return collider;
}

iggy3d::PhysicsAabbCollisionBatchResult readyPhysicsCollisionBatch() {
  iggy3d::PhysicsAabbCollisionBatchResult batch;
  batch.ok = true;
  batch.status = iggy3d::PhysicsAabbCollisionBatchStatus::Batched;
  batch.reasonCode = "physics_aabb_collision_batch_batched";
  batch.colliders.push_back(physicsCollider({1}, {0.0F, 0.5F, 0.0F},
                                            {0.5F, 0.5F, 0.5F}, false));
  batch.colliders.push_back(physicsCollider({2}, {2.0F, 0.5F, 0.0F},
                                            {0.25F, 0.5F, 0.25F}, true));
  batch.colliders.push_back(physicsCollider({3}, {0.0F, 0.5F, 2.0F},
                                            {0.5F, 0.5F, 0.5F}, false));

  iggy3d::PhysicsBroadphasePair sensorPair;
  sensorPair.firstBodyId = {1};
  sensorPair.secondBodyId = {2};
  sensorPair.firstColliderIndex = 0U;
  sensorPair.secondColliderIndex = 1U;
  sensorPair.includesSensor = true;
  batch.broadphasePairs.push_back(sensorPair);

  iggy3d::PhysicsBroadphasePair solidPair;
  solidPair.firstBodyId = {1};
  solidPair.secondBodyId = {3};
  solidPair.firstColliderIndex = 0U;
  solidPair.secondColliderIndex = 2U;
  batch.broadphasePairs.push_back(solidPair);

  iggy3d::PhysicsAabbContact sensorContact;
  sensorContact.firstBodyId = {1};
  sensorContact.secondBodyId = {2};
  sensorContact.firstColliderIndex = 0U;
  sensorContact.secondColliderIndex = 1U;
  sensorContact.pointMeters = {1.0F, 0.5F, 0.0F};
  sensorContact.penetrationMeters = 0.125F;
  sensorContact.includesSensor = true;
  batch.contacts.push_back(sensorContact);

  iggy3d::PhysicsAabbContact solidContact;
  solidContact.firstBodyId = {1};
  solidContact.secondBodyId = {3};
  solidContact.firstColliderIndex = 0U;
  solidContact.secondColliderIndex = 2U;
  solidContact.pointMeters = {0.0F, 0.5F, 1.0F};
  solidContact.penetrationMeters = 0.25F;
  batch.contacts.push_back(solidContact);
  return batch;
}

bool firstRoomProjectionContainsInitialItems() {
  const iggy3d::Session session = makeSession();
  const iggy3d::SceneProjectionResult projection = iggy3d::buildSceneProjection(session.state());
  const iggy3d::SceneItem* player = findSceneItem(projection, "player");
  const iggy3d::SceneItem* key = findSceneItem(projection, "gold_key");
  const iggy3d::SceneItem* marker = findSceneItem(projection, "tactical_marker_alpha");
  const iggy3d::SceneItem* dummy = findSceneItem(projection, "training_dummy");

  return expect(projection.items.size() == 4U, "initial projection item count") &&
         expect(player != nullptr && player->kind == iggy3d::SceneItemKind::Player,
                "player projected") &&
         expect(player != nullptr && player->owningPlayerSlot == 0U, "player owning slot") &&
         expect(player != nullptr && player->modelRef == "bean_player",
                "player bean model ref") &&
         expect(dummy != nullptr && dummy->kind == iggy3d::SceneItemKind::Npc,
                "npc projected") &&
         expect(dummy != nullptr && dummy->modelRef == "bean_npc", "npc bean model ref") &&
         expect(key != nullptr && key->kind == iggy3d::SceneItemKind::Pickup, "key projected") &&
         expect(key != nullptr && key->active && key->visible && key->interactable,
                "key active interactable") &&
         expect(key != nullptr && key->itemId == "gold_key", "key item id") &&
         expect(marker != nullptr && marker->kind == iggy3d::SceneItemKind::TacticalMarker,
                "marker projected") &&
         expect(projection.sourceStateHash == session.state().currentStateHash,
                "projection state hash copied") &&
         expect(projection.sourceTick == session.state().clock.tickIndex, "projection tick copied") &&
         expect(projection.cameraMode == iggy3d::CameraMode::ThirdPerson, "projection camera");
}

bool activeRoomProjectionCarriesFloorAndWallMeshes() {
  const iggy3d::Session session = makeSession();
  const iggy3d::RoomAsset room = floorWallProjectionRoom();
  const iggy3d::SceneProjectionResult projection =
      iggy3d::buildSceneProjection(session.state(), &room);

  const iggy3d::SceneRoomMeshItem* floor = nullptr;
  const iggy3d::SceneRoomMeshItem* wall = nullptr;
  const iggy3d::SceneRoomMeshItem* prop = nullptr;
  const iggy3d::SceneRoomMeshItem* ledge = nullptr;
  const iggy3d::SceneRoomMeshItem* opening = nullptr;
  for (const iggy3d::SceneRoomMeshItem& mesh : projection.room.meshes) {
    if (mesh.id == "floor_r1_c1") {
      floor = &mesh;
    }
    if (mesh.id == "wall_r0_c1") {
      wall = &mesh;
    }
    if (mesh.id == "crate_r1_c2") {
      prop = &mesh;
    }
    if (mesh.id == "ledge_r1_c3") {
      ledge = &mesh;
    }
    if (mesh.id == "door_panel") {
      opening = &mesh;
    }
  }

  return expect(projection.room.loaded, "room projection loaded") &&
         expect(projection.room.assetId == "ascii_floor_wall_room", "room asset id") &&
         expect(projection.room.version == 7U, "room version") &&
         expect(projection.room.sourceToml == "inline_ascii_room", "room source") &&
         expect(projection.room.sourceSubset == "floor_wall_slice", "room subset") &&
         expect(projection.room.staticMeshCount == 5U, "source mesh count") &&
         expect(projection.room.materialCount == 4U, "projected material count") &&
         expect(projection.room.anchorCount == 2U, "anchor count") &&
         expect(projection.room.meshes.size() == 4U,
                "floor wall prop ledge mesh count") &&
         expect(projection.room.floorVisible, "floor visible") &&
         expect(projection.room.wallVisible, "wall visible") &&
         expect(!projection.room.openingVisible, "opening deferred") &&
         expect(projection.room.propVisible, "prop visible") &&
         expect(projection.room.keyAnchorVisible, "key anchor visible") &&
         expect(projection.room.dummyAnchorVisible, "dummy anchor visible") &&
         expect(floor != nullptr && floor->role == "floor", "floor projected") &&
         expect(floor != nullptr &&
                    iggy3d::nearlyEqual(floor->position, iggy3d::Vec3{1.0F, 0.0F, 2.0F}),
                "floor position") &&
         expect(floor != nullptr &&
                    iggy3d::nearlyEqual(floor->size, iggy3d::Vec3{1.0F, 0.1F, 1.0F}),
                "floor size") &&
         expect(wall != nullptr && wall->role == "wall", "wall projected") &&
         expect(wall != nullptr &&
                    iggy3d::nearlyEqual(wall->position, iggy3d::Vec3{1.0F, 0.5F, 0.0F}),
                "wall position") &&
         expect(wall != nullptr &&
                    iggy3d::nearlyEqual(wall->size, iggy3d::Vec3{1.0F, 1.0F, 0.1F}),
                "wall size") &&
         expect(prop != nullptr && prop->role == "prop", "prop projected") &&
         expect(prop != nullptr &&
                    iggy3d::nearlyEqual(prop->position,
                                        iggy3d::Vec3{2.0F, 0.4F, 2.0F}),
                "prop position") &&
         expect(prop != nullptr &&
                    iggy3d::nearlyEqual(prop->size, iggy3d::Vec3{0.8F, 0.8F, 0.8F}),
                "prop size") &&
         expect(ledge != nullptr && ledge->role == "ledge", "ledge projected") &&
         expect(ledge != nullptr &&
                    iggy3d::nearlyEqual(ledge->position,
                                        iggy3d::Vec3{3.0F, 0.85F, 2.0F}),
                "ledge position") &&
         expect(ledge != nullptr &&
                    iggy3d::nearlyEqual(ledge->size,
                                        iggy3d::Vec3{2.0F, 1.7F, 1.0F}),
                "ledge size") &&
         expect(opening == nullptr, "opening not projected yet");
}

bool inactivePickupFilteringWorks() {
  const iggy3d::Session session = makePickedUpSession();
  const iggy3d::SceneProjectionResult activeOnly = iggy3d::buildSceneProjection(session.state());
  iggy3d::SceneProjectionConfig includeInactive;
  includeInactive.includeInactive = true;
  const iggy3d::SceneProjectionResult allItems =
      iggy3d::buildSceneProjection(session.state(), includeInactive);
  const iggy3d::SceneItem* hiddenKey = findSceneItem(activeOnly, "gold_key");
  const iggy3d::SceneItem* inactiveKey = findSceneItem(allItems, "gold_key");

  return expect(hiddenKey == nullptr, "inactive key hidden") &&
         expect(inactiveKey != nullptr, "inactive key included") &&
         expect(inactiveKey != nullptr && !inactiveKey->active && !inactiveKey->visible,
                "inactive key marked inactive") &&
         expect(inactiveKey != nullptr && inactiveKey->itemId == "gold_key",
                "inactive key retains item fact");
}

bool projectionDoesNotMutateRuntimeTruth() {
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::StateHashValue hashBefore = session.state().currentStateHash;
  const std::size_t logSizeBefore = session.state().commandLog.size();
  const iggy3d::CommandSequence nextSequenceBefore = session.state().commandLog.nextSequence();
  const iggy3d::CommandTick tickBefore = session.state().clock.tickIndex;

  iggy3d::SceneProjectionConfig sceneConfig;
  sceneConfig.includeInactive = true;
  const iggy3d::SceneProjectionResult scene =
      iggy3d::buildSceneProjection(session.state(), sceneConfig);
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());

  return expect(!scene.items.empty(), "scene projected") &&
         expect(!debug.items.empty(), "debug projected") &&
         expect(session.state().currentStateHash == hashBefore, "projection hash unchanged") &&
         expect(session.state().commandLog.size() == logSizeBefore, "projection log size unchanged") &&
         expect(session.state().commandLog.nextSequence() == nextSequenceBefore,
                "projection log cursor unchanged") &&
         expect(session.state().clock.tickIndex == tickBefore, "projection tick unchanged");
}

bool debugProjectionIncludesProofFacts() {
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());

  bool hasReach = false;
  bool hasObjective = false;
  for (const iggy3d::DebugProjectionItem& item : debug.items) {
    if (item.kind == iggy3d::DebugProjectionKind::ReachRadius &&
        item.actor == iggy3d::EntityId{1} && item.radiusMeters == 1.500F) {
      hasReach = true;
    }
    if (item.kind == iggy3d::DebugProjectionKind::ObjectiveState &&
        item.objectiveId == "collect_gold_key") {
      hasObjective = true;
    }
  }

  return expect(hasOutOfRangeRejection(debug), "debug out of range rejection") &&
         expect(hasReach, "debug reach radius") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::TargetCandidate),
                "debug target candidate") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::ClockMode), "debug clock") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::CameraMode), "debug camera") &&
         expect(hasObjective, "debug objective") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::StateHash), "debug hash") &&
         expect(debug.sourceStateHash == session.state().currentStateHash, "debug source hash");
}

bool npcDebugProjectionAppendsItemsAndHudLines() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::appendNpcBehaviorDebugSnapshot(debug, npcDebugSnapshot());

  const iggy3d::DebugProjectionItem* hostile = findNpcProjectionItem(debug, {2});
  const iggy3d::DebugProjectionItem* passive = findNpcProjectionItem(debug, {3});
  const iggy3d::DebugProjectionItem* ghost = findNpcProjectionItem(debug, {4});

  return expect(debug.items.size() == 3U, "npc projection item count") &&
         expect(hostile != nullptr, "hostile item projected") &&
         expect(hostile != nullptr && hostile->sourceTick == 11, "hostile source tick") &&
         expect(hostile != nullptr && hostile->actor == iggy3d::EntityId{2},
                "hostile actor") &&
         expect(hostile != nullptr && hostile->target == iggy3d::EntityId{1},
                "hostile target") &&
         expect(hostile != nullptr && hostile->hasScalar, "hostile scalar") &&
         expect(hostile != nullptr && hostile->scalarValue == 1.25F,
                "hostile distance scalar") &&
         expect(hostile != nullptr && hostile->labelCode == "npc.behavior",
                "hostile label") &&
         expect(hostile != nullptr &&
                    hostile->valueCode == "default:attacking:attack_target:profile_resolved",
                "hostile value") &&
         expect(passive != nullptr &&
                    passive->valueCode == "passive:alert:wait:profile_resolved",
                "passive value") &&
         expect(ghost != nullptr && !ghost->hasScalar, "ghost no scalar") &&
         expect(ghost != nullptr &&
                    ghost->valueCode == "ghost_profile:idle:none:profile_missing",
                "ghost value") &&
         expect(debug.npcBehaviorDebugHudLines.size() == 4U, "npc hud line count") &&
         expect(debug.npcBehaviorDebugHudLines[0] ==
                    "NPCS world=3 ai=3 resolved=2 failed=1 hostile=1 passive=1",
                "npc hud summary") &&
         expect(debug.npcBehaviorDebugHudLines[1] ==
                    "NPC 2 training_dummy default attacking/attack_target tgt=player cd=2",
                "hostile hud row") &&
         expect(debug.npcBehaviorDebugHudLines[2] ==
                    "NPC 3 observer passive alert/wait tgt=player cd=0",
                "passive hud row") &&
         expect(debug.npcBehaviorDebugHudLines[3] ==
                    "NPC 4 ghost ghost_profile idle/none tgt=none cd=0 "
                    "unresolved=profile_missing",
                "ghost hud row");
}

bool npcDebugProjectionIgnoresDisabledSnapshots() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::NpcBehaviorDebugSnapshot snapshot;
  snapshot.status = iggy3d::NpcBehaviorDebugSnapshotStatus::Disabled;
  snapshot.reasonCode = "npc_behavior_debug_disabled";
  iggy3d::appendNpcBehaviorDebugSnapshot(debug, snapshot);

  return expect(debug.items.empty(), "disabled npc projection items empty") &&
         expect(debug.npcBehaviorDebugHudLines.empty(),
                "disabled npc projection hud empty");
}

bool npcDebugProjectionPreservesRuntimeHudLines() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::appendRuntimeDebugSnapshot(debug, okRuntimeDebugSnapshot());
  const std::vector<std::string> runtimeLinesBefore = debug.runtimeDebugHudLines;

  iggy3d::appendNpcBehaviorDebugSnapshot(debug, npcDebugSnapshot());

  return expect(!runtimeLinesBefore.empty(), "runtime hud lines present") &&
         expect(debug.runtimeDebugHudLines == runtimeLinesBefore,
                "runtime hud lines unchanged") &&
         expect(!debug.npcBehaviorDebugHudLines.empty(), "npc hud lines present");
}

bool physicsDebugProjectionAppendsReadyHudLines() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::appendPhysicsDebugSnapshot(debug, physicsDebugSnapshot());

  return expect(debug.physicsDebugHudLines.size() == 4U,
                "physics hud ready line count") &&
         expect(debug.physicsDebugHudLines[0] ==
                    "PHYS packets=2 failed=0 bodies=6 colliders=7 contacts=2 sensors=1",
                "physics summary line") &&
         expect(debug.physicsDebugHudLines[1] ==
                    "PHYS BP cells=3 entries=12 bucket=4 candidates=8 tested=5 dup=1 overlaps=2",
                "physics broadphase line") &&
         expect(debug.physicsDebugHudLines[2] ==
                    "PHYS SOLVE plans=2 pos=1 vel=2 fric=1 pen=0.125 ni=3.500/2.250 fi=1.000/0.750",
                "physics solver line") &&
         expect(debug.physicsDebugHudLines[3] ==
                    "PHYS MOVE kin_iter=3 kin_hits=1 player_iter=2 player_hits=1 baked=5 skipped=2",
                "physics movement line") &&
         expect(debug.runtimeDebugHudLines.empty(),
                "physics append leaves runtime empty") &&
         expect(debug.npcBehaviorDebugHudLines.empty(),
                "physics append leaves npc empty");
}

bool physicsDebugProjectionAppendsWarningLine() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::appendPhysicsDebugSnapshot(debug, physicsWarningSnapshot());

  return expect(debug.physicsDebugHudLines.size() == 5U,
                "physics warning line count") &&
         expect(debug.physicsDebugHudLines[4] ==
                    "PHYS WARN status=physics_debug_snapshot_stats_failed "
                    "upstream=physics_frame_stats_nonfinite_scalar bp=1 pen=1 impulse=1",
                "physics warning line");
}

bool physicsDebugProjectionIgnoresInactiveSnapshots() {
  iggy3d::DebugProjectionResult debug;

  iggy3d::PhysicsDebugSnapshot disabled;
  disabled.status = iggy3d::PhysicsDebugSnapshotStatus::Disabled;
  disabled.reasonCode = "physics_debug_snapshot_disabled";
  iggy3d::appendPhysicsDebugSnapshot(debug, disabled);

  iggy3d::PhysicsDebugSnapshot missing;
  missing.ok = false;
  missing.status = iggy3d::PhysicsDebugSnapshotStatus::MissingStats;
  missing.reasonCode = "physics_debug_snapshot_missing_stats";
  iggy3d::appendPhysicsDebugSnapshot(debug, missing);

  return expect(debug.physicsDebugHudLines.empty(),
                "inactive physics snapshots ignored") &&
         expect(debug.runtimeDebugHudLines.empty(),
                "inactive physics leaves runtime empty") &&
         expect(debug.npcBehaviorDebugHudLines.empty(),
                "inactive physics leaves npc empty");
}

bool physicsDebugProjectionPreservesRuntimeAndNpcHudLines() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::appendRuntimeDebugSnapshot(debug, okRuntimeDebugSnapshot());
  iggy3d::appendNpcBehaviorDebugSnapshot(debug, npcDebugSnapshot());
  const std::vector<std::string> runtimeLinesBefore = debug.runtimeDebugHudLines;
  const std::vector<std::string> npcLinesBefore = debug.npcBehaviorDebugHudLines;

  iggy3d::appendPhysicsDebugSnapshot(debug, physicsDebugSnapshot());

  return expect(!runtimeLinesBefore.empty(), "runtime hud line fixture") &&
         expect(!npcLinesBefore.empty(), "npc hud line fixture") &&
         expect(debug.runtimeDebugHudLines == runtimeLinesBefore,
                "physics preserves runtime hud lines") &&
         expect(debug.npcBehaviorDebugHudLines == npcLinesBefore,
                "physics preserves npc hud lines") &&
	         expect(debug.physicsDebugHudLines.size() == 4U,
	                "physics hud lines appended separately");
}

bool physicsCollisionBatchProjectionAppendsAabbItems() {
  iggy3d::DebugProjectionResult debug;
  const iggy3d::PhysicsAabbCollisionBatchResult batch =
      readyPhysicsCollisionBatch();
  iggy3d::appendPhysicsCollisionBatchDebugProjection(debug, batch);

  const iggy3d::DebugProjectionItem* solid =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsAabb, 0U);
  const iggy3d::DebugProjectionItem* sensor =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsAabb, 1U);

  return expect(countDebugKind(debug,
                               iggy3d::DebugProjectionKind::PhysicsAabb) == 3U,
                "physics aabb item count") &&
         expect(solid != nullptr && solid->actor == iggy3d::EntityId{1},
                "physics aabb actor") &&
         expect(solid != nullptr && solid->hasBounds, "physics aabb bounds") &&
         expect(solid != nullptr &&
                    nearlyEqualAabb(solid->worldBounds, batch.colliders[0].bounds),
                "physics aabb bounds copied") &&
         expect(solid != nullptr && solid->hasScalar &&
                    solid->scalarValue == 0.0F,
                "solid aabb scalar") &&
         expect(solid != nullptr && solid->labelCode == "physics.aabb",
                "solid aabb label") &&
         expect(solid != nullptr && solid->valueCode == "solid",
                "solid aabb value") &&
         expect(sensor != nullptr && sensor->actor == iggy3d::EntityId{2},
                "sensor aabb actor") &&
         expect(sensor != nullptr && sensor->hasScalar &&
                    sensor->scalarValue == 1.0F,
                "sensor aabb scalar") &&
         expect(sensor != nullptr && sensor->valueCode == "sensor",
                "sensor aabb value");
}

bool physicsCollisionBatchProjectionAppendsContactNormalItems() {
  iggy3d::DebugProjectionResult debug;
  const iggy3d::PhysicsAabbCollisionBatchResult batch =
      readyPhysicsCollisionBatch();
  iggy3d::appendPhysicsCollisionBatchDebugProjection(debug, batch);

  const iggy3d::DebugProjectionItem* sensor =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsContactNormal, 0U);
  const iggy3d::DebugProjectionItem* solid =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsContactNormal, 1U);

  return expect(countDebugKind(debug,
                               iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                    2U,
                "physics contact item count") &&
         expect(sensor != nullptr && sensor->actor == iggy3d::EntityId{1},
                "sensor contact actor") &&
         expect(sensor != nullptr && sensor->target == iggy3d::EntityId{2},
                "sensor contact target") &&
         expect(sensor != nullptr && sensor->hasWorldPoint &&
                    iggy3d::nearlyEqual(sensor->worldPoint,
                                        iggy3d::Vec3{1.0F, 0.5F, 0.0F}),
                "sensor contact point") &&
         expect(sensor != nullptr && sensor->hasScalar &&
                    sensor->scalarValue == 0.125F,
                "sensor contact penetration") &&
         expect(sensor != nullptr &&
                    sensor->labelCode == "physics.contact_normal",
                "sensor contact label") &&
         expect(sensor != nullptr && sensor->valueCode == "sensor",
                "sensor contact value") &&
         expect(solid != nullptr && solid->target == iggy3d::EntityId{3},
                "solid contact target") &&
         expect(solid != nullptr && solid->valueCode == "solid",
                "solid contact value");
}

bool physicsCollisionBatchProjectionAppendsBroadphasePairItems() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::PhysicsAabbCollisionBatchResult batch = readyPhysicsCollisionBatch();
  iggy3d::PhysicsBroadphasePair invalidIndexPair;
  invalidIndexPair.firstBodyId = {2};
  invalidIndexPair.secondBodyId = {3};
  invalidIndexPair.firstColliderIndex = 1U;
  invalidIndexPair.secondColliderIndex = 99U;
  batch.broadphasePairs.push_back(invalidIndexPair);
  iggy3d::appendPhysicsCollisionBatchDebugProjection(debug, batch);

  const iggy3d::DebugProjectionItem* sensor =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsBroadphasePair, 0U);
  const iggy3d::DebugProjectionItem* invalid =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsBroadphasePair, 2U);

  return expect(countDebugKind(debug,
                               iggy3d::DebugProjectionKind::PhysicsBroadphasePair) ==
                    3U,
                "physics broadphase pair item count") &&
         expect(sensor != nullptr && sensor->actor == iggy3d::EntityId{1},
                "broadphase pair actor") &&
         expect(sensor != nullptr && sensor->target == iggy3d::EntityId{2},
                "broadphase pair target") &&
         expect(sensor != nullptr && sensor->hasWorldPoint &&
                    iggy3d::nearlyEqual(sensor->worldPoint,
                                        iggy3d::Vec3{1.0F, 0.5F, 0.0F}),
                "broadphase pair midpoint") &&
         expect(sensor != nullptr && sensor->hasScalar &&
                    sensor->scalarValue == 1.0F,
                "broadphase pair sensor scalar") &&
         expect(sensor != nullptr &&
                    sensor->labelCode == "physics.broadphase_pair",
                "broadphase pair label") &&
         expect(sensor != nullptr && sensor->valueCode == "sensor",
                "broadphase pair value") &&
         expect(invalid != nullptr && !invalid->hasWorldPoint,
                "invalid pair indices omit midpoint") &&
         expect(invalid != nullptr && invalid->valueCode == "solid",
                "invalid pair value");
}

bool physicsCollisionBatchProjectionIgnoresFailedBatch() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::PhysicsAabbCollisionBatchResult batch = readyPhysicsCollisionBatch();
  batch.ok = false;
  iggy3d::appendPhysicsCollisionBatchDebugProjection(debug, batch);

  return expect(debug.items.empty(), "failed batch appends no geometry");
}

bool physicsCollisionBatchProjectionRespectsIncludeFlagsAndCaps() {
  const iggy3d::PhysicsAabbCollisionBatchResult batch =
      readyPhysicsCollisionBatch();

  iggy3d::PhysicsDebugGeometryProjectionConfig noAabbs;
  noAabbs.includeAabbs = false;
  iggy3d::DebugProjectionResult withoutAabbs;
  iggy3d::appendPhysicsCollisionBatchDebugProjection(withoutAabbs, batch, noAabbs);

  iggy3d::PhysicsDebugGeometryProjectionConfig noContacts;
  noContacts.includeContacts = false;
  iggy3d::DebugProjectionResult withoutContacts;
  iggy3d::appendPhysicsCollisionBatchDebugProjection(withoutContacts, batch,
                                                     noContacts);

  iggy3d::PhysicsDebugGeometryProjectionConfig noPairs;
  noPairs.includeBroadphasePairs = false;
  iggy3d::DebugProjectionResult withoutPairs;
  iggy3d::appendPhysicsCollisionBatchDebugProjection(withoutPairs, batch, noPairs);

  iggy3d::PhysicsDebugGeometryProjectionConfig capped;
  capped.maxAabbs = 1U;
  capped.maxContacts = 1U;
  capped.maxPairs = 1U;
  iggy3d::DebugProjectionResult cappedResult;
  iggy3d::appendPhysicsCollisionBatchDebugProjection(cappedResult, batch, capped);

  const iggy3d::DebugProjectionItem* cappedAabb =
      nthDebugKind(cappedResult, iggy3d::DebugProjectionKind::PhysicsAabb, 0U);
  const iggy3d::DebugProjectionItem* cappedContact = nthDebugKind(
      cappedResult, iggy3d::DebugProjectionKind::PhysicsContactNormal, 0U);
  const iggy3d::DebugProjectionItem* cappedPair = nthDebugKind(
      cappedResult, iggy3d::DebugProjectionKind::PhysicsBroadphasePair, 0U);

  return expect(countDebugKind(withoutAabbs,
                               iggy3d::DebugProjectionKind::PhysicsAabb) == 0U,
                "include flag disables aabbs") &&
         expect(countDebugKind(withoutContacts,
                               iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                    0U,
                "include flag disables contacts") &&
         expect(countDebugKind(withoutPairs,
                               iggy3d::DebugProjectionKind::PhysicsBroadphasePair) ==
                    0U,
                "include flag disables pairs") &&
         expect(countDebugKind(cappedResult,
                               iggy3d::DebugProjectionKind::PhysicsAabb) == 1U,
                "aabb cap") &&
         expect(countDebugKind(cappedResult,
                               iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                    1U,
                "contact cap") &&
         expect(countDebugKind(cappedResult,
                               iggy3d::DebugProjectionKind::PhysicsBroadphasePair) ==
                    1U,
                "pair cap") &&
         expect(cappedAabb != nullptr &&
                    cappedAabb->actor == iggy3d::EntityId{1},
                "aabb cap keeps first") &&
         expect(cappedContact != nullptr &&
                    cappedContact->target == iggy3d::EntityId{2},
                "contact cap keeps first") &&
         expect(cappedPair != nullptr &&
                    cappedPair->target == iggy3d::EntityId{2},
                "pair cap keeps first");
}

bool physicsCollisionBatchProjectionPreservesHudLinesAndExistingItems() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::DebugProjectionItem existing;
  existing.kind = iggy3d::DebugProjectionKind::StateHash;
  existing.labelCode = "existing";
  debug.items.push_back(existing);
  debug.runtimeDebugHudLines.push_back("runtime line");
  debug.npcBehaviorDebugHudLines.push_back("npc line");
  debug.physicsDebugHudLines.push_back("physics line");
  const std::vector<std::string> runtimeBefore = debug.runtimeDebugHudLines;
  const std::vector<std::string> npcBefore = debug.npcBehaviorDebugHudLines;
  const std::vector<std::string> physicsBefore = debug.physicsDebugHudLines;

  iggy3d::appendPhysicsCollisionBatchDebugProjection(debug,
                                                     readyPhysicsCollisionBatch());

  return expect(debug.items.front().kind == iggy3d::DebugProjectionKind::StateHash,
                "existing item order preserved") &&
         expect(debug.items.front().labelCode == "existing",
                "existing item payload preserved") &&
         expect(debug.items.size() == 8U, "geometry appended after existing") &&
         expect(debug.runtimeDebugHudLines == runtimeBefore,
                "geometry preserves runtime hud lines") &&
         expect(debug.npcBehaviorDebugHudLines == npcBefore,
                "geometry preserves npc hud lines") &&
         expect(debug.physicsDebugHudLines == physicsBefore,
                "geometry preserves physics hud lines");
}

bool playerPhysicsMovePlannerProjectionAppendsCollidersAndHits() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      physicsCollider({8}, {1.0F, 0.5F, 0.0F}, {0.5F, 0.5F, 0.5F}, false),
      physicsCollider({9}, {2.0F, 0.5F, 0.0F}, {0.25F, 0.5F, 0.25F}, true),
  };
  std::vector<iggy3d::PhysicsKinematicMotorHit> hits;
  iggy3d::PhysicsKinematicMotorHit hit;
  hit.colliderIndex = 1U;
  hit.bodyId = {9};
  hit.centerMeters = {1.65F, 0.9F, 0.0F};
  hit.sensor = true;
  hits.push_back(hit);

  iggy3d::DebugProjectionResult debug;
  debug.runtimeDebugHudLines.push_back("runtime line");
  debug.physicsDebugHudLines.push_back("physics line");
  const std::vector<std::string> runtimeBefore = debug.runtimeDebugHudLines;
  const std::vector<std::string> physicsBefore = debug.physicsDebugHudLines;

  iggy3d::appendPlayerPhysicsMovePlannerDebugProjection(
      debug,
      iggy3d::PlayerPhysicsMovePlannerDebugProjectionRequest{&colliders,
                                                             &hits});

  const iggy3d::DebugProjectionItem* solidAabb =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsAabb, 0U);
  const iggy3d::DebugProjectionItem* sensorAabb =
      nthDebugKind(debug, iggy3d::DebugProjectionKind::PhysicsAabb, 1U);
  const iggy3d::DebugProjectionItem* movementHit = nthDebugKind(
      debug, iggy3d::DebugProjectionKind::PhysicsContactNormal, 0U);

  return expect(countDebugKind(debug,
                               iggy3d::DebugProjectionKind::PhysicsAabb) == 2U,
                "player planner aabb item count") &&
         expect(countDebugKind(debug,
                               iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                    1U,
                "player planner hit item count") &&
         expect(solidAabb != nullptr && solidAabb->actor == iggy3d::EntityId{8},
                "player planner solid actor") &&
         expect(solidAabb != nullptr && solidAabb->hasBounds &&
                    nearlyEqualAabb(solidAabb->worldBounds,
                                    colliders[0].bounds),
                "player planner solid bounds") &&
         expect(solidAabb != nullptr &&
                    solidAabb->labelCode == "physics.player_move_aabb",
                "player planner aabb label") &&
         expect(solidAabb != nullptr && solidAabb->valueCode == "solid",
                "player planner solid value") &&
         expect(sensorAabb != nullptr && sensorAabb->valueCode == "sensor",
                "player planner sensor value") &&
         expect(movementHit != nullptr &&
                    movementHit->actor == iggy3d::EntityId{9},
                "player planner hit actor") &&
         expect(movementHit != nullptr && movementHit->hasWorldPoint &&
                    iggy3d::nearlyEqual(movementHit->worldPoint,
                                        iggy3d::Vec3{1.65F, 0.9F, 0.0F}),
                "player planner hit point") &&
         expect(movementHit != nullptr &&
                    movementHit->labelCode == "physics.player_move_hit",
                "player planner hit label") &&
         expect(movementHit != nullptr && movementHit->hasScalar &&
                    movementHit->scalarValue == 1.0F,
                "player planner hit sensor scalar") &&
         expect(debug.runtimeDebugHudLines == runtimeBefore,
                "player planner geometry preserves runtime lines") &&
         expect(debug.physicsDebugHudLines == physicsBefore,
                "player planner geometry preserves physics lines");
}

bool playerPhysicsMovePlannerProjectionRespectsCapsAndFlags() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      physicsCollider({8}, {1.0F, 0.5F, 0.0F}, {0.5F, 0.5F, 0.5F}, false),
      physicsCollider({9}, {2.0F, 0.5F, 0.0F}, {0.25F, 0.5F, 0.25F}, false),
  };
  std::vector<iggy3d::PhysicsKinematicMotorHit> hits;
  iggy3d::PhysicsKinematicMotorHit firstHit;
  firstHit.bodyId = {9};
  firstHit.centerMeters = {1.5F, 0.9F, 0.0F};
  hits.push_back(firstHit);
  iggy3d::PhysicsKinematicMotorHit secondHit = firstHit;
  secondHit.bodyId = {10};
  secondHit.centerMeters = {2.5F, 0.9F, 0.0F};
  hits.push_back(secondHit);

  iggy3d::PhysicsDebugGeometryProjectionConfig capped;
  capped.maxAabbs = 1U;
  capped.maxContacts = 1U;
  capped.includeBroadphasePairs = false;
  iggy3d::DebugProjectionResult cappedResult;
  iggy3d::appendPlayerPhysicsMovePlannerDebugProjection(
      cappedResult,
      iggy3d::PlayerPhysicsMovePlannerDebugProjectionRequest{&colliders,
                                                             &hits},
      capped);

  iggy3d::PhysicsDebugGeometryProjectionConfig noAabbs;
  noAabbs.includeAabbs = false;
  iggy3d::DebugProjectionResult withoutAabbs;
  iggy3d::appendPlayerPhysicsMovePlannerDebugProjection(
      withoutAabbs,
      iggy3d::PlayerPhysicsMovePlannerDebugProjectionRequest{&colliders,
                                                             &hits},
      noAabbs);

  return expect(countDebugKind(cappedResult,
                               iggy3d::DebugProjectionKind::PhysicsAabb) == 1U,
                "player planner aabb cap") &&
         expect(countDebugKind(cappedResult,
                               iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                    1U,
                "player planner hit cap") &&
         expect(countDebugKind(withoutAabbs,
                               iggy3d::DebugProjectionKind::PhysicsAabb) == 0U,
                "player planner include flag disables aabbs") &&
         expect(countDebugKind(withoutAabbs,
                               iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                    2U,
                "player planner include flag keeps hits");
}

bool saveLoadProjectionIsEquivalent() {
  const iggy3d::SessionCreateRequest create = createRequestFromPackage();
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  iggy3d::Session loaded = iggy3d::Session::create(create).value;
  const iggy3d::SaveCompatibilityRequest compatibility{
      saved.envelope, session.state().identity.packageId, session.state().identity.scenarioId};
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibility);

  iggy3d::SceneProjectionConfig config;
  config.includeInactive = true;
  const iggy3d::SceneProjectionResult original =
      iggy3d::buildSceneProjection(session.state(), config);
  const iggy3d::SceneProjectionResult restored =
      iggy3d::buildSceneProjection(loaded.state(), config);
  const iggy3d::SceneItem* originalPlayer = findSceneItem(original, "player");
  const iggy3d::SceneItem* restoredPlayer = findSceneItem(restored, "player");
  const iggy3d::SceneItem* originalKey = findSceneItem(original, "gold_key");
  const iggy3d::SceneItem* restoredKey = findSceneItem(restored, "gold_key");

  return expect(saved.status == iggy3d::SaveLoadStatus::Ok, "save ok") &&
         expect(load.status == iggy3d::SaveLoadStatus::Ok, "load ok") &&
         expect(original.items.size() == restored.items.size(), "projection item count roundtrip") &&
         expect(original.sourceStateHash == restored.sourceStateHash, "projection hash roundtrip") &&
         expect(original.sourceTick == restored.sourceTick, "projection tick roundtrip") &&
         expect(originalPlayer != nullptr && restoredPlayer != nullptr, "player roundtrip exists") &&
         expect(originalPlayer != nullptr && restoredPlayer != nullptr &&
                    iggy3d::nearlyEqual(originalPlayer->transform.position,
                                        restoredPlayer->transform.position),
                "player position roundtrip") &&
         expect(originalKey != nullptr && restoredKey != nullptr, "key roundtrip exists") &&
         expect(originalKey != nullptr && restoredKey != nullptr &&
                    originalKey->active == restoredKey->active &&
                    originalKey->itemId == restoredKey->itemId,
                "key inactive fact roundtrip");
}

}  // namespace

int main() {
  bool ok = true;
  ok = firstRoomProjectionContainsInitialItems() && ok;
  ok = activeRoomProjectionCarriesFloorAndWallMeshes() && ok;
  ok = inactivePickupFilteringWorks() && ok;
  ok = projectionDoesNotMutateRuntimeTruth() && ok;
  ok = debugProjectionIncludesProofFacts() && ok;
  ok = npcDebugProjectionAppendsItemsAndHudLines() && ok;
  ok = npcDebugProjectionIgnoresDisabledSnapshots() && ok;
  ok = npcDebugProjectionPreservesRuntimeHudLines() && ok;
  ok = physicsDebugProjectionAppendsReadyHudLines() && ok;
  ok = physicsDebugProjectionAppendsWarningLine() && ok;
  ok = physicsDebugProjectionIgnoresInactiveSnapshots() && ok;
  ok = physicsDebugProjectionPreservesRuntimeAndNpcHudLines() && ok;
  ok = physicsCollisionBatchProjectionAppendsAabbItems() && ok;
  ok = physicsCollisionBatchProjectionAppendsContactNormalItems() && ok;
  ok = physicsCollisionBatchProjectionAppendsBroadphasePairItems() && ok;
  ok = physicsCollisionBatchProjectionIgnoresFailedBatch() && ok;
  ok = physicsCollisionBatchProjectionRespectsIncludeFlagsAndCaps() && ok;
  ok = physicsCollisionBatchProjectionPreservesHudLinesAndExistingItems() && ok;
  ok = playerPhysicsMovePlannerProjectionAppendsCollidersAndHits() && ok;
  ok = playerPhysicsMovePlannerProjectionRespectsCapsAndFlags() && ok;
  ok = saveLoadProjectionIsEquivalent() && ok;
  return ok ? 0 : 1;
}
