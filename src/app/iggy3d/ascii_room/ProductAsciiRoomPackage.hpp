#pragma once

#include <string>
#include <string_view>

#include "content/PackageLoader.hpp"
#include "content/assets/RoomAsset.hpp"

namespace iggy3d {

std::string productAsciiRoomScenarioIdForRoom(std::string_view roomId);
PackageLoadResult makeProductAsciiRoomPackage(const RoomAsset& room,
                                              std::string_view packageId,
                                              std::string_view scenarioId);

}  // namespace iggy3d
