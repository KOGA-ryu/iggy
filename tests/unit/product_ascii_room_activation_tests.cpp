#include "app/iggy3d/ProductAsciiRoomActivation.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

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
  window.asciiRoomDraftText = std::string(kTrainingRoom);
  window.asciiRoomDraftRoomId = "activation_training_room";
  window.asciiRoomDraftSourceName = "unit/activation_training_room.iggyroom.txt";
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
         expect(result.objectiveCount == 1U, "objective count") &&
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
         expect(window.gameplayActive, "window gameplay active") &&
         expect(window.runtimeSessionCreated, "window runtime session") &&
         expect(window.runtimeStateHash == session->stateHash(), "window hash") &&
         expect(window.asciiRoomActivationRuntimeHash == session->stateHash(),
                "activation hash") &&
         expect(window.asciiRoomActivationNpcCount == 1U,
                "window activation npc count") &&
         expect(window.asciiRoomActivationPickupCount == 1U,
                "window activation pickup count") &&
         expect(window.asciiRoomActivationDoorCount == 1U,
                "window activation door count") &&
         expect(window.asciiRoomActivationMarkerEntityCount == 1U,
                "window activation marker entity count") &&
         expect(window.activeRoom.loaded, "active room loaded") &&
         expect(window.activeRoom.status == "active_room_loaded",
                "active room status") &&
         expect(window.activeRoom.source == "ascii_room", "active room source") &&
         expect(window.activeRoom.roomId == "activation_training_room",
                "active room id") &&
         expect(window.activeRoom.staticMeshCount == 35U,
                "active room meshes") &&
         expect(window.activeRoom.spatialSurfaceCount == 55U,
                "active room surfaces") &&
         expect(window.activeRoom.walkableSurfaceCount == 15U,
                "walkable surfaces") &&
         expect(window.activeRoom.actorBlockerSurfaceCount == 20U,
                "actor blockers") &&
         expect(window.activeRoom.projectileBlockerSurfaceCount == 20U,
                "projectile blockers") &&
         expect(window.activeRoomCollision.ready,
                "active room collision ready") &&
         expect(window.activeRoomCollision.status ==
                    "active_room_collision_ready",
                "active room collision status") &&
         expect(window.activeRoomCollision.reasonCode ==
                    "active_room_collision_ready",
                "active room collision reason") &&
         expect(window.activeRoomCollision.roomId ==
                    "activation_training_room",
                "active room collision id") &&
         expect(window.activeRoomCollision.spatialSurfaceCount == 55U,
                "active room collision source count") &&
         expect(window.activeRoomCollision.querySurfaceCount == 55U,
                "active room collision query count") &&
         expect(window.activeRoomCollision.walkableSurfaceCount == 15U,
                "active room collision walkable count") &&
         expect(window.activeRoomCollision.actorBlockerSurfaceCount == 20U,
                "active room collision actor blocker count") &&
         expect(window.activeRoomCollision.projectileBlockerSurfaceCount == 20U,
                "active room collision projectile blocker count") &&
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
  window.asciiRoomDraftText =
      "...\n"
      "...\n";
  window.asciiRoomDraftRoomId = "missing_spawn_room";
  window.asciiRoomDraftSourceName = "unit/missing_spawn.iggyroom.txt";
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
         expect(!window.gameplayActive, "window gameplay inactive") &&
         expect(!window.runtimeSessionCreated, "window runtime absent") &&
         expect(window.asciiRoomPreviewFailedStage == "grid",
                "preview failed at grid") &&
         expect(window.asciiRoomActivationStatus ==
                    "ascii_room_missing_player_spawn",
                "window activation status") &&
         expect(!window.activeRoom.loaded, "active room not loaded") &&
         expect(window.activeRoom.status == "ascii_room_missing_player_spawn",
                "active room failure status") &&
         expect(!window.activeRoomCollision.ready,
                "active room collision not ready") &&
         expect(window.activeRoomCollision.status ==
                    "active_room_collision_unavailable",
                "active room collision failure status") &&
         expect(window.activeRoomCollision.reasonCode ==
                    "ascii_room_missing_player_spawn",
                "active room collision failure reason") &&
         expect(window.asciiRoomActivationSessionCreated == false,
                "window no activation session");
}

}  // namespace

int main() {
  const bool ok = activatesSessionFromAsciiRoom() &&
                  rejectsInvalidAsciiWithoutSession();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
