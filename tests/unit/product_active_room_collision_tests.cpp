#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ascii_room/Editing.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputAction.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/EntityState.hpp"
#include "runtime/session/SessionState.hpp"

#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr std::string_view kTrainingRoom =
    "#######\n"
    "#P..N.#\n"
    "#.+.$.#\n"
    "#..E..#\n"
    "#######\n";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool collisionProvenanceDefaultsToZero() {
  const iggy3d::ProductActiveRoomCollisionState collision;
  return expect(collision.bakedFromRoomRevision == 0U,
                "default baked room revision") &&
         expect(collision.bakedFromSessionHash == 0U,
                "default baked session hash");
}

iggy3d::RoomSpatialSurface walkableSurface(std::string id) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = surface.id + "_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {{0.0F, 0.0F, 0.0F},
                          {1.0F, 0.0F, 0.0F},
                          {1.0F, 0.0F, 1.0F},
                          {0.0F, 0.0F, 1.0F}};
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  return surface;
}

iggy3d::RoomSpatialSurface blockerSurface(std::string id,
                                          std::string runtimeOwnerStableName = {}) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = surface.id + "_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {{0.0F, 0.0F, 0.0F}, {1.0F, 2.0F, 1.0F}};
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker"};
  surface.blocksActor = true;
  surface.runtimeOwnerStableName = std::move(runtimeOwnerStableName);
  return surface;
}

iggy3d::ProductActiveRoomState packageActiveRoom(
    std::string id,
    std::initializer_list<iggy3d::RoomSpatialSurface> surfaces) {
  iggy3d::RoomAsset room;
  room.id = std::move(id);
  room.units = "m";
  room.source = "unit";
  room.sourceFile = "unit/freshness_room";
  room.sourceSubset = "collision_freshness";
  room.spatialSurfaces.assign(surfaces.begin(), surfaces.end());
  return iggy3d::buildProductActiveRoomFromPackageRoom(room, "unit", "freshness");
}

iggy3d::Session doorSession(bool doorActive) {
  iggy3d::SessionState state;
  state.lifecycle = iggy3d::SessionLifecycle::Playing;
  state.currentStateHash = 11U;
  iggy3d::EntityState door;
  door.id = iggy3d::EntityId{2};
  door.stableName = "freshness_door";
  door.kind = iggy3d::EntityKind::Door;
  door.active = doorActive;
  (void)state.world.seedEntity(std::move(door));
  return iggy3d::Session(std::move(state));
}

iggy3d::ActionState interactActions() {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::PlayerInteract,
                       true,
                       true,
                       false,
                       1.0F);
  return actions;
}

iggy3d::EntityState freshnessPlayer() {
  iggy3d::EntityState player;
  player.id = iggy3d::EntityId{1};
  player.stableName = "freshness_player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform.position = {0.0F, 0.0F, 0.0F};
  player.localBounds = iggy3d::makeAabb3({-0.20F, 0.0F, -0.20F},
                                         {0.20F, 1.80F, 0.20F});
  return player;
}

iggy3d::EntityState freshnessDoor(iggy3d::EntityId id,
                                  std::string stableName,
                                  iggy3d::Vec3 position) {
  iggy3d::EntityState door;
  door.id = id;
  door.stableName = std::move(stableName);
  door.kind = iggy3d::EntityKind::Door;
  door.transform.position = position;
  door.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                       {0.25F, 1.80F, 0.25F});
  door.targeting.targetable = true;
  door.targeting.actions = {iggy3d::TargetAction::Interact,
                            iggy3d::TargetAction::Inspect};
  door.interaction.kind = iggy3d::InteractionKind::OpenDoor;
  door.interaction.primaryEffect = iggy3d::InteractionEffectKind::EmitEventOnly;
  door.interaction.deactivateTargetOnSuccess = true;
  return door;
}

iggy3d::Session twoDoorFreshnessSession() {
  iggy3d::SessionState state;
  state.lifecycle = iggy3d::SessionLifecycle::Playing;
  (void)state.world.seedEntity(freshnessPlayer());
  (void)state.world.seedEntity(
      freshnessDoor(iggy3d::EntityId{2},
                    "freshness_door_a",
                    {1.40F, 0.0F, 0.0F}));
  (void)state.world.seedEntity(
      freshnessDoor(iggy3d::EntityId{3},
                    "freshness_door_b",
                    {0.70F, 0.0F, 0.0F}));
  iggy3d::PlayerSlot slot;
  slot.id = 0;
  slot.kind = iggy3d::PlayerSlotKind::Local;
  slot.actor = iggy3d::EntityId{1};
  slot.stableName = "freshness_player_slot";
  (void)state.players.addSlot(std::move(slot));
  state.currentStateHash = iggy3d::computeStateHash(state);
  return iggy3d::Session(std::move(state));
}

iggy3d::ProductAppWindowState twoDoorFreshnessWindow(
    const iggy3d::Session& session) {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom =
      packageActiveRoom("freshness_two_door_room",
                        {walkableSurface("floor"),
                         blockerSurface("door_a_blocker", "freshness_door_a"),
                         blockerSurface("door_b_blocker", "freshness_door_b")});
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom, session.state());
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash =
      session.state().currentStateHash;
  return window;
}

void deactivateDoorThroughHashedTick(iggy3d::Session& session,
                                     iggy3d::EntityId doorId) {
  (void)session.mutableStateForOwnedSystems().world.setActive(doorId, false);
  (void)session.tick();
}

void interactNearestDoor(iggy3d::Session& session,
                         iggy3d::ProductAppWindowState& window) {
  iggy3d::applyProductGameplayActions(session,
                                      interactActions(),
                                      window,
                                      "unit/freshness_order",
                                      iggy3d::productActiveRoomCollisionSurfaces(
                                          window.activeRoomCollision));
}

struct CollisionReaderSnapshot {
  std::uint64_t querySurfaceCount = 0;
  std::uint64_t activeDoorBlockerSurfaceCount = 0;
  std::uint64_t runtimeFilteredSurfaceCount = 0;
  std::size_t surfaceSetSize = 0;
  std::vector<std::string> surfaceIds;
  std::uint64_t roomRevision = 0;
  std::uint64_t sessionHash = 0;
};

CollisionReaderSnapshot readerSnapshot(
    const iggy3d::ProductAppWindowState& window) {
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(window.activeRoomCollision);
  CollisionReaderSnapshot snapshot;
  snapshot.querySurfaceCount = window.activeRoomCollision.querySurfaceCount;
  snapshot.activeDoorBlockerSurfaceCount =
      window.activeRoomCollision.activeDoorBlockerSurfaceCount;
  snapshot.runtimeFilteredSurfaceCount =
      window.activeRoomCollision.runtimeFilteredSurfaceCount;
  snapshot.surfaceSetSize = surfaces == nullptr ? 0U : surfaces->size();
  if (surfaces != nullptr) {
    for (const iggy3d::CollisionSurfaceView& surface : surfaces->surfaces()) {
      snapshot.surfaceIds.push_back(surface.id);
    }
  }
  snapshot.roomRevision = window.activeRoomCollision.bakedFromRoomRevision;
  snapshot.sessionHash = window.activeRoomCollision.bakedFromSessionHash;
  return snapshot;
}

bool snapshotsMatch(const CollisionReaderSnapshot& lhs,
                    const CollisionReaderSnapshot& rhs,
                    std::string_view label) {
  return expect(lhs.querySurfaceCount == rhs.querySurfaceCount,
                std::string(label) + " query count") &&
         expect(lhs.activeDoorBlockerSurfaceCount ==
                    rhs.activeDoorBlockerSurfaceCount,
                std::string(label) + " active door count") &&
         expect(lhs.runtimeFilteredSurfaceCount ==
                    rhs.runtimeFilteredSurfaceCount,
                std::string(label) + " filtered count") &&
         expect(lhs.surfaceSetSize == rhs.surfaceSetSize,
                std::string(label) + " surface set size") &&
         expect(lhs.surfaceIds == rhs.surfaceIds,
                std::string(label) + " surface ids") &&
         expect(lhs.roomRevision == rhs.roomRevision,
                std::string(label) + " room revision stamp");
}

struct OrderRunResult {
  bool ok = false;
  CollisionReaderSnapshot snapshot;
};

OrderRunResult runDoorFreshnessOrder(bool deactivateBeforeInteract) {
  iggy3d::Session session = twoDoorFreshnessSession();
  iggy3d::ProductAppWindowState window = twoDoorFreshnessWindow(session);
  bool ok = expect(window.activeRoomCollision.querySurfaceCount == 3U,
                   "order baseline query count") &&
            expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount == 2U,
                   "order baseline active door count");

  if (deactivateBeforeInteract) {
    deactivateDoorThroughHashedTick(session, iggy3d::EntityId{2});
    interactNearestDoor(session, window);
  } else {
    interactNearestDoor(session, window);
    deactivateDoorThroughHashedTick(session, iggy3d::EntityId{2});
  }

  const iggy3d::ProductActiveRoomCollisionFreshnessResult ensure =
      iggy3d::ensureActiveRoomCollisionFresh(window, &session);
  const iggy3d::EntityState* doorA =
      session.state().world.findByStableName("freshness_door_a");
  const iggy3d::EntityState* doorB =
      session.state().world.findByStableName("freshness_door_b");
  ok = ok && expect(ensure.rebaked, "order boundary ensure rebaked") &&
       expect(ensure.observedRoomRevision == window.activeRoomRevision,
              "order boundary observed room revision") &&
       expect(ensure.observedSessionHash == session.state().currentStateHash,
              "order boundary observed session hash") &&
       expect(doorA != nullptr && !doorA->active, "order door a inactive") &&
       expect(doorB != nullptr && !doorB->active, "order door b inactive") &&
       expect(window.activeRoomCollision.querySurfaceCount == 1U,
              "order final query count") &&
       expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount == 0U,
              "order final active door count") &&
       expect(window.activeRoomCollision.runtimeFilteredSurfaceCount == 2U,
              "order final filtered count");
  return {ok, readerSnapshot(window)};
}

bool idempotentAfterRebake(iggy3d::ProductAppWindowState& window,
                           const iggy3d::Session* session,
                           std::string_view message) {
  const iggy3d::ProductActiveRoomCollisionState before =
      window.activeRoomCollision;
  const iggy3d::ProductActiveRoomCollisionFreshnessResult second =
      iggy3d::ensureActiveRoomCollisionFresh(window, session);
  return expect(!second.rebaked, std::string(message) + " idempotent skip") &&
         expect(second.reasonCode == "skipped_fresh",
                std::string(message) + " idempotent reason") &&
         expect(window.activeRoomCollision.ready == before.ready,
                std::string(message) + " ready unchanged") &&
         expect(window.activeRoomCollision.status == before.status,
                std::string(message) + " status unchanged") &&
         expect(window.activeRoomCollision.reasonCode == before.reasonCode,
                std::string(message) + " collision reason unchanged") &&
         expect(window.activeRoomCollision.roomId == before.roomId,
                std::string(message) + " room id unchanged") &&
         expect(window.activeRoomCollision.querySurfaceCount ==
                    before.querySurfaceCount,
                std::string(message) + " query count unchanged") &&
         expect(window.activeRoomCollision.runtimeFilteredSurfaceCount ==
                    before.runtimeFilteredSurfaceCount,
                std::string(message) + " filtered count unchanged") &&
         expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount ==
                    before.activeDoorBlockerSurfaceCount,
                std::string(message) + " active door count unchanged") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    before.bakedFromRoomRevision,
                std::string(message) + " room stamp unchanged") &&
         expect(window.activeRoomCollision.bakedFromSessionHash ==
                    before.bakedFromSessionHash,
                std::string(message) + " session stamp unchanged");
}

bool freshnessStoreRebakesWhenRoomReplaced() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom =
      packageActiveRoom("floor_and_crate",
                        {walkableSurface("floor"),
                         blockerSurface("crate_actor"),
                         blockerSurface("crate_projectile")});
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom);
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash = 0U;

  const bool baseline =
      expect(window.activeRoomCollision.querySurfaceCount == 3U,
             "room stale baseline query count") &&
      expect(window.activeRoomCollision.bakedFromRoomRevision ==
                 window.activeRoomRevision,
             "room stale baseline provenance");
  window.activeRoom = packageActiveRoom("floor_only", {walkableSurface("floor")});
  iggy3d::bumpActiveRoomRevision(window);

  const bool mismatch =
      expect(window.activeRoom.spatialSurfaceCount == 1U,
             "room stale new authored count") &&
      expect(window.activeRoomCollision.querySurfaceCount == 3U,
             "room stale old query count remains before ensure") &&
      expect(window.activeRoomCollision.bakedFromRoomRevision !=
                 window.activeRoomRevision,
             "room stale provenance mismatch before ensure");

  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(window.activeRoomCollision);

  return baseline && mismatch &&
         expect(result.rebaked, "room stale rebaked") &&
         expect(result.reasonCode == "rebaked_room", "room stale reason") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "room stale query count refreshed") &&
         expect(window.activeRoomCollision.roomId == "floor_only",
                "room stale room id refreshed") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    window.activeRoomRevision,
                "room stale room revision stamped") &&
         expect(window.activeRoomCollision.bakedFromSessionHash == 0U,
                "room stale null session hash stamped") &&
         expect(surfaces != nullptr && surfaces->size() == 1U,
                "room stale reader surface count") &&
         idempotentAfterRebake(window, nullptr, "room stale");
}

bool freshnessStoreRebakesWhenSessionHashChanges() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom =
      packageActiveRoom("door_room",
                        {walkableSurface("floor"),
                         blockerSurface("door_blocker", "freshness_door")});
  iggy3d::Session session = doorSession(true);
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom, session.state());
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash =
      session.state().currentStateHash;

  const std::uint64_t baselineHash = session.state().currentStateHash;
  const bool baseline =
      expect(window.activeRoomCollision.runtimeOwnedSurfaceCount == 1U,
             "session stale baseline runtime owned") &&
      expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount == 1U,
             "session stale baseline active door") &&
      expect(window.activeRoomCollision.runtimeFilteredSurfaceCount == 0U,
             "session stale baseline no filter") &&
      expect(window.activeRoomCollision.querySurfaceCount == 2U,
             "session stale baseline query count");

  const iggy3d::WorldMutationResult mutation =
      session.mutableStateForOwnedSystems().world.setActive({2}, false);
  const iggy3d::StatusResult ticked = session.tick();
  const bool changed =
      expect(mutation.status == iggy3d::WorldStatus::Ok,
             "session stale door mutation") &&
      expect(ticked.status == iggy3d::ResultStatus::Ok,
             "session stale non-interact tick") &&
      expect(session.state().currentStateHash != baselineHash,
             "session stale hash changed");
  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, &session);

  return baseline && changed &&
         expect(result.rebaked, "session stale rebaked") &&
         expect(result.reasonCode == "rebaked_session", "session stale reason") &&
         expect(result.observedSessionHash == session.state().currentStateHash,
                "session stale observed hash") &&
         expect(window.activeRoomCollision.runtimeOwnedSurfaceCount == 1U,
                "session stale two-arg overload used") &&
         expect(window.activeRoomCollision.runtimeFilteredSurfaceCount == 1U,
                "session stale runtime filtered") &&
         expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount == 0U,
                "session stale active door cleared") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "session stale query count drops") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    window.activeRoomRevision,
                "session stale room stamp") &&
         expect(window.activeRoomCollision.bakedFromSessionHash ==
                    session.state().currentStateHash,
                "session stale session stamp") &&
         idempotentAfterRebake(window, &session, "session stale");
}

bool freshnessStoreRebakesUnloadedRoom() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom = packageActiveRoom("loaded_room", {walkableSurface("floor")});
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom);
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash = 0U;

  const bool baseline =
      expect(window.activeRoom.loaded, "unloaded stale baseline loaded") &&
      expect(window.activeRoomCollision.ready, "unloaded stale baseline ready");
  window.activeRoom = {};
  iggy3d::bumpActiveRoomRevision(window);
  const bool stale =
      expect(!window.activeRoom.loaded, "unloaded stale room cleared") &&
      expect(window.activeRoomCollision.ready,
             "unloaded stale collision still ready before ensure");

  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);

  return baseline && stale &&
         expect(result.rebaked, "unloaded stale rebaked") &&
         expect(result.reasonCode == "rebaked_unloaded",
                "unloaded stale reason") &&
         expect(!window.activeRoomCollision.ready,
                "unloaded stale collision unavailable") &&
         expect(window.activeRoomCollision.status ==
                    "active_room_collision_unavailable",
                "unloaded stale status") &&
         expect(window.activeRoomCollision.reasonCode == "not_loaded",
                "unloaded stale collision reason from active room") &&
         expect(window.activeRoomCollision.querySurfaceCount == 0U,
                "unloaded stale query count") &&
         expect(iggy3d::productActiveRoomCollisionSurfaces(
                    window.activeRoomCollision) == nullptr,
                "unloaded stale no surface pointer") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    window.activeRoomRevision,
                "unloaded stale room stamp") &&
         expect(window.activeRoomCollision.bakedFromSessionHash == 0U,
                "unloaded stale session stamp") &&
         idempotentAfterRebake(window, nullptr, "unloaded stale");
}

bool freshnessStoreSkipsFreshCollision() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 5U;
  window.activeRoom = packageActiveRoom("fresh_room", {walkableSurface("floor")});
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom);
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash = 0U;

  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);
  return expect(!result.rebaked, "fresh skip no rebake") &&
         expect(result.reasonCode == "skipped_fresh", "fresh skip reason") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "fresh skip query count unchanged") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision == 5U,
                "fresh skip room stamp unchanged") &&
         expect(window.activeRoomCollision.bakedFromSessionHash == 0U,
                "fresh skip session stamp unchanged");
}

bool frameBoundaryEnsureMakesDoorCollisionOrderIndependent() {
  const OrderRunResult nonInteractThenInteract = runDoorFreshnessOrder(true);
  const OrderRunResult interactThenNonInteract = runDoorFreshnessOrder(false);
  return nonInteractThenInteract.ok && interactThenNonInteract.ok &&
         snapshotsMatch(nonInteractThenInteract.snapshot,
                        interactThenNonInteract.snapshot,
                        "order independent") &&
         expect(nonInteractThenInteract.snapshot.querySurfaceCount == 1U,
                "order independent floor remains") &&
         expect(nonInteractThenInteract.snapshot.activeDoorBlockerSurfaceCount == 0U,
                "order independent all doors filtered") &&
         expect(nonInteractThenInteract.snapshot.runtimeFilteredSurfaceCount == 2U,
                "order independent both door blockers filtered");
}

CollisionReaderSnapshot runDoorTogglePermutation(
    const std::vector<iggy3d::EntityId>& order) {
  iggy3d::Session session = twoDoorFreshnessSession();
  iggy3d::ProductAppWindowState window = twoDoorFreshnessWindow(session);
  for (const iggy3d::EntityId doorId : order) {
    deactivateDoorThroughHashedTick(session, doorId);
  }
  (void)iggy3d::ensureActiveRoomCollisionFresh(window, &session);
  return readerSnapshot(window);
}

bool freshnessStoreDoorPermutationStressIsOrderIndependent() {
  const std::vector<std::vector<iggy3d::EntityId>> permutations = {
      {iggy3d::EntityId{2}, iggy3d::EntityId{2}, iggy3d::EntityId{3},
       iggy3d::EntityId{3}},
      {iggy3d::EntityId{2}, iggy3d::EntityId{3}, iggy3d::EntityId{2},
       iggy3d::EntityId{3}},
      {iggy3d::EntityId{2}, iggy3d::EntityId{3}, iggy3d::EntityId{3},
       iggy3d::EntityId{2}},
      {iggy3d::EntityId{3}, iggy3d::EntityId{2}, iggy3d::EntityId{2},
       iggy3d::EntityId{3}},
      {iggy3d::EntityId{3}, iggy3d::EntityId{2}, iggy3d::EntityId{3},
       iggy3d::EntityId{2}},
      {iggy3d::EntityId{3}, iggy3d::EntityId{3}, iggy3d::EntityId{2},
       iggy3d::EntityId{2}},
  };

  const CollisionReaderSnapshot baseline =
      runDoorTogglePermutation(permutations.front());
  bool ok = expect(baseline.querySurfaceCount == 1U,
                   "permutation baseline floor remains") &&
            expect(baseline.activeDoorBlockerSurfaceCount == 0U,
                   "permutation baseline active doors filtered") &&
            expect(baseline.runtimeFilteredSurfaceCount == 2U,
                   "permutation baseline filtered doors") &&
            expect(baseline.surfaceSetSize == 1U,
                   "permutation baseline surface set") &&
            expect(baseline.roomRevision == 1U,
                   "permutation baseline room stamp") &&
            expect(baseline.sessionHash != 0U,
                   "permutation baseline session stamp");
  for (std::size_t index = 1; index < permutations.size(); ++index) {
    const CollisionReaderSnapshot snapshot =
        runDoorTogglePermutation(permutations[index]);
    ok = snapshotsMatch(baseline,
                        snapshot,
                        "permutation " + std::to_string(index)) &&
         expect(snapshot.sessionHash == baseline.sessionHash,
                "permutation " + std::to_string(index) + " session stamp") &&
         ok;
  }
  return ok;
}

bool freshnessStoreRebakesSessionlessAfterSessionReset() {
  std::optional<iggy3d::Session> activeSession = twoDoorFreshnessSession();
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom =
      packageActiveRoom("session_reset_room",
                        {walkableSurface("floor"),
                         blockerSurface("door_a_blocker", "freshness_door_a"),
                         blockerSurface("door_b_blocker", "freshness_door_b")});
  deactivateDoorThroughHashedTick(*activeSession, iggy3d::EntityId{2});

  const iggy3d::ProductActiveRoomCollisionFreshnessResult withSession =
      iggy3d::ensureActiveRoomCollisionFresh(window, &*activeSession);
  const std::uint64_t sessionHash = activeSession->state().currentStateHash;
  const bool withSessionOk =
      expect(withSession.rebaked, "session reset initial rebake") &&
      expect(window.activeRoomCollision.bakedFromSessionHash == sessionHash,
             "session reset initial session stamp") &&
      expect(window.activeRoomCollision.bakedFromSessionHash != 0U,
             "session reset initial nonzero session stamp") &&
      expect(window.activeRoomCollision.runtimeFilteredSurfaceCount == 1U,
             "session reset initial filtered door") &&
      expect(window.activeRoomCollision.querySurfaceCount == 2U,
             "session reset initial query count");

  activeSession.reset();
  const iggy3d::ProductActiveRoomCollisionFreshnessResult withoutSession =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);
  return withSessionOk &&
         expect(withoutSession.rebaked, "session reset null rebake") &&
         expect(withoutSession.reasonCode == "rebaked_session",
                "session reset null reason") &&
         expect(withoutSession.observedSessionHash == 0U,
                "session reset observed null hash") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    window.activeRoomRevision,
                "session reset room stamp preserved") &&
         expect(window.activeRoomCollision.bakedFromSessionHash == 0U,
                "session reset null session stamp") &&
         expect(window.activeRoomCollision.runtimeOwnedSurfaceCount == 2U,
                "session reset runtime-owned surfaces retained") &&
         expect(window.activeRoomCollision.runtimeFilteredSurfaceCount == 0U,
                "session reset no null-session filtering") &&
         expect(window.activeRoomCollision.querySurfaceCount == 3U,
                "session reset sessionless query count") &&
         idempotentAfterRebake(window, nullptr, "session reset");
}

bool freshnessStoreDoesNotThrashEmptyLoadedRoom() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom.loaded = true;
  window.activeRoom.status = "active_room_loaded";
  window.activeRoom.reasonCode = "active_room_loaded";
  window.activeRoom.roomId = "empty_loaded_room";
  window.activeRoom.room.id = "empty_loaded_room";

  const iggy3d::ProductActiveRoomCollisionFreshnessResult first =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);
  const iggy3d::ProductActiveRoomCollisionState firstCollision =
      window.activeRoomCollision;
  bool ok = expect(first.rebaked, "empty thrash first rebake") &&
            expect(first.reasonCode == "rebaked_empty",
                   "empty thrash first reason") &&
            expect(!firstCollision.ready, "empty thrash unavailable") &&
            expect(firstCollision.reasonCode ==
                       "active_room_collision_missing_surfaces",
                   "empty thrash collision reason") &&
            expect(firstCollision.bakedFromRoomRevision ==
                       window.activeRoomRevision,
                   "empty thrash room stamp") &&
            expect(firstCollision.bakedFromSessionHash == 0U,
                   "empty thrash session stamp") &&
            expect(firstCollision.querySurfaceCount == 0U,
                   "empty thrash query count");
  std::uint64_t rebakeCount = first.rebaked ? 1U : 0U;
  for (std::uint64_t frame = 0; frame < 12U; ++frame) {
    const iggy3d::ProductActiveRoomCollisionFreshnessResult next =
        iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);
    if (next.rebaked) {
      ++rebakeCount;
    }
    ok = expect(!next.rebaked,
                "empty thrash skipped frame " + std::to_string(frame)) &&
         expect(next.reasonCode == "skipped_fresh",
                "empty thrash reason frame " + std::to_string(frame)) &&
         expect(window.activeRoomCollision.ready == firstCollision.ready,
                "empty thrash ready unchanged " + std::to_string(frame)) &&
         expect(window.activeRoomCollision.status == firstCollision.status,
                "empty thrash status unchanged " + std::to_string(frame)) &&
         expect(window.activeRoomCollision.reasonCode ==
                    firstCollision.reasonCode,
                "empty thrash reason unchanged " + std::to_string(frame)) &&
         expect(window.activeRoomCollision.querySurfaceCount ==
                    firstCollision.querySurfaceCount,
                "empty thrash query unchanged " + std::to_string(frame)) &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    firstCollision.bakedFromRoomRevision,
                "empty thrash room stamp unchanged " + std::to_string(frame)) &&
         expect(window.activeRoomCollision.bakedFromSessionHash ==
                    firstCollision.bakedFromSessionHash,
                "empty thrash session stamp unchanged " + std::to_string(frame)) &&
         ok;
  }
  return ok && expect(rebakeCount == 1U, "empty thrash exactly one bake");
}

iggy3d::ProductActiveRoomState trainingActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.roomId = "collision_training_room";
  request.sourceName = "unit/collision_training_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  return iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);
}

iggy3d::ProductActiveRoomState editedActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "###\n"
      "#P#\n"
      "###\n";
  request.roomId = "collision_editable_room";
  request.sourceName = "unit/collision_editable_room.iggyroom.txt";
  request.emitAssetText = false;
  iggy3d::ProductAsciiRoomEditingResult editing =
      iggy3d::buildProductAsciiRoomEditing(request);

  iggy3d::EditableRoomFloor floor;
  floor.id = "collision_edit_floor_1";
  floor.centerMeters = {2.0F, -0.05F, 0.0F};
  floor.sizeMeters = {1.0F, 0.10F, 1.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("debug_floor");
  (void)editing.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Ai,
                                  iggy3d::addFloorCommand(floor));

  iggy3d::EditableRoomWall wall;
  wall.id = "collision_edit_wall_1";
  wall.startMeters = {2.0F, 0.0F, -0.5F};
  wall.endMeters = {3.0F, 0.0F, -0.5F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.50F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = iggy3d::defaultWallSemantics("debug_wall");
  (void)editing.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Mouse,
                                  iggy3d::addWallCommand(wall));

  return iggy3d::buildProductActiveRoomFromRoomAuthoringSnapshot(
      editing.controller.snapshot());
}

iggy3d::SessionState runtimeStateWithDoor(bool doorActive) {
  iggy3d::SessionState state;
  iggy3d::EntityState door;
  door.id = iggy3d::EntityId{2};
  door.stableName = "marker_door_r2_c2";
  door.kind = iggy3d::EntityKind::Door;
  door.active = doorActive;
  state.world.seedEntity(std::move(door));
  return state;
}

bool buildsCollisionFromLoadedAsciiRoom() {
  const iggy3d::ProductActiveRoomState active = trainingActiveRoom();
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(collision);
  const iggy3d::CollisionQueryResult wallHit =
      surfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*surfaces,
                                 {1.5F, 0.0F, 1.5F},
                                 {1.5F, 0.0F, -2.25F},
                                 iggy3d::CollisionQueryKind::Actor);

  return expect(active.loaded, "active room loaded") &&
         expect(collision.ready, "collision ready") &&
         expect(collision.status == "active_room_collision_ready", "status") &&
         expect(collision.reasonCode == "active_room_collision_ready", "reason") &&
         expect(collision.roomId == "collision_training_room", "room id") &&
         expect(collision.spatialSurfaceCount == 56U, "authored surface count") &&
         expect(collision.querySurfaceCount == 56U, "query surface count") &&
         expect(collision.walkableSurfaceCount == 15U, "walkable count") &&
         expect(collision.actorBlockerSurfaceCount == 21U,
                "actor blocker count") &&
         expect(collision.projectileBlockerSurfaceCount == 21U,
                "projectile blocker count") &&
         expect(collision.runtimeOwnedSurfaceCount == 1U,
                "runtime owned surface count") &&
         expect(collision.runtimeFilteredSurfaceCount == 0U,
                "runtime filtered count") &&
         expect(collision.doorBlockerSurfaceCount == 0U,
                "no-runtime door blocker count") &&
         expect(surfaces != nullptr, "collision pointer available") &&
         expect(wallHit.status == iggy3d::CollisionQueryStatus::Hit,
                "actor query hits wall") &&
         expect(wallHit.role == iggy3d::CollisionSurfaceRole::Blocker,
                "actor query hits blocker");
}

bool runtimeDoorStateFiltersDoorCollision() {
  const iggy3d::ProductActiveRoomState active = trainingActiveRoom();
  const iggy3d::SessionState closedState = runtimeStateWithDoor(true);
  const iggy3d::SessionState openState = runtimeStateWithDoor(false);
  const iggy3d::ProductActiveRoomCollisionState closedCollision =
      iggy3d::buildProductActiveRoomCollision(active, closedState);
  const iggy3d::ProductActiveRoomCollisionState openCollision =
      iggy3d::buildProductActiveRoomCollision(active, openState);
  const iggy3d::SpatialSurfaceSet* closedSurfaces =
      iggy3d::productActiveRoomCollisionSurfaces(closedCollision);
  const iggy3d::SpatialSurfaceSet* openSurfaces =
      iggy3d::productActiveRoomCollisionSurfaces(openCollision);
  const iggy3d::CollisionQueryResult closedDoorHit =
      closedSurfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*closedSurfaces,
                                 {-2.0F, 0.0F, -1.0F},
                                 {-1.0F, 0.0F, 0.0F},
                                 iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult openDoorStride =
      openSurfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*openSurfaces,
                                 {-2.0F, 0.0F, -1.0F},
                                 {-1.0F, 0.0F, 0.0F},
                                 iggy3d::CollisionQueryKind::Actor);

  return expect(closedCollision.ready, "closed collision ready") &&
         expect(openCollision.ready, "open collision ready") &&
         expect(closedCollision.runtimeOwnedSurfaceCount == 1U,
                "closed runtime owned count") &&
         expect(closedCollision.runtimeFilteredSurfaceCount == 0U,
                "closed filtered count") &&
         expect(closedCollision.doorBlockerSurfaceCount == 1U,
                "closed door blocker count") &&
         expect(closedCollision.activeDoorBlockerSurfaceCount == 1U,
                "closed active door blocker count") &&
         expect(closedCollision.querySurfaceCount == 56U,
                "closed query surface count") &&
         expect(closedCollision.actorBlockerSurfaceCount == 21U,
                "closed actor blocker count") &&
         expect(openCollision.runtimeOwnedSurfaceCount == 1U,
                "open runtime owned count") &&
         expect(openCollision.runtimeFilteredSurfaceCount == 1U,
                "open filtered count") &&
         expect(openCollision.doorBlockerSurfaceCount == 1U,
                "open door blocker count") &&
         expect(openCollision.activeDoorBlockerSurfaceCount == 0U,
                "open active door blocker count") &&
         expect(openCollision.querySurfaceCount == 55U,
                "open query surface count") &&
         expect(openCollision.actorBlockerSurfaceCount == 20U,
                "open actor blocker count") &&
         expect(closedDoorHit.status == iggy3d::CollisionQueryStatus::Hit,
                "closed door hit") &&
         expect(closedDoorHit.surfaceId == "marker_door_r2_c2_door_blocker",
                "closed door surface id") &&
         expect(openDoorStride.status == iggy3d::CollisionQueryStatus::NoHit,
                "open door stride clear");
}

bool buildsCollisionFromEditedRoomSnapshot() {
  const iggy3d::ProductActiveRoomState active = editedActiveRoom();
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(collision);
  const iggy3d::CollisionQueryResult wallHit =
      surfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*surfaces,
                                 {2.5F, 0.0F, 0.25F},
                                 {2.5F, 0.0F, -1.25F},
                                 iggy3d::CollisionQueryKind::Actor);

  return expect(active.loaded, "edited active room loaded") &&
         expect(active.source == "editable_room", "edited active room source") &&
         expect(collision.ready, "edited collision ready") &&
         expect(collision.status == "active_room_collision_ready",
                "edited collision status") &&
         expect(collision.reasonCode == "active_room_collision_ready",
                "edited collision reason") &&
         expect(collision.roomId == "collision_editable_room",
                "edited collision room id") &&
         expect(collision.spatialSurfaceCount == 20U,
                "edited authored surface count") &&
         expect(collision.querySurfaceCount == 20U,
                "edited query surface count") &&
         expect(collision.walkableSurfaceCount == 2U,
                "edited walkable count") &&
         expect(collision.actorBlockerSurfaceCount == 9U,
                "edited actor blocker count") &&
         expect(collision.projectileBlockerSurfaceCount == 9U,
                "edited projectile blocker count") &&
         expect(collision.runtimeOwnedSurfaceCount == 0U,
                "edited runtime owned count") &&
         expect(surfaces != nullptr, "edited surfaces available") &&
         expect(wallHit.status == iggy3d::CollisionQueryStatus::Hit,
                "edited wall hit") &&
         expect(wallHit.role == iggy3d::CollisionSurfaceRole::Blocker,
                "edited wall blocker");
}

bool rejectsUnloadedActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "...\n"
      "...\n";
  request.roomId = "missing_spawn_collision_room";
  request.sourceName = "unit/missing_spawn_collision_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);

  return expect(!active.loaded, "active room not loaded") &&
         expect(!collision.ready, "collision not ready") &&
         expect(collision.status == "active_room_collision_unavailable",
                "unavailable status") &&
         expect(collision.reasonCode == "ascii_room_missing_player_spawn",
                "failure reason") &&
         expect(collision.roomId == "missing_spawn_collision_room",
                "room id preserved") &&
         expect(collision.querySurfaceCount == 0U, "no query surfaces") &&
         expect(iggy3d::productActiveRoomCollisionSurfaces(collision) == nullptr,
                "no surface pointer");
}

bool rejectsLoadedRoomWithoutSurfaces() {
  iggy3d::ProductActiveRoomState active;
  active.loaded = true;
  active.status = "active_room_loaded";
  active.reasonCode = "active_room_loaded";
  active.roomId = "empty_collision_room";
  active.room.id = "empty_collision_room";

  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);

  return expect(!collision.ready, "empty collision not ready") &&
         expect(collision.status == "active_room_collision_unavailable",
                "empty unavailable status") &&
         expect(collision.reasonCode == "active_room_collision_missing_surfaces",
                "missing surfaces reason") &&
         expect(collision.roomId == "empty_collision_room",
                "empty room id") &&
         expect(collision.querySurfaceCount == 0U, "empty query count") &&
         expect(iggy3d::productActiveRoomCollisionSurfaces(collision) == nullptr,
                "empty pointer unavailable");
}

}  // namespace

int main() {
  const bool ok = collisionProvenanceDefaultsToZero() &&
                  freshnessStoreSkipsFreshCollision() &&
                  freshnessStoreRebakesWhenRoomReplaced() &&
                  freshnessStoreRebakesWhenSessionHashChanges() &&
                  freshnessStoreRebakesUnloadedRoom() &&
                  frameBoundaryEnsureMakesDoorCollisionOrderIndependent() &&
                  freshnessStoreDoorPermutationStressIsOrderIndependent() &&
                  freshnessStoreRebakesSessionlessAfterSessionReset() &&
                  freshnessStoreDoesNotThrashEmptyLoadedRoom() &&
                  buildsCollisionFromLoadedAsciiRoom() &&
                  runtimeDoorStateFiltersDoorCollision() &&
                  buildsCollisionFromEditedRoomSnapshot() &&
                  rejectsUnloadedActiveRoom() &&
                  rejectsLoadedRoomWithoutSurfaces();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
