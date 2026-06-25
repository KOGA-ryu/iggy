#include "app/iggy3d/ProductAsciiRoomPackage.hpp"

namespace iggy3d {

std::string productAsciiRoomScenarioIdForRoom(std::string_view roomId) {
  if (roomId.empty()) {
    return "ascii_room_preview.runtime_loop";
  }
  return std::string(roomId) + ".runtime_loop";
}

PackageLoadResult makeProductAsciiRoomPackage(const RoomAsset& room,
                                              std::string_view packageId,
                                              std::string_view scenarioId) {
  PackageLoadResult package;
  package.status = PackageLoadStatus::Ok;
  package.manifest.packageId =
      packageId.empty() ? "iggy3d.ascii_room_authoring" : std::string(packageId);
  package.manifest.schemaVersion = 1;
  package.manifest.requiredRuntimeSchema = 1;
  package.manifest.scenarioPath =
      scenarioId.empty() ? "inline_ascii_room" : std::string(scenarioId);
  package.scenario.scenarioId =
      scenarioId.empty() ? productAsciiRoomScenarioIdForRoom(room.id)
                         : std::string(scenarioId);
  package.rooms.push_back(room);
  return package;
}

}  // namespace iggy3d
