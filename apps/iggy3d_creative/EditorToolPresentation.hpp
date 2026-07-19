#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "EditorDesktopCommands.hpp"
#include "EditorTerrainGeneration.hpp"
#include "EditorToolGlyphs.hpp"

namespace iggy3d_creative_app {

// The declarative tool-presentation model: one record per creative tool
// carrying glyph, name, category, options, actions, availability locks, and
// controller hints. The toolbox strip, the tool options strip, and
// (progressively) the inspector and controller overlays all consume this one
// table, so a new tool never grows surface-specific special cases. Everything
// here is pure and headless-testable; only the consumers touch ImGui.

enum class CreativeEditorToolPresentationId : std::uint8_t {
  Select,
  EstateHouse,
  BuildingShell,
  Room,
  Floor,
  Partition,
  Door,
  Window,
  Stair,
  Ramp,
  Plateau,
  Road,
  Ditch,
  TerrainRegion,
  Bridge,
  CatalogAsset,
  PlayerSpawn,
  NpcSpawn,
  Count,
};

enum class CreativeEditorToolActivation : std::uint8_t {
  WorldLayoutTool,
  BuildingTemplate,
  TerrainRegionSession,
  Count,
};

// Availability locks, evaluated against live editor state. A presentation is
// unavailable while any of its locks is engaged.
inline constexpr std::uint8_t kCreativeEditorToolLockNone = 0U;
inline constexpr std::uint8_t kCreativeEditorToolLockTemplatePlacement =
    1U << 0U;
inline constexpr std::uint8_t kCreativeEditorToolLockBuildingTransform =
    1U << 1U;
inline constexpr std::uint8_t kCreativeEditorToolLockForeignTerrainPreview =
    1U << 2U;
inline constexpr std::uint8_t kCreativeEditorToolLockExactLayoutPreview =
    1U << 3U;
inline constexpr std::uint8_t kCreativeEditorToolLockMissingTemplate =
    1U << 4U;

enum class CreativeEditorToolOptionWidget : std::uint8_t {
  GlyphChoice,
  IntDrag,
  DoubleDrag,
  SeedButton,
  Count,
};

enum class CreativeEditorToolOptionBinding : std::uint8_t {
  TerrainOperation,
  TerrainMask,
  TerrainTargetHeight,
  TerrainNoiseRelief,
  TerrainNoiseScale,
  TerrainSeed,
  TerrainFeather,
  Count,
};

struct CreativeEditorToolOptionChoice {
  CreativeEditorToolGlyph glyph = CreativeEditorToolGlyph::ParentSelect;
  std::string_view label;
  std::uint8_t value = 0U;
};

// label/compactLabel may be empty when the wording depends on live state
// (the terrain target height renames per operation); consumers resolve the
// displayed text through creativeEditorToolOptionFullLabel /
// creativeEditorToolOptionCompactLabel rather than reading these directly.
struct CreativeEditorToolOptionSpec {
  CreativeEditorToolOptionBinding binding =
      CreativeEditorToolOptionBinding::Count;
  CreativeEditorToolOptionWidget widget =
      CreativeEditorToolOptionWidget::Count;
  std::string_view label;
  std::string_view compactLabel;
  double minimum = 0.0;
  double maximum = 0.0;
  std::span<const CreativeEditorToolOptionChoice> choices;
};

enum class CreativeEditorToolActionRule : std::uint8_t {
  IdleValidRegion,
  OwnedPreview,
  RegionActivity,
  Count,
};

struct CreativeEditorToolActionSpec {
  CreativeDesktopCommandId command = CreativeDesktopCommandId::Count;
  CreativeEditorToolGlyph glyph = CreativeEditorToolGlyph::ParentSelect;
  std::string_view label;
  CreativeEditorToolActionRule rule = CreativeEditorToolActionRule::Count;
  // Enabled actions with this flag are dispatched automatically after any
  // option change (the change-requests-preview rule, expressed declaratively).
  bool runOnOptionChange = false;
};

struct CreativeEditorToolPresentation {
  CreativeEditorToolPresentationId id = CreativeEditorToolPresentationId::Count;
  CreativeEditorToolActivation activation =
      CreativeEditorToolActivation::WorldLayoutTool;
  CreativeEditorWorldLayoutPaletteCategory category =
      CreativeEditorWorldLayoutPaletteCategory::Structure;
  CreativeEditorToolGlyph glyph = CreativeEditorToolGlyph::ParentSelect;
  std::string_view name;
  std::string_view hint;
  std::string_view controllerHint;
  bool toolbox = true;
  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Select;
  std::string_view buildingTemplateId;
  std::uint8_t locks = kCreativeEditorToolLockNone;
  std::span<const CreativeEditorToolOptionSpec> options;
  std::span<const CreativeEditorToolActionSpec> actions;
};

struct CreativeEditorToolPresentationStatus {
  bool active = false;
  bool unavailable = false;
};

// The full table, ordered category-major (the toolbox draws it in order).
[[nodiscard]] std::span<const CreativeEditorToolPresentation>
creativeEditorToolPresentations() noexcept;

// Out-of-range ids resolve to the Select presentation (fail-visible default).
[[nodiscard]] const CreativeEditorToolPresentation&
findCreativeEditorToolPresentation(
    CreativeEditorToolPresentationId id) noexcept;

// The presentation the options strip follows: the terrain-region session when
// it is live, the placing building template, otherwise the active tool.
[[nodiscard]] const CreativeEditorToolPresentation&
activeCreativeEditorToolPresentation(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

[[nodiscard]] CreativeEditorToolPresentationStatus
evaluateCreativeEditorToolPresentation(
    const CreativeEditorToolPresentation& presentation,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration) noexcept;

[[nodiscard]] bool creativeEditorToolOptionVisible(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

[[nodiscard]] std::string_view creativeEditorToolOptionFullLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

[[nodiscard]] std::string_view creativeEditorToolOptionCompactLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

[[nodiscard]] double creativeEditorToolOptionScalarValue(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

// Clamps to the spec's range; returns whether the stored value changed.
bool setCreativeEditorToolOptionScalarValue(
    const CreativeEditorToolOptionSpec& option,
    CreativeEditorWorldLayoutTopographyState& topography,
    double value) noexcept;

[[nodiscard]] std::uint8_t creativeEditorToolOptionChoiceValue(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

bool setCreativeEditorToolOptionChoiceValue(
    const CreativeEditorToolOptionSpec& option,
    CreativeEditorWorldLayoutTopographyState& topography,
    std::uint8_t value) noexcept;

// Advances the terrain seed, wrapping at the numeric limit.
void advanceCreativeEditorToolOptionSeed(
    CreativeEditorWorldLayoutTopographyState& topography) noexcept;

[[nodiscard]] bool creativeEditorToolActionEnabled(
    const CreativeEditorToolActionSpec& action,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

}  // namespace iggy3d_creative_app
