#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"

#include <cerrno>
#include <cstdlib>
#include <cmath>
#include <string>
#include <utility>

#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"

namespace iggy3d {
namespace {

AsciiRoomDiagnostic diagnostic(std::string reason,
                               std::string message,
                               std::size_t row = 0,
                               std::size_t column = 0,
                               char glyph = '\0') {
  AsciiRoomDiagnostic out;
  out.severity = "error";
  out.reasonCode = std::move(reason);
  out.row = row;
  out.column = column;
  out.glyph = glyph;
  out.message = std::move(message);
  return out;
}

void reject(AsciiRoomSource& source,
            std::string reason,
            AsciiRoomDiagnostic diag) {
  source.status = reason;
  source.reasonCode = std::move(reason);
  source.diagnostics.push_back(std::move(diag));
}

std::string normalizeNewlines(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    const char c = text[index];
    if (c == '\r') {
      if (index + 1U < text.size() && text[index + 1U] == '\n') {
        ++index;
      }
      out.push_back('\n');
    } else {
      out.push_back(c);
    }
  }
  return out;
}

std::vector<std::string> splitRows(std::string_view text) {
  std::vector<std::string> rows;
  std::string row;
  for (char c : text) {
    if (c == '\n') {
      rows.push_back(row);
      row.clear();
    } else {
      row.push_back(c);
    }
  }
  rows.push_back(row);
  if (!rows.empty() && rows.back().empty()) {
    rows.pop_back();
  }
  return rows;
}

bool scaleDirective(std::string_view row) {
  return row.size() >= 2U && row.front() == '*';
}

bool parseScaleDirective(std::string_view row, float& out) {
  const std::string text{row.substr(1U)};
  char* end = nullptr;
  errno = 0;
  const float value = std::strtof(text.c_str(), &end);
  // branch-gate: BG-1162
  if (errno != 0 || end == text.c_str() || *end != '\0' ||
      !std::isfinite(value) || value <= 0.0F) {
    return false;
  }
  out = value;
  return true;
}

}  // namespace

std::string_view asciiRoomDiagnosticSeverityError() {
  return "error";
}

AsciiRoomSource parseAsciiRoomSource(std::string_view text,
                                     std::string sourceName) {
  AsciiRoomSource source;
  source.sourceName = std::move(sourceName);
  source.rawText = normalizeNewlines(text);
  source.rows = splitRows(source.rawText);

  // branch-gate: BG-1163
  if (!source.rows.empty() && scaleDirective(source.rows.front())) {
    float scale = 1.0F;
    // branch-gate: BG-1164
    if (!parseScaleDirective(source.rows.front(), scale)) {
      reject(source,
             "ascii_room_invalid_scale",
             diagnostic("ascii_room_invalid_scale",
                        "ASCII room scale directive must be a positive finite number",
                        0U,
                        0U,
                        source.rows.front().front()));
      return source;
    }
    source.tileScaleMeters = scale;
    source.hasTileScaleDirective = true;
    source.layoutSourceOffset = source.rows.front().size();
    source.layoutSourceOffset += static_cast<std::size_t>(source.layoutSourceOffset < source.rawText.size());
    source.rows.erase(source.rows.begin());
  }

  source.height = source.rows.size();
  source.width = source.rows.empty() ? 0U : source.rows.front().size();
  if (source.rows.empty() || (source.rows.size() == 1U && source.rows.front().empty())) {
    reject(source,
           "ascii_room_empty",
           diagnostic("ascii_room_empty", "ASCII room source is empty"));
    return source;
  }

  for (std::size_t row = 0; row < source.rows.size(); ++row) {
    if (source.rows[row].size() != source.width) {
      reject(source,
             "ascii_room_ragged_rows",
             diagnostic("ascii_room_ragged_rows",
                        "ASCII room rows must have equal width",
                        row,
                        source.rows[row].size()));
      return source;
    }
  }

  for (std::size_t row = 0; row < source.rows.size(); ++row) {
    for (std::size_t column = 0; column < source.rows[row].size(); ++column) {
      const char glyph = source.rows[row][column];
      if (!asciiRoomGlyphInfo(glyph).has_value()) {
        reject(source,
               "ascii_room_unknown_glyph",
               diagnostic("ascii_room_unknown_glyph",
                          "unknown ASCII room glyph",
                          row,
                          column,
                          glyph));
        return source;
      }
    }
  }

  source.status = "ascii_room_ok";
  source.reasonCode = "ascii_room_ok";
  return source;
}

std::size_t asciiRoomSourceOffset(const AsciiRoomSource& source,
                                  std::size_t row,
                                  std::size_t column) {
  std::size_t offset = source.layoutSourceOffset;
  for (std::size_t current = 0; current < row && current < source.rows.size(); ++current) {
    offset += source.rows[current].size();
    if (offset < source.rawText.size()) {
      ++offset;
    }
  }
  return offset + column;
}

}  // namespace iggy3d
