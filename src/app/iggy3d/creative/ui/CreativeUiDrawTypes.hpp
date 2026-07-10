#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace iggy3d {

enum class CreativeUiPrimitiveKind : std::uint8_t {
  Panel,
  Rect,
  Text,
  Border,
  Highlight,
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
  Status,
};

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

enum class CreativeUiThemeId : std::uint8_t {
  System,
  Journal,
};

struct CreativeUiTheme {
  std::array<CreativeUiColor, 9> tones;
};

struct CreativeUiPrimitive {
  CreativeUiPrimitiveKind kind = CreativeUiPrimitiveKind::Rect;
  CreativeUiTone tone = CreativeUiTone::Surface;
  CreativeUiRect rect;
  std::string semanticId;
  std::string text;
  bool selected = false;
  bool enabled = true;
};

enum class CreativeUiHitKind : std::uint8_t {
  None,
  Button,
  Row,
  Slider,
  Toggle,
  Viewport,
};

struct CreativeUiHitRegion {
  std::string semanticId;
  CreativeUiRect rect;
  CreativeUiHitKind kind = CreativeUiHitKind::None;
  bool enabled = true;
};

struct CreativeUiDrawList {
  bool ready = false;
  bool partial = false;
  std::string status = "creative_ui_draw_list_not_ready";
  std::string reasonCode = "creative_ui_draw_list_not_ready";
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  CreativeUiThemeId theme = CreativeUiThemeId::System;
  std::vector<CreativeUiPrimitive> primitives;
  std::vector<CreativeUiHitRegion> hitRegions;
  std::uint64_t hitRegionCount = 0;
  std::uint64_t primitiveCount = 0;
  std::uint64_t textCount = 0;
  std::uint64_t rectCount = 0;
  std::uint64_t rowCount = 0;
  std::uint64_t disabledRowCount = 0;
};

const CreativeUiTheme& creativeUiTheme(CreativeUiThemeId id);
CreativeUiColor creativeUiToneColor(CreativeUiTone tone,
                                    const CreativeUiTheme& theme);

}  // namespace iggy3d
