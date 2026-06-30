#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"

namespace iggy3d {

struct ProductDungeonDraftCursor {
  std::size_t row = 0;
  std::size_t column = 0;
};

enum class ProductDungeonDraftDirection : std::uint8_t {
  Up,
  Down,
  Left,
  Right,
};

struct ProductDungeonDraftOperationResult {
  bool ok = false;
  bool modified = false;
  std::string_view status = "dungeon_draft_not_requested";
  std::string_view reasonCode = "dungeon_draft_not_requested";
  ProductDungeonDraftCursor cursor;
  char glyph = '\0';
};

std::string_view productCustomDungeonRoomId();
bool isProductDungeonDraftGlyph(char glyph);
std::size_t productDungeonDraftRowCount(std::string_view asciiRoomText);
std::size_t productDungeonDraftColumnCount(std::string_view asciiRoomText);
ProductDungeonDraftCursor clampProductDungeonDraftCursor(
    const WorldSetupDraft& draft,
    ProductDungeonDraftCursor cursor);
ProductDungeonDraftOperationResult moveProductDungeonDraftCursor(
    const WorldSetupDraft& draft,
    ProductDungeonDraftCursor cursor,
    ProductDungeonDraftDirection direction);
ProductDungeonDraftOperationResult paintProductDungeonDraftCell(
    WorldSetupDraft& draft,
    ProductDungeonDraftCursor cursor,
    char glyph);
ProductDungeonDraftOperationResult setProductDungeonDraftCell(
    WorldSetupDraft& draft,
    std::size_t row,
    std::size_t column,
    char glyph);

}  // namespace iggy3d
