#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

class SdlWindow;
struct StaticMeshAssetCatalog;

}  // namespace iggy3d

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativePlacementClearanceCache;

enum class CreativeEditorToolOptionsCommandId : std::uint8_t {
  SetMaterialBrushSymmetryPivot,
  ClearMaterialBrushSymmetryPivot,
  EditGroupContents,
  TransformSelection,
  ResetSelectionTransform,
  DuplicateSelection,
  DeleteSelection,
  ToggleSelectionVisibility,
  ToggleSelectionLocked,
  ReattachAttachment,
  DetachAttachment,
  GroupSelection,
  UngroupSelection,
  SaveSelectionAsAsset,
  UpdateSavedAsset,
  RefreshSavedAssetInstance,
  RefreshSafeSavedAssetInstances,
  ForceRefreshSavedAssetInstances,
  SetMovingPlatformWaypointDwell,
  SetMovingPlatformSegmentSpeed,
  ToggleMovingPlatformPreview,
  RestartMovingPlatformPreview,
  RegeneratePatternRecipe,
  DetachPatternRecipe,
  Count,
};

inline constexpr std::size_t kCreativeEditorToolOptionsCommandCapacity = 24U;

struct CreativeEditorToolOptionsCommandList {
  std::array<CreativeEditorToolOptionsCommandId,
             kCreativeEditorToolOptionsCommandCapacity>
      ids{};
  std::size_t count = 0U;
};

inline constexpr std::size_t kCreativeEditorObjectActionCapabilityCount =
    static_cast<std::size_t>(
        iggy3d::creative::CreativeSemanticObjectAction::Count);

struct CreativeEditorObjectActionCapability {
  bool available = false;
  iggy3d::creative::CreativeSemanticObjectActionRoute route =
      iggy3d::creative::CreativeSemanticObjectActionRoute::Reject;
  std::string_view reasonCode = "creative_editor_object_action_not_requested";
};

struct CreativeEditorObjectActionCapabilities {
  std::array<CreativeEditorObjectActionCapability,
             kCreativeEditorObjectActionCapabilityCount>
      actions{};
};

struct CreativeEditorToolOptionsState {
  bool open = false;
  iggy3d::creative::CreativeHotbarEntry targetEntry{};
  iggy3d::creative::CreativeToolSettings draft;
  double placeCellSizeDraft = 1.0;
  iggy3d::creative::CreativeToolOptionList options;
  CreativeEditorToolOptionsCommandList commands;
  std::size_t selectedIndex = 0;
  iggy3d::creative::CreativeDocumentId contextDocumentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t contextDocumentRevision = 0U;
  iggy3d::creative::CreativeObjectId contextGroupId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId contextPrimaryObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativePatternRecipeId contextPatternRecipeId =
      iggy3d::creative::kInvalidCreativePatternRecipeId;
  iggy3d::creative::CreativePatternRecipeKind contextPatternRecipeKind =
      iggy3d::creative::CreativePatternRecipeKind::Count;
  std::uint64_t contextPatternRecipeSeed = 0U;
  std::size_t contextPatternGeneratedObjectCount = 0U;
  iggy3d::creative::CreativeObjectKind contextPrimaryObjectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  std::string contextPrimaryAssetId;
  iggy3d::creative::CreativeObjectKind contextContainerKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  std::string contextContainerAssetId;
  iggy3d::creative::CreativeObjectId contextAttachmentParentId =
      iggy3d::creative::kInvalidObjectId;
  std::string contextAttachmentSocket;
  iggy3d::creative::CreativeObjectId contextAttachmentAimTargetId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeVec3 contextAttachmentAimPoint{};
  bool contextAttachmentAimAvailable = false;
  std::size_t contextSelectionCount = 0U;
  iggy3d::creative::CreativeSemanticSelectionSetResolution
      contextSemanticSelection;
  std::uint64_t contextWorldLayoutRevision = 0U;
  std::uint64_t contextWorldLayoutGeneratedRevision = 0U;
  bool contextWorldLayoutSynchronized = false;
  bool contextPrimaryVisible = true;
  bool contextPrimaryLocked = false;
  bool contextAllUnlocked = false;
  bool contextAllMovable = false;
  bool contextAllResettable = false;
  CreativeEditorObjectActionCapabilities contextActionCapabilities;
  bool contextPrefabUpdateTransformSupported = false;
  bool contextPrefabSyncInspected = false;
  bool contextMovingPlatformPointSelected = false;
  bool contextMovingPlatformPointHasOutgoingSegment = false;
  std::uint8_t contextMovingPlatformPointIndex = 0U;
  double movingPlatformWaypointDwellDraft = 0.0;
  double movingPlatformSegmentSpeedDraft = 1.0;
  iggy3d::creative::CreativeAuthoredAssetSyncState contextPrefabSyncState =
      iggy3d::creative::CreativeAuthoredAssetSyncState::Conflict;
  std::size_t contextPrefabMatchedInstanceCount = 0U;
  std::size_t contextPrefabCurrentInstanceCount = 0U;
  std::size_t contextPrefabSourceChangedInstanceCount = 0U;
  std::size_t contextPrefabLocallyModifiedInstanceCount = 0U;
  std::size_t contextPrefabConflictInstanceCount = 0U;
};

struct CreativeEditorQuickEditState {
  iggy3d::creative::CreativeHotbarEntry targetEntry{
      iggy3d::creative::CreativeHeldItemKind::Count,
      iggy3d::creative::CreativeObjectKind::Unknown};
  iggy3d::creative::CreativeToolOptionList options;
  std::size_t selectedIndex = 0;
};

struct CreativeEditorToolOptionsFrameRequest {
  iggy3d::SdlWindow& window;
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeInputRouteResult& routedInput;
  bool openRequested = false;
  iggy3d::creative::CreativeHotbarEntry requestedEntry{};
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr;
  const CreativePlacementClearanceCache* placementClearanceCache = nullptr;
};

struct CreativeEditorToolOptionsFrameResult {
  bool blockWorldActions = false;
  bool openChanged = false;
  bool committed = false;
};

[[nodiscard]] CreativeEditorToolOptionsFrameResult
processCreativeEditorToolOptionsFrame(
    const CreativeEditorToolOptionsFrameRequest& request);

[[nodiscard]] iggy3d::creative::CreativeToolOptionList
creativeEditorToolOptionsForEntry(
    iggy3d::creative::CreativeHotbarEntry entry,
    const iggy3d::creative::CreativeToolSettings& settings) noexcept;
[[nodiscard]] CreativeEditorToolOptionsCommandList
creativeEditorToolOptionCommandsForEntry(
    iggy3d::creative::CreativeHotbarEntry entry,
    iggy3d::creative::CreativeObjectKind contextPrimaryObjectKind =
        iggy3d::creative::CreativeObjectKind::Unknown,
    iggy3d::creative::CreativePatternRecipeId contextPatternRecipeId =
        iggy3d::creative::kInvalidCreativePatternRecipeId,
    iggy3d::creative::CreativePatternRecipeKind contextPatternRecipeKind =
        iggy3d::creative::CreativePatternRecipeKind::Count) noexcept;
[[nodiscard]] std::size_t creativeEditorToolOptionsRowCount(
    const CreativeEditorToolOptionsState& state) noexcept;
[[nodiscard]] bool activateCreativeEditorToolOptionsSelection(
    CreativeEditorState& editor);
[[nodiscard]] bool activateCreativeEditorToolOptionsSelection(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr,
    const CreativePlacementClearanceCache* placementClearanceCache = nullptr);

void syncCreativeEditorQuickEdit(CreativeEditorState& editor);
[[nodiscard]] bool processCreativeEditorQuickEditAction(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action);
[[nodiscard]] std::string creativeEditorQuickEditStatusLabel(
    const CreativeEditorState& editor);

void appendCreativeEditorToolOptionsOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
