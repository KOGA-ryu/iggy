#include "app/iggy3d/world/BuiltinDungeon.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool expectEqual(std::uint64_t actual,
                 std::uint64_t expected,
                 std::string_view roomId,
                 std::string_view field) {
  if (actual != expected) {
    std::cerr << "FAIL: " << roomId << ' ' << field << " expected "
              << expected << " actual " << actual << '\n';
    return false;
  }
  return true;
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
}

std::string normalizeLineEndings(std::string text) {
  std::string normalized;
  normalized.reserve(text.size());
  for (const char ch : text) {
    if (ch != '\r') {
      normalized.push_back(ch);
    }
  }
  if (!normalized.empty() && normalized.back() != '\n') {
    normalized.push_back('\n');
  }
  return normalized;
}

struct ExpectedDungeonCounts {
  std::string_view roomId;
  std::uint64_t width = 0U;
  std::uint64_t height = 0U;
  std::uint64_t floorCount = 0U;
  std::uint64_t wallCount = 0U;
  std::uint64_t objectCount = 0U;
  std::uint64_t markerCount = 0U;
  std::uint64_t spatialSurfaceCount = 0U;
  std::uint64_t walkableSurfaceCount = 0U;
  std::uint64_t actorBlockerSurfaceCount = 0U;
  std::uint64_t projectileBlockerSurfaceCount = 0U;
};

constexpr ExpectedDungeonCounts kExpectedDungeons[] = {
    {"loop_keep_ascii", 17U, 7U, 59U, 60U, 0U, 5U, 180U, 59U, 61U, 61U},
    {"gatehouse_ascii", 11U, 7U, 33U, 44U, 0U, 5U, 122U, 33U, 45U, 45U},
    {"courtyard_vault_ascii", 13U, 7U, 46U, 45U, 0U, 5U, 137U, 46U, 46U, 46U},
    {"physics_flat_room", 7U, 5U, 15U, 20U, 0U, 3U, 55U, 15U, 20U, 20U},
    {"large_flat_room", 31U, 17U, 435U, 92U, 0U, 2U, 619U, 435U, 92U, 92U},
    {"physics_wall_corridor", 9U, 5U, 14U, 31U, 0U, 4U, 76U, 14U, 31U, 31U},
    {"physics_corner_slide", 8U, 5U, 14U, 26U, 0U, 3U, 66U, 14U, 26U, 26U},
    {"object_crate_room", 7U, 5U, 15U, 20U, 1U, 2U, 57U, 15U, 21U, 21U},
};

const ExpectedDungeonCounts* expectedCountsFor(std::string_view roomId) {
  for (const ExpectedDungeonCounts& expected : kExpectedDungeons) {
    if (expected.roomId == roomId) {
      return &expected;
    }
  }
  return nullptr;
}

bool startsWith(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0U, prefix.size()) == prefix;
}

iggy3d::ProductAsciiRoomAuthoringResult buildDungeonAuthoring(
    const iggy3d::ProductBuiltinDungeonDefinition& dungeon) {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string{dungeon.asciiRoomText};
  request.roomId = std::string{dungeon.roomId};
  request.sourceName = std::string{dungeon.sourceName};
  return iggy3d::buildProductAsciiRoomAuthoring(request);
}

iggy3d::ProductActiveRoomState buildDungeonActiveRoom(std::string_view roomId) {
  const iggy3d::ProductBuiltinDungeonDefinition* dungeon =
      iggy3d::findProductBuiltinDungeonByRoomId(roomId);
  if (dungeon == nullptr) {
    return {};
  }

  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string{dungeon->asciiRoomText};
  request.roomId = std::string{dungeon->roomId};
  request.sourceName = std::string{dungeon->sourceName};
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  return iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);
}

iggy3d::PlayerPhysicsMovePlannerResult planMoveInRoom(
    std::string_view roomId,
    iggy3d::Vec3 startCenterMeters,
    iggy3d::Vec3 desiredDisplacementMeters) {
  const iggy3d::ProductActiveRoomState active =
      buildDungeonActiveRoom(roomId);
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(collision);

  iggy3d::PlayerPhysicsMovePlannerRequest request;
  request.collisionSurfaces = surfaces;
  request.startCenterMeters = startCenterMeters;
  request.desiredDisplacementMeters = desiredDisplacementMeters;
  request.config.motor.skinMeters = 0.05F;
  request.config.motor.groundProbeDistanceMeters = 0.0F;
  request.config.motor.groundSnapDistanceMeters = 0.0F;
  request.config.motor.maxMoveDistanceMeters = 10.0F;
  return iggy3d::planPlayerPhysicsMove(request);
}

bool productDefaultDraftCreatesLoopKeepDungeon() {
  const iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_loop_keep");
  const iggy3d::WorldSetupValidation validation =
      iggy3d::validateWorldSetupDraft(draft);

  return expect(draft.worldName == "Loop Keep", "world title") &&
         expect(draft.seedText == "seed_loop_keep", "seed preserved") &&
         expect(draft.asciiRoomEnabled, "ascii room enabled") &&
         expect(draft.asciiRoomId == "loop_keep_ascii", "room id") &&
         expect(draft.asciiRoomSourceName ==
                    "fixtures/rooms/ascii/loop_keep.iggyroom.txt",
                "source name") &&
         expect(!draft.asciiRoomText.empty(), "room text present") &&
         expect(draft.selectedField == iggy3d::WorldSetupField::Create,
                "selected create") &&
         expect(validation.valid, "draft validates") &&
         expect(validation.reasonCode == "ok", "validation reason");
}

bool catalogExposesSelectableDungeons() {
  const auto catalog = iggy3d::productBuiltinDungeonCatalog();
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_selector");
  const bool startsOnLoopKeep =
      draft.asciiRoomId == "loop_keep_ascii" && draft.worldName == "Loop Keep";

  const bool nextOk = iggy3d::selectNextProductBuiltinDungeon(draft);
  const bool nextIsGatehouse =
      draft.asciiRoomId == "gatehouse_ascii" && draft.worldName == "Gatehouse";

  const bool previousOk = iggy3d::selectPreviousProductBuiltinDungeon(draft);
  const bool previousReturnsLoopKeep =
      draft.asciiRoomId == "loop_keep_ascii" && draft.worldName == "Loop Keep";

  return expect(catalog.size() == 8U, "catalog size") &&
         expect(startsOnLoopKeep, "default starts loop keep") &&
         expect(nextOk, "next select ok") &&
         expect(nextIsGatehouse, "next selects gatehouse") &&
         expect(previousOk, "previous select ok") &&
         expect(previousReturnsLoopKeep, "previous returns loop keep") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId(
                    "courtyard_vault_ascii") != nullptr,
                "find courtyard vault") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId(
                    "physics_flat_room") != nullptr,
                "find physics flat room") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId(
                    "large_flat_room") != nullptr,
                "find large flat room") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId(
                    "physics_wall_corridor") != nullptr,
                "find physics wall corridor") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId(
                    "physics_corner_slide") != nullptr,
                "find physics corner slide") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId(
                    "object_crate_room") != nullptr,
                "find object crate room") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId("missing") == nullptr,
                "missing dungeon absent");
}

bool embeddedDungeonsMatchFixturesAndBuild() {
  bool ok = true;
  for (const iggy3d::ProductBuiltinDungeonDefinition& dungeon :
       iggy3d::productBuiltinDungeonCatalog()) {
    const std::filesystem::path fixture{dungeon.sourceName};
    const std::string fixtureText = normalizeLineEndings(readTextFile(fixture));
    const std::string builtinText =
        normalizeLineEndings(std::string{dungeon.asciiRoomText});

    const iggy3d::ProductAsciiRoomAuthoringResult result =
        buildDungeonAuthoring(dungeon);
    const ExpectedDungeonCounts* expected =
        expectedCountsFor(dungeon.roomId);

    ok = expect(!fixtureText.empty(), "fixture readable") && ok;
    ok = expect(fixtureText == builtinText, "fixture text matches builtin") && ok;
    ok = expect(expected != nullptr, "expected counts exist") && ok;
    ok = expect(result.ok, "authoring ok") && ok;
    if (expected != nullptr) {
      ok = expectEqual(result.width, expected->width, dungeon.roomId, "width") &&
           ok;
      ok = expectEqual(result.height, expected->height, dungeon.roomId, "height") &&
           ok;
      ok = expectEqual(result.floorCount,
                       expected->floorCount,
                       dungeon.roomId,
                       "floor count") &&
           ok;
      ok = expectEqual(result.wallCount,
                       expected->wallCount,
                       dungeon.roomId,
                       "wall count") &&
           ok;
      ok = expectEqual(result.objectCount,
                       expected->objectCount,
                       dungeon.roomId,
                       "object count") &&
           ok;
      ok = expectEqual(result.markerCount,
                       expected->markerCount,
                       dungeon.roomId,
                       "marker count") &&
           ok;
      ok = expectEqual(result.anchorCount,
                       expected->markerCount,
                       dungeon.roomId,
                       "anchor count") &&
           ok;
    }
  }
  return ok;
}

bool loopKeepCountsRemainStable() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string{iggy3d::productBuiltinDungeonAsciiRoomText()};
  request.roomId = std::string{iggy3d::productBuiltinDungeonRoomId()};
  request.sourceName = std::string{iggy3d::productBuiltinDungeonSourceName()};

  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  return expect(result.ok, "authoring ok") &&
         expect(result.width == 17U, "width") &&
         expect(result.height == 7U, "height") &&
         expect(result.floorCount == 59U, "floor count") &&
         expect(result.wallCount == 60U, "wall count") &&
         expect(result.objectCount == 0U, "object count") &&
         expect(result.markerCount == 5U, "marker count") &&
         expect(result.anchorCount == 5U, "anchor count");
}

bool physicsRoomsBuildCollisionSurfaces() {
  bool ok = true;
  for (const ExpectedDungeonCounts& expected : kExpectedDungeons) {
    if (!startsWith(expected.roomId, "physics_")) {
      continue;
    }

    const iggy3d::ProductActiveRoomState active =
        buildDungeonActiveRoom(expected.roomId);
    const iggy3d::ProductActiveRoomCollisionState collision =
        iggy3d::buildProductActiveRoomCollision(active);
    const iggy3d::SpatialSurfaceSet* surfaces =
        iggy3d::productActiveRoomCollisionSurfaces(collision);

    ok = expect(active.loaded, "physics active room loaded") && ok;
    ok = expect(active.roomId == expected.roomId, "physics active room id") && ok;
    ok = expect(active.authoredFloorCount == expected.floorCount,
                "physics authored floor count") &&
         ok;
    ok = expect(active.authoredWallCount == expected.wallCount,
                "physics authored wall count") &&
         ok;
    ok = expect(active.authoredObjectCount == expected.objectCount,
                "physics authored object count") &&
         ok;
    ok = expect(collision.ready, "physics collision ready") && ok;
    ok = expect(collision.roomId == expected.roomId,
                "physics collision room id") &&
         ok;
    ok = expect(collision.spatialSurfaceCount ==
                    expected.spatialSurfaceCount,
                "physics spatial surface count") &&
         ok;
    ok = expect(collision.querySurfaceCount == expected.spatialSurfaceCount,
                "physics query surface count") &&
         ok;
    ok = expect(collision.walkableSurfaceCount ==
                    expected.walkableSurfaceCount,
                "physics walkable surface count") &&
         ok;
    ok = expect(collision.actorBlockerSurfaceCount ==
                    expected.actorBlockerSurfaceCount,
                "physics actor blocker count") &&
         ok;
    ok = expect(collision.projectileBlockerSurfaceCount ==
                    expected.projectileBlockerSurfaceCount,
                "physics projectile blocker count") &&
         ok;
    ok = expect(collision.runtimeOwnedSurfaceCount == 0U,
                "physics runtime owned surface count") &&
         ok;
    ok = expect(surfaces != nullptr, "physics collision pointer") && ok;
  }
  return ok;
}

std::size_t countMeshesWithRole(const iggy3d::RoomAsset& room,
                                std::string_view role) {
  std::size_t count = 0;
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

bool objectCrateRoomBuildsVisiblePropAndCollision() {
  const ExpectedDungeonCounts* expected = expectedCountsFor("object_crate_room");
  const iggy3d::ProductActiveRoomState active =
      buildDungeonActiveRoom("object_crate_room");
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(collision);

  return expect(expected != nullptr, "object room expected counts") &&
         expect(active.loaded, "object active room loaded") &&
         expect(active.roomId == "object_crate_room", "object active room id") &&
         expect(active.authoredFloorCount == expected->floorCount,
                "object authored floor count") &&
         expect(active.authoredWallCount == expected->wallCount,
                "object authored wall count") &&
         expect(active.authoredObjectCount == 1U,
                "object authored object count") &&
         expect(active.authoredMarkerCount == expected->markerCount,
                "object authored marker count") &&
         expect(active.staticMeshCount == 36U, "object static mesh count") &&
         expect(countMeshesWithRole(active.room, "prop") == 1U,
                "object prop mesh count") &&
         expect(collision.ready, "object collision ready") &&
         expect(collision.querySurfaceCount == expected->spatialSurfaceCount,
                "object collision surface count") &&
         expect(collision.walkableSurfaceCount ==
                    expected->walkableSurfaceCount,
                "object walkable surface count") &&
         expect(collision.actorBlockerSurfaceCount ==
                    expected->actorBlockerSurfaceCount,
                "object actor blocker count") &&
         expect(collision.projectileBlockerSurfaceCount ==
                    expected->projectileBlockerSurfaceCount,
                "object projectile blocker count") &&
         expect(surfaces != nullptr, "object collision pointer");
}

bool physicsPlannerUsesTestRooms() {
  const iggy3d::PlayerPhysicsMovePlannerResult flat =
      planMoveInRoom("physics_flat_room",
                     {-2.0F, 0.9F, -1.0F},
                     {2.0F, 0.0F, 0.0F});
  const iggy3d::PlayerPhysicsMovePlannerResult corridor =
      planMoveInRoom("physics_wall_corridor",
                     {-3.0F, 0.9F, 0.0F},
                     {0.0F, 0.0F, -1.0F});
  const iggy3d::PlayerPhysicsMovePlannerResult corner =
      planMoveInRoom("physics_corner_slide",
                     {-2.5F, 0.9F, -1.0F},
                     {4.0F, 0.0F, 2.0F});

  return expect(flat.ok, "flat planner ok") &&
         expect(flat.bakedColliderCount > 0U, "flat baked colliders") &&
         expect(flat.hitCount == 0U, "flat no hits") &&
         expect(!flat.blocked, "flat not blocked") &&
         expect(corridor.ok, "corridor planner ok") &&
         expect(corridor.hitCount > 0U, "corridor wall hit") &&
         expect(corridor.blocked, "corridor blocked") &&
         expect(!corridor.firstHitSourceSurfaceId.empty(),
                "corridor hit source id") &&
         expect(corner.ok, "corner planner ok") &&
         expect(corner.hitCount > 0U, "corner wall hit") &&
         expect(corner.iterationCount > 0U, "corner planner iterations") &&
         expect(!corner.firstHitSourceSurfaceId.empty(),
                "corner hit source id");
}

}  // namespace

int main() {
  const bool passed = productDefaultDraftCreatesLoopKeepDungeon() &&
                      catalogExposesSelectableDungeons() &&
                      embeddedDungeonsMatchFixturesAndBuild() &&
                      loopKeepCountsRemainStable() &&
                      physicsRoomsBuildCollisionSurfaces() &&
                      objectCrateRoomBuildsVisiblePropAndCollision() &&
                      physicsPlannerUsesTestRooms();
  std::cout << "product_builtin_dungeon_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}
