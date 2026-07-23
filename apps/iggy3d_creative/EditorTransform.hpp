#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/spatial/PlacementClearance.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"
#include "app/iggy3d/creative/tools/RecipeTransform.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

class SdlWindow;

}  // namespace iggy3d

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

struct CreativeEditorState;
struct CreativeEditorWorldLayoutState;
struct CreativePlacementClearanceCache;

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

struct CreativeEditorTransformFrameRequest {
  iggy3d::SdlWindow& window;
  cr::CreativeAppState& appState;
  CreativeEditorState& editor;
  const cr::CreativeInputRouteResult& routedInput;
  float directionX = 0.0F;
  float directionY = 0.0F;
  std::int32_t nudgeWheelSteps = 0;
  bool fineNudge = false;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct CreativeEditorTransformFrameResult {
  bool blockWorldActions = false;
  bool openChanged = false;
};

[[nodiscard]] std::string_view toString(
    CreativeEditorTransformControl control) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeEditorTransformMode mode) noexcept;
[[nodiscard]] cr::CreativeVec3 creativeEditorTransformScaleFactor(
    const CreativeEditorSelectionTransformState& state) noexcept;

[[nodiscard]] bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    CreativeEditorTransformAnchorPolicy anchorPolicy =
        CreativeEditorTransformAnchorPolicy::FollowAim,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] bool beginCreativeEditorTerrainOperationTransformPreview(
    const cr::CreativeAppState& appState,
    cr::CreativeTerrainOperationId operationId,
    CreativeEditorSelectionTransformState& state,
    std::string_view source);
[[nodiscard]] bool requestCreativeEditorSelectionTransformCommit(
    CreativeEditorSelectionTransformState& state) noexcept;
[[nodiscard]] bool cancelCreativeEditorSelectionTransformPreview(
    CreativeEditorSelectionTransformState& state,
    std::string_view source);
[[nodiscard]] bool setCreativeEditorTransformConstraint(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis constraint);
[[nodiscard]] bool setCreativeEditorTransformTargetAnchor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 targetAnchor);
[[nodiscard]] bool setCreativeEditorTransformRotationDegrees(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeAxis3 axis, double degrees);
[[nodiscard]] bool setCreativeEditorTransformScaleFactor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 scaleFactor);
[[nodiscard]] bool setCreativeEditorTransformPlacementMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementMode mode);
[[nodiscard]] bool setCreativeEditorTransformPivot(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformPivot pivot);
[[nodiscard]] bool setCreativeEditorTransformCoordinateSpace(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementCoordinateSpace coordinateSpace);
[[nodiscard]] bool resumeCreativeEditorTransformAim(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state);
[[nodiscard]] CreativeEditorTransformAxisRaySample
sampleCreativeEditorTransformAxisRay(
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection) noexcept;
[[nodiscard]] bool beginCreativeEditorFreeTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 initialAimAnchor);
[[nodiscard]] bool beginCreativeEditorAxisTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis axis,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection);
[[nodiscard]] bool updateCreativeEditorTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection);
[[nodiscard]] bool finishCreativeEditorTransformPointerGesture(
    CreativeEditorSelectionTransformState& state,
    std::string_view source);
[[nodiscard]] bool nudgeCreativeEditorSelectionTransform(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t steps,
    bool fine);
[[nodiscard]] bool cycleCreativeEditorTransformMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state);
[[nodiscard]] bool adjustCreativeEditorTransformSetting(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction);
[[nodiscard]] bool applyCreativeEditorTransformControl(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformControl control);

[[nodiscard]] CreativeEditorTransformCommitReceipt
processCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    bool secondaryPressed,
    std::string_view source,
    double snapStepMeters = 1.0,
    CreativeEditorWorldLayoutState* worldLayout = nullptr,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

[[nodiscard]] CreativeEditorTransformFrameResult
processCreativeEditorTransformFrame(
    const CreativeEditorTransformFrameRequest& request);

[[nodiscard]] std::size_t appendCreativeEditorSelectionTransformPreview(
    const CreativeEditorSelectionTransformState& state,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

void appendCreativeEditorTransformOverlay(
    const CreativeEditorSelectionTransformState& state,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
