#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "EditorEdits.hpp"
#include "EditorPicking.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;
class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativeEditorPickFrame;

struct CreativeEditorWorldTarget {
  bool valid = false;
  bool objectHit = false;
  bool voxelHit = false;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  iggy3d::creative::CreativeGridCoord3 voxelCell{};
  float distanceMeters = 0.0F;
  WorldRay ray{};
  iggy3d::creative::CreativeGridTarget grid{};
};

enum class CreativeEditorPlacementFeedbackStatus : std::uint8_t {
  None,
  Placed,
  Rejected,
};

inline constexpr std::uint64_t kCreativeEditorPlacementFeedbackFrames = 36U;

struct CreativeEditorPlacementFeedback {
  CreativeEditorPlacementFeedbackStatus status =
      CreativeEditorPlacementFeedbackStatus::None;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  std::uint64_t frameIndex = 0;
  bool voxelPlaced = false;
  iggy3d::creative::CreativeGridCoord3 voxelCell{};
  iggy3d::creative::CreativeBounds voxelBounds{};
};

inline constexpr std::size_t kCreativeMaterialStrokeVisitedCapacity = 256U;

using CreativeMaterialStrokeKind =
    iggy3d::creative::CreativeMaterialStrokeKind;
using CreativeMaterialRepeatState =
    iggy3d::creative::CreativeMaterialRepeatState;
using CreativeMaterialRepeatRequest =
    iggy3d::creative::CreativeMaterialRepeatRequest;
using CreativeMaterialRepeatResult =
    iggy3d::creative::CreativeMaterialRepeatResult;
using iggy3d::creative::stepCreativeMaterialRepeat;

struct CreativeMaterialStrokeVisitedKey {
  iggy3d::creative::CreativeGridCoord3 cell{};
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
};

struct CreativeMaterialStrokeState {
  CreativeMaterialRepeatState repeat{};
  StandaloneEditTransaction transaction{};
  std::array<CreativeMaterialStrokeVisitedKey,
             kCreativeMaterialStrokeVisitedCapacity>
      visited{};
  std::uint16_t visitedCount = 0;
  std::uint16_t acceptedMutationCount = 0;
  bool capacityReached = false;
};

[[nodiscard]] constexpr bool creativeEditorPlacementFeedbackVisible(
    const CreativeEditorPlacementFeedback& feedback,
    std::uint64_t frameIndex) noexcept {
  return feedback.status != CreativeEditorPlacementFeedbackStatus::None &&
         frameIndex >= feedback.frameIndex &&
         frameIndex - feedback.frameIndex <
             kCreativeEditorPlacementFeedbackFrames;
}

struct CreativeEditorInteractionState {
  iggy3d::creative::CreativeHotbarState hotbar{};
  iggy3d::creative::CreativeWorldActionRouterState actionRouter{};
  CreativeEditorWorldTarget target{};
  CreativeEditorPlacementFeedback placementFeedback{};
  CreativeMaterialStrokeState materialStroke{};
  iggy3d::creative::CreativeObjectId moveTargetId =
      iggy3d::creative::kInvalidObjectId;
};

struct CreativeEditorWorldInteractionFrameRequest {
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeWorldActionFrame& actions;
  iggy3d::creative::CreativeInputModifierMask modifiers =
      iggy3d::creative::kCreativeInputModifierNone;
  const iggy3d::RenderCameraFrame& camera;
  const CreativeEditorPickFrame& pickFrame;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  std::uint64_t monotonicTimeNanoseconds = 0;
  bool captureMode = false;
};

[[nodiscard]] CreativeEditorWorldTarget resolveCreativeEditorWorldTarget(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    const CreativeEditorPickFrame& pickFrame,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    double cellSize);
[[nodiscard]] double creativeEditorTargetCellSize(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor) noexcept;

void syncCreativeEditorHeldItem(iggy3d::creative::CreativeAppState& appState,
                                CreativeEditorState& editor);
[[nodiscard]] bool selectCreativeEditorHotbarSlot(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::size_t slot);
[[nodiscard]] bool confirmCreativeEditorHeldItem(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);
[[nodiscard]] bool cancelCreativeEditorHeldItem(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor);
[[nodiscard]] std::string creativeEditorHeldItemStatusLabel(
    const CreativeEditorState& editor);

void processCreativeEditorWorldInteractionFrame(
    const CreativeEditorWorldInteractionFrameRequest& request);

void processCreativeMaterialStrokeFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds);

void finalizeCreativeMaterialStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);

void appendCreativeEditorInteractionOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float wireThickness,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
