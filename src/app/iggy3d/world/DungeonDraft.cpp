#include "app/iggy3d/world/DungeonDraft.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace iggy3d {
namespace {

constexpr std::string_view kCustomRoomId = "custom_dungeon_draft";
constexpr std::string_view kCustomSourceName = "custom_dungeon_draft.iggyroom.txt";

std::vector<std::string> splitRows(std::string_view text) {
  std::vector<std::string> rows;
  std::size_t rowStart = 0;
  while (rowStart < text.size()) {
    std::size_t rowEnd = text.find('\n', rowStart);
    if (rowEnd == std::string_view::npos) {
      rowEnd = text.size();
    }
    if (rowEnd > rowStart) {
      rows.emplace_back(text.substr(rowStart, rowEnd - rowStart));
    }
    rowStart = rowEnd + 1U;
  }
  return rows;
}

std::string joinRows(const std::vector<std::string>& rows) {
  std::string text;
  for (const std::string& row : rows) {
    text += row;
    text.push_back('\n');
  }
  return text;
}

ProductDungeonDraftOperationResult result(std::string_view status,
                                          std::string_view reason,
                                          ProductDungeonDraftCursor cursor) {
  ProductDungeonDraftOperationResult out;
  out.status = status;
  out.reasonCode = reason;
  out.cursor = cursor;
  return out;
}

void markCustomDraft(WorldSetupDraft& draft) {
  draft.asciiRoomEnabled = true;
  draft.asciiRoomId = std::string(kCustomRoomId);
  draft.asciiRoomSourceName = std::string(kCustomSourceName);
}

void replaceExistingPlayerSpawns(std::vector<std::string>& rows,
                                 ProductDungeonDraftCursor keep) {
  for (std::size_t row = 0; row < rows.size(); ++row) {
    for (std::size_t column = 0; column < rows[row].size(); ++column) {
      if (row == keep.row && column == keep.column) {
        continue;
      }
      if (rows[row][column] == 'P') {
        rows[row][column] = '.';
      }
    }
  }
}

}  // namespace

std::string_view productCustomDungeonRoomId() {
  return kCustomRoomId;
}

std::string_view productCustomDungeonSourceName() {
  return kCustomSourceName;
}

bool isProductDungeonDraftGlyph(char glyph) {
  switch (glyph) {
    case '#':
    case '.':
    case 'P':
    case 'K':
    case '$':
    case 'E':
    case '+':
    case 'C':
    case 'L':
    case 'J':
    case '^':
    case 'v':
    case '<':
    case '>':
      return true;
  }
  return false;
}

std::size_t productDungeonDraftRowCount(std::string_view asciiRoomText) {
  return splitRows(asciiRoomText).size();
}

std::size_t productDungeonDraftColumnCount(std::string_view asciiRoomText) {
  const std::vector<std::string> rows = splitRows(asciiRoomText);
  std::size_t width = 0;
  for (const std::string& row : rows) {
    width = std::max(width, row.size());
  }
  return width;
}

ProductDungeonDraftCursor clampProductDungeonDraftCursor(
    const WorldSetupDraft& draft,
    ProductDungeonDraftCursor cursor) {
  const std::vector<std::string> rows = splitRows(draft.asciiRoomText);
  if (rows.empty()) {
    return ProductDungeonDraftCursor{};
  }
  cursor.row = std::min(cursor.row, rows.size() - 1U);
  if (rows[cursor.row].empty()) {
    cursor.column = 0;
  } else {
    cursor.column = std::min(cursor.column, rows[cursor.row].size() - 1U);
  }
  return cursor;
}

ProductDungeonDraftOperationResult moveProductDungeonDraftCursor(
    const WorldSetupDraft& draft,
    ProductDungeonDraftCursor cursor,
    ProductDungeonDraftDirection direction) {
  const std::vector<std::string> rows = splitRows(draft.asciiRoomText);
  if (rows.empty()) {
    return result("dungeon_draft_cursor_unavailable",
                  "dungeon_draft_empty",
                  ProductDungeonDraftCursor{});
  }
  cursor = clampProductDungeonDraftCursor(draft, cursor);
  switch (direction) {
    case ProductDungeonDraftDirection::Up:
      cursor.row = cursor.row == 0U ? 0U : cursor.row - 1U;
      break;
    case ProductDungeonDraftDirection::Down:
      cursor.row = std::min(cursor.row + 1U, rows.size() - 1U);
      break;
    case ProductDungeonDraftDirection::Left:
      cursor.column = cursor.column == 0U ? 0U : cursor.column - 1U;
      break;
    case ProductDungeonDraftDirection::Right:
      if (!rows[cursor.row].empty()) {
        cursor.column = std::min(cursor.column + 1U, rows[cursor.row].size() - 1U);
      }
      break;
  }
  cursor = clampProductDungeonDraftCursor(draft, cursor);
  ProductDungeonDraftOperationResult out =
      result("dungeon_draft_cursor_moved", "dungeon_draft_cursor_moved", cursor);
  out.ok = true;
  return out;
}

ProductDungeonDraftOperationResult paintProductDungeonDraftCell(
    WorldSetupDraft& draft,
    ProductDungeonDraftCursor cursor,
    char glyph) {
  cursor = clampProductDungeonDraftCursor(draft, cursor);
  if (!isProductDungeonDraftGlyph(glyph)) {
    return result("dungeon_draft_invalid_glyph",
                  "dungeon_draft_invalid_glyph",
                  cursor);
  }

  std::vector<std::string> rows = splitRows(draft.asciiRoomText);
  if (rows.empty() || cursor.row >= rows.size() ||
      cursor.column >= rows[cursor.row].size()) {
    return result("dungeon_draft_cursor_out_of_range",
                  "dungeon_draft_cursor_out_of_range",
                  cursor);
  }

  if (glyph == 'P') {
    replaceExistingPlayerSpawns(rows, cursor);
  }
  rows[cursor.row][cursor.column] = glyph;
  draft.asciiRoomText = joinRows(rows);
  markCustomDraft(draft);

  ProductDungeonDraftOperationResult out =
      result("dungeon_draft_cell_painted",
             "dungeon_draft_cell_painted",
             cursor);
  out.ok = true;
  out.modified = true;
  out.glyph = glyph;
  return out;
}

ProductDungeonDraftOperationResult setProductDungeonDraftCell(
    WorldSetupDraft& draft,
    std::size_t row,
    std::size_t column,
    char glyph) {
  return paintProductDungeonDraftCell(draft,
                                      ProductDungeonDraftCursor{row, column},
                                      glyph);
}

}  // namespace iggy3d
