#pragma once

#include <cstddef>
#include <cstdint>

namespace iggy3d::creative {

enum class CreativeWheelPolarity : std::uint8_t {
  Natural,
  Reversed,
  Count,
};

enum class CreativeWheelStepMode : std::uint8_t {
  RoundedMagnitude,
  Unit,
  Count,
};

struct CreativeWheelProfile {
  CreativeWheelPolarity polarity = CreativeWheelPolarity::Natural;
  CreativeWheelStepMode stepMode = CreativeWheelStepMode::RoundedMagnitude;
  float epsilon = 1.0e-4F;
};

struct CreativePointerSample {
  float logicalX = 0.0F;
  float logicalY = 0.0F;
  std::uint32_t logicalWidth = 0U;
  std::uint32_t logicalHeight = 0U;
  std::uint32_t drawableWidth = 0U;
  std::uint32_t drawableHeight = 0U;
  bool moved = false;
  bool primaryPressed = false;
};

// x/y use drawable top-left coordinates. centeredX/centeredY use a centered,
// Y-up convention suitable for radial input.
struct CreativeDrawablePointer {
  float x = 0.0F;
  float y = 0.0F;
  float centeredX = 0.0F;
  float centeredY = 0.0F;
  bool valid = false;
  bool moved = false;
  bool primaryPressed = false;
};

// Drawable-pixel sub-rectangle of the window. valid is false when the input
// is degenerate (zero-area result or zero drawable extent); callers then fall
// back to the full-frame content viewport. See docs/creative_desktop_ui_plan.md
// DL-6 — the single home for logical<->drawable rect conversion.
struct CreativeContentRect {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::uint32_t width = 0U;
  std::uint32_t height = 0U;
  bool valid = false;
};

struct CreativeWrappedIndexResult {
  std::size_t index = 0U;
  bool valid = false;
  bool changed = false;
};

struct CreativeRadialSectorResult {
  std::size_t index = 0U;
  bool valid = false;
};

inline constexpr std::size_t kCreativeRadialSectorCapacity = 256U;

[[nodiscard]] std::int32_t quantizeCreativeWheelSteps(
    float wheelDelta,
    CreativeWheelProfile profile = {}) noexcept;

[[nodiscard]] CreativeDrawablePointer resolveCreativeDrawablePointer(
    CreativePointerSample sample) noexcept;

// Maps a logical-window sub-rectangle (given as min/max corners in the same
// coordinate space ImGui uses for DisplaySize) to drawable pixels via per-axis
// scale, then clamps to [0, drawable]. Returns valid=false for a degenerate
// result so the caller can fall back to the full-frame content viewport.
[[nodiscard]] CreativeContentRect resolveCreativeContentViewport(
    float logicalMinX,
    float logicalMinY,
    float logicalMaxX,
    float logicalMaxY,
    float scaleX,
    float scaleY,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept;

[[nodiscard]] CreativeWrappedIndexResult stepCreativeWrappedIndex(
    std::size_t current,
    std::size_t count,
    std::int64_t steps) noexcept;

// Sectors run clockwise from up. Half-sector ties select the clockwise sector.
[[nodiscard]] CreativeRadialSectorResult resolveCreativeRadialSector(
    float x,
    float y,
    std::size_t sectorCount,
    float deadzone = 0.0F) noexcept;

}  // namespace iggy3d::creative
