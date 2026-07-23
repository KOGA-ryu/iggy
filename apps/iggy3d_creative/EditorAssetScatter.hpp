#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "EditorEdits.hpp"
#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/AssetScatter.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;
class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativePlacementClearanceCache;

enum class CreativeEditorAssetScatterCandidateStatus : std::uint8_t {
  Ready,
  DensityRejected,
  SpacingRejected,
  OutsideMask,
  Excluded,
  Duplicate,
  CapacityRejected,
  MissingSurface,
  SlopeRejected,
  InvalidPlacement,
  Obstructed,
  Occupied,
};

struct CreativeEditorAssetScatterRejectedCandidate {
  iggy3d::creative::CreativeBounds worldBounds{};
  CreativeEditorAssetScatterCandidateStatus status =
      CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
  bool valid = false;
};

struct CreativeEditorAssetScatterCandidate {
  CreativeBrushPlacementPlan placement{};
  iggy3d::creative::CreativeVec3 surfacePosition{};
  std::uint64_t visitedKey = 0;
  CreativeEditorAssetScatterCandidateStatus status =
      CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
  bool placeable = false;
};

struct CreativeEditorAssetScatterPlan {
  std::array<CreativeEditorAssetScatterCandidate,
             iggy3d::creative::kCreativeAssetScatterGeneratedObjectCapacity>
      candidates{};
  std::array<CreativeEditorAssetScatterRejectedCandidate,
             iggy3d::creative::kCreativeAssetScatterEvaluationCapacity>
      rejectedCandidates{};
  std::size_t candidateCount = 0;
  std::size_t rejectedCandidateCount = 0;
  std::size_t placeableCount = 0;
  iggy3d::creative::CreativeAssetScatterStatus kernelStatus =
      iggy3d::creative::CreativeAssetScatterStatus::NotRequested;
  bool requested = false;
  bool accepted = false;
  bool truncated = false;

  [[nodiscard]] std::span<const CreativeEditorAssetScatterCandidate> items()
      const noexcept {
    return {candidates.data(), candidateCount};
  }

  [[nodiscard]] std::span<const CreativeEditorAssetScatterRejectedCandidate>
  rejectedItems() const noexcept {
    return {rejectedCandidates.data(), rejectedCandidateCount};
  }
};

struct CreativeAssetScatterStrokeState {
  iggy3d::creative::CreativeWorldGestureRepeatState repeat{};
  StandaloneEditTransaction transaction{};
  iggy3d::creative::CreativeWorldGestureVisitedKeys visited{};
  CreativeEditorAssetScatterPlan preview{};
  iggy3d::creative::CreativeAssetScatterRecipe previewRecipeKey{};
  std::vector<iggy3d::creative::CreativeObjectId>
      previewSelectionFilterKey;
  iggy3d::creative::CreativeAssetScatterRecipe recipe{};
  std::vector<iggy3d::creative::CreativeObjectId> selectionFilterObjectIds;
  iggy3d::creative::CreativePatternRecipeId recipeId =
      iggy3d::creative::kInvalidCreativePatternRecipeId;
  std::uint16_t acceptedMutationCount = 0;
  std::uint64_t previewDocumentId = 0U;
  std::uint64_t previewDocumentRevision = 0U;
  std::uint64_t previewBuildCount = 0U;
  bool recipeActive = false;
  bool capacityReached = false;
  bool previewCacheValid = false;
  bool previewPlannerRequested = false;
};

[[nodiscard]] bool creativeEditorUsesAssetScatter(
    const iggy3d::creative::CreativeHotbarEntry& held,
    const iggy3d::creative::CreativeToolSettings& settings) noexcept;

[[nodiscard]] CreativeEditorAssetScatterPlan
buildCreativeEditorAssetScatterPlan(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeHotbarEntry& held,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

[[nodiscard]] iggy3d::creative::CreativeAssetScatterRecipe
makeCreativeEditorAssetScatterRecipe(
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeHotbarEntry& held);

[[nodiscard]] CreativeEditorAssetScatterPlan
buildCreativeEditorAssetScatterRecipePlan(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeAssetScatterRecipe& recipe,
    std::span<const iggy3d::creative::CreativeObjectId>
        selectionFilterObjectIds = {},
    std::span<const iggy3d::creative::CreativeObjectId> ignoredObjectIds = {},
    const CreativePlacementClearanceCache* clearanceCache = nullptr,
    bool includeKernelRejections = false);

void processCreativeAssetScatterFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

void finalizeCreativeAssetScatterStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);

[[nodiscard]] iggy3d::creative::CreativeAssetScatterRecipeMutationReceipt
regenerateCreativeEditorAssetScatterRecipeWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    iggy3d::creative::CreativePatternRecipeId recipeId,
    const CreativePlacementClearanceCache* clearanceCache,
    std::string_view source);

[[nodiscard]] std::size_t appendCreativeEditorAssetScatterWireframes(
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
