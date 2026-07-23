#pragma once

#include "app/iggy3d/creative/document/TerrainContours.hpp"
#include "app/iggy3d/creative/document/TerrainField.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"
#include "app/iggy3d/creative/tools/TerrainPath.hpp"
#include "app/iggy3d/creative/tools/TerrainProfile.hpp"
#include "app/iggy3d/creative/tools/TerrainSculpt.hpp"
#include "app/iggy3d/creative/tools/TerrainStamp.hpp"
#include "app/iggy3d/creative/world/WorldLayoutTerrainImpact.hpp"
#include "render/FrameInput.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

struct CreativeAppState;
class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct WorldRay;

inline constexpr std::size_t kCreativeTerrainStrokeVisitedCapacity =
    iggy3d::creative::kCreativeTerrainControlCapacity;

struct CreativeTerrainStrokeState {
  iggy3d::creative::CreativeMaterialRepeatState repeat{};
  iggy3d::creative::CreativeDocumentHistoryTransaction transaction{};
  std::array<iggy3d::creative::CreativeTerrainCoord2,
             kCreativeTerrainStrokeVisitedCapacity>
      visited{};
  std::uint16_t visitedCount = 0U;
  std::uint16_t acceptedMutationCount = 0U;
  bool capacityReached = false;
  bool cancelOnly = false;
};

enum class CreativeTerrainGradeHandle : std::uint8_t {
  Start,
  End,
  Count,
};

enum class CreativeTerrainGradeControl : std::uint8_t {
  StartHeight,
  EndHeight,
  HalfWidth,
  CrossSlope,
  Falloff,
  Count,
};

struct CreativeTerrainGradeState {
  bool active = false;
  CreativeTerrainGradeHandle selectedHandle = CreativeTerrainGradeHandle::End;
  CreativeTerrainGradeControl selectedControl =
      CreativeTerrainGradeControl::EndHeight;
  iggy3d::creative::CreativeTerrainOperationId editingOperationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  iggy3d::creative::CreativeTerrainGradeRecipe recipe{};
  iggy3d::creative::CreativeTerrainOperationMutationPlan operationPreview{};
  iggy3d::creative::CreativeDocumentId sourceDocumentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t sourceDocumentRevision = 0U;
  std::uint64_t previewCount = 0U;
  std::string statusMessage;
};

struct CreativeTerrainSculptStrokeState {
  iggy3d::creative::CreativeMaterialRepeatState repeat{};
  iggy3d::creative::CreativeDocumentHistoryTransaction transaction{};
  std::uint16_t acceptedMutationCount = 0U;
};

struct CreativeTerrainSculptPreviewCache {
  bool valid = false;
  bool renderAccepted = false;
  std::uint64_t documentId = 0U;
  std::uint64_t documentRevision = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeTerrainCoord2 center{};
  iggy3d::creative::CreativeTerrainSculptMode mode =
      iggy3d::creative::CreativeTerrainSculptMode::Flatten;
  iggy3d::creative::CreativeTerrainSculptFalloff falloff =
      iggy3d::creative::CreativeTerrainSculptFalloff::Uniform;
  iggy3d::creative::CreativeTerrainSculptMask mask =
      iggy3d::creative::CreativeTerrainSculptMask::Circle;
  std::uint16_t radiusCells = 4U;
  std::uint16_t strengthCells = 1U;
  std::uint16_t targetHeightCells = 4U;
  std::uint16_t contourIntervalCells = 2U;
  std::uint16_t contourMajorEvery = 5U;
  std::uint64_t sampledColumnCoordinateCount = 0U;
  std::uint64_t candidatePatchCoordinateCount = 0U;
  iggy3d::creative::CreativeTerrainSculptPlan plan{};
  iggy3d::creative::CreativeTerrainContourPlan contours{};
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch> patches;
};

struct CreativeTerrainSculptState {
  CreativeTerrainSculptStrokeState stroke{};
  CreativeTerrainSculptPreviewCache preview{};
};

enum class CreativeTerrainProfileControl : std::uint8_t {
  BaseHeight,
  Radius,
  Amplitude,
  Direction,
  Frequency,
  Spacing,
  Seed,
  Count,
};

struct CreativeTerrainProfilePreviewCache {
  bool valid = false;
  bool renderAccepted = false;
  std::uint64_t documentId = 0U;
  std::uint64_t documentRevision = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeTerrainProfileRecipe recipe{};
  iggy3d::creative::CreativeTerrainOperationMutationPlan operationPreview{};
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch> patches;
};

struct CreativeTerrainProfileState {
  iggy3d::creative::CreativeTerrainOperationId editingOperationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  bool baseLocked = false;
  std::uint16_t lockedBaseHeightCells = 4U;
  std::uint16_t resolvedBaseHeightCells = 4U;
  CreativeTerrainProfileControl selectedControl =
      CreativeTerrainProfileControl::Amplitude;
  CreativeTerrainProfilePreviewCache preview{};
};

struct CreativeTerrainPathPreviewCache {
  bool valid = false;
  bool renderAccepted = false;
  std::uint64_t documentId = 0U;
  std::uint64_t documentRevision = 0U;
  std::uint64_t buildCount = 0U;
  std::uint8_t lockedPointCount = 0U;
  iggy3d::creative::CreativeTerrainPathSourceRecipe recipe{};
  iggy3d::creative::CreativeTerrainPathSourceCache sourceCache{};
  iggy3d::creative::CreativeTerrainPathDirtySegments dirtySegments{};
  std::uint64_t generatedControlCount = 0U;
  std::uint64_t rebuiltSegmentCount = 0U;
  std::uint64_t reusedSegmentCount = 0U;
  iggy3d::creative::CreativeTerrainOperationMutationPlan operationPreview{};
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch> patches;
};

struct CreativeTerrainPathState {
  std::array<iggy3d::creative::CreativeTerrainPathSourcePoint,
             iggy3d::creative::kCreativeTerrainPathPointCapacity>
      points{};
  std::uint8_t pointCount = 0U;
  iggy3d::creative::CreativeTerrainPathSourcePointId nextPointId = 1U;
  iggy3d::creative::CreativeTerrainPathSourcePointId selectedPointId =
      iggy3d::creative::kInvalidCreativeTerrainPathSourcePointId;
  bool selectedPointFollowsPointer = false;
  iggy3d::creative::CreativeTerrainOperationId editingOperationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  iggy3d::creative::CreativeTerrainPathCurvePolicy curve =
      iggy3d::creative::CreativeTerrainPathCurvePolicy::Linear;
  iggy3d::creative::CreativeTerrainPathCrossSection crossSection =
      iggy3d::creative::CreativeTerrainPathCrossSection::Flat;
  iggy3d::creative::CreativeTerrainPathEndpointJoin startJoin =
      iggy3d::creative::CreativeTerrainPathEndpointJoin::Open;
  iggy3d::creative::CreativeTerrainPathEndpointJoin endJoin =
      iggy3d::creative::CreativeTerrainPathEndpointJoin::Open;
  std::uint16_t falloffCells = 2U;
  bool paintSurface = true;
  iggy3d::creative::CreativeTerrainMaterial material =
      iggy3d::creative::CreativeTerrainMaterial::Dirt;
  CreativeTerrainPathPreviewCache preview{};
};

struct CreativeTerrainRegionPreviewCache {
  bool valid = false;
  bool renderAccepted = false;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeTerrainOperationId editingOperationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  iggy3d::creative::CreativeTerrainRegionRecipe recipe{};
  iggy3d::creative::CreativeTerrainOperationMutationPlan operationPreview{};
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch> patches;
};

enum class CreativeTerrainStampTransformControl : std::uint8_t {
  Rotation,
  MirrorX,
  MirrorZ,
  HeightOffset,
  Count,
};

struct CreativeTerrainStampPreviewCache {
  bool valid = false;
  bool renderAccepted = false;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t stampSignature = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeTerrainStampRecipe recipe{};
  iggy3d::creative::CreativeTerrainStampPlan plan{};
  iggy3d::creative::CreativeTerrainOperationMutationPlan operationPreview{};
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch> patches;
};

struct CreativeTerrainStampPlacementState {
  bool active = false;
  std::uint8_t quarterTurns = 0U;
  bool mirrorX = false;
  bool mirrorZ = false;
  std::int16_t heightOffsetCells = 0;
  CreativeTerrainStampTransformControl selectedControl =
      CreativeTerrainStampTransformControl::Rotation;
  iggy3d::creative::CreativeTerrainStampCopyReceipt lastCopy{};
  iggy3d::creative::CreativeTerrainOperationMutationReceipt lastMutation{};
  CreativeTerrainStampPreviewCache preview{};
};

struct CreativeTerrainRegionState {
  iggy3d::creative::CreativeTerrainOperationId editingOperationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  CreativeTerrainRegionPreviewCache preview{};
  CreativeTerrainStampPlacementState stamp{};
};

struct CreativeTerrainContourDisplayState {
  bool visible = false;
  std::uint16_t intervalCells = 2U;
  std::uint16_t majorEvery = 5U;
  bool cacheValid = false;
  bool sourceOverride = false;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t terrainRevision = 0U;
  std::uint64_t terrainHeightRevision = 0U;
  std::uint64_t sourceKey = 0U;
  std::uint16_t cachedIntervalCells = 0U;
  std::uint16_t cachedMajorEvery = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeTerrainContourPlan plan{};
};

struct CreativeEditorTerrainState {
  std::uint16_t heightCells = 4U;
  std::uint16_t radiusCells = 4U;
  std::uint64_t documentId = 0U;
  bool hoverValid = false;
  iggy3d::creative::CreativeTerrainCoord2 hoverCoord{};
  bool selectionValid = false;
  iggy3d::creative::CreativeTerrainCoord2 selectedCoord{};
  iggy3d::creative::CreativeTerrainControlPoint selectedOriginal{};
  iggy3d::creative::CreativeTerrainMutationReceipt lastMutation{};
  CreativeTerrainStrokeState stroke{};
  CreativeTerrainGradeState grade{};
  CreativeTerrainSculptState sculpt{};
  CreativeTerrainProfileState profile{};
  CreativeTerrainPathState path{};
  CreativeTerrainRegionState region{};
  CreativeTerrainContourDisplayState contours{};
};

enum class CreativeEditorTerrainEditKind : std::uint8_t {
  Upsert,
  Remove,
  Sample,
};

struct CreativeEditorTerrainEditReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainEditKind kind =
      CreativeEditorTerrainEditKind::Upsert;
  iggy3d::creative::CreativeTerrainCoord2 coord{};
  iggy3d::creative::CreativeTerrainMutationReceipt mutation{};
  std::string_view reasonCode = "creative_editor_terrain_not_requested";
};

enum class CreativeEditorTerrainGradeAction : std::uint8_t {
  Anchor,
  Apply,
  Cancel,
};

struct CreativeEditorTerrainGradeReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainGradeAction action =
      CreativeEditorTerrainGradeAction::Anchor;
  iggy3d::creative::CreativeTerrainCoord2 targetCoord{};
  iggy3d::creative::CreativeTerrainOperationMutationPlan plan{};
  iggy3d::creative::CreativeTerrainOperationMutationReceipt operation{};
  iggy3d::creative::CreativeHistoryRecordReceipt history{};
  std::string_view reasonCode = "creative_editor_terrain_grade_not_requested";
};

enum class CreativeEditorTerrainSculptAction : std::uint8_t {
  Apply,
  SampleHeight,
  Cancel,
};

struct CreativeEditorTerrainSculptReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainSculptAction action =
      CreativeEditorTerrainSculptAction::Apply;
  iggy3d::creative::CreativeTerrainCoord2 targetCoord{};
  iggy3d::creative::CreativeTerrainSculptPlan plan{};
  iggy3d::creative::CreativeTerrainMutationReceipt mutation{};
  std::string_view reasonCode = "creative_editor_terrain_sculpt_not_requested";
};

enum class CreativeEditorTerrainProfileAction : std::uint8_t {
  Apply,
  SelectOperation,
  LockBase,
  UnlockBase,
};

struct CreativeEditorTerrainProfileReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainProfileAction action =
      CreativeEditorTerrainProfileAction::Apply;
  iggy3d::creative::CreativeTerrainCoord2 targetCoord{};
  iggy3d::creative::CreativeTerrainOperationMutationPlan plan{};
  iggy3d::creative::CreativeTerrainOperationMutationReceipt operation{};
  iggy3d::creative::CreativeHistoryRecordReceipt history{};
  std::string_view reasonCode = "creative_editor_terrain_profile_not_requested";
};

enum class CreativeEditorTerrainPathAction : std::uint8_t {
  AddPoint,
  RemovePoint,
  ReorderPoint,
  Apply,
  Cancel,
};

struct CreativeEditorTerrainPathReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainPathAction action =
      CreativeEditorTerrainPathAction::AddPoint;
  iggy3d::creative::CreativeTerrainPathSourcePoint targetPoint{};
  iggy3d::creative::CreativeTerrainOperationMutationPlan plan{};
  iggy3d::creative::CreativeTerrainOperationMutationReceipt operation{};
  iggy3d::creative::CreativeHistoryRecordReceipt history{};
  std::string_view reasonCode = "creative_editor_terrain_path_not_requested";
};

enum class CreativeEditorTerrainRegionAction : std::uint8_t {
  Apply,
  SelectOperation,
  SampleHeight,
  Cancel,
};

struct CreativeEditorTerrainRegionReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainRegionAction action =
      CreativeEditorTerrainRegionAction::Apply;
  iggy3d::creative::CreativeTerrainOperationId operationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  iggy3d::creative::CreativeTerrainOperationMutationPlan plan{};
  iggy3d::creative::CreativeTerrainOperationMutationReceipt operation{};
  iggy3d::creative::CreativeHistoryRecordReceipt history{};
  std::string_view reasonCode = "creative_editor_terrain_region_not_requested";
};

enum class CreativeEditorTerrainStampAction : std::uint8_t {
  Copy,
  BeginPreview,
  Apply,
  Cancel,
};

struct CreativeEditorTerrainStampReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorTerrainStampAction action =
      CreativeEditorTerrainStampAction::Copy;
  iggy3d::creative::CreativeTerrainStampCopyReceipt copy{};
  iggy3d::creative::CreativeTerrainStampPlan plan{};
  iggy3d::creative::CreativeTerrainOperationMutationReceipt operation{};
  iggy3d::creative::CreativeHistoryRecordReceipt history{};
  std::string_view reasonCode = "creative_editor_terrain_stamp_not_requested";
};

[[nodiscard]] bool resolveCreativeEditorTerrainPointerCoord(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2& target) noexcept;

[[nodiscard]] bool resolveCreativeEditorTerrainGradeTarget(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2& target) noexcept;

[[nodiscard]] CreativeEditorTerrainEditReceipt
applyCreativeEditorTerrainEditWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorTerrainEditKind kind,
    std::string_view source);

[[nodiscard]] CreativeEditorTerrainGradeReceipt
beginCreativeEditorTerrainGrade(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor);
[[nodiscard]] CreativeEditorTerrainGradeReceipt
applyCreativeEditorTerrainGradeWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] CreativeEditorTerrainGradeReceipt
cancelCreativeEditorTerrainGrade(CreativeEditorState& editor) noexcept;
[[nodiscard]] bool refreshCreativeEditorTerrainGradePreview(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor);
[[nodiscard]] bool creativeEditorTerrainGradePreviewMatches(
    const CreativeTerrainGradeState& state,
    const iggy3d::creative::CreativeDocument& document) noexcept;

[[nodiscard]] iggy3d::creative::CreativeTerrainSculptPlan
planCreativeEditorTerrainSculpt(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2 target) noexcept;
[[nodiscard]] CreativeEditorTerrainSculptReceipt
applyCreativeEditorTerrainSculptWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] CreativeEditorTerrainSculptReceipt
sampleCreativeEditorTerrainSculptHeight(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainSculptReceipt
cancelCreativeEditorTerrainSculpt(CreativeEditorState& editor) noexcept;

[[nodiscard]] iggy3d::creative::CreativeTerrainOperationMutationPlan
planCreativeEditorTerrainProfile(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainCoord2 target);
[[nodiscard]] CreativeEditorTerrainProfileReceipt
selectCreativeEditorTerrainProfileOperation(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor,
    iggy3d::creative::CreativeTerrainOperationId operationId) noexcept;
[[nodiscard]] CreativeEditorTerrainProfileReceipt
applyCreativeEditorTerrainProfileWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] CreativeEditorTerrainProfileReceipt
lockCreativeEditorTerrainProfileBase(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainProfileReceipt
unlockCreativeEditorTerrainProfileBase(CreativeEditorState& editor) noexcept;

[[nodiscard]] iggy3d::creative::CreativeTerrainOperationMutationPlan
planCreativeEditorTerrainPath(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor);
[[nodiscard]] CreativeEditorTerrainPathReceipt
addCreativeEditorTerrainPathPoint(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainPathReceipt
removeCreativeEditorTerrainPathPoint(CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainPathReceipt
reorderCreativeEditorTerrainPathPoint(CreativeEditorState& editor,
                                      int direction) noexcept;
[[nodiscard]] CreativeEditorTerrainPathReceipt
cancelCreativeEditorTerrainPath(CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainPathReceipt
applyCreativeEditorTerrainPathWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);

[[nodiscard]] iggy3d::creative::CreativeTerrainOperationMutationPlan
planCreativeEditorTerrainRegion(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeVolumeSelection& selection) noexcept;
[[nodiscard]] bool buildCreativeEditorTerrainRegionRecipe(
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeVolumeSelection& selection,
    iggy3d::creative::CreativeTerrainRegionRecipe& output) noexcept;
[[nodiscard]] CreativeEditorTerrainRegionReceipt
selectCreativeEditorTerrainRegionOperationAtPointer(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainRegionReceipt
applyCreativeEditorTerrainRegionWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] CreativeEditorTerrainRegionReceipt
sampleCreativeEditorTerrainRegionHeight(
    const iggy3d::creative::CreativeDocument& document,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainRegionReceipt
cancelCreativeEditorTerrainRegion(CreativeEditorState& editor) noexcept;

[[nodiscard]] CreativeEditorTerrainStampReceipt
copyCreativeEditorTerrainRegionToStamp(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainStampReceipt
beginCreativeEditorTerrainStampPreview(
    const iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor) noexcept;
[[nodiscard]] CreativeEditorTerrainStampReceipt
applyCreativeEditorTerrainStampWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] CreativeEditorTerrainStampReceipt
cancelCreativeEditorTerrainStamp(CreativeEditorState& editor) noexcept;

[[nodiscard]] bool processCreativeEditorTerrainQuickEdit(
    CreativeEditorTerrainState& state,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainQuickEditLabel(
    const CreativeEditorTerrainState& state);
[[nodiscard]] bool processCreativeEditorTerrainGradeQuickEdit(
    CreativeTerrainGradeState& state,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainGradeQuickEditLabel(
    const CreativeTerrainGradeState& state);
[[nodiscard]] bool processCreativeEditorTerrainSculptQuickEdit(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainSculptQuickEditLabel(
    const CreativeEditorState& editor);
[[nodiscard]] bool processCreativeEditorTerrainProfileQuickEdit(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainProfileQuickEditLabel(
    const CreativeEditorState& editor);
[[nodiscard]] bool processCreativeEditorTerrainPathQuickEdit(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainPathQuickEditLabel(
    const CreativeEditorState& editor);
[[nodiscard]] bool processCreativeEditorTerrainRegionQuickEdit(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainRegionQuickEditLabel(
    const CreativeEditorState& editor);
[[nodiscard]] bool processCreativeEditorTerrainStampQuickEdit(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;
[[nodiscard]] std::string creativeEditorTerrainStampQuickEditLabel(
    const CreativeEditorState& editor);

void clearCreativeEditorTerrainInteraction(
    CreativeEditorTerrainState& state,
    std::uint64_t documentId) noexcept;
void updateCreativeEditorTerrainAim(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    WorldRay ray,
    float occluderDistanceMeters) noexcept;

void processCreativeTerrainStrokeFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds);
void finalizeCreativeTerrainStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);
void processCreativeTerrainSculptStrokeFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds);
void finalizeCreativeTerrainSculptStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);

[[nodiscard]] bool refreshCreativeEditorTerrainSculptPreview(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor);
void appendCreativeEditorTerrainSculptOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);
[[nodiscard]] bool refreshCreativeEditorTerrainProfilePreview(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor);
void appendCreativeEditorTerrainProfileOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);
[[nodiscard]] bool refreshCreativeEditorTerrainPathPreview(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor);
void appendCreativeEditorTerrainPathOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);
[[nodiscard]] bool refreshCreativeEditorTerrainRegionPreview(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor);
void appendCreativeEditorTerrainRegionOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);
[[nodiscard]] bool refreshCreativeEditorTerrainStampPreview(
    CreativeEditorTerrainState& state,
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeTerrainStamp& stamp);
void appendCreativeEditorTerrainStampOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

void appendCreativeEditorTerrainFootprintOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    iggy3d::creative::CreativeGridSettings grid,
    iggy3d::creative::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness);
void appendCreativeEditorTerrainControlGuide(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    iggy3d::creative::CreativeGridSettings grid,
    iggy3d::creative::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness);
void appendCreativeEditorTerrainPatchSlopeTriangles(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const iggy3d::creative::CreativeTerrainSurfacePatch& patch,
    float thickness);

struct CreativeEditorTerrainSourceImpactOverlayFacts {
  bool active = false;
  iggy3d::creative::CreativeWorldLayoutTerrainImpactStatus status =
      iggy3d::creative::CreativeWorldLayoutTerrainImpactStatus::NoEffect;
  std::size_t controlCount = 0U;
  std::size_t materialCellCount = 0U;
  std::size_t edgeCount = 0U;
  bool influenceCellsClipped = false;
};

[[nodiscard]] CreativeEditorTerrainSourceImpactOverlayFacts
appendCreativeEditorWorldLayoutTerrainImpactOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode = false);

void appendCreativeEditorTerrainOverlay(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode = false);

[[nodiscard]] bool refreshCreativeEditorTerrainContours(
    CreativeTerrainContourDisplayState& state,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeTerrainSurfacePlan* surfaceOverride =
        nullptr,
    std::uint64_t sourceKey = 0U);

[[nodiscard]] std::size_t appendCreativeEditorTerrainContourPlan(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeTerrainContourPlan& plan,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

[[nodiscard]] std::size_t appendCreativeEditorTerrainContours(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode = false);

}  // namespace iggy3d_creative_app
