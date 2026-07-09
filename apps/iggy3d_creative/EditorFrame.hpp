#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

#include "CreativeEditorState.hpp"
#include "EditorCapture.hpp"
#include "StandalonePicking.hpp"

namespace iggy3d {

class VulkanBackend;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorFrameInputResult {
  bool keepRunning = true;
  bool skipFrame = false;
  iggy3d::SdlDrawableExtent extent{};
  const bool* keyboardState = nullptr;
};

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorState& editor);

void applyCreativeEditorCommandInput(
    const bool* keys,
    bool captureMode,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId);

[[nodiscard]] iggy3d::creative::CreativeToolWorldPoint
resolveCreativeEditorGroundPoint(const iggy3d::RenderCameraFrame& camera);

[[nodiscard]] iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize);

struct CreativeEditorPickFrame {
  std::vector<ObjectVisualPickBounds> objectPickCandidates;
  bool haveFloorBounds = false;
  iggy3d::Vec3 floorBoxMin{};
  iggy3d::Vec3 floorBoxMax{};
};

void applyCreativeEditorClickSelection(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    CreativeEditorState& editor,
    bool captureMode);

struct CreativeEditorSelectionFrame {
  iggy3d::creative::Id selectedId = 0;
  const iggy3d::creative::CreativeObject* selected = nullptr;
  bool hasSelection = false;
  iggy3d::Vec3 boxMin{-0.5F, 0.0F, -0.5F};
  iggy3d::Vec3 boxMax{0.5F, 1.0F, 0.5F};
};

[[nodiscard]] CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade);

void applyCreativeEditorPlacementInput(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    iggy3d::Vec3 aimCellCenter,
    bool captureMode);

[[nodiscard]] CreativeEditorPickFrame buildCreativeEditorPickFrame(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    iggy3d::creative::CreativeObjectId floorObjectId,
    StandaloneCaptureScript& captureScript,
    bool captureMode);

void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode);

}  // namespace iggy3d_creative_app
