#pragma once

#include <array>
#include <cstddef>

#include "core/math/Vec3.hpp"

namespace iggy3d {

inline constexpr float kGeneratedOpenFrameOpeningWidthRatio = 0.60F;
inline constexpr float kGeneratedOpenFrameOpeningHeightRatio = 0.75F;
inline constexpr std::size_t kGeneratedOpenFramePartCount = 3U;

struct GeneratedOpenFramePart {
  Vec3 center{};
  Vec3 size{};
};

struct GeneratedOpenFrameLayout {
  std::array<GeneratedOpenFramePart, kGeneratedOpenFramePartCount> parts{};
  bool valid = false;
};

// Returns three centered local-space boxes: left pier, right pier, lintel.
// Rendering, previews, and collision all consume this layout so the opening
// cannot drift between systems.
[[nodiscard]] inline GeneratedOpenFrameLayout generatedOpenFrameLayout(
    Vec3 outerSize) noexcept {
  GeneratedOpenFrameLayout result;
  if (!isFinite(outerSize) || outerSize.x <= 0.0F || outerSize.y <= 0.0F ||
      outerSize.z <= 0.0F) {
    return result;
  }

  const float openingWidth =
      outerSize.x * kGeneratedOpenFrameOpeningWidthRatio;
  const float openingHeight =
      outerSize.y * kGeneratedOpenFrameOpeningHeightRatio;
  const float pierWidth = (outerSize.x - openingWidth) * 0.5F;
  const float lintelHeight = outerSize.y - openingHeight;
  const float pierCenterX = openingWidth * 0.5F + pierWidth * 0.5F;

  result.parts[0] = {{-pierCenterX, -lintelHeight * 0.5F, 0.0F},
                     {pierWidth, openingHeight, outerSize.z}};
  result.parts[1] = {{pierCenterX, -lintelHeight * 0.5F, 0.0F},
                     {pierWidth, openingHeight, outerSize.z}};
  result.parts[2] = {{0.0F, openingHeight * 0.5F, 0.0F},
                     {outerSize.x, lintelHeight, outerSize.z}};
  result.valid = true;
  return result;
}

}  // namespace iggy3d
