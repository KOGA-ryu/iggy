#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "EditorDesktopCommands.hpp"
#include "EditorTerrainGeneration.hpp"
#include "EditorToolGlyphs.hpp"

#include "app/iggy3d/creative/input/Catalog.hpp"

namespace iggy3d_creative_app {

// Product-facing tool identity. Settings, commands, templates, and UI widgets
// are deliberately absent: they are not tools. A row may bind the same product
// capability to the 3D held-item frontend, the 2D drafting frontend, or both.
enum class CreativeEditorToolId : std::uint8_t {
  Select,
  Transform,
  Group,
  MaterialPlacement,
  CatalogAssetPlacement,
  MaterialBrush,
  VolumeSelect,
  VolumeFill,
  VolumeHollow,
  VolumeReplace,
  VolumeErase,
  VolumeClone,
  ConnectedFill,
  SurfaceExtrude,
  LinearArray,
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
  Bridge,
  TerrainControl,
  TerrainPaint,
  TerrainGrade,
  TerrainSculpt,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  PlayerSpawn,
  NpcSpawn,
  LogicLink,
  Measure,
  Count,
};

enum class CreativeEditorToolCategory : std::uint8_t {
  Workspace,
  Placement,
  Volume,
  Building,
  Terrain,
  Object,
  Gameplay,
  Logic,
  Count,
};

enum class CreativeEditorToolMaturity : std::uint8_t {
  M0NameOnly,
  M1Prototype,
  M2StableRecipe,
  M3Product,
  M4Integrated,
  Count,
};

enum class CreativeEditorToolExposure : std::uint8_t {
  Workspace,
  Default,
  Experimental,
  Internal,
  Count,
};

enum class CreativeEditorToolLifecycle : std::uint8_t {
  Observational,
  Parametric,
  Destructive,
  Count,
};

enum class CreativeEditorToolOptionFilterProfile : std::uint8_t {
  Standard,
  MaterialPlacement,
  Count,
};

enum class CreativeEditorToolCommandProfile : std::uint8_t {
  None,
  MaterialBrush,
  ObjectGroup,
  ObjectMove,
  Count,
};

enum class CreativeEditorQuickEditProfile : std::uint8_t {
  None,
  Generic,
  TerrainControl,
  TerrainGrade,
  TerrainSculpt,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  LogicLink,
  Count,
};

enum class CreativeEditorActionHintProfile : std::uint8_t {
  None,
  Material,
  MaterialBrush,
  ConnectedFill,
  SurfaceExtrude,
  TerrainControl,
  TerrainPaint,
  TerrainGrade,
  TerrainSculpt,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  ObjectSelect,
  ObjectMove,
  ObjectGroup,
  VolumeSelect,
  DirectShapeVolume,
  VolumeOperation,
  LinearArray,
  LogicLink,
  BuildingRoom,
  Measurement,
  Count,
};

enum class CreativeEditorToolDisplayProfile : std::uint8_t {
  Standard,
  Material,
  Selection,
  Count,
};

enum class CreativeEditorVolumeSettingsProfile : std::uint8_t {
  None,
  Fill,
  Hollow,
  Replace,
  Erase,
  Clone,
  Count,
};

enum class CreativeEditorWorldLayoutToolActivation : std::uint8_t {
  None,
  Tool,
  TerrainRegionSession,
  Count,
};

// Orthogonal canvas policies. The descriptor owns interaction shape while the
// world-layout kernels retain exhaustive dispatch for tool-specific geometry.
enum class CreativeEditorWorldLayoutInputProfile : std::uint8_t {
  Point,
  Drag,
  Polyline,
  Count,
};

enum class CreativeEditorWorldLayoutPreviewProfile : std::uint8_t {
  None,
  Rectangle,
  AxisLine,
  DoorOpening,
  WindowOpening,
  TerrainPath,
  CatalogAsset,
  Count,
};

enum class CreativeEditorWorldLayoutPreviewFill : std::uint8_t {
  None,
  Structure,
  VerticalConnector,
  Ramp,
  Road,
  Ditch,
  Count,
};

// Availability locks, evaluated against live editor state. A descriptor is
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
  TerrainAmount,
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

struct CreativeEditorToolOptionSpec {
  CreativeEditorToolOptionBinding binding =
      CreativeEditorToolOptionBinding::Count;
  CreativeEditorToolOptionWidget widget = CreativeEditorToolOptionWidget::Count;
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
  bool runOnOptionChange = false;
};

struct CreativeEditorToolDescriptor {
  CreativeEditorToolId id = CreativeEditorToolId::Count;
  CreativeEditorToolCategory category = CreativeEditorToolCategory::Workspace;
  CreativeEditorToolMaturity maturity =
      CreativeEditorToolMaturity::M0NameOnly;
  CreativeEditorToolExposure exposure = CreativeEditorToolExposure::Internal;
  CreativeEditorToolLifecycle lifecycle =
      CreativeEditorToolLifecycle::Observational;
  CreativeEditorToolGlyph glyph = CreativeEditorToolGlyph::ParentSelect;
  std::string_view name;
  std::string_view aliases;
  std::string_view purpose;
  std::string_view controllerHint;

  iggy3d::creative::CreativeHeldItemKind heldItemKind =
      iggy3d::creative::CreativeHeldItemKind::Count;
  bool catalogTool = false;
  bool defaultWheelEligible = false;

  CreativeEditorWorldLayoutToolActivation worldLayoutActivation =
      CreativeEditorWorldLayoutToolActivation::None;
  CreativeEditorWorldLayoutTool worldLayoutTool =
      CreativeEditorWorldLayoutTool::Count;
  CreativeEditorWorldLayoutInputProfile worldLayoutInputProfile =
      CreativeEditorWorldLayoutInputProfile::Count;
  CreativeEditorWorldLayoutPreviewProfile worldLayoutPreviewProfile =
      CreativeEditorWorldLayoutPreviewProfile::Count;
  CreativeEditorWorldLayoutPreviewFill worldLayoutPreviewFill =
      CreativeEditorWorldLayoutPreviewFill::Count;
  std::string_view worldLayoutInteractionPrompt;
  std::uint8_t locks = kCreativeEditorToolLockNone;

  CreativeEditorToolOptionFilterProfile optionFilterProfile =
      CreativeEditorToolOptionFilterProfile::Standard;
  CreativeEditorToolCommandProfile commandProfile =
      CreativeEditorToolCommandProfile::None;
  CreativeEditorQuickEditProfile quickEditProfile =
      CreativeEditorQuickEditProfile::None;
  CreativeEditorActionHintProfile actionHintProfile =
      CreativeEditorActionHintProfile::None;
  CreativeEditorToolDisplayProfile displayProfile =
      CreativeEditorToolDisplayProfile::Standard;
  CreativeEditorVolumeSettingsProfile volumeSettingsProfile =
      CreativeEditorVolumeSettingsProfile::None;
  bool keyboardQuickEditHints = false;

  std::span<const CreativeEditorToolOptionSpec> options;
  std::span<const CreativeEditorToolActionSpec> actions;
};

struct CreativeEditorToolStatus {
  bool active = false;
  bool unavailable = false;
};

enum class CreativeEditorToolDescriptorValidationStatus : std::uint8_t {
  Valid,
  WrongCount,
  MisorderedId,
  InvalidEnum,
  MissingIdentity,
  DuplicateHeldItem,
  InvalidCatalogBinding,
  InvalidWorldLayoutBinding,
  DuplicateWorldLayoutBinding,
  MissingWorldLayoutBinding,
  InvalidWorldLayoutPolicy,
  DefaultProductBelowM3,
  InvalidDefaultWheelExposure,
  Count,
};

struct CreativeEditorToolDescriptorValidation {
  CreativeEditorToolDescriptorValidationStatus status =
      CreativeEditorToolDescriptorValidationStatus::Valid;
  CreativeEditorToolId tool = CreativeEditorToolId::Count;
};

[[nodiscard]] std::span<const CreativeEditorToolDescriptor>
creativeEditorToolDescriptors() noexcept;

[[nodiscard]] const CreativeEditorToolDescriptor&
describeCreativeEditorTool(CreativeEditorToolId id) noexcept;

[[nodiscard]] const CreativeEditorToolDescriptor&
describeCreativeEditorHeldItemTool(
    iggy3d::creative::CreativeHeldItemKind kind) noexcept;

[[nodiscard]] const CreativeEditorToolDescriptor&
describeCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutTool tool) noexcept;

[[nodiscard]] const CreativeEditorToolDescriptor&
activeCreativeEditorWorldLayoutToolDescriptor(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

[[nodiscard]] CreativeEditorToolStatus evaluateCreativeEditorTool(
    const CreativeEditorToolDescriptor& descriptor,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration) noexcept;

[[nodiscard]] CreativeEditorToolDescriptorValidation
validateCreativeEditorToolDescriptors(
    std::span<const CreativeEditorToolDescriptor> descriptors) noexcept;

[[nodiscard]] bool creativeEditorToolDefaultVisible(
    const CreativeEditorToolDescriptor& descriptor) noexcept;
[[nodiscard]] bool creativeEditorToolExperimental(
    const CreativeEditorToolDescriptor& descriptor) noexcept;
[[nodiscard]] bool creativeEditorToolHasWorldLayoutSurface(
    const CreativeEditorToolDescriptor& descriptor) noexcept;

// Catalog projection. The catalog remains a generic input model and does not
// own product-tool policy; it receives these rows from this descriptor table.
[[nodiscard]] std::span<const iggy3d::creative::CreativeCatalogToolSpec>
creativeEditorCatalogToolSpecs() noexcept;

[[nodiscard]] iggy3d::creative::CreativeHotbarState
makeCreativeEditorDefaultHotbar(
    std::span<const iggy3d::creative::CreativeObjectKind> materialPalette)
    noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPaletteCategory
creativeEditorWorldLayoutPaletteCategory(
    const CreativeEditorToolDescriptor& descriptor) noexcept;

[[nodiscard]] std::string_view creativeEditorToolOptionFullLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;
[[nodiscard]] std::string_view creativeEditorToolOptionCompactLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;
[[nodiscard]] bool creativeEditorToolOptionVisible(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;
[[nodiscard]] double creativeEditorToolOptionScalarValue(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;
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
void advanceCreativeEditorToolOptionSeed(
    CreativeEditorWorldLayoutTopographyState& topography) noexcept;
[[nodiscard]] bool creativeEditorToolActionEnabled(
    const CreativeEditorToolActionSpec& action,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept;

}  // namespace iggy3d_creative_app
