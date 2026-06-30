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

constexpr std::string_view kLargeFlatRoomText =
    "###############################\n"
    "#P...........................E#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "#.............................#\n"
    "###############################\n";

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

constexpr std::string_view kMovementGymText =
    "*10\n"
    "#############################################\n"
    "#P.CK.............#............J..........E.#\n"
    "#.................#............J............#\n"
    "#....########.....#...11111....J...22222....#\n"
    "#....#......#.....#...11111....J...22222....#\n"
    "#....#......#.....#...111L1....J...22L22....#\n"
    "#....#......#.....#...11111....J...22222....#\n"
    "#....########......##########..J..###########\n"
    "#.......................C...................#\n"
    "#......^^^^^^^^^...............vvvvvvvvv....#\n"
    "#......111111111.....#######...333333333....#\n"
    "#......111111111.....#.....#...333333333....#\n"
    "#......>>>>>>>>1.....#..C..#...3<<<<<<33....#\n"
    "#......000000000.....#.....#...333333333....#\n"
    "#....................#######................#\n"
    "#....L.............................L........#\n"
    "#.......###########.C.......###########.....#\n"
    "#.......#...$.....#.........#...K.....#.....#\n"
    "#.......#.........#.........#.........#.....#\n"
    "#.......###########.........###########.....#\n"
    "#.........C.......!!!!!!!!!.......C.........#\n"
    "#...........................................#\n"
    "#############################################\n";

constexpr std::string_view kMovementWallRunCorridorText =
    "*5\n"
    "########################\n"
    "#P....................E#\n"
    "#......................#\n"
    "#......................#\n"
    "#......................#\n"
    "########################\n";

constexpr std::string_view kSlopeGymText =
    "*5\n"
    "###################\n"
    "#P....>>>>1111...E#\n"
    "#.....>>>>1111....#\n"
    "#....^^^^1111.....#\n"
    "#....0000vvvv.....#\n"
    "#....2222<<<<.....#\n"
    "#....2222<<<<..K..#\n"
    "#.................#\n"
    "###################\n";

constexpr std::string_view kLayeredJumpGymText =
    "floor1\n"
    "P..RR.......\n"
    "............\n"
    "............\n"
    "...RR.......\n"
    "...RR.......\n"
    "............\n"
    "............\n"
    ".......RR...\n"
    "floor2\n"
    "...  .......\n"
    "...RR.......\n"
    "...RR.......\n"
    "...  .......\n"
    "...  .......\n"
    "......RR....\n"
    "............\n"
    ".......  ...\n"
    "floor3\n"
    "............\n"
    "...  .......\n"
    "...  .......\n"
    "............\n"
    "............\n"
    "......  ....\n"
    "............\n"
    "..........E.\n";

constexpr std::array<ProductBuiltinDungeonDefinition, 12> kCatalog = {{
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
        "Large Flat Room",
        "large_flat_room",
        "fixtures/rooms/ascii/large_flat_room.iggyroom.txt",
        kLargeFlatRoomText,
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
    ProductBuiltinDungeonDefinition{
        "Movement Gym",
        "movement_gym",
        "fixtures/rooms/ascii/movement_gym.iggyroom.txt",
        kMovementGymText,
    },
    ProductBuiltinDungeonDefinition{
        "Wall Run Corridor",
        "movement_wall_run_corridor",
        "fixtures/rooms/ascii/movement_wall_run_corridor.iggyroom.txt",
        kMovementWallRunCorridorText,
    },
    ProductBuiltinDungeonDefinition{
        "Slope Gym",
        "slope_gym",
        "fixtures/rooms/ascii/slope_gym.iggyroom.txt",
        kSlopeGymText,
    },
    ProductBuiltinDungeonDefinition{
        "Layered Jump Gym",
        "layered_jump_gym",
        "fixtures/rooms/ascii/layered_jump_gym.iggyroom.txt",
        kLayeredJumpGymText,
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
