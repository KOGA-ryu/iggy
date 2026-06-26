#include "app/iggy3d/ProductBuiltinDungeon.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"

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

bool embeddedDungeonMatchesFixture() {
  const std::filesystem::path fixture{
      iggy3d::productBuiltinDungeonSourceName()};
  const std::string fixtureText = normalizeLineEndings(readTextFile(fixture));
  const std::string builtinText = normalizeLineEndings(
      std::string{iggy3d::productBuiltinDungeonAsciiRoomText()});
  return expect(!fixtureText.empty(), "fixture readable") &&
         expect(fixtureText == builtinText, "fixture text matches builtin");
}

bool embeddedDungeonBuildsAuthoredRoom() {
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
                      embeddedDungeonMatchesFixture() &&
                      embeddedDungeonBuildsAuthoredRoom();
  std::cout << "product_builtin_dungeon_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}
