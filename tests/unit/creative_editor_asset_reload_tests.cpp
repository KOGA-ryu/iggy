#include "EditorAssets.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

#include "app/iggy3d/creative/document/Document.hpp"

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::StaticMeshAssetCatalogEntry catalogEntry(
    std::string assetId,
    iggy3d::Vec3 boundsMin,
    iggy3d::Vec3 boundsMax) {
  iggy3d::StaticMeshAssetCatalogEntry entry;
  entry.assetId = std::move(assetId);
  entry.label = entry.assetId;
  entry.boundsMin = boundsMin;
  entry.boundsMax = boundsMax;
  return entry;
}

cr::CreativeDocumentCreateReceipt createAssetObject(
    cr::CreativeDocument& document,
    std::string assetId,
    cr::CreativeVec3 pivot,
    cr::CreativeBounds bounds) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Prop;
  request.assetId = std::move(assetId);
  request.transform.position = pivot;
  request.hasTransformOverride = true;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

bool discoveryReportsValidBrokenAndFatalRoots() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_creative_asset_reload_discovery";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  if (error) {
    return expect(false, "temporary asset directory created");
  }
  std::filesystem::copy_file("assets/creative/boulder_01.glb",
                             root / "boulder_01.glb",
                             std::filesystem::copy_options::overwrite_existing,
                             error);
  if (error) {
    std::filesystem::remove_all(root, error);
    return expect(false, "valid GLB fixture copied");
  }
  {
    std::ofstream broken(root / "broken_prop.glb", std::ios::binary);
    broken << "not a glb";
  }

  const app::CreativeCatalogAssetDiscovery discovered =
      app::discoverCreativeCatalogAssets(root);
  const app::CreativeCatalogAssetDiscovery missing =
      app::discoverCreativeCatalogAssets(root / "missing");
  std::filesystem::remove_all(root, error);

  return expect(!discovered.fatal && discovered.assets.size() == 1U &&
                    discovered.failures.size() == 1U &&
                    discovered.assets[0].assetId == "boulder_01" &&
                    discovered.assets[0].objectKind ==
                        cr::CreativeObjectKind::Rock &&
                    discovered.failures[0].reasonCode ==
                        "static_mesh_glb_parse_failed",
                "discovery keeps valid assets and explicit per-file errors") &&
         expect(missing.fatal &&
                    missing.fatalReasonCode ==
                        "static_mesh_asset_root_not_directory",
                "missing root is a fatal reload error");
}

bool deletedAssetsRemainExplicitFailures() {
  iggy3d::StaticMeshAssetCatalog previous;
  previous.entries.push_back(
      catalogEntry("deleted/column", {-1.0F, 0.0F, -1.0F},
                   {1.0F, 2.0F, 1.0F}));
  app::CreativeCatalogAssetDiscovery discovery;
  app::appendDeletedCreativeAssetFailures(
      discovery, previous, std::filesystem::path{"assets/creative"});
  return expect(discovery.failures.size() == 1U &&
                    discovery.failures[0].label == "column" &&
                    discovery.failures[0].sourcePath ==
                        "assets/creative/deleted/column.glb" &&
                    discovery.failures[0].reasonCode ==
                        "static_mesh_asset_deleted",
                "deleted assets stay visible as catalog errors");
}

bool boundsRefreshUpdatesOnlyNaturalAssetBounds() {
  iggy3d::StaticMeshAssetCatalog previous;
  previous.entries.push_back(catalogEntry(
      "prop", {-1.0F, -2.0F, -3.0F}, {1.0F, 2.0F, 3.0F}));
  iggy3d::StaticMeshAssetCatalog next;
  next.entries.push_back(catalogEntry(
      "prop", {-2.0F, -1.0F, -4.0F}, {2.0F, 3.0F, 4.0F}));

  cr::CreativeDocument document = cr::CreativeDocument::create("reload");
  static_cast<void>(document.assignId(7U));
  const cr::CreativeVec3 pivot{10.0, 5.0, -2.0};
  const cr::CreativeBounds natural{{9.0, 3.0, -5.0},
                                   {11.0, 7.0, 1.0}};
  const cr::CreativeBounds custom{{9.0, 3.0, -5.0},
                                  {12.0, 7.0, 1.0}};
  const cr::CreativeDocumentCreateReceipt naturalCreated =
      createAssetObject(document, "prop", pivot, natural);
  const cr::CreativeDocumentCreateReceipt customCreated =
      createAssetObject(document, "prop", pivot, custom);
  const cr::CreativeDocumentCreateReceipt missingCreated =
      createAssetObject(document, "missing", pivot, natural);
  const app::CreativeAssetBoundsRefreshPlan plan =
      app::planCreativeAssetBoundsRefresh(document, previous, next);
  if (plan.mutations.empty()) {
    return expect(false, "natural asset emits one bounds mutation");
  }
  const auto* payload = std::get_if<cr::SetBoundsMutation>(
      &plan.mutations[0].payload.value);
  const cr::CreativeBounds expected{{8.0, 4.0, -6.0},
                                    {12.0, 8.0, 2.0}};
  return expect(naturalCreated.accepted && customCreated.accepted &&
                    missingCreated.accepted &&
                    plan.inspectedObjectCount == 3U &&
                    plan.mutations.size() == 1U &&
                    plan.mutations[0].objectId == naturalCreated.objectId &&
                    payload != nullptr &&
                    cr::creativeBoundsExactlyEqual(payload->bounds, expected),
                "source-origin bounds refresh emits the exact new envelope") &&
         expect(plan.customBoundsSkippedCount == 1U &&
                    plan.missingAssetCount == 1U,
                "custom and missing asset bounds remain untouched");
}

bool catalogAndHotbarPreserveStableAssetIdentity() {
  constexpr std::array palette{cr::CreativeObjectKind::Crate};
  cr::CreativeCatalogAsset previousAsset;
  previousAsset.objectKind = cr::CreativeObjectKind::Rock;
  previousAsset.assetId = "boulder_01";
  previousAsset.label = "Boulder 01";
  previousAsset.sourceBounds = {{-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}};
  cr::CreativeCatalogState previous =
      cr::makeCreativeCatalog(palette, std::span{&previousAsset, 1U});
  static_cast<void>(
      cr::setCreativeCatalogPage(previous, cr::CreativeCatalogPage::Assets));
  static_cast<void>(cr::setCreativeCatalogQuery(previous, "boulder"));
  previous.open = true;

  app::CreativeCatalogAssetDiscovery discovery;
  cr::CreativeCatalogAsset refreshed = previousAsset;
  refreshed.objectKind = cr::CreativeObjectKind::Bridge;
  refreshed.sourceBounds = {{-2.0, -0.5, -1.0}, {2.0, 0.5, 1.0}};
  discovery.assets.push_back(refreshed);
  cr::CreativeCatalogState replacement = app::rebuildCreativeAssetCatalog(
      previous, palette, discovery, "boulder_01");
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeCatalogEntry(replacement);

  cr::CreativeHotbarState hotbar = cr::makeDefaultCreativeHotbar(palette);
  static_cast<void>(cr::setCreativeHotbarAsset(
      hotbar.entries[0], previousAsset.assetId, previousAsset.sourceBounds));
  hotbar.entries[0].objectKind = previousAsset.objectKind;
  static_cast<void>(cr::setCreativeHotbarAsset(
      hotbar.entries[1], "deleted_asset", previousAsset.sourceBounds));
  const cr::CreativeHotbarEntry deletedBefore = hotbar.entries[1];
  const std::size_t changed =
      app::refreshCreativeHotbarAssetFacts(hotbar, discovery.assets);

  return expect(replacement.open &&
                    replacement.page == cr::CreativeCatalogPage::Assets &&
                    replacement.query == "boulder" && selected != nullptr &&
                    cr::creativeHotbarAssetId(selected->hotbarEntry) ==
                        "boulder_01",
                "catalog page, query, open state, and asset selection survive") &&
         expect(changed == 1U &&
                    hotbar.entries[0].objectKind ==
                        cr::CreativeObjectKind::Bridge &&
                    cr::creativeBoundsExactlyEqual(
                        hotbar.entries[0].assetSourceBounds,
                        refreshed.sourceBounds) &&
                    hotbar.entries[1].assetId == deletedBefore.assetId &&
                    cr::creativeBoundsExactlyEqual(
                        hotbar.entries[1].assetSourceBounds,
                        deletedBefore.assetSourceBounds),
                "surviving hotbar assets refresh while deleted IDs persist");
}

}  // namespace

int main() {
  bool ok = true;
  ok = discoveryReportsValidBrokenAndFatalRoots() && ok;
  ok = deletedAssetsRemainExplicitFailures() && ok;
  ok = boundsRefreshUpdatesOnlyNaturalAssetBounds() && ok;
  ok = catalogAndHotbarPreserveStableAssetIdentity() && ok;
  return ok ? 0 : 1;
}
