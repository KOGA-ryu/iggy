#include "EditorDraftingStyle.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace iggy3d_creative_app {

namespace {

using Role = CreativeEditorDraftingRole;
using Stroke = CreativeEditorDraftingStrokeClass;
using Fill = CreativeEditorDraftingFillPattern;
using Style = CreativeEditorDraftingStyle;
using Color = CreativeEditorDraftingColor;

// Stroke class widths in grid cells. Hairline is handled separately (always
// the style's minimum pixel width); None draws nothing.
constexpr float kLightCells = 0.07F;
constexpr float kMediumCells = 0.11F;
constexpr float kHeavyCells = 0.18F;

constexpr Style stroke(Color tint, Stroke strokeClass, std::uint8_t drawOrder,
                       float dashCells = 0.0F, float gapCells = 0.0F,
                       bool normal = true, bool overhead = false,
                       bool context = false) {
  Style style;
  style.tint = tint;
  style.strokeClass = strokeClass;
  style.dashCells = dashCells;
  style.gapCells = gapCells;
  style.drawOrder = drawOrder;
  style.normalLayer = normal;
  style.overheadLayer = overhead;
  style.contextLayer = context;
  return style;
}

constexpr Style filled(Color tint, float fillAlpha, std::uint8_t drawOrder,
                       bool normal = true, bool overhead = false,
                       bool context = false) {
  Style style;
  style.tint = tint;
  style.strokeClass = Stroke::None;
  style.fillPattern = Fill::Solid;
  style.fillAlpha = fillAlpha;
  style.drawOrder = drawOrder;
  style.normalLayer = normal;
  style.overheadLayer = overhead;
  style.contextLayer = context;
  return style;
}

constexpr Style overlay(Color tint, Stroke strokeClass, Fill fillPattern,
                        float fillAlpha, std::uint8_t drawOrder,
                        float dashCells = 0.0F, float gapCells = 0.0F,
                        bool normal = true, bool overhead = false,
                        bool context = false) {
  Style style;
  style.tint = tint;
  style.strokeClass = strokeClass;
  style.dashCells = dashCells;
  style.gapCells = gapCells;
  style.fillPattern = fillPattern;
  style.fillAlpha = fillAlpha;
  style.drawOrder = drawOrder;
  style.normalLayer = normal;
  style.overheadLayer = overhead;
  style.contextLayer = context;
  return style;
}

// Drafting palette over the dark canvas (#1B1F22): warm paper whites for
// architecture, desaturated earth tones for terrain, category hues for
// objects. State overlays differ structurally, never by hue alone.
constexpr Color kPaper{232, 228, 216, 255};
constexpr Color kPaperDim{201, 196, 180, 255};
constexpr Color kGlass{159, 196, 214, 255};
constexpr Color kRoof{168, 155, 120, 255};
constexpr Color kEarthDim{110, 103, 83, 255};
constexpr Color kEarth{138, 129, 104, 255};
constexpr Color kMesa{153, 144, 111, 255};
constexpr Color kValley{106, 128, 101, 255};
constexpr Color kCrater{151, 112, 105, 255};
constexpr Color kRidge{183, 158, 104, 255};
constexpr Color kRoadGray{169, 169, 180, 255};
constexpr Color kWater{94, 110, 120, 255};
constexpr Color kTimber{180, 169, 143, 255};
constexpr Color kMoss{74, 90, 70, 255};
constexpr Color kNature{127, 163, 107, 255};
constexpr Color kCover{95, 168, 160, 255};
constexpr Color kProp{176, 141, 110, 255};
constexpr Color kPlayer{111, 220, 140, 255};
constexpr Color kNpc{224, 179, 74, 255};
constexpr Color kWhite{255, 255, 255, 255};
constexpr Color kSelect{79, 169, 255, 255};
constexpr Color kValid{82, 240, 122, 255};
constexpr Color kInvalid{240, 90, 80, 255};
constexpr Color kLocked{154, 160, 166, 255};
constexpr Color kGenerated{127, 184, 255, 255};
constexpr Color kOverhead{232, 228, 216, 140};
constexpr Color kGhost{140, 135, 120, 110};

constexpr Style terrain(Color tint, Stroke strokeClass,
                        std::uint8_t drawOrder, float fillAlpha,
                        float dashCells = 0.0F,
                        float gapCells = 0.0F) {
  Style style = stroke(tint, strokeClass, drawOrder, dashCells, gapCells);
  style.fillPattern = Fill::Solid;
  style.fillAlpha = fillAlpha;
  return style;
}

constexpr std::size_t kRoleCount = static_cast<std::size_t>(Role::Count);

constexpr std::array<Style, kRoleCount> kStyles = {{
    // Architecture
    stroke(kPaper, Stroke::Heavy, 40U, 0.0F, 0.0F, true, false, true),
    stroke(kPaperDim, Stroke::Medium, 38U, 0.0F, 0.0F, true, false, true),
    stroke(kPaperDim, Stroke::Medium, 39U, 1.2F, 0.8F, true, false, true),
    filled(Color{139, 133, 112, 255}, 0.16F, 10U, true, false, true),
    stroke(kPaper, Stroke::Medium, 42U, 0.0F, 0.0F, true, false, true),
    stroke(kPaper, Stroke::Hairline, 43U, 0.9F, 0.9F, true, false, true),
    stroke(kGlass, Stroke::Medium, 41U, 0.0F, 0.0F, true, false, true),
    stroke(kPaperDim, Stroke::Light, 36U, 0.0F, 0.0F, true, false, true),
    stroke(kPaperDim, Stroke::Light, 36U, 0.0F, 0.0F, true, false, true),
    stroke(kRoof, Stroke::Light, 60U, 1.5F, 1.0F, false, true, false),
    stroke(kRoof, Stroke::Hairline, 61U, 0.8F, 0.8F, false, true, false),
    // Terrain
    stroke(kEarthDim, Stroke::Hairline, 6U),
    stroke(kEarth, Stroke::Light, 7U),
    terrain(kMesa, Stroke::Light, 8U, 0.10F),
    terrain(kValley, Stroke::Light, 8U, 0.08F, 0.9F, 0.7F),
    terrain(kCrater, Stroke::Medium, 8U, 0.08F, 0.6F, 0.5F),
    terrain(kRidge, Stroke::Medium, 9U, 0.08F),
    terrain(kMesa, Stroke::Medium, 8U, 0.16F),
    terrain(kRoadGray, Stroke::Medium, 9U, 0.16F),
    terrain(kWater, Stroke::Medium, 9U, 0.18F),
    terrain(kWater, Stroke::Light, 8U, 0.12F),
    terrain(kRidge, Stroke::Medium, 10U, 0.08F, 1.3F, 0.8F),
    stroke(kTimber, Stroke::Medium, 12U),
    filled(kMoss, 0.10F, 2U),
    stroke(kPaper, Stroke::Light, 80U, 2.5F, 2.0F),
    // Objects
    stroke(kTimber, Stroke::Light, 30U, 0.0F, 0.0F, true, false, true),
    stroke(kTimber, Stroke::Medium, 31U, 0.0F, 0.0F, true, false, true),
    stroke(kNature, Stroke::Light, 30U, 0.0F, 0.0F, true, false, true),
    stroke(kRoadGray, Stroke::Light, 30U, 0.0F, 0.0F, true, false, true),
    stroke(kCover, Stroke::Medium, 32U, 0.0F, 0.0F, true, false, true),
    stroke(kProp, Stroke::Light, 30U, 0.0F, 0.0F, true, false, true),
    // Gameplay
    stroke(kPlayer, Stroke::Medium, 50U),
    stroke(kNpc, Stroke::Medium, 50U),
    // State overlays
    overlay(kWhite, Stroke::Light, Fill::Solid, 0.08F, 90U),
    overlay(kSelect, Stroke::Medium, Fill::Solid, 0.10F, 92U),
    overlay(kValid, Stroke::Light, Fill::Solid, 0.08F, 94U, 1.5F, 1.0F),
    overlay(kInvalid, Stroke::Light, Fill::Hatched, 0.22F, 95U, 0.8F, 0.8F),
    overlay(kLocked, Stroke::Hairline, Fill::Hatched, 0.10F, 91U, 0.6F, 1.2F),
    overlay(kGenerated, Stroke::Light, Fill::None, 0.0F, 89U, 2.0F, 1.2F),
    overlay(kOverhead, Stroke::Hairline, Fill::None, 0.0F, 88U, 1.5F, 1.5F,
            false, true, false),
    overlay(kGhost, Stroke::Hairline, Fill::None, 0.0F, 4U, 0.0F, 0.0F, false,
            false, true),
}};

constexpr std::array<std::string_view, kRoleCount> kRoleNames = {{
    "exterior wall",
    "interior partition",
    "shared boundary",
    "room floor",
    "door",
    "door swing",
    "window",
    "stair",
    "ramp",
    "roof outline",
    "roof ridge",
    "contour minor",
    "contour major",
    "hill",
    "valley",
    "crater",
    "ridge",
    "plateau",
    "road",
    "river",
    "ditch",
    "ridge line",
    "bridge",
    "elevation band",
    "region mask",
    "object bounds",
    "object point",
    "object nature",
    "object architecture",
    "object cover",
    "object prop",
    "player spawn",
    "npc spawn",
    "hover overlay",
    "selected overlay",
    "preview valid overlay",
    "preview invalid overlay",
    "locked overlay",
    "generated overlay",
    "overhead overlay",
    "lower level ghost overlay",
}};

// Sheet-only names: shown as Reserved on the reference sheets, deliberately
// absent from the production enum above.
constexpr std::array<std::string_view, 5U> kReservedSymbolNames = {{
    "objective marker",
    "asset thumbnail frame",
    "asset thumbnail missing",
    "asset thumbnail selected",
    "asset category badge",
}};

}  // namespace

const CreativeEditorDraftingStyle& creativeEditorDraftingStyle(
    CreativeEditorDraftingRole role) noexcept {
  const auto index = static_cast<std::size_t>(role);
  if (index >= kRoleCount) {
    return kStyles.front();
  }
  return kStyles[index];
}

std::string_view toString(CreativeEditorDraftingRole role) noexcept {
  const auto index = static_cast<std::size_t>(role);
  if (index >= kRoleCount) {
    return "unknown";
  }
  return kRoleNames[index];
}

float creativeEditorDraftingStrokeThicknessPixels(
    const CreativeEditorDraftingStyle& style, float pixelsPerCell) noexcept {
  switch (style.strokeClass) {
    case Stroke::None:
      return 0.0F;
    case Stroke::Hairline:
      return style.minimumThicknessPixels;
    case Stroke::Light:
      return std::max(style.minimumThicknessPixels,
                      kLightCells * pixelsPerCell);
    case Stroke::Medium:
      return std::max(style.minimumThicknessPixels,
                      kMediumCells * pixelsPerCell);
    case Stroke::Heavy:
      return std::max(style.minimumThicknessPixels,
                      kHeavyCells * pixelsPerCell);
    case Stroke::Count:
      break;
  }
  return style.minimumThicknessPixels;
}

std::span<const std::string_view>
creativeEditorDraftingReservedSymbolNames() noexcept {
  return kReservedSymbolNames;
}

}  // namespace iggy3d_creative_app
