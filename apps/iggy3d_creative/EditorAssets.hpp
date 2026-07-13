#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/input/Catalog.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d {

class VulkanBackend;

}  // namespace iggy3d

namespace iggy3d::creative {

struct CreativeAppState;
class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorSceneCache;
struct CreativeEditorState;

struct CreativeCatalogAssetDiscovery {
  iggy3d::StaticMeshAssetCatalog catalog;
  std::vector<iggy3d::creative::CreativeCatalogAsset> assets;
  std::vector<iggy3d::creative::CreativeCatalogAssetFailure> failures;
  bool fatal = false;
  std::string fatalReasonCode;
};

struct CreativeAssetBoundsRefreshPlan {
  std::vector<iggy3d::creative::CreativeMutationRequest> mutations;
  std::size_t inspectedObjectCount = 0;
  std::size_t missingAssetCount = 0;
  std::size_t customBoundsSkippedCount = 0;
};

struct CreativeEditorAssetReloadRequest {
  iggy3d::VulkanBackend& backend;
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  CreativeEditorSceneCache& sceneCache;
  iggy3d::StaticMeshAssetCatalog& liveCatalog;
  std::filesystem::path assetRoot;
};

struct CreativeEditorAssetReloadReceipt {
  bool requested = false;
  bool accepted = false;
  bool rendererReloaded = false;
  bool sceneInvalidated = false;
  std::size_t readyAssetCount = 0;
  std::size_t failedAssetCount = 0;
  std::size_t refreshedHotbarSlotCount = 0;
  std::size_t refreshedObjectBoundsCount = 0;
  std::size_t customBoundsSkippedCount = 0;
  std::string reasonCode = "creative_asset_reload_not_requested";
};

[[nodiscard]] CreativeCatalogAssetDiscovery discoverCreativeCatalogAssets(
    const std::filesystem::path& root);

[[nodiscard]] iggy3d::creative::CreativeBounds creativeAssetBoundsAtPivot(
    const iggy3d::StaticMeshAssetCatalogEntry& entry,
    iggy3d::creative::CreativeVec3 pivot) noexcept;

void appendDeletedCreativeAssetFailures(
    CreativeCatalogAssetDiscovery& discovery,
    const iggy3d::StaticMeshAssetCatalog& previousCatalog,
    const std::filesystem::path& root);

[[nodiscard]] CreativeAssetBoundsRefreshPlan planCreativeAssetBoundsRefresh(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& previousCatalog,
    const iggy3d::StaticMeshAssetCatalog& nextCatalog);

[[nodiscard]] std::size_t refreshCreativeHotbarAssetFacts(
    iggy3d::creative::CreativeHotbarState& hotbar,
    std::span<const iggy3d::creative::CreativeCatalogAsset> assets);

[[nodiscard]] iggy3d::creative::CreativeCatalogState
rebuildCreativeAssetCatalog(
    const iggy3d::creative::CreativeCatalogState& previous,
    std::span<const iggy3d::creative::CreativeObjectKind> materialPalette,
    const CreativeCatalogAssetDiscovery& discovery,
    std::string_view selectedAssetId);

[[nodiscard]] CreativeEditorAssetReloadReceipt reloadCreativeEditorAssets(
    const CreativeEditorAssetReloadRequest& request);

}  // namespace iggy3d_creative_app
