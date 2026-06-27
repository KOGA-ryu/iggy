#include "app/iggy3d/ProductBuiltinDungeon.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include "app/iggy3d/ascii_room/Authoring.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
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

  return expect(catalog.size() == 3U, "catalog size") &&
         expect(startsOnLoopKeep, "default starts loop keep") &&
         expect(nextOk, "next select ok") &&
         expect(nextIsGatehouse, "next selects gatehouse") &&
         expect(previousOk, "previous select ok") &&
         expect(previousReturnsLoopKeep, "previous returns loop keep") &&
         expect(iggy3d::findProductBuiltinDungeonByRoomId(
                    "courtyard_vault_ascii") != nullptr,
                "find courtyard vault") &&
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

    iggy3d::ProductAsciiRoomAuthoringRequest request;
    request.sourceText = std::string{dungeon.asciiRoomText};
    request.roomId = std::string{dungeon.roomId};
    request.sourceName = std::string{dungeon.sourceName};

    const iggy3d::ProductAsciiRoomAuthoringResult result =
        iggy3d::buildProductAsciiRoomAuthoring(request);

    ok = expect(!fixtureText.empty(), "fixture readable") && ok;
    ok = expect(fixtureText == builtinText, "fixture text matches builtin") && ok;
    ok = expect(result.ok, "authoring ok") && ok;
    ok = expect(result.width > 0U, "width positive") && ok;
    ok = expect(result.height > 0U, "height positive") && ok;
    ok = expect(result.floorCount > 0U, "floor count positive") && ok;
    ok = expect(result.wallCount > 0U, "wall count positive") && ok;
    ok = expect(result.markerCount >= 3U, "marker count useful") && ok;
    ok = expect(result.anchorCount >= 3U, "anchor count useful") && ok;
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
         expect(result.markerCount == 5U, "marker count") &&
         expect(result.anchorCount == 5U, "anchor count");
}

}  // namespace

int main() {
  const bool passed = productDefaultDraftCreatesLoopKeepDungeon() &&
                      catalogExposesSelectableDungeons() &&
                      embeddedDungeonsMatchFixturesAndBuild() &&
                      loopKeepCountsRemainStable();
  std::cout << "product_builtin_dungeon_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}
