#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/document/LogicLink.hpp"
#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"
#include "app/iggy3d/creative/tools/MeasurementAnnotation.hpp"
#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"
#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

// Typed payloads for the desktop command families (Step 3 / plan DD-7). Each
// command id reads exactly one payload alternative; a mismatched id/payload is a
// no-op failure in the dispatcher, never a reinterpretation. This replaces the
// former single std::string arg so widgets (Step 4+) depend on a typed contract.
// The payload lives inside a fixed-capacity, copyable command frame; the vector/
// string members keep it copyable and default-constructible.

// SaveDocumentAs: the target save id (the former bare std::string arg).
struct CreativeDesktopSaveAsPayload {
  std::string saveId;
};

// RegenerateMapTemplate: explicit replacement of the current document and its
// synchronized world-layout source from one built-in map recipe.
struct CreativeDesktopMapTemplatePayload {
  std::string templateId;
};

struct CreativeDesktopMeasurementAnnotationPayload {
  iggy3d::creative::CreativeMeasurementAnnotationId annotationId =
      iggy3d::creative::kInvalidCreativeMeasurementAnnotationId;
  std::string name;
};

// SelectObjects: replace the persistent selection with these ids. An empty list
// clears selection. primaryObjectId picks the primary; kInvalidObjectId falls
// back to the last surviving id.
struct CreativeDesktopSelectPayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  iggy3d::creative::CreativeObjectId primaryObjectId =
      iggy3d::creative::kInvalidObjectId;
};

// SetLogicSource reads sourceObjectId. SetLogicLink and RemoveLogicLink read
// both endpoints; RemoveLogicLink ignores action. One typed payload keeps the
// desktop dispatcher aligned with the canonical CreativeLogicLink contract.
struct CreativeDesktopLogicLinkPayload {
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId targetObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeLogicLinkAction action =
      iggy3d::creative::CreativeLogicLinkAction::Toggle;
};

// RenameObject: absolute rename of a single object.
struct CreativeDesktopRenamePayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::string name;
};

// SetObjectsVisible / SetObjectsLocked: an absolute bool over an id list (or the
// current selection when empty).
struct CreativeDesktopObjectFlagPayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  bool value = true;
};

// SetObjectTransform: absolute transform of a single object. The component flags
// select which of position/rotation/scale to write.
struct CreativeDesktopTransformPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeTransform transform;
  bool setPosition = false;
  bool setRotation = false;
  bool setScale = false;
};

struct CreativeDesktopGroupPivotPayload {
  iggy3d::creative::CreativeObjectId groupObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeVec3 pivot;
};

struct CreativeDesktopMovingPlatformPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeMovingPlatformSettings settings;
};

struct CreativeDesktopPlayerSpawnPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativePlayerSpawnSettings settings;
};

struct CreativeDesktopNpcSpawnPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeNpcSpawnSettings settings;
};

struct CreativeDesktopLootPointPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeLootPointSettings settings;
};

struct CreativeDesktopExitPointPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeExitPointSettings settings;
};

struct CreativeDesktopMovingPlatformPreviewPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  double normalizedProgress = 0.0;
};

struct CreativeDesktopMovingPlatformWaypointPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t pointIndex = 0U;
  double dwellSeconds = 0.0;
};

struct CreativeDesktopTerrainOperationPayload {
  iggy3d::creative::CreativeTerrainOperationId operationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  bool enabled = true;
  std::size_t targetIndex = 0U;
};

struct CreativeDesktopTerrainStampPayload {
  std::string assetId;
  std::string label;
  iggy3d::creative::CreativeTerrainOperationId operationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  bool replaceExisting = false;
};

struct CreativeDesktopWorldLayoutToolPayload {
  CreativeEditorWorldLayoutTool tool =
      CreativeEditorWorldLayoutTool::Select;
};

struct CreativeDesktopWorldLayoutCatalogAssetPayload {
  std::string assetId;
};

struct CreativeDesktopWorldLayoutBuildingBlockoutPayload {
  CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
};

struct CreativeDesktopWorldLayoutBuildingBlockoutUpdatePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
};

struct CreativeDesktopWorldLayoutBuildingSelectionPayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutSourcePayload {
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  // Optional for immediate commands; delayed dialogs provide it so an index
  // shift cannot redirect the action to another source symbol.
  std::string stableKey;
  // Plan-view commands carry the exact visible storey. Other callers leave it
  // invalid and retain the source owner's normal active-level resolution.
  std::size_t preferredLevelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutObjectSourcePayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
};

struct CreativeDesktopWorldLayoutSourceRenamePayload {
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string name;
};

struct CreativeDesktopWorldLayoutLevelOperationPayload {
  CreativeEditorWorldLayoutLevelOperation operation =
      CreativeEditorWorldLayoutLevelOperation::Select;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutLevelSettingsPayload {
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutLevelSettings settings;
};

struct CreativeDesktopWorldLayoutLevelDatumPayload {
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutLevelEditScope scope =
      CreativeEditorWorldLayoutLevelEditScope::Selected;
  double floorTopLayer = 0.0;
};

struct CreativeDesktopWorldLayoutRoofApertureCreatePayload {
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeStructuralRoofApertureKind kind =
      iggy3d::creative::CreativeStructuralRoofApertureKind::Skylight;
};

struct CreativeDesktopWorldLayoutRoofApertureManipulationPayload {
  CreativeEditorWorldLayoutRoofApertureManipulationPhase phase =
      CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutRoofManipulationPayload {
  CreativeEditorWorldLayoutRoofManipulationPhase phase =
      CreativeEditorWorldLayoutRoofManipulationPhase::Begin;
  CreativeEditorWorldLayoutRoofTarget target;
  double coordinateCells = 0.0;
};

struct CreativeDesktopGeneratedLevelSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutLevelSettings settings;
};

struct CreativeDesktopWorldLayoutObjectSettingsPayload {
  std::size_t objectIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutObjectSettings settings;
};

struct CreativeDesktopWorldLayoutBuildingManipulationPayload {
  CreativeEditorWorldLayoutBuildingManipulationPhase phase =
      CreativeEditorWorldLayoutBuildingManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutBuildingDuplicatePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
};

struct CreativeDesktopWorldLayoutBuildingTransformPayload {
  CreativeEditorWorldLayoutBuildingTransformPhase phase =
      CreativeEditorWorldLayoutBuildingTransformPhase::Preview;
  iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation operation =
      iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation::
      RotateRight90;
};

struct CreativeDesktopGeneratedBuildingOperationPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutGeneratedBuildingOperation operation =
      CreativeEditorWorldLayoutGeneratedBuildingOperation::Move;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
};

struct CreativeDesktopWorldLayoutBuildingGroundingPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutBuildingGroundingSettings settings;
};

struct CreativeDesktopWorldLayoutBuildingArchitecturePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  iggy3d::creative::CreativeWorldLayoutArchitecturalProfile profile;
};

struct CreativeDesktopWorldLayoutBuildingTemplateCapturePayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string label;
};

struct CreativeDesktopWorldLayoutBuildingTemplateSyncPayload {
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutBuildingTemplateRefreshMode mode =
      iggy3d::creative::CreativeWorldLayoutBuildingTemplateRefreshMode::
          SafeInstances;
};

struct CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload {
  std::size_t templateIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload {
  CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase =
      CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation operation =
      iggy3d::creative::CreativeWorldLayoutBuildingTransformOperation::
          RotateRight90;
};

struct CreativeDesktopWorldLayoutPointPayload {
  CreativeEditorWorldLayoutPoint point;
};

struct CreativeDesktopWorldLayoutGesturePayload {
  CreativeEditorWorldLayoutGesturePhase phase =
      CreativeEditorWorldLayoutGesturePhase::Begin;
  CreativeEditorWorldLayoutPoint point;
};

struct CreativeDesktopGeneratedRoomSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t roomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutRoomSettings settings;
};

struct CreativeDesktopWorldLayoutRoomSplitPayload {
  std::size_t roomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutRoomSplitAxis axis =
      iggy3d::creative::CreativeWorldLayoutRoomSplitAxis::Count;
  std::int32_t coordinate = 0;
};

struct CreativeDesktopWorldLayoutRoomMergePayload {
  std::size_t primaryRoomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::size_t secondaryRoomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutWallSplitPayload {
  std::size_t topologyEdgeIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::uint32_t offsetCells = 0U;
};

struct CreativeDesktopWorldLayoutWallMergePayload {
  std::size_t primaryTopologyEdgeIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::size_t secondaryTopologyEdgeIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutRoomManipulationPayload {
  CreativeEditorWorldLayoutRoomManipulationPhase phase =
      CreativeEditorWorldLayoutRoomManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload {
  CreativeEditorWorldLayoutRoomBoundaryManipulationPhase phase =
      CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutRoomCornerManipulationPayload {
  CreativeEditorWorldLayoutRoomCornerManipulationPhase phase =
      CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopGeneratedVerticalConnectorSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutVerticalConnectorSettings settings;
};

struct CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload {
  CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase =
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
  CreativeEditorWorldLayoutVerticalConnectorTarget target;
};

struct CreativeDesktopWorldLayoutBoxSettingsPayload {
  std::size_t boxIndex = iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutBoxSettings settings;
};

struct CreativeDesktopWorldLayoutBoxManipulationPayload {
  CreativeEditorWorldLayoutBoxManipulationPhase phase =
      CreativeEditorWorldLayoutBoxManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutWallSettingsPayload {
  std::size_t wallIndex = iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutWallSettings settings;
};

struct CreativeDesktopGeneratedWallSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutWallSettings settings;
};

struct CreativeDesktopWorldLayoutWallManipulationPayload {
  CreativeEditorWorldLayoutWallManipulationPhase phase =
      CreativeEditorWorldLayoutWallManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutOpeningSettingsPayload {
  std::size_t openingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutOpeningSettings settings;
};

struct CreativeDesktopGeneratedOpeningSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutOpeningSettings settings;
};

enum class CreativeDesktopWorldLayoutPropertyEditPhase : std::uint8_t {
  Preview,
  Commit,
  Cancel,
  Count,
};

using CreativeDesktopWorldLayoutPropertySettings =
    CreativeEditorWorldLayoutPropertySettings;

template <typename Settings>
[[nodiscard]] constexpr iggy3d::creative::CreativeWorldLayoutTable
creativeDesktopWorldLayoutPropertyTable() noexcept {
  if constexpr (std::is_same_v<Settings,
                               CreativeEditorWorldLayoutLevelSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Level;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutRoomMetadata>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Room;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutRoomSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Room;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutTopologyEdgeSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::TopologyEdge;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutVerticalConnectorSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::VerticalConnector;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutBoxSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Box;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutWallSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Wall;
  } else if constexpr (
      std::is_same_v<Settings, CreativeEditorWorldLayoutOpeningSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Opening;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutRoofApertureSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::RoofAperture;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutTerrainProfileSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::TerrainProfile;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutTerrainPathSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::TerrainPath;
  } else {
    static_assert(
        std::is_same_v<Settings, CreativeEditorWorldLayoutObjectSettings>);
    return iggy3d::creative::CreativeWorldLayoutTable::Object;
  }
}

struct CreativeDesktopWorldLayoutPropertyEditPayload {
  CreativeDesktopWorldLayoutPropertyEditPhase phase =
      CreativeDesktopWorldLayoutPropertyEditPhase::Preview;
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeDesktopWorldLayoutPropertySettings settings;
};

struct CreativeDesktopWorldLayoutOpeningInsertPayload {
  std::size_t openingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutOpeningInsertOperation operation =
      CreativeEditorWorldLayoutOpeningInsertOperation::FitAssetToOpening;
  std::string assetId;
  iggy3d::creative::CreativeVec3 assetScale{1.0, 1.0, 1.0};
};

enum class CreativeDesktopWorldLayoutAssetRepairOperation : std::uint8_t {
  RefreshBounds,
  ReplaceAsset,
  UseProceduralInsert,
  Count,
};

struct CreativeDesktopWorldLayoutAssetRepairPayload {
  CreativeDesktopWorldLayoutAssetRepairOperation operation =
      CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds;
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string expectedAssetId;
  std::string replacementAssetId;
};

struct CreativeDesktopWorldLayoutBuildingRepairPayload {
  iggy3d::creative::CreativeWorldLayoutBuildingUsabilityIssue issue;
  std::string stableKey;
};

struct CreativeDesktopWorldLayoutOpeningManipulationPayload {
  CreativeEditorWorldLayoutOpeningManipulationPhase phase =
      CreativeEditorWorldLayoutOpeningManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutConfirmPayload {
  std::vector<iggy3d::creative::CreativeWorldLayoutConflictDecision>
      conflictDecisions;
  std::vector<iggy3d::creative::CreativeWorldLayoutTerrainConflictDecision>
      terrainConflictDecisions;
};

// The discriminated payload carried by every command (monostate = no payload).
using CreativeDesktopCommandPayload = std::variant<
    std::monostate,
    CreativeDesktopSaveAsPayload,
    CreativeDesktopMapTemplatePayload,
    CreativeDesktopMeasurementAnnotationPayload,
    CreativeDesktopSelectPayload,
    CreativeDesktopLogicLinkPayload,
    CreativeDesktopRenamePayload,
    CreativeDesktopObjectFlagPayload,
    CreativeDesktopTransformPayload,
    CreativeDesktopGroupPivotPayload,
    CreativeDesktopMovingPlatformPayload,
    CreativeDesktopPlayerSpawnPayload,
    CreativeDesktopNpcSpawnPayload,
    CreativeDesktopLootPointPayload,
    CreativeDesktopExitPointPayload,
    CreativeDesktopMovingPlatformPreviewPayload,
    CreativeDesktopMovingPlatformWaypointPayload,
    CreativeDesktopTerrainOperationPayload,
    CreativeDesktopTerrainStampPayload,
    CreativeDesktopWorldLayoutToolPayload,
    CreativeDesktopWorldLayoutCatalogAssetPayload,
    CreativeDesktopWorldLayoutBuildingBlockoutPayload,
    CreativeDesktopWorldLayoutBuildingBlockoutUpdatePayload,
    CreativeDesktopWorldLayoutBuildingSelectionPayload,
    CreativeDesktopWorldLayoutSourcePayload,
    CreativeDesktopWorldLayoutObjectSourcePayload,
    CreativeDesktopWorldLayoutSourceRenamePayload,
    CreativeDesktopWorldLayoutLevelOperationPayload,
    CreativeDesktopWorldLayoutLevelSettingsPayload,
    CreativeDesktopWorldLayoutLevelDatumPayload,
    CreativeDesktopWorldLayoutRoofApertureCreatePayload,
    CreativeDesktopWorldLayoutRoofApertureManipulationPayload,
    CreativeDesktopWorldLayoutRoofManipulationPayload,
    CreativeDesktopGeneratedLevelSettingsPayload,
    CreativeDesktopWorldLayoutObjectSettingsPayload,
    CreativeDesktopWorldLayoutBuildingManipulationPayload,
    CreativeDesktopWorldLayoutBuildingDuplicatePayload,
    CreativeDesktopWorldLayoutBuildingTransformPayload,
    CreativeDesktopGeneratedBuildingOperationPayload,
    CreativeDesktopWorldLayoutBuildingGroundingPayload,
    CreativeDesktopWorldLayoutBuildingArchitecturePayload,
    CreativeDesktopWorldLayoutBuildingTemplateCapturePayload,
    CreativeDesktopWorldLayoutBuildingTemplateSyncPayload,
    CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload,
    CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload,
    CreativeDesktopWorldLayoutPointPayload,
    CreativeDesktopWorldLayoutGesturePayload,
    CreativeDesktopGeneratedRoomSettingsPayload,
    CreativeDesktopWorldLayoutRoomSplitPayload,
    CreativeDesktopWorldLayoutRoomMergePayload,
    CreativeDesktopWorldLayoutWallSplitPayload,
    CreativeDesktopWorldLayoutWallMergePayload,
    CreativeDesktopWorldLayoutRoomManipulationPayload,
    CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload,
    CreativeDesktopWorldLayoutRoomCornerManipulationPayload,
    CreativeDesktopGeneratedVerticalConnectorSettingsPayload,
    CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload,
    CreativeDesktopWorldLayoutBoxSettingsPayload,
    CreativeDesktopWorldLayoutBoxManipulationPayload,
    CreativeDesktopWorldLayoutWallSettingsPayload,
    CreativeDesktopGeneratedWallSettingsPayload,
    CreativeDesktopWorldLayoutWallManipulationPayload,
    CreativeDesktopWorldLayoutOpeningSettingsPayload,
    CreativeDesktopGeneratedOpeningSettingsPayload,
    CreativeDesktopWorldLayoutPropertyEditPayload,
    CreativeDesktopWorldLayoutOpeningInsertPayload,
    CreativeDesktopWorldLayoutAssetRepairPayload,
    CreativeDesktopWorldLayoutBuildingRepairPayload,
    CreativeDesktopWorldLayoutOpeningManipulationPayload,
    CreativeDesktopWorldLayoutConfirmPayload>;

}  // namespace iggy3d_creative_app
