#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {

struct DebugHudGlyphQuad {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  char source = ' ';
};

struct DebugHudLayoutResult {
  bool projected = false;
  std::size_t lineCount = 0;
  std::size_t glyphCount = 0;
  std::vector<DebugHudGlyphQuad> quads;
};

DebugHudLayoutResult layoutDebugHudText(std::span<const std::string> lines,
                                        std::uint32_t viewportWidth,
                                        std::uint32_t viewportHeight);
DebugHudLayoutResult layoutDebugHudTextAt(std::string_view text,
                                          std::int32_t x,
                                          std::int32_t y,
                                          std::uint32_t viewportWidth,
                                          std::uint32_t viewportHeight);

}  // namespace iggy3d
