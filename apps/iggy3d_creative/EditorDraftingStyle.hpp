#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d_creative_app {

// The drafting style table: one declarative record per visual role in the 2D
// plan language. The eventual canvas renderer resolves a plan primitive's
// role through this table and never grows role-specific branches; the roles
// name WHAT something is, the semantic plan projection (Codex's lane) decides
// WHICH role a primitive carries. Everything here is pure data, ImGui-free,
// and headless-testable.
//
// Visual laws encoded by this table (pinned by tests):
// - Exterior walls read heavier than interior partitions.
// - Overhead geometry (roof) reads lighter and dashed, and never joins the
//   normal layer.
// - Lower-storey context is ghosted: reduced alpha, painted under the active
//   storey.
// - Every state overlay stays distinguishable in grayscale: any two overlays
//   differ in structure (stroke class, dash pattern, or fill pattern), never
//   in hue alone. Invalid previews carry a shape cue (hatch + short dash).

struct CreativeEditorDraftingColor {
  std::uint8_t r = 255U;
  std::uint8_t g = 255U;
  std::uint8_t b = 255U;
  std::uint8_t a = 255U;
};

enum class CreativeEditorDraftingStrokeClass : std::uint8_t {
  None,      // fill-only roles
  Hairline,  // fixed one-pixel line
  Light,
  Medium,
  Heavy,
  Count,
};

enum class CreativeEditorDraftingFillPattern : std::uint8_t {
  None,
  Solid,
  Hatched,  // diagonal hatch — the shape cue for invalid/locked states
  Count,
};

enum class CreativeEditorDraftingRole : std::uint8_t {
  // Architecture
  ExteriorWall,
  InteriorPartition,
  SharedBoundary,
  RoomFloor,
  Door,
  DoorSwing,
  Window,
  Stair,
  Ramp,
  RoofOutline,
  RoofRidge,
  // Terrain
  ContourMinor,
  ContourMajor,
  Plateau,
  Road,
  Ditch,
  Bridge,
  ElevationBand,
  RegionMask,
  // Objects
  ObjectBounds,
  ObjectPoint,
  ObjectNature,
  ObjectArchitecture,
  ObjectCover,
  ObjectProp,
  // Gameplay
  PlayerSpawn,
  NpcSpawn,
  // State overlays
  HoverOverlay,
  SelectedOverlay,
  PreviewValidOverlay,
  PreviewInvalidOverlay,
  LockedOverlay,
  GeneratedOverlay,
  OverheadOverlay,
  LowerLevelGhostOverlay,
  Count,
};

struct CreativeEditorDraftingStyle {
  CreativeEditorDraftingColor tint;
  CreativeEditorDraftingStrokeClass strokeClass =
      CreativeEditorDraftingStrokeClass::Light;
  float dashCells = 0.0F;  // 0 = solid stroke
  float gapCells = 0.0F;
  CreativeEditorDraftingFillPattern fillPattern =
      CreativeEditorDraftingFillPattern::None;
  float fillAlpha = 0.0F;
  float minimumThicknessPixels = 1.0F;
  std::uint8_t drawOrder = 0U;  // lower paints first
  bool normalLayer = true;
  bool overheadLayer = false;
  bool contextLayer = false;
};

[[nodiscard]] const CreativeEditorDraftingStyle& creativeEditorDraftingStyle(
    CreativeEditorDraftingRole role) noexcept;

[[nodiscard]] std::string_view toString(
    CreativeEditorDraftingRole role) noexcept;

// Stroke width in pixels for a style at the current canvas zoom: the stroke
// class's cell width scaled by pixels-per-cell, floored by the style's
// minimum. Hairline stays at the minimum regardless of zoom; None is zero.
[[nodiscard]] float creativeEditorDraftingStrokeThicknessPixels(
    const CreativeEditorDraftingStyle& style, float pixelsPerCell) noexcept;

// Names reserved on the reference sheets but deliberately absent from the
// production role enum (future capabilities must not be implied by enums).
[[nodiscard]] std::span<const std::string_view>
creativeEditorDraftingReservedSymbolNames() noexcept;

}  // namespace iggy3d_creative_app
