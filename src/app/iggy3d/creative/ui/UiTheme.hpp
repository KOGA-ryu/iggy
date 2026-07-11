#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace iggy3d {

struct CreativeUiRect {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

struct CreativeUiColor {
  float r = 0.0F;
  float g = 0.0F;
  float b = 0.0F;
  float a = 1.0F;
};

enum class CreativeUiTone : std::uint8_t {
  Surface,
  SurfaceRaised,
  TextPrimary,
  TextMuted,
  Accent,
  Selected,
  Disabled,
  Border,
  Count,
};

struct CreativeUiTheme {
  std::array<CreativeUiColor,
             static_cast<std::size_t>(CreativeUiTone::Count)> tones;
};

[[nodiscard]] const CreativeUiTheme& creativeUiTheme();
[[nodiscard]] CreativeUiColor creativeUiToneColor(
    CreativeUiTone tone,
    const CreativeUiTheme& theme);

}  // namespace iggy3d
