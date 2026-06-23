#include "app/frontend/SaveSlotModel.hpp"
#include "content/PackageLoader.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_save_slot_model_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

iggy3d::Session makeFixtureSession() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({"fixtures/demos/movement_playground/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest request;
  request.packageId = package.manifest.packageId;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  return iggy3d::Session::create(request).value;
}

iggy3d::SaveAuthoredRoomSection authoredRoomFixture() {
  iggy3d::SaveAuthoredRoomSection authoredRoom;
  authoredRoom.present = true;
  iggy3d::SaveAuthoredRoomFloorRecord floor;
  floor.id = "floor_preview";
  authoredRoom.floors.push_back(floor);
  iggy3d::SaveAuthoredRoomWallRecord wall;
  wall.id = "wall_preview";
  authoredRoom.walls.push_back(wall);
  return authoredRoom;
}

bool compatibleSavePreviewIncludesMetadata() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveAuthoredRoomSection authoredRoom = authoredRoomFixture();
  iggy3d::SaveFileWriteRequest request;
  request.root = root;
  request.idHint = "save_010";
  request.state = &session.state();
  request.authoredRoom = &authoredRoom;
  const iggy3d::SaveFileWriteResult written = iggy3d::writeSessionSaveFile(request);
  const iggy3d::SaveSlotList slots = iggy3d::buildSaveSlotList(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");

  return expect(written.ok, "save written") && expect(slots.slots.size() == 1U, "one slot") &&
         expect(slots.compatibleCount == 1U, "compatible count") &&
         expect(slots.corruptCount == 0U, "corrupt count") &&
         expect(slots.slots.front().id == "save_010", "slot id") &&
         expect(slots.slots.front().enabled, "slot enabled") &&
         expect(slots.slots.front().packageId == "iggy3d.movement_playground",
                "package id") &&
         expect(slots.slots.front().scenarioId == "movement_playground.runtime_loop",
                "scenario id") &&
         expect(slots.slots.front().savedStateHashHex == written.record.savedStateHashHex,
                "hash hex") &&
         expect(slots.slots.front().authoredFloorCount == 1U, "authored floor count") &&
         expect(slots.slots.front().authoredWallCount == 1U, "authored wall count");
}

bool corruptSaveIsVisibleDisabledRow() {
  const std::filesystem::path root = testRoot();
  {
    std::ofstream output(root / "save_999.iggy3d.save");
    output << "not an iggy3d save\n";
  }
  const iggy3d::SaveSlotList slots = iggy3d::buildSaveSlotList(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(slots.slots.size() == 1U, "one corrupt slot") &&
         expect(slots.compatibleCount == 0U, "no compatible corrupt") &&
         expect(slots.corruptCount == 1U, "corrupt counted") &&
         expect(slots.slots.front().id == "save_999", "corrupt id") &&
         expect(!slots.slots.front().enabled, "corrupt disabled") &&
         expect(slots.slots.front().compatibility == iggy3d::SaveSlotCompatibility::DecodeFailed,
                "decode failed compatibility");
}

bool deterministicOrdering() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  for (std::string_view id : {"save_020", "save_001"}) {
    iggy3d::SaveFileWriteRequest request;
    request.root = root;
    request.idHint = std::string(id);
    request.state = &session.state();
    const iggy3d::SaveFileWriteResult written = iggy3d::writeSessionSaveFile(request);
    if (!written.ok) {
      return expect(false, "ordered save write");
    }
  }
  const iggy3d::SaveSlotList slots = iggy3d::buildSaveSlotList(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(slots.slots.size() == 2U, "two ordered slots") &&
         expect(slots.slots[0].id == "save_001", "save_001 first") &&
         expect(slots.slots[1].id == "save_020", "save_020 second");
}

}  // namespace

int main() {
  const bool ok = compatibleSavePreviewIncludesMetadata() &&
                  corruptSaveIsVisibleDisabledRow() && deterministicOrdering();
  return ok ? 0 : 1;
}
