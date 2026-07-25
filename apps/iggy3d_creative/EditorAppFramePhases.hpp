#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/platform/SdlWindow.hpp"
#include "content/assets/StaticMeshAsset.hpp"
#include "render/FrameInput.hpp"

#include "EditorBootstrap.hpp"
#include "EditorFrame.hpp"
#include "EditorGamepad.hpp"
#include "EditorGizmo.hpp"
#include "EditorPlaytestProcess.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorPlayerSpawnPreview.hpp"

namespace iggy3d {

class VulkanBackend;

}  // namespace iggy3d

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeObjectId
firstCreativeEditorFloorObjectId(
    const iggy3d::creative::CreativeDocument& document) noexcept;

enum class CreativeEditorAppFrameDisposition : std::uint8_t {
  Continue,
  Skip,
  Stop,
};

struct CreativeEditorAppInputPhaseRequest {
  iggy3d::SdlWindow& window;
  iggy3d::VulkanBackend& backend;
  CreativeEditorGamepad& gamepad;
  CreativeEditorState& editor;
  iggy3d::creative::CreativeAppState& appState;
  PlaytestProcessOwner& playtestOwner;
  bool captureMode = false;
};

struct CreativeEditorAppInputPhaseReceipt {
  CreativeEditorAppFrameDisposition disposition =
      CreativeEditorAppFrameDisposition::Continue;
  CreativeEditorFrameInputResult frameInput;
};

[[nodiscard]] CreativeEditorAppInputPhaseReceipt
runCreativeEditorAppInputPhase(
    const CreativeEditorAppInputPhaseRequest& request);

struct CreativeEditorAppToolPhaseRequest {
  iggy3d::SdlWindow& window;
  iggy3d::VulkanBackend& backend;
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  CreativeEditorFrameInputResult& frameInput;
  const std::filesystem::path& saveRoot;
  std::string& saveId;
  const std::filesystem::path& controlsPath;
  const std::filesystem::path& toolWheelPath;
  iggy3d::StaticMeshAssetCatalog& assetCatalog;
  const std::filesystem::path& assetRoot;
  PlaytestProcessOwner& playtestOwner;
  CreativeEditorSceneCache& sceneCache;
  CreativeEditorGeneratedTerrainPreviewCache& terrainPreviewCache;
  iggy3d::creative::CreativeObjectId& floorObjectId;
};

struct CreativeEditorAppToolPhaseReceipt {
  iggy3d::creative::CreativeAppState* activeAppState = nullptr;
  bool modalBlocksWorldActions = false;
};

[[nodiscard]] CreativeEditorAppToolPhaseReceipt
runCreativeEditorAppToolPhase(
    const CreativeEditorAppToolPhaseRequest& request);

struct CreativeEditorAppPreviewPhaseRequest {
  iggy3d::creative::CreativeAppState& activeAppState;
  CreativeEditorState& editor;
  CreativeEditorSceneCache& sceneCache;
  CreativeEditorGeneratedTerrainPreviewCache& terrainPreviewCache;
  CreativeEditorVolumeScenePreviewCache& volumePreviewCache;
  const iggy3d::StaticMeshAssetCatalog& assetCatalog;
  bool modalBlocksWorldActions = false;
  bool captureMode = false;
};

struct CreativeEditorAppPreviewPhaseReceipt {
  const iggy3d::creative::CreativeDocument* renderDocument = nullptr;
  StandaloneRoomBakePreviewScene* selectedPreview = nullptr;
  const iggy3d::creative::CreativeTerrainSurfacePlan* terrainContourSurface =
      nullptr;
  std::uint64_t terrainContourSurfaceKey = 0U;
};

[[nodiscard]] CreativeEditorAppPreviewPhaseReceipt
prepareCreativeEditorAppPreviewPhase(
    const CreativeEditorAppPreviewPhaseRequest& request);

void refreshCreativeEditorAppPreviewAfterInteraction(
    const CreativeEditorAppPreviewPhaseRequest& request,
    CreativeEditorAppPreviewPhaseReceipt& receipt,
    iggy3d::FrameInput& frame);

struct CreativeEditorAppRenderPhaseRequest {
  iggy3d::VulkanBackend& backend;
  iggy3d::creative::CreativeAppState& activeAppState;
  CreativeEditorState& editor;
  CreativeEditorSceneCache& sceneCache;
  CreativeEditorGeneratedTerrainPreviewCache& terrainPreviewCache;
  CreativeEditorVolumeScenePreviewCache& volumePreviewCache;
  CreativePlayerSpawnPreviewCache& playerSpawnPreviewCache;
  const CreativeEditorAppPreviewPhaseReceipt& preview;
  iggy3d::FrameInput& frame;
  const iggy3d::creative::CreativeSpatialProjectionRequest& wireProjection;
  const CreativeEditorFrameInputResult& frameInput;
  const iggy3d::StaticMeshAssetCatalog& assetCatalog;
  std::uint32_t drawableWidth = 0U;
  std::uint32_t drawableHeight = 0U;
  float gizmoAxisLengthMeters = 0.0F;
  float gizmoThicknessMeters = 0.0F;
  bool captureMode = false;
  std::uint64_t maxFrames = 0U;
};

[[nodiscard]] bool runCreativeEditorAppRenderPhase(
    const CreativeEditorAppRenderPhaseRequest& request);

}  // namespace iggy3d_creative_app
