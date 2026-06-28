#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"

#include <cerrno>
#include <cstdlib>
#include <cmath>
#include <cstdint>
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

std::vector<std::size_t> sourceLineOffsets(std::string_view text) {
  std::vector<std::size_t> offsets;
  offsets.push_back(0U);
  for (std::size_t index = 0; index < text.size(); ++index) {
    // branch-gate: BG-1166
    if (text[index] == '\n' && index + 1U < text.size()) {
      offsets.push_back(index + 1U);
    }
  }
  return offsets;
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

bool layerDirective(std::string_view row, std::int32_t& storyIndex) {
  constexpr std::string_view prefix = "floor";
  // branch-gate: BG-1166
  if (row.size() <= prefix.size() || row.substr(0U, prefix.size()) != prefix) {
    return false;
  }
  std::int32_t number = 0;
  for (std::size_t index = prefix.size(); index < row.size(); ++index) {
    const char digit = row[index];
    // branch-gate: BG-1166
    if (digit < '0' || digit > '9') {
      return false;
    }
    number = number * 10 + static_cast<std::int32_t>(digit - '0');
  }
  // branch-gate: BG-1166
  if (number <= 0) {
    return false;
  }
  storyIndex = number - 1;
  return true;
}

bool layerHoleGlyph(char glyph) {
  return glyph == ' ';
}

bool parseLayeredRows(AsciiRoomSource& source) {
  std::vector<AsciiRoomSourceLayer> layers;
  AsciiRoomSourceLayer* current = nullptr;
  for (std::size_t row = 0; row < source.rows.size(); ++row) {
    std::int32_t storyIndex = 0;
    // branch-gate: BG-1166
    if (layerDirective(source.rows[row], storyIndex)) {
      layers.push_back(AsciiRoomSourceLayer{source.rows[row],
                                            storyIndex,
                                            {},
                                            row + 1U});
      current = &layers.back();
      continue;
    }
    // branch-gate: BG-1166
    if (current == nullptr) {
      return false;
    }
    current->rows.push_back(source.rows[row]);
  }

  // branch-gate: BG-1166
  if (layers.empty()) {
    return false;
  }

  // branch-gate: BG-1166
  if (layers.front().rows.empty()) {
    reject(source,
           "ascii_room_empty_layer",
           diagnostic("ascii_room_empty_layer",
                      "ASCII room floor layer is empty",
                      layers.front().sourceRowStart));
    return true;
  }

  const std::size_t expectedHeight = layers.front().rows.size();
  const std::size_t expectedWidth = layers.front().rows.front().size();
  for (const AsciiRoomSourceLayer& layer : layers) {
    // branch-gate: BG-1166
    if (layer.rows.empty()) {
      reject(source,
             "ascii_room_empty_layer",
             diagnostic("ascii_room_empty_layer",
                        "ASCII room floor layer is empty",
                        layer.sourceRowStart));
      return true;
    }
    // branch-gate: BG-1166
    if (layer.rows.size() != expectedHeight) {
      reject(source,
             "ascii_room_layer_size_mismatch",
             diagnostic("ascii_room_layer_size_mismatch",
                        "ASCII room floor layers must have equal height",
                        layer.sourceRowStart));
      return true;
    }
    for (std::size_t row = 0; row < layer.rows.size(); ++row) {
      // branch-gate: BG-1166
      if (layer.rows[row].size() != expectedWidth) {
        reject(source,
               "ascii_room_layer_size_mismatch",
               diagnostic("ascii_room_layer_size_mismatch",
                          "ASCII room floor layers must have equal width",
                          layer.sourceRowStart + row,
                          layer.rows[row].size()));
        return true;
      }
    }
  }

  source.layers = std::move(layers);
  source.hasLayerDirectives = true;
  source.width = expectedWidth;
  source.height = expectedHeight;
  source.rows = source.layers.front().rows;
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
  source.sourceLineOffsets = sourceLineOffsets(source.rawText);

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

  const bool parsedLayered = parseLayeredRows(source);
  // branch-gate: BG-1166
  if (source.status != "ascii_room_ok") {
    return source;
  }
  // branch-gate: BG-1166
  if (parsedLayered) {
    for (std::size_t layer = 0; layer < source.layers.size(); ++layer) {
      const AsciiRoomSourceLayer& sourceLayer = source.layers[layer];
      for (std::size_t row = 0; row < sourceLayer.rows.size(); ++row) {
        for (std::size_t column = 0; column < sourceLayer.rows[row].size();
             ++column) {
          const char glyph = sourceLayer.rows[row][column];
          // branch-gate: BG-1166
          if (!layerHoleGlyph(glyph) && !asciiRoomGlyphInfo(glyph).has_value()) {
            reject(source,
                   "ascii_room_unknown_glyph",
                   diagnostic("ascii_room_unknown_glyph",
                              "unknown ASCII room glyph",
                              sourceLayer.sourceRowStart + row,
                              column,
                              glyph));
            return source;
          }
        }
      }
    }
    source.status = "ascii_room_ok";
    source.reasonCode = "ascii_room_ok";
    return source;
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
  // branch-gate: BG-1166
  if (source.hasLayerDirectives) {
    return asciiRoomSourceOffset(source, 0U, row, column);
  }
  std::size_t offset = source.layoutSourceOffset;
  for (std::size_t current = 0; current < row && current < source.rows.size(); ++current) {
    offset += source.rows[current].size();
    if (offset < source.rawText.size()) {
      ++offset;
    }
  }
  return offset + column;
}

std::size_t asciiRoomSourceOffset(const AsciiRoomSource& source,
                                  std::size_t layerIndex,
                                  std::size_t row,
                                  std::size_t column) {
  // branch-gate: BG-1166
  if (layerIndex >= source.layers.size()) {
    return asciiRoomSourceOffset(source, row, column);
  }
  const AsciiRoomSourceLayer& layer = source.layers[layerIndex];
  const std::size_t sourceRow = layer.sourceRowStart + row;
  // branch-gate: BG-1166
  if (sourceRow >= source.sourceLineOffsets.size()) {
    return asciiRoomSourceOffset(source, row, column);
  }
  return source.sourceLineOffsets[sourceRow] + column;
}

}  // namespace iggy3d
