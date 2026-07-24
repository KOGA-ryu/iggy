#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/creative/spatial/PlacementClearance.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/creative/tools/RecipeTransform.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

enum class CreativeEditorTransformOwnershipRoute : std::uint8_t {
  AuthoredObjects,
  PatternRecipe,
  TerrainOperation,
  WorldLayoutBuilding,
};

enum class CreativeEditorTransformSource : std::uint8_t {
  Clipboard,
  Selection,
};

enum class CreativeEditorTransformMode : std::uint8_t {
  Move,
  Rotate,
  Scale,
  Count,
};

enum class CreativeEditorTransformAnchorPolicy : std::uint8_t {
  FollowAim,
  FixedSource,
  FixedTarget,
};

enum class CreativeEditorTransformPointerGestureKind : std::uint8_t {
  None,
  Free,
  Axis,
};

struct CreativeEditorTransformAxisRaySample {
  bool valid = false;
  double rayParameter = 0.0;
  double axisParameter = 0.0;
};

struct CreativeEditorTransformPointerGestureState {
  CreativeEditorTransformPointerGestureKind kind =
      CreativeEditorTransformPointerGestureKind::None;
  cr::CreativeSelectionPlacementAxis axis =
      cr::CreativeSelectionPlacementAxis::Free;
  cr::CreativeVec3 initialAimAnchor{};
  cr::CreativeVec3 axisOrigin{};
  cr::CreativeVec3 axisDirection{};
  double initialAxisParameter = 0.0;
  bool changed = false;
};

enum class CreativeEditorTransformPivot : std::uint8_t {
  SelectionAnchor,
  ActiveObjectOrigin,
  IndividualOrigins,
  Count,
};

inline constexpr std::array<double, 9U> kCreativeEditorScaleFactors{
    0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};
inline constexpr std::size_t kCreativeEditorDefaultScaleIndex = 3U;

enum class CreativeEditorTransformControl : std::uint8_t {
  RotatePositive,
  MirrorX,
  CycleConstraint,
  ToggleMode,
  Confirm,
  Cancel,
  MirrorZ,
  RotateNegative,
  Reset,
  Count,
};

inline constexpr std::size_t kCreativeEditorTransformControlCount =
    static_cast<std::size_t>(CreativeEditorTransformControl::Count);

struct CreativeEditorTransformCommitReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  cr::CreativeSelectionPlacementMode mode =
      cr::CreativeSelectionPlacementMode::Copy;
  cr::CreativeClipboardPasteReceipt copyReceipt{};
  cr::CreativeSelectionPlacementReceipt moveReceipt{};
  cr::CreativePatternRecipeTranslationReceipt patternReceipt{};
  cr::CreativeTerrainOperationTranslationReceipt terrainReceipt{};
  cr::CreativeWorldLayoutApplyReceipt worldLayoutReceipt{};
  std::string reasonCode = "editor_transform_not_requested";
};

struct CreativeEditorTransformPreflight {
  bool requested = false;
  bool accepted = false;
  cr::CreativeSelectionPlacementCapabilities capabilities{};
  CreativeEditorTransformOwnershipRoute ownershipRoute =
      CreativeEditorTransformOwnershipRoute::AuthoredObjects;
  cr::CreativeSemanticSelectionOwner blockedOwner =
      cr::CreativeSemanticSelectionOwner::None;
  cr::CreativeWorldLayoutSourceRef worldLayoutSource{};
  cr::CreativePatternRecipeId patternRecipeId =
      cr::kInvalidCreativePatternRecipeId;
  cr::CreativeTerrainOperationId terrainOperationId =
      cr::kInvalidCreativeTerrainOperationId;
  std::uint64_t sourceDocumentRevision = 0U;
  std::uint64_t worldLayoutRevision = 0U;
  std::uint64_t worldLayoutSourceEpoch = 0U;
  cr::CreativeObjectId failedObjectId = cr::kInvalidObjectId;
  std::string reasonCode = "editor_transform_preflight_not_requested";
};

struct CreativeEditorSelectionTransformState {
  bool active = false;
  bool aimTargetPositionable = false;
  bool targetPositionable = false;
  bool commitRequested = false;
  bool controlsOpen = false;
  bool moveAvailable = false;
  bool fineNudgeActive = false;
  CreativeEditorTransformSource source =
      CreativeEditorTransformSource::Clipboard;
  CreativeEditorTransformMode transformMode =
      CreativeEditorTransformMode::Move;
  CreativeEditorTransformAnchorPolicy anchorPolicy =
      CreativeEditorTransformAnchorPolicy::FollowAim;
  CreativeEditorTransformPivot pivot =
      CreativeEditorTransformPivot::SelectionAnchor;
  cr::CreativeSelectionPlacementMode mode =
      cr::CreativeSelectionPlacementMode::Copy;
  cr::CreativeSelectionPlacementAxis constraint =
      cr::CreativeSelectionPlacementAxis::Free;
  cr::CreativeAxis3 rotationAxis = cr::CreativeAxis3::Y;
  cr::CreativeObjectId activeObjectId = cr::kInvalidObjectId;
  std::size_t selectedControl = 0;
  std::array<std::size_t, 3U> scaleFactorIndices{
      kCreativeEditorDefaultScaleIndex,
      kCreativeEditorDefaultScaleIndex,
      kCreativeEditorDefaultScaleIndex};
  std::int8_t rotationQuarterSteps = 0;
  double rotationDegrees = 0.0;
  cr::CreativeVec3 aimTargetAnchor{};
  cr::CreativeVec3 nudgeOffset{};
  double snapStepMeters = 1.0;
  cr::CreativeClipboard sourceClipboard{};
  std::vector<cr::CreativeObjectId> sourceObjectIds;
  cr::CreativeWorldLayout sourceWorldLayout{};
  cr::CreativeWorldLayout candidateWorldLayout{};
  cr::CreativeWorldLayoutPlan candidateWorldLayoutPlan{};
  std::uint64_t sourceWorldLayoutNextStableOrdinal = 1U;
  std::uint64_t candidateWorldLayoutNextStableOrdinal = 1U;
  std::size_t candidateWorldLayoutBuildingIndex =
      cr::kInvalidCreativeWorldLayoutIndex;
  bool candidateWorldLayoutReady = false;
  bool candidateWorldLayoutChanged = false;
  cr::CreativePatternRecipeTranslationPlan candidatePatternTranslation{};
  cr::CreativeTerrainOperationTranslationPlan candidateTerrainTranslation{};
  cr::CreativeSelectionPlacementRequest request{};
  cr::CreativeSelectionPlacementTargetResult targetResolution{};
  cr::CreativeSelectionPlacementNudgeReceipt lastNudge{};
  cr::CreativeSelectionPlacementPlan plan{};
  CreativeEditorTransformPreflight preflight{};
  CreativeEditorTransformCommitReceipt lastCommit{};
  CreativeEditorTransformPointerGestureState pointerGesture{};
  cr::CreativePlacementClearanceResult clearance{};
  cr::CreativeObjectId clearanceCandidateObjectId = cr::kInvalidObjectId;
  std::uint64_t clearanceCandidateObjectCount = 0U;
};

}  // namespace iggy3d_creative_app
