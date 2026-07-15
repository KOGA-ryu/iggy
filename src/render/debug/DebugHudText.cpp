#include "render/debug/DebugHudText.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

namespace iggy3d {
namespace {

constexpr std::uint32_t kGlyphWidth = 5;
constexpr std::uint32_t kGlyphHeight = 7;
constexpr std::uint32_t kPixel = 2;
constexpr std::uint32_t kGlyphAdvance = (kGlyphWidth + 1U) * kPixel;
constexpr std::uint32_t kLineAdvance = (kGlyphHeight + 2U) * kPixel;
constexpr std::uint32_t kMargin = 12;

using GlyphRows = std::array<std::string_view, kGlyphHeight>;

GlyphRows glyphRows(char raw) {
  const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
  switch (c) {
    case '0': return {"11111", "10001", "10011", "10101", "11001", "10001", "11111"};
    case '1': return {"00100", "01100", "00100", "00100", "00100", "00100", "01110"};
    case '2': return {"11110", "00001", "00001", "11110", "10000", "10000", "11111"};
    case '3': return {"11110", "00001", "00001", "01110", "00001", "00001", "11110"};
    case '4': return {"10010", "10010", "10010", "11111", "00010", "00010", "00010"};
    case '5': return {"11111", "10000", "10000", "11110", "00001", "00001", "11110"};
    case '6': return {"01111", "10000", "10000", "11110", "10001", "10001", "01110"};
    case '7': return {"11111", "00001", "00010", "00100", "01000", "01000", "01000"};
    case '8': return {"01110", "10001", "10001", "01110", "10001", "10001", "01110"};
    case '9': return {"01110", "10001", "10001", "01111", "00001", "00001", "11110"};
    case 'A': return {"01110", "10001", "10001", "11111", "10001", "10001", "10001"};
    case 'B': return {"11110", "10001", "10001", "11110", "10001", "10001", "11110"};
    case 'C': return {"01111", "10000", "10000", "10000", "10000", "10000", "01111"};
    case 'D': return {"11110", "10001", "10001", "10001", "10001", "10001", "11110"};
    case 'E': return {"11111", "10000", "10000", "11110", "10000", "10000", "11111"};
    case 'F': return {"11111", "10000", "10000", "11110", "10000", "10000", "10000"};
    case 'G': return {"01111", "10000", "10000", "10011", "10001", "10001", "01111"};
    case 'H': return {"10001", "10001", "10001", "11111", "10001", "10001", "10001"};
    case 'I': return {"11111", "00100", "00100", "00100", "00100", "00100", "11111"};
    case 'J': return {"00111", "00010", "00010", "00010", "10010", "10010", "01100"};
    case 'K': return {"10001", "10010", "10100", "11000", "10100", "10010", "10001"};
    case 'L': return {"10000", "10000", "10000", "10000", "10000", "10000", "11111"};
    case 'M': return {"10001", "11011", "10101", "10101", "10001", "10001", "10001"};
    case 'N': return {"10001", "11001", "10101", "10011", "10001", "10001", "10001"};
    case 'O': return {"01110", "10001", "10001", "10001", "10001", "10001", "01110"};
    case 'P': return {"11110", "10001", "10001", "11110", "10000", "10000", "10000"};
    case 'Q': return {"01110", "10001", "10001", "10001", "10101", "10010", "01101"};
    case 'R': return {"11110", "10001", "10001", "11110", "10100", "10010", "10001"};
    case 'S': return {"01111", "10000", "10000", "01110", "00001", "00001", "11110"};
    case 'T': return {"11111", "00100", "00100", "00100", "00100", "00100", "00100"};
    case 'U': return {"10001", "10001", "10001", "10001", "10001", "10001", "01110"};
    case 'V': return {"10001", "10001", "10001", "10001", "10001", "01010", "00100"};
    case 'W': return {"10001", "10001", "10001", "10101", "10101", "11011", "10001"};
    case 'X': return {"10001", "10001", "01010", "00100", "01010", "10001", "10001"};
    case 'Y': return {"10001", "10001", "01010", "00100", "00100", "00100", "00100"};
    case 'Z': return {"11111", "00001", "00010", "00100", "01000", "10000", "11111"};
    case '.': return {"00000", "00000", "00000", "00000", "00000", "01100", "01100"};
    case '-': return {"00000", "00000", "00000", "11111", "00000", "00000", "00000"};
    case '_': return {"00000", "00000", "00000", "00000", "00000", "00000", "11111"};
    case ':': return {"00000", "01100", "01100", "00000", "01100", "01100", "00000"};
    case '/': return {"00001", "00010", "00010", "00100", "01000", "01000", "10000"};
    case ' ': return {"00000", "00000", "00000", "00000", "00000", "00000", "00000"};
    default: return {"11111", "00001", "00010", "00100", "00100", "00000", "00100"};
  }
}

bool blankGlyph(const GlyphRows& rows) {
  for (std::string_view row : rows) {
    if (row.find('1') != std::string_view::npos) {
      return false;
    }
  }
  return true;
}

template <typename AppendQuad>
void appendGlyphQuadsForText(std::size_t& glyphCount,
                             std::string_view text,
                             std::int32_t baseX,
                             std::int32_t baseY,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight,
                             AppendQuad appendQuad) {
  // branch-gate: BG-1077
  if (baseX < 0 || baseY < 0) {
    return;
  }
  const std::uint32_t originX = static_cast<std::uint32_t>(baseX);
  const std::uint32_t originY = static_cast<std::uint32_t>(baseY);
  // branch-gate: BG-1077
  const std::uint32_t maxX = viewportWidth > kMargin ? viewportWidth - kMargin : viewportWidth;
  // branch-gate: BG-1077
  const std::uint32_t maxY = viewportHeight > kMargin ? viewportHeight - kMargin : viewportHeight;
  // branch-gate: BG-1077
  if (originY + kGlyphHeight * kPixel > maxY) {
    return;
  }
  for (std::size_t charIndex = 0; charIndex < text.size(); ++charIndex) {
    const std::uint32_t glyphBaseX =
        originX + static_cast<std::uint32_t>(charIndex) * kGlyphAdvance;
    // branch-gate: BG-1077
    if (glyphBaseX + kGlyphWidth * kPixel > maxX) {
      break;
    }
    const GlyphRows rows = glyphRows(text[charIndex]);
    // branch-gate: BG-1077
    if (blankGlyph(rows)) {
      continue;
    }
    ++glyphCount;
    for (std::uint32_t row = 0; row < kGlyphHeight; ++row) {
      for (std::uint32_t column = 0; column < kGlyphWidth; ++column) {
        // branch-gate: BG-1077
        if (rows[row][column] != '1') {
          continue;
        }
        appendQuad({static_cast<std::int32_t>(
                        glyphBaseX + column * kPixel),
                    static_cast<std::int32_t>(originY + row * kPixel),
                    kPixel,
                    kPixel,
                    text[charIndex]});
      }
    }
  }
}

}  // namespace

DebugHudLayoutResult layoutDebugHudText(std::span<const std::string> lines,
                                        std::uint32_t viewportWidth,
                                        std::uint32_t viewportHeight) {
  DebugHudLayoutResult result;
  if (lines.empty() || viewportWidth == 0U || viewportHeight == 0U) {
    return result;
  }

  result.projected = true;
  result.lineCount = lines.size();
  const std::uint32_t maxY = viewportHeight > kMargin ? viewportHeight - kMargin : viewportHeight;
  for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
    const std::uint32_t baseY = kMargin + static_cast<std::uint32_t>(lineIndex) * kLineAdvance;
    if (baseY + kGlyphHeight * kPixel > maxY) {
      break;
    }
    appendGlyphQuadsForText(
        result.glyphCount, lines[lineIndex],
        static_cast<std::int32_t>(kMargin),
        static_cast<std::int32_t>(baseY), viewportWidth, viewportHeight,
        [&result](DebugHudGlyphQuad quad) {
          result.quads.push_back(quad);
        });
  }
  return result;
}

DebugHudLayoutResult layoutDebugHudTextAt(std::string_view text,
                                          std::int32_t x,
                                          std::int32_t y,
                                          std::uint32_t viewportWidth,
                                          std::uint32_t viewportHeight) {
  DebugHudLayoutResult result;
  // branch-gate: BG-1077
  if (text.empty() || viewportWidth == 0U || viewportHeight == 0U) {
    return result;
  }
  result.projected = true;
  result.lineCount = 1U;
  appendGlyphQuadsForText(
      result.glyphCount, text, x, y, viewportWidth, viewportHeight,
      [&result](DebugHudGlyphQuad quad) { result.quads.push_back(quad); });
  return result;
}

DebugHudFixedLayoutResult layoutDebugHudTextAtInto(
    std::string_view text,
    std::int32_t x,
    std::int32_t y,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight,
    std::span<DebugHudGlyphQuad> output) noexcept {
  DebugHudFixedLayoutResult result;
  if (text.empty() || viewportWidth == 0U || viewportHeight == 0U) {
    return result;
  }
  result.projected = true;
  result.lineCount = 1U;
  appendGlyphQuadsForText(
      result.glyphCount, text, x, y, viewportWidth, viewportHeight,
      [&result, output](DebugHudGlyphQuad quad) {
        if (result.quadCount >= output.size()) {
          result.capacityExceeded = true;
          return;
        }
        output[result.quadCount++] = quad;
      });
  return result;
}

}  // namespace iggy3d
