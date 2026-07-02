#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {

struct AsciiRoomDiagnostic {
  std::string severity = "error";
  std::string reasonCode = "ascii_room_ok";
  std::size_t row = 0;
  std::size_t column = 0;
  char glyph = '\0';
  std::string message;
};

struct AsciiRoomSourceLayer {
  std::string name;
  std::int32_t storyIndex = 0;
  std::vector<std::string> rows;
  std::size_t sourceRowStart = 0;
};

struct AsciiRoomSource {
  std::string sourceName;
  std::string rawText;
  std::vector<std::string> rows;
  std::vector<std::size_t> sourceLineOffsets;
  std::vector<AsciiRoomSourceLayer> layers;
  std::size_t width = 0;
  std::size_t height = 0;
  float tileScaleMeters = 1.0F;
  bool hasTileScaleDirective = false;
  bool hasLayerDirectives = false;
  float layerFloorSpacingMeters = 4.0F;
  std::size_t layoutSourceOffset = 0;
  std::string status = "ascii_room_ok";
  std::string reasonCode = "ascii_room_ok";
  std::vector<AsciiRoomDiagnostic> diagnostics;
};

AsciiRoomSource parseAsciiRoomSource(std::string_view text,
                                     std::string sourceName = {});
std::size_t asciiRoomSourceOffset(const AsciiRoomSource& source,
                                  std::size_t row,
                                  std::size_t column);
std::size_t asciiRoomSourceOffset(const AsciiRoomSource& source,
                                  std::size_t layerIndex,
                                  std::size_t row,
                                  std::size_t column);

}  // namespace iggy3d
