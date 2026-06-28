#include "app/iggy3d/world/BuiltinDungeon.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace iggy3d {
namespace {

constexpr std::string_view kLoopKeepText =
    "#################\n"
    "#P.....#.......E#\n"
    "#.###..#..###...#\n"
    "#...K..+..$.....#\n"
    "#.###..#..###...#\n"
    "#.......#.......#\n"
    "#################\n";

constexpr std::string_view kGatehouseText =
    "###########\n"
    "#P...+...E#\n"
    "#.###.###.#\n"
    "#...K.$...#\n"
    "#.###.###.#\n"
    "#.........#\n"
    "###########\n";

constexpr std::string_view kCourtyardVaultText =
    "#############\n"
    "#P....#....E#\n"
    "#.##..+..##.#\n"
    "#.....$.....#\n"
    "#.##.....##.#\n"
    "#....K......#\n"
    "#############\n";

constexpr std::string_view kPhysicsFlatRoomText =
    "#######\n"
    "#P.K.E#\n"
    "#.....#\n"
    "#.....#\n"
    "#######\n";

constexpr std::string_view kPhysicsWallCorridorText =
    "#########\n"
    "#PK....E#\n"
    "#########\n"
    "#K......#\n"
    "#########\n";

constexpr std::string_view kPhysicsCornerSlideText =
    "########\n"
    "#P....E#\n"
    "#####..#\n"
    "#..K...#\n"
    "########\n";

constexpr std::string_view kObjectCrateRoomText =
    "#######\n"
    "#P.CE.#\n"
    "#.....#\n"
    "#.....#\n"
    "#######\n";

constexpr std::array<ProductBuiltinDungeonDefinition, 7> kCatalog = {{
    ProductBuiltinDungeonDefinition{
        "Loop Keep",
        "loop_keep_ascii",
        "fixtures/rooms/ascii/loop_keep.iggyroom.txt",
        kLoopKeepText,
    },
    ProductBuiltinDungeonDefinition{
        "Gatehouse",
        "gatehouse_ascii",
        "fixtures/rooms/ascii/gatehouse.iggyroom.txt",
        kGatehouseText,
    },
    ProductBuiltinDungeonDefinition{
        "Courtyard Vault",
        "courtyard_vault_ascii",
        "fixtures/rooms/ascii/courtyard_vault.iggyroom.txt",
        kCourtyardVaultText,
    },
    ProductBuiltinDungeonDefinition{
        "Physics Flat Room",
        "physics_flat_room",
        "fixtures/rooms/ascii/physics_flat_room.iggyroom.txt",
        kPhysicsFlatRoomText,
    },
    ProductBuiltinDungeonDefinition{
        "Physics Wall Corridor",
        "physics_wall_corridor",
        "fixtures/rooms/ascii/physics_wall_corridor.iggyroom.txt",
        kPhysicsWallCorridorText,
    },
    ProductBuiltinDungeonDefinition{
        "Physics Corner Slide",
        "physics_corner_slide",
        "fixtures/rooms/ascii/physics_corner_slide.iggyroom.txt",
        kPhysicsCornerSlideText,
    },
    ProductBuiltinDungeonDefinition{
        "Object Crate Room",
        "object_crate_room",
        "fixtures/rooms/ascii/object_crate_room.iggyroom.txt",
        kObjectCrateRoomText,
    },
}};

constexpr std::size_t kNotFound = static_cast<std::size_t>(-1);

const ProductBuiltinDungeonDefinition& dungeonAtOrDefault(std::size_t index) {
  if (index < kCatalog.size()) {
    return kCatalog[index];
  }
  return kCatalog.front();
}

}  // namespace

std::span<const ProductBuiltinDungeonDefinition> productBuiltinDungeonCatalog() {
  return kCatalog;
}

const ProductBuiltinDungeonDefinition& productDefaultBuiltinDungeon() {
  return kCatalog.front();
}

const ProductBuiltinDungeonDefinition* findProductBuiltinDungeonByRoomId(
    std::string_view roomId) {
  for (const ProductBuiltinDungeonDefinition& dungeon : kCatalog) {
    if (dungeon.roomId == roomId) {
      return &dungeon;
    }
  }
  return nullptr;
}

std::size_t productBuiltinDungeonIndexForRoomId(std::string_view roomId) {
  for (std::size_t index = 0; index < kCatalog.size(); ++index) {
    if (kCatalog[index].roomId == roomId) {
      return index;
    }
  }
  return kNotFound;
}

std::string_view productBuiltinDungeonWorldTitle() {
  return productDefaultBuiltinDungeon().worldTitle;
}

std::string_view productBuiltinDungeonRoomId() {
  return productDefaultBuiltinDungeon().roomId;
}

std::string_view productBuiltinDungeonSourceName() {
  return productDefaultBuiltinDungeon().sourceName;
}

std::string_view productBuiltinDungeonAsciiRoomText() {
  return productDefaultBuiltinDungeon().asciiRoomText;
}

bool applyProductBuiltinDungeonToDraft(std::size_t index,
                                       WorldSetupDraft& draft) {
  if (index >= kCatalog.size()) {
    return false;
  }
  const ProductBuiltinDungeonDefinition& dungeon = dungeonAtOrDefault(index);
  draft.worldName = std::string(dungeon.worldTitle);
  draft.asciiRoomEnabled = true;
  draft.asciiRoomText = std::string(dungeon.asciiRoomText);
  draft.asciiRoomId = std::string(dungeon.roomId);
  draft.asciiRoomSourceName = std::string(dungeon.sourceName);
  draft.selectedField = WorldSetupField::Create;
  return true;
}

bool selectNextProductBuiltinDungeon(WorldSetupDraft& draft) {
  const std::size_t current = productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  const std::size_t next = current == kNotFound ? 0U : (current + 1U) % kCatalog.size();
  return applyProductBuiltinDungeonToDraft(next, draft);
}

bool selectPreviousProductBuiltinDungeon(WorldSetupDraft& draft) {
  const std::size_t current = productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  const std::size_t previous =
      current == kNotFound || current == 0U ? kCatalog.size() - 1U : current - 1U;
  return applyProductBuiltinDungeonToDraft(previous, draft);
}

WorldSetupDraft makeProductDefaultWorldSetupDraft(
    std::string_view generatedSeedText) {
  WorldSetupDraft draft = makeDefaultWorldSetupDraft(generatedSeedText);
  applyProductBuiltinDungeonToDraft(0U, draft);
  return draft;
}

}  // namespace iggy3d
