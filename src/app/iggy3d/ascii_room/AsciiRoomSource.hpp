#pragma once

#include <cstddef>
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

struct AsciiRoomSource {
  std::string sourceName;
  std::string rawText;
  std::vector<std::string> rows;
  std::size_t width = 0;
  std::size_t height = 0;
  std::string status = "ascii_room_ok";
  std::string reasonCode = "ascii_room_ok";
  std::vector<AsciiRoomDiagnostic> diagnostics;
};

std::string_view asciiRoomDiagnosticSeverityError();
AsciiRoomSource parseAsciiRoomSource(std::string_view text,
                                     std::string sourceName = {});
std::size_t asciiRoomSourceOffset(const AsciiRoomSource& source,
                                  std::size_t row,
                                  std::size_t column);

}  // namespace iggy3d
