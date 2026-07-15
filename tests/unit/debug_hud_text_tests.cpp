#include "render/debug/DebugHudText.hpp"

#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool knownStringProducesBoundedQuads() {
  const std::vector<std::string> lines{"POS 1.000 2.000 3.000", "PHASE grounded"};
  const iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudText(lines, 640, 360);
  bool bounded = true;
  for (const iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    bounded = bounded && quad.x >= 0 && quad.y >= 0 && quad.width > 0U &&
              quad.height > 0U &&
              static_cast<std::uint32_t>(quad.x) + quad.width <= 640U &&
              static_cast<std::uint32_t>(quad.y) + quad.height <= 360U;
  }
  return expect(layout.projected, "hud projected") &&
         expect(layout.lineCount == lines.size(), "line count") &&
         expect(layout.glyphCount > 0U, "glyph count") &&
         expect(!layout.quads.empty(), "quad count") &&
         expect(bounded, "bounded quads");
}

bool unknownGlyphIsDeterministic() {
  const std::vector<std::string> lines{"POS ~"};
  const iggy3d::DebugHudLayoutResult first =
      iggy3d::layoutDebugHudText(lines, 320, 200);
  const iggy3d::DebugHudLayoutResult second =
      iggy3d::layoutDebugHudText(lines, 320, 200);
  bool same = first.projected == second.projected &&
              first.lineCount == second.lineCount &&
              first.glyphCount == second.glyphCount &&
              first.quads.size() == second.quads.size();
  for (std::size_t index = 0; same && index < first.quads.size(); ++index) {
    same = first.quads[index].x == second.quads[index].x &&
           first.quads[index].y == second.quads[index].y &&
           first.quads[index].width == second.quads[index].width &&
           first.quads[index].height == second.quads[index].height;
  }
  return expect(first.projected, "unknown projected") &&
         expect(first.glyphCount > 0U, "unknown glyph count") &&
         expect(same, "unknown deterministic");
}

bool emptyOrZeroViewportIsUnavailable() {
  const std::vector<std::string> lines{"POS 0.000"};
  const iggy3d::DebugHudLayoutResult empty =
      iggy3d::layoutDebugHudText({}, 320, 200);
  const iggy3d::DebugHudLayoutResult zero =
      iggy3d::layoutDebugHudText(lines, 0, 200);
  return expect(!empty.projected, "empty unavailable") &&
         expect(!zero.projected, "zero unavailable");
}

bool positionedTextProducesBoundedQuads() {
  const iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudTextAt("NEW WORLD", 120, 80, 640, 360);
  bool bounded = true;
  for (const iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    bounded = bounded && quad.x >= 120 && quad.y >= 80 && quad.width > 0U &&
              quad.height > 0U &&
              static_cast<std::uint32_t>(quad.x) + quad.width <= 640U &&
              static_cast<std::uint32_t>(quad.y) + quad.height <= 360U;
  }
  return expect(layout.projected, "positioned projected") &&
         expect(layout.lineCount == 1U, "positioned line count") &&
         expect(layout.glyphCount > 0U, "positioned glyph count") &&
         expect(!layout.quads.empty(), "positioned quads") &&
         expect(bounded, "positioned bounded");
}

bool fixedLayoutMatchesVectorLayoutAndReportsCapacity() {
  const iggy3d::DebugHudLayoutResult dynamic =
      iggy3d::layoutDebugHudTextAt("PLAY HP 10/10", 24, 30, 640, 360);
  std::array<iggy3d::DebugHudGlyphQuad, 512U> fixedStorage{};
  const iggy3d::DebugHudFixedLayoutResult fixed =
      iggy3d::layoutDebugHudTextAtInto(
          "PLAY HP 10/10", 24, 30, 640, 360, fixedStorage);
  bool same = fixed.quadCount == dynamic.quads.size();
  for (std::size_t index = 0U; same && index < fixed.quadCount; ++index) {
    const iggy3d::DebugHudGlyphQuad& lhs = fixedStorage[index];
    const iggy3d::DebugHudGlyphQuad& rhs = dynamic.quads[index];
    same = lhs.x == rhs.x && lhs.y == rhs.y &&
           lhs.width == rhs.width && lhs.height == rhs.height &&
           lhs.source == rhs.source;
  }
  std::array<iggy3d::DebugHudGlyphQuad, 1U> insufficient{};
  const iggy3d::DebugHudFixedLayoutResult overflow =
      iggy3d::layoutDebugHudTextAtInto(
          "PLAY", 24, 30, 640, 360, insufficient);
  return expect(fixed.projected && !fixed.capacityExceeded,
                "fixed layout projects inside capacity") &&
         expect(fixed.glyphCount == dynamic.glyphCount && same,
                "fixed and vector layouts are source equivalent") &&
         expect(overflow.projected && overflow.capacityExceeded &&
                    overflow.quadCount == insufficient.size(),
                "fixed layout reports truncation without overrunning output");
}

}  // namespace

int main() {
  const bool ok = knownStringProducesBoundedQuads() && unknownGlyphIsDeterministic() &&
                  emptyOrZeroViewportIsUnavailable() && positionedTextProducesBoundedQuads() &&
                  fixedLayoutMatchesVectorLayoutAndReportsCapacity();
  return ok ? 0 : 1;
}
