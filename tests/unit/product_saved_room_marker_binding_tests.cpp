#include "app/iggy3d/gameplay/ProductActiveRoomState.hpp"
#include "app/iggy3d/ascii_room/ProductAsciiRoomAuthoring.hpp"
#include "app/iggy3d/save/ProductSavedRoomMarkerBinding.hpp"
#include "config/RuntimeConfig.hpp"
#include "runtime/session/Session.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

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

iggy3d::Transform3 transformAt(iggy3d::Vec3 position) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = position;
  return transform;
}

iggy3d::ScenarioEntitySeed playerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "player";
  seed.kind = iggy3d::EntityKind::Player;
  seed.transform = transformAt({0.0F, 0.0F, 0.0F});
  seed.localBounds =
      iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  return seed;
}

iggy3d::FixtureScenarioSeed basePlayerOnlySeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "marker_binding_base";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities.push_back(playerSeed());

  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "base_objective";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.playerSlot = 0;
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  seed.objectives.push_back(std::move(objective));
  return seed;
}

iggy3d::Session makeBaseSession() {
  iggy3d::SessionCreateRequest request;
  request.packageId = "iggy3d.marker_binding_base";
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = basePlayerOnlySeed();
  const iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  return created.value;
}

iggy3d::ProductActiveRoomState makeSavedActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.roomId = "saved_marker_room";
  request.sourceName = "unit/saved_marker_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomAuthoringResult authored =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  if (!authored.ok) {
    return {};
  }
  return iggy3d::buildProductActiveRoomFromSavedAuthoredRoom(
      authored.authoredRoom.authoredRoom);
}

const iggy3d::EntityState* findEntity(const iggy3d::Session& session,
                                      std::string_view stableName) {
  return session.state().world.findByStableName(std::string(stableName));
}

bool hasObjective(const iggy3d::Session& session, std::string_view objectiveId) {
  for (const iggy3d::ObjectiveRecord& objective :
       session.state().objectives.objectives) {
    if (objective.objectiveId == objectiveId) {
      return true;
    }
  }
  return false;
}

bool bindsMissingRuntimeRecords() {
  iggy3d::Session session = makeBaseSession();
  const iggy3d::ProductActiveRoomState activeRoom = makeSavedActiveRoom();
  const std::uint64_t beforeHash = session.stateHash();

  const iggy3d::ProductSavedRoomMarkerBindingResult bound =
      iggy3d::bindSavedRoomMarkersToSession(activeRoom, session);

  const iggy3d::EntityState* npc =
      findEntity(session, "marker_npc_spawn_r1_c4");
  const iggy3d::EntityState* treasure =
      findEntity(session, "marker_treasure_r2_c4");
  const iggy3d::EntityState* door =
      findEntity(session, "marker_door_r2_c2");
  const iggy3d::EntityState* exit =
      findEntity(session, "marker_exit_r3_c3");

  return expect(activeRoom.loaded, "active room loaded") &&
         expect(activeRoom.authoredMarkerCount == 5U, "marker count") &&
         expect(bound.ok, "bind ok") &&
         expect(bound.requested, "bind requested") &&
         expect(bound.sessionReplaced, "session replaced") &&
         expect(bound.status == "saved_marker_bind_applied", "bind status") &&
         expect(bound.roomId == "saved_marker_room", "room id") &&
         expect(bound.markerCount == 5U, "bound marker count") &&
         expect(bound.seedEntityCount == 5U, "seed entity count") &&
         expect(bound.addedEntityCount == 4U, "added entity count") &&
         expect(bound.existingEntityCount == 1U, "existing player count") &&
         expect(bound.addedObjectiveCount == 2U, "added objective count") &&
         expect(bound.existingObjectiveCount == 0U, "existing objective count") &&
         expect(bound.addedCombatantCount == 2U, "added combatant count") &&
         expect(bound.existingCombatantCount == 0U, "existing combatant count") &&
         expect(bound.pickupCount == 1U, "pickup count") &&
         expect(bound.doorCount == 1U, "door count") &&
         expect(bound.markerEntityCount == 1U, "exit marker entity count") &&
         expect(bound.npcCount == 1U, "npc count") &&
         expect(bound.previousHash == beforeHash, "previous hash") &&
         expect(bound.boundHash == session.stateHash(), "bound hash") &&
         expect(bound.boundHash != beforeHash, "hash changed") &&
         expect(session.state().world.size() == 5U, "world count") &&
         expect(session.state().combat.combatants.size() == 2U,
                "combatant count") &&
         expect(session.state().objectives.objectives.size() == 3U,
                "objective count") &&
         expect(npc != nullptr && npc->kind == iggy3d::EntityKind::Npc,
                "npc bound") &&
         expect(treasure != nullptr &&
                    treasure->kind == iggy3d::EntityKind::Pickup &&
                    treasure->interaction.itemId == "marker_treasure_r2_c4",
                "treasure bound") &&
         expect(door != nullptr && door->kind == iggy3d::EntityKind::Door,
                "door bound") &&
         expect(exit != nullptr && exit->kind == iggy3d::EntityKind::Marker,
                "exit bound") &&
         expect(hasObjective(session, "collect_marker_treasure_r2_c4"),
                "treasure objective") &&
         expect(hasObjective(session, "exit_marker_exit_r3_c3"),
                "exit objective");
}

bool bindingIsIdempotentAndResetDurable() {
  iggy3d::Session session = makeBaseSession();
  const iggy3d::ProductActiveRoomState activeRoom = makeSavedActiveRoom();
  const iggy3d::ProductSavedRoomMarkerBindingResult first =
      iggy3d::bindSavedRoomMarkersToSession(activeRoom, session);
  const std::uint64_t firstHash = session.stateHash();
  const iggy3d::ProductSavedRoomMarkerBindingResult second =
      iggy3d::bindSavedRoomMarkersToSession(activeRoom, session);
  const std::uint64_t secondHash = session.stateHash();
  const iggy3d::SessionResetResult reset = session.resetToBaseline();

  return expect(first.ok, "first bind ok") &&
         expect(second.ok, "second bind ok") &&
         expect(!second.sessionReplaced, "second bind no replace") &&
         expect(second.status == "saved_marker_bind_noop", "second status") &&
         expect(second.addedEntityCount == 0U, "second no entities") &&
         expect(second.existingEntityCount == 5U, "second existing entities") &&
         expect(second.addedObjectiveCount == 0U, "second no objectives") &&
         expect(second.existingObjectiveCount == 2U,
                "second existing objectives") &&
         expect(second.addedCombatantCount == 0U, "second no combatants") &&
         expect(second.existingCombatantCount == 2U,
                "second existing combatants") &&
         expect(secondHash == firstHash, "hash stable") &&
         expect(reset.reset, "reset ok") &&
         expect(findEntity(session, "marker_npc_spawn_r1_c4") != nullptr,
                "reset keeps npc") &&
         expect(findEntity(session, "marker_treasure_r2_c4") != nullptr,
                "reset keeps treasure") &&
         expect(findEntity(session, "marker_door_r2_c2") != nullptr,
                "reset keeps door") &&
         expect(findEntity(session, "marker_exit_r3_c3") != nullptr,
                "reset keeps exit");
}

bool skipsWhenNoSavedAuthoredRoom() {
  iggy3d::Session session = makeBaseSession();
  const iggy3d::ProductActiveRoomState activeRoom;
  const iggy3d::ProductSavedRoomMarkerBindingResult bound =
      iggy3d::bindSavedRoomMarkersToSession(activeRoom, session);
  return expect(bound.ok, "skip ok") &&
         expect(!bound.requested, "not requested") &&
         expect(bound.status == "saved_marker_bind_not_requested", "skip status") &&
         expect(session.state().world.size() == 1U, "world unchanged");
}

}  // namespace

int main() {
  const bool ok = bindsMissingRuntimeRecords() &&
                  bindingIsIdempotentAndResetDurable() &&
                  skipsWhenNoSavedAuthoredRoom();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
