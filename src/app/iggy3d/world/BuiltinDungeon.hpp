#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"

namespace iggy3d {

struct ProductBuiltinDungeonDefinition {
  std::string_view worldTitle;
  std::string_view roomId;
  std::string_view sourceName;
  std::string_view asciiRoomText;
};

std::span<const ProductBuiltinDungeonDefinition> productBuiltinDungeonCatalog();
const ProductBuiltinDungeonDefinition& productDefaultBuiltinDungeon();
const ProductBuiltinDungeonDefinition* findProductBuiltinDungeonByRoomId(
    std::string_view roomId);
std::size_t productBuiltinDungeonIndexForRoomId(std::string_view roomId);

std::string_view productBuiltinDungeonWorldTitle();
std::string_view productBuiltinDungeonRoomId();
std::string_view productBuiltinDungeonSourceName();
std::string_view productBuiltinDungeonAsciiRoomText();

ProductAsciiRoomAuthoringRequest productBuiltinDungeonAuthoringRequest(
    const ProductBuiltinDungeonDefinition& dungeon);
ProductAsciiRoomAuthoringRequest productWorldSetupAuthoringRequest(
    const WorldSetupDraft& draft);

bool applyProductBuiltinDungeonToDraft(std::size_t index,
                                       WorldSetupDraft& draft);
bool selectNextProductBuiltinDungeon(WorldSetupDraft& draft);
bool selectPreviousProductBuiltinDungeon(WorldSetupDraft& draft);

WorldSetupDraft makeProductDefaultWorldSetupDraft(
    std::string_view generatedSeedText = "new_world_seed");

}  // namespace iggy3d
