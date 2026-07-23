#include "EditorAssets.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include "EditorEdits.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolDescriptor.hpp"
#include "EditorWorldLayout.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] cr::CreativeObjectKind classifyAssetText(
    std::string_view value) {
  std::string lowered(value);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](char byte) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(byte)));
  });
  constexpr std::array classifications{
      std::pair{std::string_view{"boulder"}, cr::CreativeObjectKind::Rock},
      std::pair{std::string_view{"rock"}, cr::CreativeObjectKind::Rock},
      std::pair{std::string_view{"walkway"}, cr::CreativeObjectKind::Bridge},
      std::pair{std::string_view{"bridge"}, cr::CreativeObjectKind::Bridge},
      std::pair{std::string_view{"door"}, cr::CreativeObjectKind::Door},
      std::pair{std::string_view{"window"}, cr::CreativeObjectKind::Window},
      std::pair{std::string_view{"prop"}, cr::CreativeObjectKind::Prop},
  };
  const auto found = std::find_if(
      classifications.begin(), classifications.end(),
      [&lowered](const auto& classification) {
        return lowered.find(classification.first) != std::string::npos;
      });
  return found == classifications.end() ? cr::CreativeObjectKind::Unknown
                                         : found->second;
}

[[nodiscard]] cr::CreativeObjectKind assetObjectKind(
    std::string_view assetId,
    std::string_view categoryId) {
  const cr::CreativeObjectKind categoryKind = classifyAssetText(categoryId);
  if (categoryKind != cr::CreativeObjectKind::Unknown) {
    return categoryKind;
  }
  const cr::CreativeObjectKind filenameKind = classifyAssetText(assetId);
  return filenameKind == cr::CreativeObjectKind::Unknown
             ? cr::CreativeObjectKind::Prop
             : filenameKind;
}

[[nodiscard]] std::string failureLabel(
    const std::filesystem::path& path) {
  const std::string stem = path.stem().string();
  return stem.empty() ? path.generic_string() : stem;
}

[[nodiscard]] bool fatalCatalogFailure(std::string_view reasonCode) noexcept {
  return reasonCode == "static_mesh_asset_root_not_directory" ||
         reasonCode == "static_mesh_asset_catalog_scan_failed";
}

[[nodiscard]] const cr::CreativeCatalogAsset* findCatalogAsset(
    std::span<const cr::CreativeCatalogAsset> assets,
    std::string_view assetId) noexcept {
  const auto found = std::find_if(
      assets.begin(), assets.end(), [assetId](const auto& asset) {
        return asset.assetId == assetId;
      });
  return found == assets.end() ? nullptr : &*found;
}

void restoreCatalogState(const cr::CreativeCatalogState& previous,
                         std::string_view selectedAssetId,
                         cr::CreativeCatalogState& replacement) {
  replacement.open = previous.open;
  replacement.selectedActionIndex = previous.selectedActionIndex;
  replacement.pendingActionConfirmation = previous.pendingActionConfirmation;
  static_cast<void>(cr::setCreativeCatalogPage(replacement, previous.page));
  static_cast<void>(cr::setCreativeCatalogQuery(replacement, previous.query));
  if (selectedAssetId.empty()) {
    return;
  }
  for (std::size_t filteredIndex = 0;
       filteredIndex < replacement.filteredEntryIndices.size(); ++filteredIndex) {
    const cr::CreativeCatalogEntry* entry =
        cr::creativeCatalogEntryAtFilteredIndex(replacement, filteredIndex);
    if (entry != nullptr &&
        entry->category == cr::CreativeCatalogEntryCategory::Asset &&
        cr::creativeHotbarAssetId(entry->hotbarEntry) == selectedAssetId) {
      static_cast<void>(cr::selectCreativeCatalogFilteredIndex(
          replacement, filteredIndex));
      return;
    }
  }
}

[[nodiscard]] std::size_t applyAssetBoundsRefresh(
    cr::CreativeAppState& appState,
    const CreativeAssetBoundsRefreshPlan& plan) {
  if (plan.mutations.empty()) {
    return 0U;
  }
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, "creative_asset_reload_bounds");
  cr::CreativeDocumentMutationOptions options;
  options.applyOptions.rejectLockedObjects = false;
  const cr::CreativeDocumentBatchMutationReceipt mutation =
      cr::applyDocumentMutationsAtomically(
          appState.facade.documentForPersistence(), plan.mutations, options);
  const bool changed = mutation.committed && mutation.changed &&
                       cr::documentMutationSucceeded(mutation.status);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade, changed,
      changed ? "creative_asset_reload_bounds_applied"
              : "creative_asset_reload_bounds_rejected"));
  return changed ? static_cast<std::size_t>(mutation.appliedCount) : 0U;
}

}  // namespace

cr::CreativeBounds creativeAssetBoundsAtPivot(
    const iggy3d::StaticMeshAssetCatalogEntry& entry,
    cr::CreativeVec3 pivot) noexcept {
  return {{pivot.x + static_cast<double>(entry.boundsMin.x),
           pivot.y + static_cast<double>(entry.boundsMin.y),
           pivot.z + static_cast<double>(entry.boundsMin.z)},
          {pivot.x + static_cast<double>(entry.boundsMax.x),
           pivot.y + static_cast<double>(entry.boundsMax.y),
           pivot.z + static_cast<double>(entry.boundsMax.z)}};
}

CreativeCatalogAssetDiscovery discoverCreativeCatalogAssets(
    const std::filesystem::path& root) {
  CreativeCatalogAssetDiscovery output;
  output.catalog = iggy3d::discoverStaticMeshAssetCatalog(root);
  output.assets.reserve(output.catalog.entries.size());
  output.failures.reserve(output.catalog.failures.size());
  for (const iggy3d::StaticMeshAssetCatalogEntry& source :
       output.catalog.entries) {
    cr::CreativeCatalogAsset asset;
    asset.objectKind =
        assetObjectKind(source.assetId, source.authoringMetadata.categoryId);
    asset.assetId = source.assetId;
    asset.label = source.label;
    asset.contentHash = source.contentHash;
    asset.sourceBounds =
        {{source.boundsMin.x, source.boundsMin.y, source.boundsMin.z},
         {source.boundsMax.x, source.boundsMax.y, source.boundsMax.z}};
    asset.authoringMetadata = source.authoringMetadata;
    asset.collisionParts = source.collisionParts;
    asset.attachmentSockets = source.attachmentSockets;
    asset.materialVariants = source.materialVariants;
    asset.materialCount = source.materialCount;
    asset.thumbnail = source.thumbnail;
    output.assets.push_back(std::move(asset));
  }
  for (const iggy3d::StaticMeshAssetCatalogFailure& source :
       output.catalog.failures) {
    output.failures.push_back(
        {failureLabel(source.sourcePath), source.sourcePath.generic_string(),
         source.reasonCode});
    if (fatalCatalogFailure(source.reasonCode)) {
      output.fatal = true;
      output.fatalReasonCode = source.reasonCode;
    }
  }
  return output;
}

void appendDeletedCreativeAssetFailures(
    CreativeCatalogAssetDiscovery& discovery,
    const iggy3d::StaticMeshAssetCatalog& previousCatalog,
    const std::filesystem::path& root) {
  for (const iggy3d::StaticMeshAssetCatalogEntry& previous :
       previousCatalog.entries) {
    if (discovery.catalog.find(previous.assetId) != nullptr) {
      continue;
    }
    const std::filesystem::path expected = root / (previous.assetId + ".glb");
    const bool alreadyReported = std::any_of(
        discovery.catalog.failures.begin(), discovery.catalog.failures.end(),
        [&expected](const iggy3d::StaticMeshAssetCatalogFailure& failure) {
          return failure.sourcePath == expected;
        });
    if (!alreadyReported) {
      discovery.failures.push_back(
          {failureLabel(expected), expected.generic_string(),
           "static_mesh_asset_deleted"});
    }
  }
}

CreativeAssetBoundsRefreshPlan planCreativeAssetBoundsRefresh(
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& previousCatalog,
    const iggy3d::StaticMeshAssetCatalog& nextCatalog) {
  CreativeAssetBoundsRefreshPlan plan;
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.assetId.empty()) {
      continue;
    }
    ++plan.inspectedObjectCount;
    const iggy3d::StaticMeshAssetCatalogEntry* previous =
        previousCatalog.find(object.assetId);
    const iggy3d::StaticMeshAssetCatalogEntry* next =
        nextCatalog.find(object.assetId);
    if (previous == nullptr || next == nullptr) {
      ++plan.missingAssetCount;
      continue;
    }
    const cr::CreativeBounds expectedPrevious =
        creativeAssetBoundsAtPivot(*previous, object.transform.position);
    const bool naturalBounds =
        cr::creativeBoundsExactlyEqual(object.bounds, expectedPrevious);
    if (!naturalBounds) {
      ++plan.customBoundsSkippedCount;
    }
    const cr::CreativeBounds expectedNext =
        creativeAssetBoundsAtPivot(*next, object.transform.position);
    const cr::CreativeBounds replacementBounds =
        naturalBounds ? expectedNext : object.bounds;
    const bool variantRetained =
        object.assetMaterialVariant.empty() ||
        iggy3d::findStaticMeshMaterialVariantIndex(
            next->materialVariants, object.assetMaterialVariant)
            .has_value();
    const std::string_view replacementVariant =
        variantRetained ? std::string_view(object.assetMaterialVariant)
                        : std::string_view{};
    const bool boundsChanged = !cr::creativeBoundsExactlyEqual(
        object.bounds, replacementBounds);
    const bool identityChanged = object.assetContentHash != next->contentHash;
    const bool variantChanged =
        object.assetMaterialVariant != replacementVariant;
    if (!boundsChanged && !identityChanged && !variantChanged) {
      continue;
    }
    cr::CreativeMutationRequest mutation;
    mutation.objectId = object.id;
    mutation.kind = cr::CreativeMutationKind::SetAsset;
    mutation.payload = cr::makeAssetPayload(
        object.kind, object.assetId, replacementBounds, next->contentHash,
        std::string(replacementVariant));
    plan.mutations.push_back(std::move(mutation));
    plan.boundsUpdateCount += boundsChanged ? 1U : 0U;
    plan.identityUpdateCount += identityChanged ? 1U : 0U;
    plan.materialVariantResetCount += variantChanged ? 1U : 0U;
  }
  return plan;
}

CreativeWorldLayoutAssetBoundsRefreshPlan
planCreativeWorldLayoutAssetBoundsRefresh(
    const cr::CreativeWorldLayout& layout,
    const iggy3d::StaticMeshAssetCatalog& previousCatalog,
    const iggy3d::StaticMeshAssetCatalog& nextCatalog) {
  CreativeWorldLayoutAssetBoundsRefreshPlan plan;
  const auto consider = [&](CreativeEditorWorldLayoutAssetBoundsTarget target,
                            std::size_t index, std::string_view assetId,
                            bool hasSourceBounds,
                            cr::CreativeBounds sourceBounds) {
    if (assetId.empty()) {
      return;
    }
    ++plan.inspectedSourceCount;
    const iggy3d::StaticMeshAssetCatalogEntry* previous =
        previousCatalog.find(assetId);
    const iggy3d::StaticMeshAssetCatalogEntry* next =
        nextCatalog.find(assetId);
    if (previous == nullptr || next == nullptr) {
      ++plan.missingAssetCount;
      return;
    }
    const cr::CreativeBounds previousBounds =
        creativeAssetBoundsAtPivot(*previous, {});
    if (!hasSourceBounds ||
        !cr::creativeBoundsExactlyEqual(sourceBounds, previousBounds)) {
      ++plan.customBoundsSkippedCount;
      return;
    }
    const cr::CreativeBounds nextBounds =
        creativeAssetBoundsAtPivot(*next, {});
    if (cr::creativeBoundsExactlyEqual(sourceBounds, nextBounds)) {
      return;
    }
    plan.updates.push_back(
        {target, index, std::string(assetId), nextBounds});
  };

  for (std::size_t index = 0U; index < layout.objects.size(); ++index) {
    const cr::CreativeWorldLayoutObject& object = layout.objects[index];
    consider(CreativeEditorWorldLayoutAssetBoundsTarget::Object, index,
             object.assetId, object.hasAssetSourceBounds,
             object.assetSourceBoundsMeters);
  }
  for (std::size_t index = 0U; index < layout.openings.size(); ++index) {
    const cr::CreativeWorldLayoutOpening& opening = layout.openings[index];
    consider(CreativeEditorWorldLayoutAssetBoundsTarget::OpeningInsert, index,
             opening.insertAssetId,
             opening.hasInsertAssetSourceBounds,
             opening.insertAssetSourceBoundsMeters);
  }
  return plan;
}

std::size_t refreshCreativeHotbarAssetFacts(
    cr::CreativeHotbarState& hotbar,
    std::span<const cr::CreativeCatalogAsset> assets) {
  std::size_t changedCount = 0;
  for (cr::CreativeHotbarEntry& entry : hotbar.entries) {
    const std::string_view assetId = cr::creativeHotbarAssetId(entry);
    if (assetId.empty()) {
      continue;
    }
    const cr::CreativeCatalogAsset* asset = findCatalogAsset(assets, assetId);
    if (asset == nullptr) {
      continue;
    }
    const std::string_view currentVariant =
        cr::creativeHotbarAssetMaterialVariant(entry);
    const bool variantRetained =
        currentVariant.empty() ||
        iggy3d::findStaticMeshMaterialVariantIndex(asset->materialVariants,
                                                   currentVariant)
            .has_value();
    const std::string_view refreshedVariant =
        variantRetained ? currentVariant : std::string_view{};
    const bool changed = entry.objectKind != asset->objectKind ||
                         !entry.hasAssetBounds ||
                         entry.assetContentHash != asset->contentHash ||
                         currentVariant != refreshedVariant ||
                         !cr::creativeBoundsExactlyEqual(
                             entry.assetSourceBounds, asset->sourceBounds);
    const std::string stableAssetId(assetId);
    const std::string stableVariant(refreshedVariant);
    entry.objectKind = asset->objectKind;
    static_cast<void>(cr::setCreativeHotbarAsset(
        entry, stableAssetId, asset->sourceBounds, asset->contentHash,
        stableVariant));
    changedCount += changed ? 1U : 0U;
  }
  return changedCount;
}

cr::CreativeCatalogState rebuildCreativeAssetCatalog(
    const cr::CreativeCatalogState& previous,
    std::span<const cr::CreativeObjectKind> materialPalette,
    const CreativeCatalogAssetDiscovery& discovery,
    std::string_view selectedAssetId) {
  cr::CreativeCatalogState replacement = cr::makeCreativeCatalog(
      materialPalette, discovery.assets, discovery.failures.size(),
      discovery.failures, creativeEditorCatalogToolSpecs());
  restoreCatalogState(previous, selectedAssetId, replacement);
  return replacement;
}

CreativeEditorAssetReloadReceipt reloadCreativeEditorAssets(
    const CreativeEditorAssetReloadRequest& request) {
  CreativeEditorAssetReloadReceipt receipt;
  receipt.requested = true;
  CreativeCatalogAssetDiscovery discovery =
      discoverCreativeCatalogAssets(request.assetRoot);
  appendDeletedCreativeAssetFailures(discovery, request.liveCatalog,
                                     request.assetRoot);
  std::vector<cr::CreativeCatalogAsset> authoredAssets =
      creativeEditorAuthoredAssetCatalogEntries(
          request.editor.authoredAssets);
  discovery.assets.insert(
      discovery.assets.end(),
      std::make_move_iterator(authoredAssets.begin()),
      std::make_move_iterator(authoredAssets.end()));
  receipt.readyAssetCount = discovery.assets.size();
  receipt.failedAssetCount = discovery.failures.size();
  if (discovery.fatal) {
    receipt.reasonCode = discovery.fatalReasonCode;
    request.editor.catalog.statusLabel =
        "ASSET RELOAD FAILED: " + receipt.reasonCode;
    return receipt;
  }

  const CreativeAssetBoundsRefreshPlan boundsPlan =
      planCreativeAssetBoundsRefresh(request.appState.facade.document(),
                                     request.liveCatalog, discovery.catalog);
  const CreativeWorldLayoutAssetBoundsRefreshPlan worldLayoutBoundsPlan =
      planCreativeWorldLayoutAssetBoundsRefresh(
          request.editor.worldLayout.source, request.liveCatalog,
          discovery.catalog);
  const std::string selectedAssetId(
      cr::creativeHotbarAssetId(cr::selectedCreativeHotbarEntry(
          request.editor.interaction.hotbar)));
  cr::CreativeCatalogState replacement = rebuildCreativeAssetCatalog(
      request.editor.catalog.model, request.editor.brushPalette, discovery,
      selectedAssetId);

  const iggy3d::VulkanStaticMeshAssetReloadResult renderer =
      request.backend.reloadStaticMeshAssets();
  if (!renderer.ok()) {
    receipt.reasonCode = std::string(renderer.reason.code);
    request.editor.catalog.statusLabel =
        "ASSET RELOAD FAILED: " + receipt.reasonCode;
    return receipt;
  }
  receipt.rendererReloaded = true;

  const CreativeEditorWorldLayoutEditReceipt worldLayoutRefresh =
      applyCreativeEditorWorldLayoutAssetBoundsUpdates(
          request.editor.worldLayout, worldLayoutBoundsPlan.updates);
  if (!worldLayoutRefresh.accepted) {
    receipt.reasonCode = worldLayoutRefresh.reasonCode;
    request.editor.catalog.statusLabel =
        "ASSET RELOAD FAILED: " + receipt.reasonCode;
    invalidateCreativeEditorSceneCache(request.sceneCache);
    receipt.sceneInvalidated = true;
    return receipt;
  }
  receipt.refreshedWorldLayoutAssetBoundsCount =
      worldLayoutRefresh.changed ? worldLayoutBoundsPlan.updates.size() : 0U;
  receipt.customWorldLayoutBoundsSkippedCount =
      worldLayoutBoundsPlan.customBoundsSkippedCount;
  receipt.missingWorldLayoutAssetCount =
      worldLayoutBoundsPlan.missingAssetCount;

  const std::size_t refreshedObjectCount =
      applyAssetBoundsRefresh(request.appState, boundsPlan);
  receipt.refreshedObjectBoundsCount =
      refreshedObjectCount == boundsPlan.mutations.size()
          ? boundsPlan.boundsUpdateCount
          : 0U;
  receipt.refreshedObjectIdentityCount =
      refreshedObjectCount == boundsPlan.mutations.size()
          ? boundsPlan.identityUpdateCount
          : 0U;
  receipt.resetObjectMaterialVariantCount =
      refreshedObjectCount == boundsPlan.mutations.size()
          ? boundsPlan.materialVariantResetCount
          : 0U;
  receipt.customBoundsSkippedCount = boundsPlan.customBoundsSkippedCount;
  receipt.refreshedHotbarSlotCount = refreshCreativeHotbarAssetFacts(
      request.editor.interaction.hotbar, discovery.assets);
  request.editor.catalog.model = std::move(replacement);
  request.editor.catalog.scrollOffset = 0U;
  request.liveCatalog = std::move(discovery.catalog);
  invalidateCreativeEditorSceneCache(request.sceneCache);
  receipt.sceneInvalidated = true;
  receipt.accepted = true;
  receipt.reasonCode = "creative_asset_reload_applied";

  char status[192];
  std::snprintf(status, sizeof(status),
                "ASSETS RELOADED: %zu READY | %zu ERRORS | %zu OBJECTS | %zu LAYOUT",
                receipt.readyAssetCount, receipt.failedAssetCount,
                receipt.refreshedObjectBoundsCount,
                receipt.refreshedWorldLayoutAssetBoundsCount);
  request.editor.catalog.statusLabel = status;
  SDL_Log("iggy3d_creative: asset reload ready=%llu failed=%llu hotbar=%llu "
          "bounds=%llu customBoundsSkipped=%llu layoutBounds=%llu "
          "layoutCustomSkipped=%llu layoutMissing=%llu",
          static_cast<unsigned long long>(receipt.readyAssetCount),
          static_cast<unsigned long long>(receipt.failedAssetCount),
          static_cast<unsigned long long>(receipt.refreshedHotbarSlotCount),
          static_cast<unsigned long long>(receipt.refreshedObjectBoundsCount),
          static_cast<unsigned long long>(receipt.customBoundsSkippedCount),
          static_cast<unsigned long long>(
              receipt.refreshedWorldLayoutAssetBoundsCount),
          static_cast<unsigned long long>(
              receipt.customWorldLayoutBoundsSkippedCount),
          static_cast<unsigned long long>(
              receipt.missingWorldLayoutAssetCount));
  return receipt;
}

}  // namespace iggy3d_creative_app
