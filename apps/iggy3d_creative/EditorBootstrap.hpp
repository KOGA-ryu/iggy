#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "app/platform/SdlWindow.hpp"
#include "content/assets/StaticMeshAsset.hpp"
#include "render/RendererConfig.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorState.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorBootstrapData {
  CreativeEditorState editor;
  iggy3d::ProductMapMakerGridSnapshot gridSnapshot;
  iggy3d::creative::CreativeAppState appState;
  // Replaced only by the explicit Assets-catalog reload transaction. That
  // transaction invalidates the document-revision scene cache after the swap.
  iggy3d::StaticMeshAssetCatalog staticMeshAssetCatalog;
  std::filesystem::path assetRoot;
  iggy3d::creative::CreativeObjectId floorObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::filesystem::path saveRoot;
  std::string saveId = "scene";
  iggy3d::creative::CreativeSpatialProjectionRequest wireProjectionRequest;
  float gizmoAxisLengthMeters = 1.5F;
  float gizmoThicknessMeters = 0.05F;
};

void initializeCreativeEditorBootstrapData(
    CreativeEditorBootstrapData& output,
    bool captureMode);

[[nodiscard]] iggy3d::RendererConfig makeCreativeVulkanRendererConfig();
[[nodiscard]] std::unique_ptr<iggy3d::VulkanBackend> createCreativeRenderer(
    iggy3d::SdlWindow& window);
[[nodiscard]] bool captureFrameToPng(iggy3d::VulkanBackend& backend,
                                     const std::string& pngPath);

}  // namespace iggy3d_creative_app
