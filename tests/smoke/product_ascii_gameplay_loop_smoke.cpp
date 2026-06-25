#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductActiveRoomState.hpp"
#include "app/iggy3d/ProductPackageSessionSeed.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/session/Session.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const iggy3d::EntityState* findEntity(const iggy3d::Session& session,
                                      std::string_view stableName) {
  return session.state().world.findByStableName(std::string(stableName));
}

iggy3d::CommandRecord interact(iggy3d::EntityId target) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  return command;
}

iggy3d::CommandRecord moveTo(iggy3d::Vec3 position) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = position;
  return command;
}

iggy3d::CommandRecord attack(iggy3d::EntityId target, std::int32_t damage) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Attack;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  command.payload.attackDamage = damage;
  return command;
}

std::uint32_t itemCount(const iggy3d::Session& session, std::string_view itemId) {
  const iggy3d::PlayerInventory* inventory =
      iggy3d::findInventory(session.state().inventory, 0);
  if (inventory == nullptr) {
    return 0;
  }
  for (const iggy3d::InventoryStack& stack : inventory->stacks) {
    if (stack.itemId == itemId) {
      return stack.count;
    }
  }
  return 0;
}

const iggy3d::CombatantState* combatantFor(const iggy3d::Session& session,
                                           iggy3d::EntityId entity) {
  for (const iggy3d::CombatantState& combatant : session.state().combat.combatants) {
    if (combatant.entity == entity) {
      return &combatant;
    }
  }
  return nullptr;
}

const char* outcomeName(iggy3d::SessionOutcome outcome) {
  switch (outcome) {
    case iggy3d::SessionOutcome::None:
      return "None";
    case iggy3d::SessionOutcome::DemoComplete:
      return "DemoComplete";
    case iggy3d::SessionOutcome::Victory:
      return "Victory";
    case iggy3d::SessionOutcome::Defeat:
      return "Defeat";
    case iggy3d::SessionOutcome::Failed:
      return "Failed";
  }
  return "None";
}

iggy3d::Result<iggy3d::Session> createSessionFromSeed(
    const iggy3d::FixtureScenarioSeed& seed) {
  iggy3d::SessionCreateRequest create;
  create.packageId = "iggy3d.ascii_gameplay_loop";
  create.config = seed.config;
  create.seed = seed;
  return iggy3d::Session::create(create);
}

bool submitAcceptedAndTick(iggy3d::Session& session,
                           const iggy3d::CommandRecord& command,
                           std::string_view label) {
  const iggy3d::SessionCommandResult submitted = session.submitCommand(command);
  if (!expect(submitted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
              std::string(label) + " accepted")) {
    return false;
  }
  return expect(session.tick().status == iggy3d::ResultStatus::Ok,
                std::string(label) + " tick ok");
}

iggy3d::PackageLoadResult makePackageFromAscii(const iggy3d::RoomAsset& room) {
  iggy3d::PackageLoadResult package;
  package.status = iggy3d::PackageLoadStatus::Ok;
  package.manifest.packageId = "iggy3d.ascii_gameplay_loop";
  package.manifest.schemaVersion = 1;
  package.manifest.requiredRuntimeSchema = 1;
  package.manifest.scenarioPath = "inline_ascii_gameplay_loop";
  package.scenario.scenarioId = "ascii_gameplay_loop.runtime_loop";
  package.rooms.push_back(room);
  return package;
}

bool runNpcCombatLoop() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceName = "smoke/ascii_npc_combat.iggyroom.txt";
  request.roomId = "ascii_npc_combat";
  request.centerOnOrigin = false;
  request.emitAssetText = false;
  request.sourceText =
      "######\n"
      "#PN$E#\n"
      "######\n";

  const iggy3d::ProductAsciiRoomAuthoringResult authored =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  if (!expect(authored.ok, "npc ascii authoring ok")) {
    return false;
  }

  const iggy3d::ProductPackageSessionSeedResult seed =
      iggy3d::buildProductPackageSessionSeed(makePackageFromAscii(authored.roomAsset.room));
  if (!expect(seed.ok, "npc package seed ok")) {
    return false;
  }
  iggy3d::Result<iggy3d::Session> created = createSessionFromSeed(seed.seed);
  if (!expect(created.status == iggy3d::ResultStatus::Ok,
              "npc session create ok")) {
    return false;
  }

  iggy3d::Session session = std::move(created.value);
  const iggy3d::EntityState* npc = findEntity(session, "marker_npc_spawn_r1_c2");
  bool ok = expect(npc != nullptr, "npc exists") &&
            expect(seed.npcCount == 1U, "npc count") &&
            expect(seed.entityCount == 4U, "npc room entity count") &&
            expect(npc != nullptr && npc->kind == iggy3d::EntityKind::Npc,
                   "npc kind") &&
            expect(npc != nullptr &&
                       iggy3d::isTargetActionSupported(npc->targeting,
                                                       iggy3d::TargetAction::Attack),
                   "npc attack targetable");
  if (!ok || npc == nullptr) {
    return false;
  }

  const iggy3d::CombatantState* before = combatantFor(session, npc->id);
  ok = ok && expect(before != nullptr, "npc combatant") &&
       expect(before != nullptr && before->hitPoints == 3, "npc hp before");

  ok = ok && submitAcceptedAndTick(session, attack(npc->id, 1), "npc attack one");
  const iggy3d::CombatantState* wounded = combatantFor(session, npc->id);
  ok = ok && expect(wounded != nullptr && wounded->hitPoints == 2,
                    "npc hp after wound") &&
       expect(wounded != nullptr && !wounded->defeated, "npc not defeated");

  ok = ok && submitAcceptedAndTick(session, attack(npc->id, 2), "npc attack defeat");
  const iggy3d::CombatantState* defeated = combatantFor(session, npc->id);
  ok = ok && expect(defeated != nullptr && defeated->hitPoints == 0,
                    "npc hp zero") &&
       expect(defeated != nullptr && defeated->defeated, "npc defeated") &&
       expect(npc->active, "npc entity remains active after combat");

  std::cout << "npc_room_ready=" << (authored.ok ? "true" : "false") << "\n";
  std::cout << "npc_targetable="
            << (npc != nullptr && npc->targeting.targetable ? "true" : "false")
            << "\n";
  std::cout << "npc_attackable="
            << (npc != nullptr &&
                        iggy3d::isTargetActionSupported(npc->targeting,
                                                        iggy3d::TargetAction::Attack)
                    ? "true"
                    : "false")
            << "\n";
  std::cout << "npc_defeated="
            << (defeated != nullptr && defeated->defeated ? "true" : "false")
            << "\n";
  return ok;
}

bool runAsciiGameplayLoop() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceName = "smoke/ascii_gameplay_loop.iggyroom.txt";
  request.roomId = "ascii_gameplay_loop";
  request.centerOnOrigin = false;
  request.emitAssetText = false;
  request.sourceText =
      "#######\n"
      "#PKs$E#\n"
      "#######\n";

  const iggy3d::ProductAsciiRoomAuthoringResult authored =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  if (!expect(authored.ok, "ascii authoring ok")) {
    return false;
  }

  const iggy3d::ProductPackageSessionSeedResult seed =
      iggy3d::buildProductPackageSessionSeed(makePackageFromAscii(authored.roomAsset.room));
  if (!expect(seed.ok, "package seed ok")) {
    return false;
  }

  iggy3d::Result<iggy3d::Session> created = createSessionFromSeed(seed.seed);
  if (!expect(created.status == iggy3d::ResultStatus::Ok, "session create ok")) {
    return false;
  }

  iggy3d::Session session = std::move(created.value);
  const iggy3d::ProductActiveRoomState activeRoom =
      iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authored);
  const iggy3d::EntityState* key = findEntity(session, "marker_key_r1_c2");
  const iggy3d::EntityState* secretDoor =
      findEntity(session, "marker_secret_door_r1_c3");
  const iggy3d::EntityState* treasure = findEntity(session, "marker_treasure_r1_c4");
  const iggy3d::EntityState* exit = findEntity(session, "marker_exit_r1_c5");

  bool ok = expect(key != nullptr, "key exists") &&
            expect(secretDoor != nullptr, "secret door exists") &&
            expect(treasure != nullptr, "treasure exists") &&
            expect(exit != nullptr, "exit exists") &&
            expect(seed.pickupCount == 2U, "key and treasure pickups") &&
            expect(seed.doorCount == 1U, "secret door count") &&
            expect(seed.markerEntityCount == 1U, "exit marker count") &&
            expect(seed.objectiveCount == 3U, "objectives count");
  if (!ok) {
    return false;
  }

  ok = submitAcceptedAndTick(session, interact(key->id), "key pickup") &&
       expect(itemCount(session, key->stableName) == 1U, "key item acquired") &&
       expect(iggy3d::objectiveComplete(session.state().objectives,
                                        "collect_marker_key_r1_c2"),
              "key objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::None,
              "key does not finish");

  ok = ok && submitAcceptedAndTick(session,
                                   moveTo(key->transform.position),
                                   "move to key cell");
  const iggy3d::ProductActiveRoomCollisionState closedDoorCollision =
      iggy3d::buildProductActiveRoomCollision(activeRoom, session.state());
  ok = ok && submitAcceptedAndTick(session, interact(secretDoor->id), "open secret door") &&
       expect(!session.state().world.findById(secretDoor->id)->active,
              "secret door opened");
  const iggy3d::ProductActiveRoomCollisionState openDoorCollision =
      iggy3d::buildProductActiveRoomCollision(activeRoom, session.state());
  ok = ok && expect(closedDoorCollision.ready, "closed door collision ready") &&
       expect(closedDoorCollision.doorBlockerSurfaceCount == 1U,
              "closed door blocker counted") &&
       expect(closedDoorCollision.activeDoorBlockerSurfaceCount == 1U,
              "closed active door blocker counted") &&
       expect(openDoorCollision.ready, "open door collision ready") &&
       expect(openDoorCollision.doorBlockerSurfaceCount == 1U,
              "open door blocker counted") &&
       expect(openDoorCollision.activeDoorBlockerSurfaceCount == 0U,
              "open active door blocker filtered") &&
       expect(openDoorCollision.runtimeFilteredSurfaceCount == 1U,
              "open door surface filtered");

  ok = ok && submitAcceptedAndTick(session,
                                   moveTo(treasure->transform.position),
                                   "move to treasure cell");
  const iggy3d::SessionCommandResult rejectedExit = session.submitCommand(interact(exit->id));
  ok = ok &&
       expect(rejectedExit.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
              "exit rejected before treasure") &&
       expect(rejectedExit.command.rejection ==
                  iggy3d::CommandRejectionReason::RequiredItemMissing,
              "exit rejected because treasure missing") &&
       expect(!iggy3d::objectiveComplete(session.state().objectives,
                                         "exit_marker_exit_r1_c5"),
              "exit objective still incomplete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::None,
              "rejected exit keeps no outcome");

  ok = ok && submitAcceptedAndTick(session, interact(treasure->id), "treasure pickup") &&
       expect(itemCount(session, treasure->stableName) == 1U, "treasure item acquired") &&
       expect(iggy3d::objectiveComplete(session.state().objectives,
                                        "collect_marker_treasure_r1_c4"),
              "treasure objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::None,
              "treasure does not finish");

  ok = ok && submitAcceptedAndTick(session, interact(exit->id), "exit objective") &&
       expect(iggy3d::objectiveComplete(session.state().objectives,
                                        "exit_marker_exit_r1_c5"),
              "exit objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::Victory,
              "session victory") &&
       expect(session.state().lifecycle == iggy3d::SessionLifecycle::Playing,
              "session still playable after outcome proof");
  const bool npcLoop = runNpcCombatLoop();
  ok = ok && npcLoop;

  std::cout << "smoke=product_ascii_gameplay_loop\n";
  std::cout << "ascii_room_ready=" << (authored.ok ? "true" : "false") << "\n";
  std::cout << "package_seed_ready=" << (seed.ok ? "true" : "false") << "\n";
  std::cout << "session_created=true\n";
  std::cout << "key_collected="
            << (itemCount(session, "marker_key_r1_c2") == 1U ? "true" : "false") << "\n";
  std::cout << "secret_door_opened="
            << (!session.state().world.findById(secretDoor->id)->active ? "true" : "false")
            << "\n";
  std::cout << "secret_door_collision_filtered="
            << (openDoorCollision.activeDoorBlockerSurfaceCount == 0U ? "true" : "false")
            << "\n";
  std::cout << "treasure_collected="
            << (itemCount(session, "marker_treasure_r1_c4") == 1U ? "true" : "false")
            << "\n";
  std::cout << "exit_objective_complete="
            << (iggy3d::objectiveComplete(session.state().objectives,
                                          "exit_marker_exit_r1_c5")
                    ? "true"
                    : "false")
            << "\n";
  std::cout << "session_outcome=" << outcomeName(session.state().outcome) << "\n";
  std::cout << "final_tick=" << session.state().clock.tickIndex << "\n";
  std::cout << "command_count=" << session.state().commandLog.records().size() << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (ok ? "pass" : "fail") << "\n";
  std::cout << "reason_code="
            << (ok ? "ascii_gameplay_loop_pass" : "ascii_gameplay_loop_failed") << "\n";
  return ok;
}

}  // namespace

int main() {
  return runAsciiGameplayLoop() ? EXIT_SUCCESS : EXIT_FAILURE;
}
