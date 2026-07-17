#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

enum class CreativeEditorWorldLayoutTool : std::uint8_t {
  Select,
  Room,
  Floor,
  Wall,
  Door,
  Window,
  Plateau,
  Road,
  Ditch,
  Bridge,
  Boulder,
  PlayerSpawn,
  NpcSpawn,
  BuildingShell,
  Count,
};

enum class CreativeEditorWorldLayoutPaletteCategory : std::uint8_t {
  Structure,
  Terrain,
  Object,
  Gameplay,
  Count,
};

enum class CreativeEditorWorldLayoutPaletteActivation : std::uint8_t {
  Tool,
  BuildingTemplate,
  Count,
};

struct CreativeEditorWorldLayoutPaletteEntry {
  CreativeEditorWorldLayoutPaletteCategory category =
      CreativeEditorWorldLayoutPaletteCategory::Structure;
  std::string_view label;
  CreativeEditorWorldLayoutPaletteActivation activation =
      CreativeEditorWorldLayoutPaletteActivation::Tool;
  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Select;
  std::string_view buildingTemplateId;
};

enum class CreativeEditorWorldLayoutSelectionKind : std::uint8_t {
  None,
  Room,
  Box,
  Wall,
  Opening,
  Building,
  TerrainProfile,
  TerrainPath,
  Object,
};

struct CreativeEditorWorldLayoutSelection {
  CreativeEditorWorldLayoutSelectionKind kind =
      CreativeEditorWorldLayoutSelectionKind::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeEditorWorldLayoutPoint {
  double x = 0.0;
  double z = 0.0;
};

struct CreativeEditorWorldLayoutRoomSettings {
  cr::CreativeWorldLayoutRect footprint;
  double floorTopLayer = 0.0;
  std::uint16_t wallHeightCells =
      cr::kDefaultCreativeWorldLayoutWallHeightCells;
  double wallThicknessCells =
      cr::kDefaultCreativeWorldLayoutWallThicknessCells;
  std::uint16_t floorThicknessLayers = 1U;
};

enum class CreativeEditorWorldLayoutRectHandle : std::uint8_t {
  None,
  Move,
  North,
  East,
  South,
  West,
  NorthWest,
  NorthEast,
  SouthEast,
  SouthWest,
  Count,
};

using CreativeEditorWorldLayoutRoomHandle =
    CreativeEditorWorldLayoutRectHandle;
using CreativeEditorWorldLayoutBoxHandle = CreativeEditorWorldLayoutRectHandle;

struct CreativeEditorWorldLayoutRoomTarget {
  std::size_t roomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutRoomHandle handle =
      CreativeEditorWorldLayoutRoomHandle::None;
};

enum class CreativeEditorWorldLayoutRectManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

using CreativeEditorWorldLayoutRoomManipulationPhase =
    CreativeEditorWorldLayoutRectManipulationPhase;
using CreativeEditorWorldLayoutBoxManipulationPhase =
    CreativeEditorWorldLayoutRectManipulationPhase;

struct CreativeEditorWorldLayoutRoomManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutRoomTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeWorldLayoutRect originalFootprint;
  cr::CreativeWorldLayoutRect previewFootprint;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_room_manipulation_inactive";
};

struct CreativeEditorWorldLayoutBoxSettings {
  cr::CreativeWorldLayoutRect footprint;
  double anchorLayer = 0.0;
  std::uint16_t layerCount = 1U;
};

struct CreativeEditorWorldLayoutBoxSettingsDraft {
  bool active = false;
  std::size_t boxIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutBoxSettings settings;
};

struct CreativeEditorWorldLayoutBoxTarget {
  std::size_t boxIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutBoxHandle handle =
      CreativeEditorWorldLayoutBoxHandle::None;
};

struct CreativeEditorWorldLayoutBoxManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutBoxTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeWorldLayoutRect originalFootprint;
  cr::CreativeWorldLayoutRect previewFootprint;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_box_manipulation_inactive";
};

struct CreativeEditorWorldLayoutWallSettings {
  cr::CreativeTerrainCoord2 start;
  cr::CreativeTerrainCoord2 end;
  double baseLayer = 0.0;
  std::uint16_t heightCells =
      cr::kDefaultCreativeWorldLayoutWallHeightCells;
  double thicknessCells =
      cr::kDefaultCreativeWorldLayoutWallThicknessCells;
};

struct CreativeEditorWorldLayoutWallSettingsDraft {
  bool active = false;
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutWallSettings settings;
};

enum class CreativeEditorWorldLayoutWallHandle : std::uint8_t {
  None,
  Move,
  Start,
  End,
  Count,
};

struct CreativeEditorWorldLayoutWallTarget {
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutWallHandle handle =
      CreativeEditorWorldLayoutWallHandle::None;
};

enum class CreativeEditorWorldLayoutWallManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutWallManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutWallTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeTerrainCoord2 originalStart;
  cr::CreativeTerrainCoord2 originalEnd;
  cr::CreativeTerrainCoord2 previewStart;
  cr::CreativeTerrainCoord2 previewEnd;
  double previewOpeningOffsetDeltaCells = 0.0;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_wall_manipulation_inactive";
};

using CreativeEditorWorldLayoutBuildingBounds =
    cr::CreativeWorldLayoutBuildingBounds;

enum class CreativeEditorWorldLayoutBuildingManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutBuildingManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutPoint startPoint;
  CreativeEditorWorldLayoutBuildingBounds originalBounds;
  std::int64_t previewDeltaXCells = 0;
  std::int64_t previewDeltaZCells = 0;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_building_manipulation_inactive";
};

enum class CreativeEditorWorldLayoutBuildingTransformPhase : std::uint8_t {
  Preview,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutBuildingTransformState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutBuildingTransformOperation operation =
      cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
  CreativeEditorWorldLayoutBuildingBounds sourceBounds;
  CreativeEditorWorldLayoutBuildingBounds previewBounds;
  cr::CreativeWorldLayout candidate;
  std::string reasonCode =
      "creative_editor_world_layout_building_transform_inactive";
};

inline constexpr std::size_t
    kCreativeEditorWorldLayoutBuildingTemplateCapacity = 256U;

struct CreativeEditorWorldLayoutBuildingTemplateLibrary {
  std::filesystem::path root;
  std::vector<cr::CreativeWorldLayoutBuildingTemplate> templates;
  std::uint64_t nextTemplateOrdinal = 1U;
  std::size_t selectedIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::string statusMessage = "building template library not loaded";
};

struct CreativeEditorWorldLayoutBuildingTemplateLoadReceipt {
  bool requested = false;
  bool accepted = false;
  std::size_t loadedCount = 0U;
  std::size_t rejectedCount = 0U;
  std::string reasonCode =
      "creative_editor_world_layout_building_template_load_not_requested";
};

struct CreativeEditorWorldLayoutBuildingTemplateInstallReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::size_t templateIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::string reasonCode =
      "creative_editor_world_layout_building_template_install_not_requested";
};

enum class CreativeEditorWorldLayoutBuildingTemplatePlacementPhase
    : std::uint8_t {
  Begin,
  Update,
  Transform,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutBuildingTemplatePlacementState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t templateIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutBuildingTemplate orientedTemplate;
  cr::CreativeTerrainCoord2 anchor;
  cr::CreativeWorldLayoutBuildingBounds previewBounds;
  cr::CreativeWorldLayout candidate;
  std::size_t resultBuildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t nextStableOrdinal = 1U;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_building_template_placement_inactive";
};

struct CreativeEditorWorldLayoutOpeningSettings {
  double centerOffsetCells = 0.0;
  double widthCells = 1.0;
  double sillHeightCells = 0.0;
  double heightCells = 2.1;
  cr::CreativeBuildingOpeningPose pose =
      cr::CreativeBuildingOpeningPose::Closed;
  bool includeInsert = true;
};

struct CreativeEditorWorldLayoutOpeningSettingsDraft {
  bool active = false;
  std::size_t openingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutOpeningSettings settings;
};

struct CreativeEditorWorldLayoutOpeningHost {
  bool valid = false;
  CreativeEditorWorldLayoutPoint start;
  CreativeEditorWorldLayoutPoint end;
  double lengthCells = 0.0;
  double wallHeightCells = 0.0;
};

enum class CreativeEditorWorldLayoutOpeningHandle : std::uint8_t {
  None,
  Move,
  Start,
  End,
  Count,
};

struct CreativeEditorWorldLayoutOpeningTarget {
  std::size_t openingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutOpeningHandle handle =
      CreativeEditorWorldLayoutOpeningHandle::None;
};

enum class CreativeEditorWorldLayoutOpeningManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutOpeningManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutOpeningTarget target;
  double startPointerOffsetCells = 0.0;
  double originalCenterOffsetCells = 0.0;
  double originalWidthCells = 0.0;
  double previewCenterOffsetCells = 0.0;
  double previewWidthCells = 0.0;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_opening_manipulation_inactive";
};

enum class CreativeEditorWorldLayoutGesturePhase : std::uint8_t {
  Begin,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutSnapshot {
  cr::CreativeWorldLayout source;
  std::uint64_t revision = 1U;
  std::uint64_t savedRevision = 1U;
  std::uint64_t generatedRevision = 1U;
  std::uint64_t nextStableOrdinal = 1U;
};

struct CreativeEditorWorldLayoutState {
  cr::CreativeWorldLayout source;
  std::uint64_t revision = 1U;
  std::uint64_t savedRevision = 1U;
  std::uint64_t generatedRevision = 1U;
  std::uint64_t nextStableOrdinal = 1U;
  CreativeEditorWorldLayoutSnapshot generatedBaseline;

  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Select;
  CreativeEditorWorldLayoutSelection selection;
  bool anchorActive = false;
  cr::CreativeTerrainCoord2 anchor{};
  CreativeEditorWorldLayoutRoomManipulationState roomManipulation;
  CreativeEditorWorldLayoutBoxManipulationState boxManipulation;
  CreativeEditorWorldLayoutWallManipulationState wallManipulation;
  CreativeEditorWorldLayoutBuildingManipulationState buildingManipulation;
  CreativeEditorWorldLayoutBuildingTransformState buildingTransform;
  CreativeEditorWorldLayoutBuildingTemplateLibrary buildingTemplates;
  CreativeEditorWorldLayoutBuildingTemplatePlacementState
      buildingTemplatePlacement;
  CreativeEditorWorldLayoutOpeningManipulationState openingManipulation;
  CreativeEditorWorldLayoutBoxSettingsDraft boxSettingsDraft;
  CreativeEditorWorldLayoutWallSettingsDraft wallSettingsDraft;
  CreativeEditorWorldLayoutOpeningSettingsDraft openingSettingsDraft;

  bool previewVisible = false;
  std::uint64_t previewLayoutRevision = 0U;
  cr::CreativeWorldLayoutPreviewResult preview;
  std::string statusMessage = "layout ready";

  // Canvas-only view state. It is neither document nor layout truth.
  float canvasPixelsPerCell = 28.0F;
  float canvasPanX = 0.0F;
  float canvasPanZ = 0.0F;
};

struct CreativeEditorWorldLayoutEditReceipt {
  bool accepted = false;
  bool changed = false;
  std::string reasonCode = "creative_editor_world_layout_not_requested";
};

struct CreativeEditorWorldLayoutPreviewReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutStatus status =
      cr::CreativeWorldLayoutStatus::NotRequested;
  std::string reasonCode = "creative_editor_world_layout_preview_not_requested";
};

struct CreativeEditorWorldLayoutApplyReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode = "creative_editor_world_layout_apply_not_requested";
};

[[nodiscard]] const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutPaletteCategoryLabel(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept;
[[nodiscard]] std::span<const CreativeEditorWorldLayoutPaletteEntry>
creativeEditorWorldLayoutPaletteEntries() noexcept;

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey = "world_layout");
void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout);
void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept;

// Links a picked generated object back to the synchronized 2D source symbol.
// The ordinary document selection remains available to 3D tools.
[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object);
[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object,
    cr::CreativeGridSettings grid,
    cr::CreativeVec3 worldPoint);

[[nodiscard]] bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] const cr::CreativeDocument&
creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept;
[[nodiscard]] const cr::CreativeWorldLayout&
creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTool(CreativeEditorWorldLayoutState& state,
                                 CreativeEditorWorldLayoutTool tool);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutPoint(CreativeEditorWorldLayoutState& state,
                                    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutGesture(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutGesturePhase phase,
    CreativeEditorWorldLayoutPoint point = {});
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingShell(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutRoomTarget
findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool readCreativeEditorWorldLayoutBoxSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutBoxSettings(
    CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutBoxTarget
findCreativeEditorWorldLayoutBoxTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBoxManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBoxManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool readCreativeEditorWorldLayoutWallSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutWallSettings(
    CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutWallTarget
findCreativeEditorWorldLayoutWallTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutWallManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutWallManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] std::size_t creativeEditorWorldLayoutSelectedBuilding(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool readCreativeEditorWorldLayoutBuildingBounds(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBounds& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutBuilding(CreativeEditorWorldLayoutState& state,
                                        std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
clearCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTransform(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingTransformPhase phase,
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
[[nodiscard]] bool defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t& deltaXCells, std::int64_t& deltaZCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
duplicateCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t deltaXCells, std::int64_t deltaZCells);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutBuilding(CreativeEditorWorldLayoutState& state,
                                        std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutBuildingTemplateLoadReceipt
loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const std::filesystem::path& creativeSaveRoot);
[[nodiscard]] CreativeEditorWorldLayoutBuildingTemplateInstallReceipt
installCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const cr::CreativeWorldLayoutBuildingTemplate& sourceTemplate);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
captureCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    std::string label = {});
[[nodiscard]] cr::CreativeWorldLayoutBuildingTemplateSyncReceipt
inspectCreativeEditorWorldLayoutBuildingTemplateSync(
    const CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingTemplateFromInstance(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    cr::CreativeWorldLayoutBuildingTemplateRefreshMode mode);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t templateIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
[[nodiscard]] bool readCreativeEditorWorldLayoutOpeningSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutOpeningHost
resolveCreativeEditorWorldLayoutOpeningHost(
    const CreativeEditorWorldLayoutState& state,
    std::size_t openingIndex) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutOpeningTarget
findCreativeEditorWorldLayoutOpeningTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSelection(CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 const cr::CreativeDocument& document);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
confirmCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 cr::CreativeAppState& appState);

}  // namespace iggy3d_creative_app
