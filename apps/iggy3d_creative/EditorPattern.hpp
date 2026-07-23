#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/tools/Pattern.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;
struct CreativeToolSettings;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativePlacementClearanceCache;

struct CreativeEditorPatternState {
  iggy3d::creative::CreativeLinearArrayReceipt lastReceipt;
  iggy3d::creative::CreativeRadialArrayReceipt lastRadialReceipt;
};

enum class CreativeEditorPatternPreviewStatus : std::uint8_t {
  NotRequested,
  ToolInactive,
  EmptySource,
  MissingSource,
  PivotUnavailable,
  DegeneratePivot,
  PlanRejected,
  InvalidGeometry,
  Collision,
  Ready,
};

struct CreativeEditorPatternPreviewObject {
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::uint32_t instanceOrdinal = 0U;
  iggy3d::creative::CreativeBounds worldBounds{};
  bool colliding = false;
};

struct CreativeEditorPatternPreviewReceipt {
  bool requested = false;
  bool accepted = false;
  bool collisionFree = false;
  CreativeEditorPatternPreviewStatus status =
      CreativeEditorPatternPreviewStatus::NotRequested;
  iggy3d::creative::CreativePatternRecipeKind kind =
      iggy3d::creative::CreativePatternRecipeKind::Count;
  iggy3d::creative::CreativeLinearArrayRequest linear{};
  iggy3d::creative::CreativeRadialArrayRequest radial{};
  std::uint64_t sourceObjectCount = 0U;
  std::uint64_t generatedObjectCount = 0U;
  std::uint64_t collidingObjectCount = 0U;
  std::uint64_t testedDocumentObjectCount = 0U;
  std::uint64_t testedGeneratedPairCount = 0U;
  iggy3d::creative::CreativeObjectId blockingObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeBounds finalBounds{};
  bool hasFinalBounds = false;
  std::array<CreativeEditorPatternPreviewObject,
             iggy3d::creative::kCreativeLinearArrayGeneratedObjectCapacity>
      objects{};
  std::size_t objectCount = 0U;
  std::string_view reasonCode = "creative_pattern_preview_not_requested";

  [[nodiscard]] std::span<const CreativeEditorPatternPreviewObject>
  generatedObjects() const noexcept {
    return {objects.data(), objectCount};
  }
};

[[nodiscard]] const iggy3d::creative::CreativePatternRecipe*
creativeEditorSelectedPatternRecipe(
    const iggy3d::creative::CreativeAppState& appState) noexcept;

// Loads the durable relationship parameters into the normal shared tool
// settings. The caller keeps cellSize separately because that scalar is the
// exact recipe value, while the common settings expose snap as fixed choices.
[[nodiscard]] bool loadCreativeEditorPatternRecipeSettings(
    const iggy3d::creative::CreativePatternRecipe& recipe,
    iggy3d::creative::CreativeToolSettings& settings,
    double& cellSize) noexcept;

[[nodiscard]] iggy3d::creative::CreativeLinearArrayRequest
creativeEditorLinearArrayRequest(
    const iggy3d::creative::CreativeToolSettings& settings,
    double cellSize) noexcept;

[[nodiscard]] iggy3d::creative::CreativeRadialArrayRequest
creativeEditorRadialArrayRequest(
    const iggy3d::creative::CreativeToolSettings& settings,
    iggy3d::creative::CreativeVec3 pivot) noexcept;

[[nodiscard]] iggy3d::creative::CreativeLinearArrayReceipt
applyCreativeEditorLinearArrayWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const iggy3d::creative::CreativeToolSettings& settings,
    double cellSize,
    std::string_view source);

[[nodiscard]] iggy3d::creative::CreativeRadialArrayReceipt
applyCreativeEditorRadialArrayWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const iggy3d::creative::CreativeToolSettings& settings,
    iggy3d::creative::CreativeVec3 pivot,
    std::string_view source);

[[nodiscard]] bool applyCreativeEditorArrayWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorPatternState& state,
    const iggy3d::creative::CreativeToolSettings& settings,
    double cellSize,
    bool pivotValid,
    iggy3d::creative::CreativeVec3 pivot,
    std::string_view source);

[[nodiscard]] iggy3d::creative::CreativePatternRecipeMutationReceipt
detachCreativeEditorPatternRecipeWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    iggy3d::creative::CreativePatternRecipeId recipeId,
    std::string_view source);

// Bounded transient evaluation shared by rendering and headless product tests.
// Collision is advisory: legacy overlap behavior remains available, while the
// preview makes the consequence visible before commit.
[[nodiscard]] CreativeEditorPatternPreviewReceipt
evaluateCreativeEditorArrayPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

[[nodiscard]] std::size_t appendCreativeEditorLinearArrayPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

[[nodiscard]] std::size_t appendCreativeEditorRadialArrayPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

[[nodiscard]] std::size_t appendCreativeEditorArrayPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

}  // namespace iggy3d_creative_app
