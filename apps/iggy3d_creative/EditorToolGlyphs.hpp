#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "EditorWorldLayoutState.hpp"
#include "EditorWorldLayoutTopography.hpp"

struct ImDrawList;

namespace iggy3d_creative_app {

// Procedural tool-icon vocabulary for the drafting UI. Every glyph is a small
// list of primitive ops authored on a fixed 24x24 grid; one interpreter renders
// any glyph into an ImDrawList at any size from a single tint. The data and
// geometry helpers are ImGui-free so coverage and bounds are headless-testable.
inline constexpr float kCreativeEditorToolGlyphGridCells = 24.0F;
inline constexpr float kCreativeEditorToolGlyphStrokeCells = 1.25F;

enum class CreativeEditorToolGlyph : std::uint8_t {
  ParentSelect,
  ParentStructure,
  ParentTerrain,
  ParentObject,
  ParentGameplay,
  EstateHouse,
  BuildingShell,
  AddRoom,
  Floor,
  Partition,
  Door,
  Window,
  Stair,
  Ramp,
  Plateau,
  Road,
  Ditch,
  RegionFlatten,
  RegionRaise,
  RegionLower,
  RegionSmooth,
  RegionNoise,
  MaskRectangle,
  MaskEllipse,
  Bridge,
  AssetBox,
  AssetArchitecture,
  AssetNature,
  AssetCover,
  AssetProps,
  PlayerSpawn,
  NpcSpawn,
  ActionPreview,
  ActionApply,
  ActionCancel,
  ActionNewSeed,
  ActionPlay,
  BadgeAdd,
  BadgeRemove,
  BadgeRaise,
  BadgeLower,
  BadgeNoise,
  BadgeTemplate,
  ViewPlan,
  ViewElevation,
  View3d,
  LevelUp,
  LevelDown,
  FitAll,
  FitSelection,
  RoofVisibility,
  LowerLevelContext,
  Contours,
  Dimensions,
  Snap,
  Count,
};

enum class CreativeEditorToolGlyphOpKind : std::uint8_t {
  Stroke,
  Fill,
  Ellipse,
  EllipseFilled,
  Path,
  Count,
};

struct CreativeEditorToolGlyphPoint {
  float x = 0.0F;
  float y = 0.0F;
};

// Path ops hold a cubic chain: 1 start point followed by (control, control,
// end) triples. Ellipse ops hold exactly 2 points: center then radii. Dash
// lengths of zero mean a solid stroke.
struct CreativeEditorToolGlyphOp {
  CreativeEditorToolGlyphOpKind kind = CreativeEditorToolGlyphOpKind::Stroke;
  bool closed = false;
  float widthCells = kCreativeEditorToolGlyphStrokeCells;
  float alpha = 1.0F;
  float dashCells = 0.0F;
  float gapCells = 0.0F;
  std::span<const CreativeEditorToolGlyphPoint> points;
};

struct CreativeEditorToolGlyphBounds {
  bool present = false;
  float minimumX = 0.0F;
  float minimumY = 0.0F;
  float maximumX = 0.0F;
  float maximumY = 0.0F;
};

[[nodiscard]] std::span<const CreativeEditorToolGlyphOp>
creativeEditorToolGlyphOps(CreativeEditorToolGlyph glyph) noexcept;

[[nodiscard]] std::string_view toString(
    CreativeEditorToolGlyph glyph) noexcept;

[[nodiscard]] CreativeEditorToolGlyph creativeEditorToolGlyphForWorldLayoutTool(
    CreativeEditorWorldLayoutTool tool) noexcept;

[[nodiscard]] CreativeEditorToolGlyph
creativeEditorToolGlyphForTerrainRegionOperation(
    CreativeEditorWorldLayoutTerrainRegionOperation operation) noexcept;

[[nodiscard]] CreativeEditorToolGlyph creativeEditorToolGlyphForTerrainMask(
    cr::CreativeTerrainCompositionMask mask) noexcept;

[[nodiscard]] CreativeEditorToolGlyph creativeEditorToolGlyphForPaletteCategory(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept;

[[nodiscard]] CreativeEditorToolGlyph creativeEditorToolGlyphForAssetCategory(
    CreativeEditorWorldLayoutAssetCategory category) noexcept;

[[nodiscard]] CreativeEditorToolGlyphBounds measureCreativeEditorToolGlyphBounds(
    CreativeEditorToolGlyph glyph) noexcept;

// Pixel stroke width for a glyph rendered at sizePixels: proportional to the
// authored cell width with a 1px floor so small icons stay crisp.
[[nodiscard]] float creativeEditorToolGlyphStrokeThicknessPixels(
    float widthCells, float sizePixels) noexcept;

void flattenCreativeEditorToolGlyphPath(
    std::span<const CreativeEditorToolGlyphPoint> chain,
    std::vector<CreativeEditorToolGlyphPoint>& out);

void flattenCreativeEditorToolGlyphEllipse(
    CreativeEditorToolGlyphPoint center, CreativeEditorToolGlyphPoint radii,
    std::vector<CreativeEditorToolGlyphPoint>& out);

void appendCreativeEditorToolGlyphDashes(
    std::span<const CreativeEditorToolGlyphPoint> points, bool closed,
    float dashCells, float gapCells,
    std::vector<std::pair<CreativeEditorToolGlyphPoint,
                          CreativeEditorToolGlyphPoint>>& out);

// The single ImGui-facing entry: renders one glyph into a drawList with its
// top-left at (leftPixels, topPixels). Per-op alpha modulates the tint alpha.
void drawCreativeEditorToolGlyph(ImDrawList& drawList,
                                 CreativeEditorToolGlyph glyph,
                                 float leftPixels, float topPixels,
                                 float sizePixels, std::uint32_t tint);

}  // namespace iggy3d_creative_app
