#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/document/LogicLink.hpp"
#include "EditorWorldLayout.hpp"

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

// SelectObjects: replace the persistent selection with these ids. primaryObjectId
// picks the primary; kInvalidObjectId falls back to the last surviving id.
// ClearSelection ignores its payload (an empty list selects nothing).
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

// DeleteObjects: an explicit id list, or the current selection when empty.
struct CreativeDesktopDeletePayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
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

struct CreativeDesktopMovingPlatformPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeMovingPlatformSettings settings;
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

// EditAssetSource lifecycle phase (unused by the other asset ops).
enum class CreativeDesktopAssetEditPhase : std::uint8_t {
  None,
  Begin,
  Save,
  Cancel,
};

// EquipAsset / EditAssetSource / RenameAsset / DuplicateAsset / DeleteAsset.
// name is used only by RenameAsset; editPhase only by EditAssetSource.
struct CreativeDesktopAssetOpPayload {
  std::string assetId;
  std::string name;
  CreativeDesktopAssetEditPhase editPhase = CreativeDesktopAssetEditPhase::None;
};

// RefreshInstances (uses mode) / UpdateAssetFromInstance (ignores mode). Both
// key off the in-document instance-root/group object id.
struct CreativeDesktopInstanceRefreshPayload {
  iggy3d::creative::CreativeObjectId instanceRootObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeAuthoredAssetRefreshMode mode =
      iggy3d::creative::CreativeAuthoredAssetRefreshMode::ForceAll;
};

struct CreativeDesktopWorldLayoutToolPayload {
  CreativeEditorWorldLayoutTool tool =
      CreativeEditorWorldLayoutTool::Select;
};

struct CreativeDesktopWorldLayoutCatalogAssetPayload {
  std::string assetId;
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

struct CreativeDesktopGeneratedLevelSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutLevelSettings settings;
};

struct CreativeDesktopWorldLayoutTerrainProfileSettingsPayload {
  std::size_t profileIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutTerrainProfileSettings settings;
};

struct CreativeDesktopWorldLayoutTerrainPathSettingsPayload {
  std::size_t pathIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutTerrainPathSettings settings;
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

struct CreativeDesktopWorldLayoutRoomSettingsPayload {
  std::size_t roomIndex = iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutRoomSettings settings;
};

struct CreativeDesktopGeneratedRoomSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutRoomSettings settings;
};

struct CreativeDesktopWorldLayoutRoomManipulationPayload {
  CreativeEditorWorldLayoutRoomManipulationPhase phase =
      CreativeEditorWorldLayoutRoomManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutVerticalConnectorSettingsPayload {
  std::size_t connectorIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutVerticalConnectorSettings settings;
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

struct CreativeDesktopWorldLayoutOpeningManipulationPayload {
  CreativeEditorWorldLayoutOpeningManipulationPhase phase =
      CreativeEditorWorldLayoutOpeningManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutConfirmPayload {
  std::vector<iggy3d::creative::CreativeWorldLayoutConflictDecision>
      conflictDecisions;
};

// The discriminated payload carried by every command (monostate = no payload).
using CreativeDesktopCommandPayload = std::variant<
    std::monostate,
    CreativeDesktopSaveAsPayload,
    CreativeDesktopSelectPayload,
    CreativeDesktopLogicLinkPayload,
    CreativeDesktopDeletePayload,
    CreativeDesktopRenamePayload,
    CreativeDesktopObjectFlagPayload,
    CreativeDesktopTransformPayload,
    CreativeDesktopMovingPlatformPayload,
    CreativeDesktopMovingPlatformPreviewPayload,
    CreativeDesktopMovingPlatformWaypointPayload,
    CreativeDesktopAssetOpPayload,
    CreativeDesktopInstanceRefreshPayload,
    CreativeDesktopWorldLayoutToolPayload,
    CreativeDesktopWorldLayoutCatalogAssetPayload,
    CreativeDesktopWorldLayoutBuildingSelectionPayload,
    CreativeDesktopWorldLayoutSourcePayload,
    CreativeDesktopWorldLayoutObjectSourcePayload,
    CreativeDesktopWorldLayoutSourceRenamePayload,
    CreativeDesktopWorldLayoutLevelOperationPayload,
    CreativeDesktopWorldLayoutLevelSettingsPayload,
    CreativeDesktopGeneratedLevelSettingsPayload,
    CreativeDesktopWorldLayoutTerrainProfileSettingsPayload,
    CreativeDesktopWorldLayoutTerrainPathSettingsPayload,
    CreativeDesktopWorldLayoutObjectSettingsPayload,
    CreativeDesktopWorldLayoutBuildingManipulationPayload,
    CreativeDesktopWorldLayoutBuildingDuplicatePayload,
    CreativeDesktopWorldLayoutBuildingTransformPayload,
    CreativeDesktopWorldLayoutBuildingTemplateCapturePayload,
    CreativeDesktopWorldLayoutBuildingTemplateSyncPayload,
    CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload,
    CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload,
    CreativeDesktopWorldLayoutPointPayload,
    CreativeDesktopWorldLayoutGesturePayload,
    CreativeDesktopWorldLayoutRoomSettingsPayload,
    CreativeDesktopGeneratedRoomSettingsPayload,
    CreativeDesktopWorldLayoutRoomManipulationPayload,
    CreativeDesktopWorldLayoutVerticalConnectorSettingsPayload,
    CreativeDesktopGeneratedVerticalConnectorSettingsPayload,
    CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload,
    CreativeDesktopWorldLayoutBoxSettingsPayload,
    CreativeDesktopWorldLayoutBoxManipulationPayload,
    CreativeDesktopWorldLayoutWallSettingsPayload,
    CreativeDesktopGeneratedWallSettingsPayload,
    CreativeDesktopWorldLayoutWallManipulationPayload,
    CreativeDesktopWorldLayoutOpeningSettingsPayload,
    CreativeDesktopGeneratedOpeningSettingsPayload,
    CreativeDesktopWorldLayoutOpeningInsertPayload,
    CreativeDesktopWorldLayoutAssetRepairPayload,
    CreativeDesktopWorldLayoutOpeningManipulationPayload,
    CreativeDesktopWorldLayoutConfirmPayload>;

}  // namespace iggy3d_creative_app
