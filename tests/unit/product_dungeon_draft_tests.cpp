#include "app/iggy3d/world/DungeonDraft.hpp"

#include <iostream>
#include <string_view>

#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool cursorMovementClampsToDraftBounds() {
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_cursor");
  iggy3d::ProductDungeonDraftCursor cursor{1U, 1U};
  const iggy3d::ProductDungeonDraftOperationResult left =
      iggy3d::moveProductDungeonDraftCursor(
          draft, cursor, iggy3d::ProductDungeonDraftDirection::Left);
  const iggy3d::ProductDungeonDraftOperationResult up =
      iggy3d::moveProductDungeonDraftCursor(
          draft, left.cursor, iggy3d::ProductDungeonDraftDirection::Up);

  return expect(left.ok, "left move ok") &&
         expect(left.cursor.row == 1U, "left row") &&
         expect(left.cursor.column == 0U, "left column") &&
         expect(up.ok, "up move ok") &&
         expect(up.cursor.row == 0U, "up row") &&
         expect(up.cursor.column == 0U, "up column");
}

bool paintingMarksDraftCustomAndBuilds() {
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_paint");
  const iggy3d::ProductDungeonDraftOperationResult painted =
      iggy3d::setProductDungeonDraftCell(draft, 1U, 2U, '#');

  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = draft.asciiRoomText;
  request.roomId = draft.asciiRoomId;
  request.sourceName = draft.asciiRoomSourceName;
  const iggy3d::ProductAsciiRoomAuthoringResult authored =
      iggy3d::buildProductAsciiRoomAuthoring(request);

  return expect(painted.ok, "paint ok") &&
         expect(painted.modified, "paint modified") &&
         expect(painted.glyph == '#', "paint glyph") &&
         expect(draft.asciiRoomId == "custom_dungeon_draft", "custom room id") &&
         expect(draft.asciiRoomSourceName == "custom_dungeon_draft.iggyroom.txt",
                "custom source") &&
         expect(authored.ok, "authored ok") &&
         expect(authored.wallCount == 61U, "wall count changed");
}

bool paintingCrateMarksDraftCustomAndBuildsObject() {
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_crate");
  const iggy3d::ProductDungeonDraftOperationResult painted =
      iggy3d::setProductDungeonDraftCell(draft, 1U, 2U, 'C');

  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = draft.asciiRoomText;
  request.roomId = draft.asciiRoomId;
  request.sourceName = draft.asciiRoomSourceName;
  const iggy3d::ProductAsciiRoomAuthoringResult authored =
      iggy3d::buildProductAsciiRoomAuthoring(request);

  return expect(iggy3d::isProductDungeonDraftGlyph('C'), "crate glyph valid") &&
         expect(painted.ok, "paint crate ok") &&
         expect(painted.modified, "paint crate modified") &&
         expect(painted.glyph == 'C', "paint crate glyph") &&
         expect(draft.asciiRoomId == "custom_dungeon_draft",
                "crate custom room id") &&
         expect(authored.ok, "crate authored ok") &&
         expect(authored.floorCount == 59U, "crate keeps floor count") &&
         expect(authored.objectCount == 1U, "crate object count");
}

bool paintingLedgeMarksDraftCustomAndBuildsClamberObject() {
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_ledge");
  const iggy3d::ProductDungeonDraftOperationResult painted =
      iggy3d::setProductDungeonDraftCell(draft, 1U, 2U, 'L');

  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = draft.asciiRoomText;
  request.roomId = draft.asciiRoomId;
  request.sourceName = draft.asciiRoomSourceName;
  const iggy3d::ProductAsciiRoomAuthoringResult authored =
      iggy3d::buildProductAsciiRoomAuthoring(request);

  return expect(iggy3d::isProductDungeonDraftGlyph('L'), "ledge glyph valid") &&
         expect(painted.ok, "paint ledge ok") &&
         expect(painted.modified, "paint ledge modified") &&
         expect(painted.glyph == 'L', "paint ledge glyph") &&
         expect(draft.asciiRoomId == "custom_dungeon_draft",
                "ledge custom room id") &&
         expect(authored.ok, "ledge authored ok") &&
         expect(authored.floorCount == 59U, "ledge keeps floor count") &&
         expect(authored.objectCount == 1U, "ledge object count");
}

bool paintingPlayerKeepsSingleSpawn() {
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_player");
  const iggy3d::ProductDungeonDraftOperationResult painted =
      iggy3d::setProductDungeonDraftCell(draft, 1U, 2U, 'P');
  std::size_t count = 0;
  for (const char ch : draft.asciiRoomText) {
    if (ch == 'P') {
      ++count;
    }
  }
  return expect(painted.ok, "paint player ok") &&
         expect(count == 1U, "single player spawn");
}

bool invalidGlyphIsRejected() {
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_invalid");
  const iggy3d::ProductDungeonDraftOperationResult painted =
      iggy3d::setProductDungeonDraftCell(draft, 1U, 2U, 'N');
  return expect(!painted.ok, "invalid rejected") &&
         expect(painted.reasonCode == "dungeon_draft_invalid_glyph",
                "invalid reason") &&
         expect(draft.asciiRoomId == "loop_keep_ascii",
                "invalid keeps original id");
}

}  // namespace

int main() {
  const bool passed = cursorMovementClampsToDraftBounds() &&
                      paintingMarksDraftCustomAndBuilds() &&
                      paintingCrateMarksDraftCustomAndBuildsObject() &&
                      paintingLedgeMarksDraftCustomAndBuildsClamberObject() &&
                      paintingPlayerKeepsSingleSpawn() &&
                      invalidGlyphIsRejected();
  std::cout << "product_dungeon_draft_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}
