#pragma once

#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"

namespace iggy3d {

std::string_view productBuiltinDungeonWorldTitle();
std::string_view productBuiltinDungeonRoomId();
std::string_view productBuiltinDungeonSourceName();
std::string_view productBuiltinDungeonAsciiRoomText();

WorldSetupDraft makeProductDefaultWorldSetupDraft(
    std::string_view generatedSeedText = "new_world_seed");

}  // namespace iggy3d
