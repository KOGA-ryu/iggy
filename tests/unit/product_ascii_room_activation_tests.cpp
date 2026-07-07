#include "app/iggy3d/ascii_room/Activation.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "projection/scene/SceneProjection.hpp"

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

const iggy3d::SceneItem* findSceneItem(const iggy3d::SceneProjectionResult& scene,
                                       std::string_view stableName) {
  for (const iggy3d::SceneItem& item : scene.items) {
    if (item.stableName == stableName) {
      return &item;
    }
  }
  return nullptr;
}

iggy3d::ProductAppWindowState trainingRoomWindow() {
  iggy3d::ProductAppWindowState window;
  window.creativeAuthoring.asciiRoomDraft.text = std::string(kTrainingRoom);
  window.creativeAuthoring.asciiRoomDraft.roomId = "activation_training_room";
  window.creativeAuthoring.asciiRoomDraft.sourceName = "unit/activation_training_room.iggyroom.txt";
  return window;
}

bool activatesSessionFromAsciiRoom() {
  iggy3d::ProductAppWindowState window = trainingRoomWindow();
  std::optional<iggy3d::Session> session;

  const iggy3d::ProductAsciiRoomActivationResult result =
      iggy3d::activateProductAsciiRoomPreview(session, window);
  const iggy3d::SceneProjectionResult scene =
      session.has_value() ? iggy3d::buildSceneProjection(session->state())
                          : iggy3d::SceneProjectionResult{};
  const iggy3d::SceneItem* player = findSceneItem(scene, "player");
  const iggy3d::SceneItem* npc =
      findSceneItem(scene, "marker_npc_spawn_r1_c4");
  const iggy3d::SceneItem* treasure =
      findSceneItem(scene, "marker_treasure_r2_c4");

  return expect(result.ok, "activation ok") &&
         expect(result.status == "ascii_room_activated", "activation status") &&
         expect(result.reasonCode == "ascii_room_activated", "activation reason") &&
         expect(result.roomId == "activation_training_room", "activation room") &&
         expect(result.packageId == "iggy3d.ascii_room_authoring",
                "activation package") &&
         expect(result.scenarioId == "activation_training_room.runtime_loop",
                "activation scenario") &&
         expect(result.sessionCreated, "session created") &&
         expect(result.playerSpawned, "player spawned") &&
         expect(result.playerCount == 1U, "player count") &&
         expect(result.entityCount == 5U, "entity count") &&
         expect(result.npcCount == 1U, "npc count") &&
         expect(result.pickupCount == 1U, "pickup count") &&
         expect(result.doorCount == 1U, "door count") &&
         expect(result.markerEntityCount == 1U, "marker entity count") &&
         expect(result.objectiveCount == 2U, "objective count") &&
         expect(result.wallCount == 20U, "wall count") &&
         expect(result.markerCount == 5U, "marker count") &&
         expect(session.has_value(), "session present") &&
         expect(session->state().identity.packageId ==
                    "iggy3d.ascii_room_authoring",
                "session package") &&
         expect(session->state().identity.scenarioId ==
                    "activation_training_room.runtime_loop",
                "session scenario") &&
         expect(session->state().world.size() == 5U, "world entity count") &&
         expect(window.gameplay.gameplayActive, "window gameplay active") &&
         expect(window.gameplay.runtimeSessionCreated, "window runtime session") &&
         expect(window.runtimeStateHash == session->stateHash(), "window hash") &&
         expect(window.creativeAuthoring.asciiRoomActivation.runtimeHash == session->stateHash(),
                "activation hash") &&
         expect(window.creativeAuthoring.asciiRoomActivation.npcCount == 1U,
                "window activation npc count") &&
         expect(window.creativeAuthoring.asciiRoomActivation.pickupCount == 1U,
                "window activation pickup count") &&
         expect(window.creativeAuthoring.asciiRoomActivation.doorCount == 1U,
                "window activation door count") &&
         expect(window.creativeAuthoring.asciiRoomActivation.markerEntityCount == 1U,
                "window activation marker entity count") &&
         expect(iggy3d::activeRoom(window).loaded, "active room loaded") &&
         expect(iggy3d::activeRoom(window).status == "active_room_loaded",
                "active room status") &&
         expect(iggy3d::activeRoom(window).source == "ascii_room", "active room source") &&
         expect(iggy3d::activeRoom(window).roomId == "activation_training_room",
                "active room id") &&
         expect(iggy3d::activeRoom(window).staticMeshCount == 36U,
                "active room meshes") &&
         expect(iggy3d::activeRoom(window).spatialSurfaceCount == 56U,
                "active room surfaces") &&
         expect(iggy3d::activeRoom(window).walkableSurfaceCount == 15U,
                "walkable surfaces") &&
         expect(iggy3d::activeRoom(window).actorBlockerSurfaceCount == 21U,
                "actor blockers") &&
         expect(iggy3d::activeRoom(window).projectileBlockerSurfaceCount == 21U,
                "projectile blockers") &&
         expect(iggy3d::activeRoomCollision(window).ready,
                "active room collision ready") &&
         expect(iggy3d::activeRoomCollision(window).status ==
                    "active_room_collision_ready",
                "active room collision status") &&
         expect(iggy3d::activeRoomCollision(window).reasonCode ==
                    "active_room_collision_ready",
                "active room collision reason") &&
         expect(iggy3d::activeRoomCollision(window).roomId ==
                    "activation_training_room",
                "active room collision id") &&
         expect(iggy3d::activeRoomCollision(window).spatialSurfaceCount == 56U,
                "active room collision source count") &&
         expect(iggy3d::activeRoomCollision(window).querySurfaceCount == 56U,
                "active room collision query count") &&
         expect(iggy3d::activeRoomCollision(window).walkableSurfaceCount == 15U,
                "active room collision walkable count") &&
         expect(iggy3d::activeRoomCollision(window).actorBlockerSurfaceCount == 21U,
                "active room collision actor blocker count") &&
         expect(iggy3d::activeRoomCollision(window).projectileBlockerSurfaceCount == 21U,
                "active room collision projectile blocker count") &&
         expect(iggy3d::activeRoomCollision(window).runtimeOwnedSurfaceCount == 1U,
                "active room collision runtime owned count") &&
         expect(iggy3d::activeRoomCollision(window).runtimeFilteredSurfaceCount == 0U,
                "active room collision runtime filtered count") &&
         expect(iggy3d::activeRoomCollision(window).doorBlockerSurfaceCount == 1U,
                "active room collision door blocker count") &&
         expect(iggy3d::activeRoomCollision(window).activeDoorBlockerSurfaceCount == 1U,
                "active room collision active door blocker count") &&
         expect(player != nullptr && player->kind == iggy3d::SceneItemKind::Player,
                "player projected") &&
         expect(npc != nullptr && npc->kind == iggy3d::SceneItemKind::Npc,
                "npc projected") &&
         expect(treasure != nullptr &&
                    treasure->kind == iggy3d::SceneItemKind::Pickup,
                "treasure projected");
}

bool rejectsInvalidAsciiWithoutSession() {
  iggy3d::ProductAppWindowState window;
  window.creativeAuthoring.asciiRoomDraft.text =
      "...\n"
      "...\n";
  window.creativeAuthoring.asciiRoomDraft.roomId = "missing_spawn_room";
  window.creativeAuthoring.asciiRoomDraft.sourceName = "unit/missing_spawn.iggyroom.txt";
  std::optional<iggy3d::Session> session;

  const iggy3d::ProductAsciiRoomActivationResult result =
      iggy3d::activateProductAsciiRoomPreview(session, window);

  return expect(!result.ok, "activation rejected") &&
         expect(result.status == "ascii_room_missing_player_spawn", "status") &&
         expect(result.reasonCode == "ascii_room_missing_player_spawn",
                "reason") &&
         expect(result.roomId == "missing_spawn_room", "room") &&
         expect(!result.sessionCreated, "no session created") &&
         expect(!result.playerSpawned, "no player spawned") &&
         expect(!session.has_value(), "session absent") &&
         expect(!window.gameplay.gameplayActive, "window gameplay inactive") &&
         expect(!window.gameplay.runtimeSessionCreated, "window runtime absent") &&
         expect(window.creativeAuthoring.asciiRoomPreview.failedStage == "grid",
                "preview failed at grid") &&
         expect(window.creativeAuthoring.asciiRoomActivation.status ==
                    "ascii_room_missing_player_spawn",
                "window activation status") &&
         expect(!iggy3d::activeRoom(window).loaded, "active room not loaded") &&
         expect(iggy3d::activeRoom(window).status == "ascii_room_missing_player_spawn",
                "active room failure status") &&
         expect(!iggy3d::activeRoomCollision(window).ready,
                "active room collision not ready") &&
         expect(iggy3d::activeRoomCollision(window).status ==
                    "active_room_collision_unavailable",
                "active room collision failure status") &&
         expect(iggy3d::activeRoomCollision(window).reasonCode ==
                    "ascii_room_missing_player_spawn",
                "active room collision failure reason") &&
         expect(window.creativeAuthoring.asciiRoomActivation.sessionCreated == false,
                "window no activation session");
}

}  // namespace

int main() {
  const bool ok = activatesSessionFromAsciiRoom() &&
                  rejectsInvalidAsciiWithoutSession();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
