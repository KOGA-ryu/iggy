#include "app/iggy3d/ProductDungeonDraft.hpp"

#include <iostream>
#include <string_view>

#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ProductBuiltinDungeon.hpp"

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
                      paintingPlayerKeepsSingleSpawn() &&
                      invalidGlyphIsRejected();
  std::cout << "product_dungeon_draft_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}
