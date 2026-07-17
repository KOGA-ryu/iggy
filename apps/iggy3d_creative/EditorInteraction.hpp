#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "EditorConnectedFill.hpp"
#include "EditorAssetScatter.hpp"
#include "EditorAuthoredAssets.hpp"
#include "EditorEdits.hpp"
#include "EditorPicking.hpp"
#include "EditorPathEditing.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorStructuralPlacement.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;
class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d {

struct StaticMeshAssetCatalog;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativeEditorPickFrame;

struct CreativeEditorWorldTarget {
  bool valid = false;
  bool objectHit = false;
  bool voxelHit = false;
  bool terrainHit = false;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  iggy3d::creative::CreativeGridCoord3 voxelCell{};
  iggy3d::creative::CreativeTerrainCoord2 terrainCell{};
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

struct CreativeMaterialBrushPivotState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  bool aimAvailable = false;
  bool locked = false;
  iggy3d::creative::CreativeGridCoord3 aimCell{};
  iggy3d::creative::CreativeGridCoord3 lockedCell{};
};

void synchronizeCreativeMaterialBrushPivotDocument(
    CreativeMaterialBrushPivotState& state,
    iggy3d::creative::CreativeDocumentId documentId) noexcept;
void resetCreativeMaterialBrushPivot(
    CreativeMaterialBrushPivotState& state,
    iggy3d::creative::CreativeDocumentId documentId) noexcept;
void updateCreativeMaterialBrushPivotAim(
    CreativeMaterialBrushPivotState& state,
    iggy3d::creative::CreativeDocumentId documentId,
    bool aimAvailable,
    iggy3d::creative::CreativeGridCoord3 aimCell = {}) noexcept;
[[nodiscard]] bool lockCreativeMaterialBrushPivotFromAim(
    CreativeMaterialBrushPivotState& state) noexcept;
[[nodiscard]] bool clearCreativeMaterialBrushPivot(
    CreativeMaterialBrushPivotState& state) noexcept;
[[nodiscard]] bool creativeMaterialBrushLockedPivot(
    const CreativeMaterialBrushPivotState& state,
    iggy3d::creative::CreativeDocumentId documentId,
    iggy3d::creative::CreativeGridCoord3& pivot) noexcept;

struct CreativeMaterialBrushGestureConfig {
  iggy3d::creative::CreativeMaterialBrushShape shape =
      iggy3d::creative::CreativeMaterialBrushShape::Sphere;
  iggy3d::creative::CreativeAxis3 axis =
      iggy3d::creative::CreativeAxis3::Y;
  iggy3d::creative::CreativeMaterialBrushSize size =
      iggy3d::creative::CreativeMaterialBrushSize::ThreeCells;
  iggy3d::creative::CreativeMaterialBrushFill fill =
      iggy3d::creative::CreativeMaterialBrushFill::Solid;
  iggy3d::creative::CreativeMaterialBrushGuide guide =
      iggy3d::creative::CreativeMaterialBrushGuide::Free;
  iggy3d::creative::CreativeMaterialBrushSymmetry symmetry =
      iggy3d::creative::CreativeMaterialBrushSymmetry::Off;
  iggy3d::creative::CreativeMaterialBrushMask mask =
      iggy3d::creative::CreativeMaterialBrushMask::Overwrite;
  iggy3d::creative::CreativeObjectKind replaceSourceKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
};

[[nodiscard]] inline CreativeMaterialBrushGestureConfig
creativeMaterialBrushGestureConfig(
    const iggy3d::creative::CreativeToolSettings& settings) noexcept {
  return {settings.materialBrushShape,
          settings.materialBrushAxis,
          settings.materialBrushSize,
          settings.materialBrushFill,
          settings.materialBrushGuide,
          settings.materialBrushSymmetry,
          settings.materialBrushMask,
          settings.materialBrushReplaceSourceKind};
}

[[nodiscard]] inline iggy3d::creative::CreativeMaterialBrushStampRequest
creativeMaterialBrushStampRequest(
    const CreativeMaterialBrushGestureConfig& config,
    iggy3d::creative::CreativeGridCoord3 center = {}) noexcept {
  iggy3d::creative::CreativeMaterialBrushStampRequest request;
  request.shape = config.shape;
  request.size = config.size;
  request.centerCell = center;
  request.axis = config.axis;
  request.guide = config.guide;
  request.fill = config.fill;
  return request;
}

inline void applyCreativeMaterialBrushGestureConfig(
    iggy3d::creative::CreativeToolSettings& settings,
    const CreativeMaterialBrushGestureConfig& config) noexcept {
  settings.materialBrushShape = config.shape;
  settings.materialBrushAxis = config.axis;
  settings.materialBrushSize = config.size;
  settings.materialBrushFill = config.fill;
  settings.materialBrushGuide = config.guide;
  settings.materialBrushSymmetry = config.symmetry;
  settings.materialBrushMask = config.mask;
  settings.materialBrushReplaceSourceKind = config.replaceSourceKind;
}

struct CreativeMaterialBrushPresetBank {
  std::array<CreativeMaterialBrushGestureConfig,
             iggy3d::creative::kCreativeHotbarSlotCount>
      slots{};
  std::array<std::uint8_t, iggy3d::creative::kCreativeHotbarSlotCount>
      initialized{};
};

[[nodiscard]] bool storeSelectedCreativeMaterialBrushPreset(
    CreativeMaterialBrushPresetBank& presets,
    const iggy3d::creative::CreativeHotbarState& hotbar,
    const iggy3d::creative::CreativeToolSettings& settings) noexcept;
[[nodiscard]] bool activateSelectedCreativeMaterialBrushPreset(
    CreativeMaterialBrushPresetBank& presets,
    const iggy3d::creative::CreativeHotbarState& hotbar,
    iggy3d::creative::CreativeToolSettings& settings) noexcept;
void clearCreativeMaterialBrushPresetSlot(
    CreativeMaterialBrushPresetBank& presets,
    std::size_t slot) noexcept;
[[nodiscard]] bool creativeMaterialBrushPresetForSlot(
    const CreativeMaterialBrushPresetBank& presets,
    std::size_t slot,
    CreativeMaterialBrushGestureConfig& preset) noexcept;
[[nodiscard]] std::string creativeMaterialBrushPresetHotbarLabel(
    const CreativeMaterialBrushGestureConfig& preset);

struct CreativeMaterialStrokeState {
  CreativeMaterialRepeatState repeat{};
  StandaloneEditTransaction transaction{};
  std::array<CreativeMaterialStrokeVisitedKey,
             kCreativeMaterialStrokeVisitedCapacity>
      visited{};
  std::uint16_t visitedCount = 0;
  std::uint16_t acceptedMutationCount = 0;
  bool capacityReached = false;
  bool hasLastBrushCenter = false;
  iggy3d::creative::CreativeGridCoord3 lastBrushCenter{};
  bool hasBrushAnchor = false;
  iggy3d::creative::CreativeGridCoord3 brushAnchor{};
  bool hasSymmetryPivot = false;
  iggy3d::creative::CreativeGridCoord3 symmetryPivot{};
  CreativeMaterialBrushGestureConfig brushConfig{};
};

enum class CreativeEditorContinuousGestureOwner : std::uint8_t {
  None,
  Material,
  AuthoredAsset,
  AssetScatter,
  TerrainControl,
  TerrainPaint,
  TerrainSculpt,
  Count,
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
  iggy3d::creative::CreativeHeldItemKind synchronizedHeldItemKind =
      iggy3d::creative::CreativeHeldItemKind::Count;
  CreativeEditorWorldTarget target{};
  CreativeEditorPlacementFeedback placementFeedback{};
  CreativeMaterialBrushPivotState materialBrushPivot{};
  CreativeMaterialBrushPresetBank materialBrushPresets{};
  CreativeMaterialStrokeState materialStroke{};
  CreativeEditorStructuralSpanState structuralSpan{};
  CreativeEditorStructuralSpanEditState structuralSpanEdit{};
  CreativeAssetScatterStrokeState assetScatter{};
  CreativeAuthoredAssetStrokeState authoredAssetStroke{};
  CreativeEditorConnectedFillCache connectedFill{};
  CreativeEditorSurfaceExtrudeCache surfaceExtrude{};
  CreativeMovingPlatformPathEditState movingPlatformPathEdit{};
  iggy3d::creative::CreativeObjectId moveTargetId =
      iggy3d::creative::kInvalidObjectId;
};

void clearCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction) noexcept;
void setCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    CreativeEditorPlacementFeedbackStatus status,
    std::uint64_t frameIndex,
    iggy3d::creative::CreativeObjectKind objectKind =
        iggy3d::creative::CreativeObjectKind::Unknown,
    iggy3d::creative::CreativeObjectId objectId =
        iggy3d::creative::kInvalidObjectId) noexcept;
void setCreativeEditorVoxelPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    iggy3d::creative::CreativeObjectKind objectKind,
    iggy3d::creative::CreativeGridCoord3 voxelCell,
    iggy3d::creative::CreativeBounds voxelBounds) noexcept;

struct CreativeEditorWorldInteractionFrameRequest {
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeWorldActionFrame& actions;
  iggy3d::creative::CreativeInputModifierMask modifiers =
      iggy3d::creative::kCreativeInputModifierNone;
  const iggy3d::RenderCameraFrame& camera;
  const CreativeEditorPickFrame& pickFrame;
  // The 3D viewport sub-rectangle (drawable px); crosshair picking is centered
  // on it so aiming matches the visible region when panels frame the viewport.
  iggy3d::RenderContentViewport contentRegion;
  std::uint64_t monotonicTimeNanoseconds = 0;
  bool captureMode = false;
  const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr;
};

[[nodiscard]] CreativeEditorWorldTarget resolveCreativeEditorWorldTarget(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    const CreativeEditorPickFrame& pickFrame,
    const iggy3d::RenderContentViewport& region,
    const iggy3d::creative::CreativePlacementGridFrame& placementGrid);
[[nodiscard]] CreativeEditorWorldTarget resolveCreativeEditorWorldTarget(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    const CreativeEditorPickFrame& pickFrame,
    const iggy3d::RenderContentViewport& region,
    double cellSize);
[[nodiscard]] iggy3d::creative::CreativePlacementGridFrame
creativeEditorPlacementGridFrame(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    double activePlaneY = 0.0,
    bool useActivePlaneOverride = false) noexcept;
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
    std::uint64_t monotonicTimeNanoseconds,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr);

void finalizeCreativeMaterialStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);
void finalizeCreativeEditorContinuousGestures(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);
[[nodiscard]] CreativeEditorContinuousGestureOwner
creativeEditorContinuousGestureOwner(const CreativeEditorState& editor) noexcept;
void finalizeCreativeEditorContinuousGesturesExcept(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorContinuousGestureOwner owner,
    std::string_view reasonCode);

void appendCreativeEditorInteractionOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float wireThickness,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

// The center aim reticle, split out of the interaction overlay so it can be
// drawn even when the legacy HUD is suppressed on keyboard/mouse (plan DD-15).
// Centered on the content region so it tracks the 3D viewport sub-rectangle.
void appendCreativeEditorCrosshairOverlay(
    const CreativeEditorState& editor,
    const iggy3d::RenderContentViewport& region,
    std::vector<iggy3d::RenderUiRect>& uiRects);

}  // namespace iggy3d_creative_app
