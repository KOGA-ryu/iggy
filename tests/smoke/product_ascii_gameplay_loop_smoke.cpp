#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"
#include "app/iggy3d/ProductPackageSessionSeed.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/session/Session.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

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

  iggy3d::SessionCreateRequest create;
  create.packageId = "iggy3d.ascii_gameplay_loop";
  create.config = seed.seed.config;
  create.seed = seed.seed;
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(create);
  if (!expect(created.status == iggy3d::ResultStatus::Ok, "session create ok")) {
    return false;
  }

  iggy3d::Session session = std::move(created.value);
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
  ok = ok && submitAcceptedAndTick(session, interact(secretDoor->id), "open secret door") &&
       expect(!session.state().world.findById(secretDoor->id)->active,
              "secret door opened");

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

  std::cout << "smoke=product_ascii_gameplay_loop\n";
  std::cout << "ascii_room_ready=" << (authored.ok ? "true" : "false") << "\n";
  std::cout << "package_seed_ready=" << (seed.ok ? "true" : "false") << "\n";
  std::cout << "session_created=true\n";
  std::cout << "key_collected="
            << (itemCount(session, "marker_key_r1_c2") == 1U ? "true" : "false") << "\n";
  std::cout << "secret_door_opened="
            << (!session.state().world.findById(secretDoor->id)->active ? "true" : "false")
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
