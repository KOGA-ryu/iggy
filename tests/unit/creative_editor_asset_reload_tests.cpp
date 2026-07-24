#include "EditorAssets.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorEdits.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorToolDescriptor.hpp"
#include "EditorWorldLayout.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

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

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameTransform(const cr::CreativeTransform& lhs,
                   const cr::CreativeTransform& rhs) {
  return sameVec3(lhs.position, rhs.position) &&
         sameVec3(lhs.rotationEulerRadians, rhs.rotationEulerRadians) &&
         sameVec3(lhs.scale, rhs.scale);
}

bool sameMetadata(const cr::CreativeObject& lhs,
                  const cr::CreativeObject& rhs) {
  return lhs.id == rhs.id && lhs.name == rhs.name &&
         sameTransform(lhs.transform, rhs.transform) &&
         lhs.layerId == rhs.layerId && lhs.visible == rhs.visible &&
         lhs.locked == rhs.locked && lhs.tags == rhs.tags &&
         lhs.parentId == rhs.parentId &&
         lhs.attachmentSocket == rhs.attachmentSocket &&
         lhs.pathPoints.size() == rhs.pathPoints.size();
}

iggy3d::StaticMeshAssetCatalog replacementCatalog() {
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "asset_a", {-1.0F, 0.0F, -1.0F}, {1.0F, 2.0F, 1.0F}));
  catalog.entries.push_back(catalogEntry(
      "asset_b", {-0.5F, 0.0F, -2.0F}, {0.5F, 4.0F, 2.0F}));
  catalog.entries[0].contentHash = 11U;
  catalog.entries[0].materialVariants = {{{"Weathered"}}};
  catalog.entries[1].contentHash = 22U;
  catalog.entries[1].materialVariants = {{{"Weathered"}}, {{"Painted"}}};
  return catalog;
}

cr::CreativeDocumentCreateReceipt createRichAssetObject(
    cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalogEntry& source,
    cr::CreativeVec3 pivot,
    std::string name,
    cr::CreativeLayerId layerId,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt,
    std::string attachmentSocket = {}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Prop;
  request.name = std::move(name);
  request.assetId = source.assetId;
  request.assetContentHash = source.contentHash;
  request.assetMaterialVariant =
      source.materialVariants.empty() ? std::string{}
                                      : source.materialVariants.front().name;
  request.transform.position = pivot;
  request.transform.rotationEulerRadians = {0.1, 0.2, 0.3};
  request.transform.scale = {1.25, 0.75, 1.5};
  request.hasTransformOverride = true;
  request.bounds = app::creativeAssetBoundsAtPivot(source, pivot);
  request.hasBoundsOverride = true;
  request.layerId = layerId;
  request.hasLayerOverride = true;
  request.tags = {"authored", "replace-test"};
  request.parentId = parentId;
  request.attachmentSocket = std::move(attachmentSocket);
  return document.createObject(request);
}

void selectObject(cr::Facade& facade,
                  cr::CreativeObjectId objectId,
                  bool additive) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  input.pointer.modifiers = additive ? cr::kCreativeToolModifierShift
                                     : cr::kCreativeToolModifierNone;
  static_cast<void>(facade.dispatchToolInput(input));
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
  previous.entries[0].contentHash = 101U;
  previous.entries[0].materialVariants = {{{"Legacy"}}};
  next.entries[0].contentHash = 202U;
  next.entries[0].materialVariants = {{{"Current"}}};

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
  document.findObject(naturalCreated.objectId)->assetContentHash = 101U;
  document.findObject(naturalCreated.objectId)->assetMaterialVariant = "Legacy";
  document.findObject(customCreated.objectId)->assetContentHash = 101U;
  document.findObject(customCreated.objectId)->assetMaterialVariant = "Legacy";
  const app::CreativeAssetBoundsRefreshPlan plan =
      app::planCreativeAssetBoundsRefresh(document, previous, next);
  if (plan.mutations.size() != 2U) {
    return expect(false, "natural and custom assets emit identity mutations");
  }
  const auto* naturalPayload = std::get_if<cr::SetAssetMutation>(
      &plan.mutations[0].payload.value);
  const auto* customPayload = std::get_if<cr::SetAssetMutation>(
      &plan.mutations[1].payload.value);
  const cr::CreativeBounds expected{{8.0, 4.0, -6.0},
                                    {12.0, 8.0, 2.0}};
  return expect(naturalCreated.accepted && customCreated.accepted &&
                    missingCreated.accepted &&
                    plan.inspectedObjectCount == 3U &&
                    plan.mutations.size() == 2U &&
                    plan.mutations[0].objectId == naturalCreated.objectId &&
                    naturalPayload != nullptr &&
                    naturalPayload->assetContentHash == 202U &&
                    naturalPayload->assetMaterialVariant.empty() &&
                    cr::creativeBoundsExactlyEqual(naturalPayload->bounds,
                                                   expected),
                "natural asset refreshes envelope version and variant") &&
         expect(plan.mutations[1].objectId == customCreated.objectId &&
                    customPayload != nullptr &&
                    customPayload->assetContentHash == 202U &&
                    customPayload->assetMaterialVariant.empty() &&
                    cr::creativeBoundsExactlyEqual(customPayload->bounds,
                                                   custom),
                "custom envelope remains authored while identity advances") &&
         expect(plan.customBoundsSkippedCount == 1U &&
                    plan.missingAssetCount == 1U &&
                    plan.boundsUpdateCount == 1U &&
                    plan.identityUpdateCount == 2U &&
                    plan.materialVariantResetCount == 2U,
                "reload reports bounds identity and variant work separately");
}

bool boundsRefreshPreservesAttachmentRelationshipAndWorldPose() {
  iggy3d::StaticMeshAssetCatalog previous;
  previous.entries.push_back(catalogEntry(
      "prop", {-1.0F, -2.0F, -3.0F}, {1.0F, 2.0F, 3.0F}));
  previous.entries[0].contentHash = 101U;
  iggy3d::StaticMeshAssetCatalog next;
  next.entries.push_back(catalogEntry(
      "prop", {-2.0F, -1.0F, -4.0F}, {2.0F, 3.0F, 4.0F}));
  next.entries[0].contentHash = 202U;

  cr::CreativeDocument document = cr::CreativeDocument::create("Attached Reload");
  static_cast<void>(document.assignId(8U));
  cr::CreativeDocumentCreateRequest hostRequest;
  hostRequest.kind = cr::CreativeObjectKind::Prop;
  hostRequest.name = "Socket Host";
  hostRequest.transform.position = {20.0, 0.0, 0.0};
  hostRequest.hasTransformOverride = true;
  hostRequest.bounds = {{19.0, 0.0, -1.0}, {21.0, 2.0, 1.0}};
  hostRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt host =
      document.createObject(hostRequest);
  const cr::CreativeDocumentCreateReceipt child = createRichAssetObject(
      document, previous.entries[0], {10.0, 5.0, -2.0}, "Attached Prop",
      3U, host.objectId, "fixture_socket");
  if (!expect(host.accepted && child.accepted,
              "attached reload fixtures created")) {
    return false;
  }
  const cr::CreativeObject before = *document.findObject(child.objectId);

  const app::CreativeAssetBoundsRefreshPlan plan =
      app::planCreativeAssetBoundsRefresh(document, previous, next);
  const cr::CreativeDocumentBatchMutationReceipt applied =
      cr::applyDocumentMutationsAtomically(document, plan.mutations);
  const cr::CreativeObject* after = document.findObject(child.objectId);
  const cr::CreativeBounds expected =
      app::creativeAssetBoundsAtPivot(next.entries[0],
                                     before.transform.position);
  return expect(plan.mutations.size() == 1U && applied.committed &&
                    applied.changed && after != nullptr &&
                    after->assetContentHash == 202U &&
                    cr::creativeBoundsExactlyEqual(after->bounds, expected),
                "attached asset reload advances mesh identity and bounds") &&
         expect(sameTransform(after->transform, before.transform) &&
                    after->parentId == host.objectId &&
                    after->attachmentSocket == "fixture_socket",
                "asset reload preserves attachment and exact world pose");
}

bool worldLayoutBoundsRefreshIsAtomicAndPreservesCustomSources() {
  iggy3d::StaticMeshAssetCatalog previous;
  previous.entries.push_back(catalogEntry(
      "deleted", {-0.5F, 0.0F, -0.5F}, {0.5F, 1.0F, 0.5F}));
  previous.entries.push_back(catalogEntry(
      "door", {-0.5F, 0.0F, -0.1F}, {0.5F, 2.0F, 0.1F}));
  previous.entries.push_back(catalogEntry(
      "prop", {-1.0F, 0.0F, -0.5F}, {1.0F, 2.0F, 0.5F}));
  iggy3d::StaticMeshAssetCatalog next;
  next.entries.push_back(catalogEntry(
      "door", {-0.6F, 0.0F, -0.15F}, {0.6F, 2.2F, 0.15F}));
  next.entries.push_back(catalogEntry(
      "prop", {-2.0F, -0.25F, -1.0F}, {2.0F, 2.25F, 1.0F}));

  const iggy3d::StaticMeshAssetCatalogEntry* previousDeleted =
      previous.find("deleted");
  const iggy3d::StaticMeshAssetCatalogEntry* previousDoorEntry =
      previous.find("door");
  const iggy3d::StaticMeshAssetCatalogEntry* previousPropEntry =
      previous.find("prop");
  const iggy3d::StaticMeshAssetCatalogEntry* nextDoorEntry = next.find("door");
  const iggy3d::StaticMeshAssetCatalogEntry* nextPropEntry = next.find("prop");
  if (!expect(previousDeleted != nullptr && previousDoorEntry != nullptr &&
                  previousPropEntry != nullptr && nextDoorEntry != nullptr &&
                  nextPropEntry != nullptr,
              "layout refresh catalogs obey sorted lookup ownership")) {
    return false;
  }

  const cr::CreativeBounds previousProp =
      app::creativeAssetBoundsAtPivot(*previousPropEntry, {});
  const cr::CreativeBounds previousDoor =
      app::creativeAssetBoundsAtPivot(*previousDoorEntry, {});
  const cr::CreativeBounds nextProp =
      app::creativeAssetBoundsAtPivot(*nextPropEntry, {});
  const cr::CreativeBounds nextDoor =
      app::creativeAssetBoundsAtPivot(*nextDoorEntry, {});

  cr::CreativeWorldLayout layout;
  layout.stableKey = "asset_refresh_layout";
  cr::CreativeWorldLayoutObject naturalObject;
  naturalObject.kind = cr::CreativeObjectKind::Prop;
  naturalObject.stableKey = "object_natural";
  naturalObject.name = "Natural Prop";
  naturalObject.assetId = "prop";
  naturalObject.assetSourceBoundsMeters = previousProp;
  naturalObject.hasAssetSourceBounds = true;
  naturalObject.pointCells = {8.0, 1.0, 4.0};
  layout.objects.push_back(naturalObject);
  cr::CreativeWorldLayoutObject customObject = naturalObject;
  customObject.stableKey = "object_custom";
  customObject.name = "Custom Prop";
  customObject.assetSourceBoundsMeters.max.x += 0.5;
  layout.objects.push_back(customObject);
  cr::CreativeWorldLayoutObject missingObject = naturalObject;
  missingObject.stableKey = "object_missing";
  missingObject.name = "Deleted Prop";
  missingObject.assetId = "deleted";
  missingObject.assetSourceBoundsMeters =
      app::creativeAssetBoundsAtPivot(*previousDeleted, {});
  layout.objects.push_back(missingObject);

  cr::CreativeWorldLayoutOpening naturalOpening;
  naturalOpening.stableKey = "opening_natural";
  naturalOpening.name = "Dormant Door";
  naturalOpening.includeInsert = false;
  naturalOpening.insertAssetId = "door";
  naturalOpening.insertAssetSourceBoundsMeters = previousDoor;
  naturalOpening.hasInsertAssetSourceBounds = true;
  layout.openings.push_back(naturalOpening);
  cr::CreativeWorldLayoutOpening customOpening = naturalOpening;
  customOpening.stableKey = "opening_custom";
  customOpening.name = "Custom Door";
  customOpening.insertAssetSourceBoundsMeters.max.y += 0.25;
  layout.openings.push_back(customOpening);

  const app::CreativeWorldLayoutAssetBoundsRefreshPlan plan =
      app::planCreativeWorldLayoutAssetBoundsRefresh(layout, previous, next);
  if (!expect(plan.updates.size() == 2U,
              "layout refresh emits the two natural source updates")) {
    return false;
  }
  app::CreativeEditorWorldLayoutState state;
  app::installCreativeEditorWorldLayout(state, layout);
  const std::uint64_t revisionBefore = state.revision;
  const std::size_t undoBefore = state.sourceHistory.undoEntries.size();
  const app::CreativeEditorWorldLayoutEditReceipt applied =
      app::applyCreativeEditorWorldLayoutAssetBoundsUpdates(state,
                                                             plan.updates);
  const std::uint64_t revisionAfter = state.revision;
  const std::size_t undoAfter = state.sourceHistory.undoEntries.size();
  const app::CreativeEditorWorldLayoutEditReceipt repeated =
      app::applyCreativeEditorWorldLayoutAssetBoundsUpdates(state,
                                                             plan.updates);

  std::vector<app::CreativeEditorWorldLayoutAssetBoundsUpdate> duplicate =
      plan.updates;
  duplicate.push_back(plan.updates[0]);
  const cr::CreativeWorldLayout beforeDuplicate = state.source;
  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::applyCreativeEditorWorldLayoutAssetBoundsUpdates(state, duplicate);

  return expect(plan.inspectedSourceCount == 5U &&
                    plan.customBoundsSkippedCount == 2U &&
                    plan.missingAssetCount == 1U,
                "layout refresh separates natural custom and deleted assets") &&
         expect(plan.updates[0].target ==
                        app::CreativeEditorWorldLayoutAssetBoundsTarget::Object &&
                    plan.updates[0].index == 0U &&
                    plan.updates[1].target ==
                        app::CreativeEditorWorldLayoutAssetBoundsTarget::
                            OpeningInsert &&
                    plan.updates[1].index == 0U,
                "layout refresh plan owns exact object and opening targets") &&
         expect(applied.accepted && applied.changed &&
                    revisionAfter == revisionBefore + 1U &&
                    undoAfter == undoBefore + 1U &&
                    cr::creativeBoundsExactlyEqual(
                        state.source.objects[0].assetSourceBoundsMeters,
                        nextProp) &&
                    cr::creativeBoundsExactlyEqual(
                        state.source.openings[0]
                            .insertAssetSourceBoundsMeters,
                        nextDoor) &&
                    !state.source.openings[0].includeInsert,
                "natural layout sources refresh together in one undo step") &&
         expect(cr::creativeBoundsExactlyEqual(
                    state.source.objects[1].assetSourceBoundsMeters,
                    customObject.assetSourceBoundsMeters) &&
                    cr::creativeBoundsExactlyEqual(
                        state.source.openings[1]
                            .insertAssetSourceBoundsMeters,
                        customOpening.insertAssetSourceBoundsMeters),
                "custom source envelopes remain authored truth") &&
         expect(repeated.accepted && !repeated.changed &&
                    state.revision == revisionAfter &&
                    state.sourceHistory.undoEntries.size() == undoAfter,
                "repeating a refresh is history-neutral") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == revisionAfter &&
                    state.source.stableKey == beforeDuplicate.stableKey &&
                    cr::creativeBoundsExactlyEqual(
                        state.source.objects[0].assetSourceBoundsMeters,
                        beforeDuplicate.objects[0].assetSourceBoundsMeters),
                "duplicate refresh targets reject without partial mutation");
}

bool catalogAndHotbarPreserveStableAssetIdentity() {
  constexpr std::array palette{cr::CreativeObjectKind::Crate};
  cr::CreativeCatalogAsset previousAsset;
  previousAsset.objectKind = cr::CreativeObjectKind::Rock;
  previousAsset.assetId = "boulder_01";
  previousAsset.label = "Boulder 01";
  previousAsset.contentHash = 11U;
  previousAsset.materialVariants = {{{"Mossy"}}, {{"Dry"}}};
  previousAsset.sourceBounds = {{-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}};
  cr::CreativeCatalogState previous =
      cr::makeCreativeCatalog(palette, std::span{&previousAsset, 1U}, 0U, {},
                              app::creativeEditorCatalogToolSpecs());
  static_cast<void>(
      cr::setCreativeCatalogPage(previous, cr::CreativeCatalogPage::Assets));
  static_cast<void>(cr::setCreativeCatalogQuery(previous, "boulder"));
  previous.open = true;

  app::CreativeCatalogAssetDiscovery discovery;
  cr::CreativeCatalogAsset refreshed = previousAsset;
  refreshed.objectKind = cr::CreativeObjectKind::Bridge;
  refreshed.contentHash = 22U;
  refreshed.materialVariants = {{{"Mossy"}}, {{"Wet"}}};
  refreshed.sourceBounds = {{-2.0, -0.5, -1.0}, {2.0, 0.5, 1.0}};
  discovery.assets.push_back(refreshed);
  cr::CreativeCatalogState replacement = app::rebuildCreativeAssetCatalog(
      previous, palette, discovery, "boulder_01");
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeCatalogEntry(replacement);

  cr::CreativeHotbarState hotbar;
  static_cast<void>(cr::setCreativeHotbarAsset(
      hotbar.entries[0], previousAsset.assetId, previousAsset.sourceBounds,
      previousAsset.contentHash, "Mossy"));
  hotbar.entries[0].objectKind = previousAsset.objectKind;
  static_cast<void>(cr::setCreativeHotbarAsset(
      hotbar.entries[1], "deleted_asset", previousAsset.sourceBounds));
  static_cast<void>(cr::setCreativeHotbarAsset(
      hotbar.entries[2], previousAsset.assetId, previousAsset.sourceBounds,
      previousAsset.contentHash, "Dry"));
  const cr::CreativeHotbarEntry deletedBefore = hotbar.entries[1];
  const std::size_t changed =
      app::refreshCreativeHotbarAssetFacts(hotbar, discovery.assets);

  return expect(replacement.open &&
                    replacement.page == cr::CreativeCatalogPage::Assets &&
                    replacement.query == "boulder" && selected != nullptr &&
                    cr::creativeHotbarAssetId(selected->hotbarEntry) ==
                        "boulder_01",
                "catalog page, query, open state, and asset selection survive") &&
         expect(changed == 2U &&
                    cr::creativeHotbarAssetId(hotbar.entries[0]) ==
                        "boulder_01" &&
                    hotbar.entries[0].objectKind ==
                        cr::CreativeObjectKind::Bridge &&
                    hotbar.entries[0].assetContentHash == 22U &&
                    cr::creativeHotbarAssetMaterialVariant(hotbar.entries[0]) ==
                        "Mossy" &&
                    cr::creativeBoundsExactlyEqual(
                        hotbar.entries[0].assetSourceBounds,
                        refreshed.sourceBounds) &&
                    cr::creativeHotbarAssetMaterialVariant(hotbar.entries[2])
                        .empty() &&
                    cr::creativeHotbarAssetId(hotbar.entries[2]) ==
                        "boulder_01" &&
                    hotbar.entries[1].assetId == deletedBefore.assetId &&
                    cr::creativeBoundsExactlyEqual(
                        hotbar.entries[1].assetSourceBounds,
                        deletedBefore.assetSourceBounds),
                "hotbar refreshes versions preserves valid variants and clears stale ones");
}

bool replacementPreviewCommitAndUndoAreAtomic() {
  const iggy3d::StaticMeshAssetCatalog catalog = replacementCatalog();
  cr::CreativeDocument document = cr::CreativeDocument::create("Replace");
  static_cast<void>(document.assignId(81U));
  const cr::CreativeDocumentCreateReceipt first = createRichAssetObject(
      document, catalog.entries[0], {4.0, 1.0, -3.0}, "First", 7U);
  const cr::CreativeDocumentCreateReceipt second = createRichAssetObject(
      document, catalog.entries[0], {-2.0, 0.5, 6.0}, "Second", 9U,
      first.objectId, "replacement_socket");
  if (!expect(first.accepted && second.accepted,
              "replacement fixtures created")) {
    return false;
  }
  const cr::CreativeObject firstBefore = *document.findObject(first.objectId);
  const cr::CreativeObject secondBefore = *document.findObject(second.objectId);

  cr::CreativeAppState appState;
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "replacement document installed")) {
    return false;
  }
  selectObject(appState.facade, first.objectId, false);
  selectObject(appState.facade, second.objectId, true);
  const std::uint64_t sourceRevision = appState.facade.document().revision();
  const std::vector<cr::TargetRef> selectionBefore(
      cr::selectedTargetList(appState.facade.selectionState()).begin(),
      cr::selectedTargetList(appState.facade.selectionState()).end());

  app::CreativeEditorAssetReplacementState state;
  const app::CreativeAssetReplacementBeginReceipt begin =
      app::beginCreativeEditorAssetReplacement(
          appState, catalog, cr::CreativeObjectKind::Rock, "asset_b", state);
  const cr::CreativeObject* liveFirst =
      appState.facade.document().findObject(first.objectId);
  const cr::CreativeObject* previewFirst =
      state.previewDocument.findObject(first.objectId);
  const cr::CreativeObject* previewSecond =
      state.previewDocument.findObject(second.objectId);
  const cr::CreativeBounds expectedFirst = app::creativeAssetBoundsAtPivot(
      catalog.entries[1], firstBefore.transform.position);
  const cr::CreativeBounds expectedSecond = app::creativeAssetBoundsAtPivot(
      catalog.entries[1], secondBefore.transform.position);
  app::CreativeEditorSceneCache sceneCache;
  const cr::CreativeDocument& renderDocument =
      app::creativeEditorAssetReplacementRenderDocument(
          state, appState.facade.document());
  const bool previewSceneBuilt = app::refreshCreativeEditorSceneCache(
      sceneCache, renderDocument, &catalog);
  const std::size_t replacementMeshCount =
      static_cast<std::size_t>(std::count_if(
          sceneCache.preview.roomBake.room.staticMeshes.begin(),
          sceneCache.preview.roomBake.room.staticMeshes.end(),
          [](const iggy3d::RoomStaticMeshAsset& mesh) {
            return mesh.meshId == "asset:asset_b";
          }));
  bool ok = expect(begin.accepted && begin.objectCount == 2U && state.active,
                   "replacement preview starts for multi-selection") &&
            expect(liveFirst != nullptr &&
                       liveFirst->assetId == firstBefore.assetId &&
                       appState.facade.document().revision() == sourceRevision,
                   "replacement preview leaves live document unchanged") &&
            expect(previewFirst != nullptr && previewSecond != nullptr &&
                       previewFirst->kind == cr::CreativeObjectKind::Rock &&
                       previewSecond->kind == cr::CreativeObjectKind::Rock &&
                       previewFirst->assetId == "asset_b" &&
                       previewSecond->assetId == "asset_b" &&
                       cr::creativeBoundsExactlyEqual(previewFirst->bounds,
                                                      expectedFirst) &&
                       cr::creativeBoundsExactlyEqual(previewSecond->bounds,
                                                      expectedSecond) &&
                       state.previewDocument.revision() == sourceRevision + 1U,
                   "replacement preview uses one staged atomic revision") &&
            expect(&app::creativeEditorAssetReplacementRenderDocument(
                       state, appState.facade.document()) ==
                       &state.previewDocument,
                   "active replacement supplies staged render document") &&
            expect(previewSceneBuilt && sceneCache.refreshCount == 1U &&
                       replacementMeshCount == 2U,
                   "staged scene renders the exact replacement asset meshes");

  cr::CreativeInputRouteResult confirm;
  confirm.context = cr::CreativeInputContext::AssetReplacementPreview;
  confirm.actions[0] = {cr::CreativeInputActionId::ConfirmActiveTool,
                        cr::CreativeInputKey::GamepadConfirm};
  confirm.actionCount = 1U;
  const app::CreativeEditorAssetReplacementFrameResult frame =
      app::processCreativeEditorAssetReplacementFrame(
          {appState, state, confirm});
  const cr::CreativeObject* firstAfter =
      appState.facade.document().findObject(first.objectId);
  const cr::CreativeObject* secondAfter =
      appState.facade.document().findObject(second.objectId);
  const std::span<const cr::TargetRef> selectionAfter =
      cr::selectedTargetList(appState.facade.selectionState());
  const bool liveSceneRebuilt = app::refreshCreativeEditorSceneCache(
      sceneCache, appState.facade.document(), &catalog);
  ok = expect(frame.blockWorldActions && frame.finished &&
                  frame.commitReceipt.accepted &&
                  frame.commitReceipt.changed &&
                  frame.commitReceipt.objectCount == 2U && !state.active,
              "confirm action commits replacement and closes preview") &&
       expect(appState.facade.document().revision() == sourceRevision + 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  frame.commitReceipt.historyReceipt.recorded,
              "replacement records one revision and one undo") &&
       expect(!liveSceneRebuilt && sceneCache.refreshCount == 1U,
              "confirmed document reuses its identical staged scene") &&
       expect(firstAfter != nullptr && secondAfter != nullptr &&
                  firstAfter->kind == cr::CreativeObjectKind::Rock &&
                  secondAfter->kind == cr::CreativeObjectKind::Rock &&
                  firstAfter->assetId == "asset_b" &&
                  secondAfter->assetId == "asset_b" &&
                  cr::creativeBoundsExactlyEqual(firstAfter->bounds,
                                                 expectedFirst) &&
                  cr::creativeBoundsExactlyEqual(secondAfter->bounds,
                                                 expectedSecond),
              "confirmed replacement publishes staged asset facts") &&
       expect(firstAfter != nullptr && secondAfter != nullptr &&
                  sameMetadata(*firstAfter, firstBefore) &&
                  sameMetadata(*secondAfter, secondBefore),
              "replacement preserves ids transforms hierarchy and metadata") &&
       expect(selectionAfter.size() == selectionBefore.size() &&
                  std::equal(selectionAfter.begin(), selectionAfter.end(),
                             selectionBefore.begin(), selectionBefore.end(),
                             [](cr::TargetRef lhs, cr::TargetRef rhs) {
                               return lhs.value == rhs.value;
                             }),
              "replacement preserves ordered selection") &&
       ok;

  const bool undone = app::undoLastEdit(appState, "asset_replace_test_undo");
  const cr::CreativeObject* firstUndone =
      appState.facade.document().findObject(first.objectId);
  const cr::CreativeObject* secondUndone =
      appState.facade.document().findObject(second.objectId);
  return expect(undone && firstUndone != nullptr && secondUndone != nullptr,
                "replacement undo applies") &&
         expect(firstUndone->kind == firstBefore.kind &&
                    secondUndone->kind == secondBefore.kind &&
                    firstUndone->assetId == firstBefore.assetId &&
                    secondUndone->assetId == secondBefore.assetId &&
                    cr::creativeBoundsExactlyEqual(firstUndone->bounds,
                                                   firstBefore.bounds) &&
                    cr::creativeBoundsExactlyEqual(secondUndone->bounds,
                                                   secondBefore.bounds),
                "one undo restores every replaced asset") &&
         ok;
}

bool replacementPlansFailClosedAndCancelCleanly() {
  const iggy3d::StaticMeshAssetCatalog catalog = replacementCatalog();
  cr::CreativeDocument document = cr::CreativeDocument::create("Reject");
  static_cast<void>(document.assignId(82U));
  const cr::CreativeDocumentCreateReceipt created = createRichAssetObject(
      document, catalog.entries[0], {1.0, 2.0, 3.0}, "Source", 3U);
  if (!expect(created.accepted, "replacement rejection fixture created")) {
    return false;
  }
  const std::array selection{created.objectId};
  const std::array duplicate{created.objectId, created.objectId};
  std::vector<cr::CreativeObjectId> overCapacity(
      app::kCreativeAssetReplacementCapacity + 1U, created.objectId);

  cr::CreativeDocument custom = document;
  custom.findObject(created.objectId)->bounds.max.x += 0.25;
  cr::CreativeDocument locked = document;
  locked.findObject(created.objectId)->locked = true;
  cr::CreativeDocument missing = document;
  missing.findObject(created.objectId)->assetId = "missing_asset";
  cr::CreativeDocument unsupported = document;
  unsupported.findObject(created.objectId)->kind = cr::CreativeObjectKind::Crate;
  cr::CreativeDocument patternOwned = document;
  cr::CreativeDocumentCreateRequest patternSourceRequest;
  patternSourceRequest.kind = cr::CreativeObjectKind::Crate;
  const cr::CreativeDocumentCreateReceipt patternSource =
      patternOwned.createObject(patternSourceRequest);
  cr::CreativePatternRecipeMutationRequest addPattern;
  addPattern.kind = cr::CreativePatternRecipeMutationKind::Add;
  addPattern.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  addPattern.recipe.sourceObjectIds = {patternSource.objectId};
  addPattern.recipe.generatedObjectIds = {created.objectId};
  const cr::CreativePatternRecipeMutationReceipt patternRecipe =
      patternOwned.applyPatternRecipeMutation(addPattern);
  cr::CreativeDocument worldOwned = document;
  worldOwned.findObject(created.objectId)
      ->tags.push_back("creative_world_layout:asset_replace_fixture");
  iggy3d::StaticMeshAssetCatalog invalidBoundsCatalog = catalog;
  invalidBoundsCatalog.entries[1].boundsMax.x =
      std::numeric_limits<float>::infinity();

  const app::CreativeAssetReplacementPlan noChange =
      app::planCreativeAssetReplacement(
          document, selection, catalog, cr::CreativeObjectKind::Prop,
          "asset_a");
  const app::CreativeAssetReplacementPlan customPlan =
      app::planCreativeAssetReplacement(
          custom, selection, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan lockedPlan =
      app::planCreativeAssetReplacement(
          locked, selection, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan missingPlan =
      app::planCreativeAssetReplacement(
          missing, selection, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan unsupportedPlan =
      app::planCreativeAssetReplacement(
          unsupported, selection, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan patternOwnedPlan =
      app::planCreativeAssetReplacement(
          patternOwned, selection, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan worldOwnedPlan =
      app::planCreativeAssetReplacement(
          worldOwned, selection, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan duplicatePlan =
      app::planCreativeAssetReplacement(
          document, duplicate, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan capacityPlan =
      app::planCreativeAssetReplacement(
          document, overCapacity, catalog, cr::CreativeObjectKind::Rock,
          "asset_b");
  const app::CreativeAssetReplacementPlan invalidTarget =
      app::planCreativeAssetReplacement(
          document, selection, catalog, cr::CreativeObjectKind::Unknown,
          "asset_b");
  const app::CreativeAssetReplacementPlan invalidTargetBounds =
      app::planCreativeAssetReplacement(
          document, selection, invalidBoundsCatalog,
          cr::CreativeObjectKind::Rock, "asset_b");
  const auto* customPayload =
      customPlan.mutations.empty()
          ? nullptr
          : std::get_if<cr::SetAssetMutation>(
                &customPlan.mutations.front().payload.value);
  bool ok = expect(noChange.status ==
                       app::CreativeAssetReplacementStatus::NoChange &&
                       !noChange.accepted,
                   "identical replacement is a no-change") &&
            expect(customPlan.accepted &&
                       customPlan.status ==
                           app::CreativeAssetReplacementStatus::Ready &&
                       customPayload != nullptr &&
                       cr::creativeBoundsExactlyEqual(
                           customPayload->bounds,
                           custom.findObject(created.objectId)->bounds),
                   "replacement preserves custom authored bounds") &&
            expect(lockedPlan.status ==
                           app::CreativeAssetReplacementStatus::LockedObject &&
                       missingPlan.status ==
                           app::CreativeAssetReplacementStatus::MissingSourceAsset &&
                       unsupportedPlan.status ==
                           app::CreativeAssetReplacementStatus::UnsupportedObject,
                   "unsafe source objects fail closed") &&
            expect(duplicatePlan.status ==
                       app::CreativeAssetReplacementStatus::InvalidSelection &&
                       capacityPlan.status ==
                           app::CreativeAssetReplacementStatus::CapacityExceeded &&
                       invalidTarget.status ==
                           app::CreativeAssetReplacementStatus::InvalidTarget &&
                       invalidTargetBounds.status ==
                           app::CreativeAssetReplacementStatus::InvalidTarget,
                   "invalid selection capacity and target fail closed") &&
            expect(patternSource.accepted && patternRecipe.accepted &&
                       patternOwnedPlan.status ==
                           app::CreativeAssetReplacementStatus::SourceOwned &&
                       patternOwnedPlan.reasonCode ==
                           "creative_semantic_action_pattern_owned" &&
                       worldOwnedPlan.status ==
                           app::CreativeAssetReplacementStatus::SourceOwned &&
                       worldOwnedPlan.reasonCode ==
                           "creative_semantic_action_world_layout_owned",
                   "generated assets reject structural replacement");

  cr::CreativeAppState patternAppState;
  const cr::CreativeFacadeDocumentInstallReceipt patternInstalled =
      patternAppState.facade.installDocument(std::move(patternOwned));
  selectObject(patternAppState.facade, created.objectId, false);
  app::CreativeEditorAssetReplacementState patternState;
  const std::uint64_t patternRevision =
      patternAppState.facade.document().revision();
  const app::CreativeAssetReplacementBeginReceipt patternBegin =
      app::beginCreativeEditorAssetReplacement(
          patternAppState, catalog, cr::CreativeObjectKind::Rock, "asset_b",
          patternState);
  cr::CreativeAppState worldAppState;
  const cr::CreativeFacadeDocumentInstallReceipt worldInstalled =
      worldAppState.facade.installDocument(std::move(worldOwned));
  selectObject(worldAppState.facade, created.objectId, false);
  app::CreativeEditorAssetReplacementState worldState;
  const std::uint64_t worldRevision =
      worldAppState.facade.document().revision();
  const app::CreativeAssetReplacementBeginReceipt worldBegin =
      app::beginCreativeEditorAssetReplacement(
          worldAppState, catalog, cr::CreativeObjectKind::Rock, "asset_b",
          worldState);
  ok = expect(patternInstalled.accepted && worldInstalled.accepted &&
                  !patternBegin.accepted && !worldBegin.accepted &&
                  patternBegin.status ==
                      app::CreativeAssetReplacementStatus::SourceOwned &&
                  worldBegin.status ==
                      app::CreativeAssetReplacementStatus::SourceOwned &&
                  !patternState.active && !worldState.active,
              "generated replacements never open editable previews") &&
       expect(patternAppState.facade.document().revision() == patternRevision &&
                  worldAppState.facade.document().revision() == worldRevision &&
                  cr::creativeUndoDepth(patternAppState.history) == 0U &&
                  cr::creativeUndoDepth(worldAppState.history) == 0U,
              "generated preview rejection leaves documents and history "
              "unchanged") &&
       ok;

  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectObject(appState.facade, created.objectId, false);
  app::CreativeEditorAssetReplacementState state;
  const app::CreativeAssetReplacementBeginReceipt begin =
      app::beginCreativeEditorAssetReplacement(
          appState, catalog, cr::CreativeObjectKind::Rock, "asset_b", state);
  const cr::CreativeDocumentMutationReceipt externalChange =
      appState.facade.mutateObject(
          created.objectId, cr::CreativeMutationKind::Rename,
          cr::makeRenamePayload("Changed elsewhere"));
  const app::CreativeAssetReplacementCommitReceipt stale =
      app::commitCreativeEditorAssetReplacement(appState, state);
  ok = expect(begin.accepted && externalChange.changed && !stale.accepted &&
                  stale.status ==
                      app::CreativeAssetReplacementStatus::StaleDocument &&
                  !state.active && cr::creativeUndoDepth(appState.history) == 0U,
              "stale preview cannot overwrite a newer document") &&
       ok;

  const app::CreativeAssetReplacementBeginReceipt restart =
      app::beginCreativeEditorAssetReplacement(
          appState, catalog, cr::CreativeObjectKind::Rock, "asset_b", state);
  const std::uint64_t revisionBeforeCancel =
      appState.facade.document().revision();
  cr::CreativeInputRouteResult cancel;
  cancel.context = cr::CreativeInputContext::AssetReplacementPreview;
  cancel.actions[0] = {cr::CreativeInputActionId::CancelActiveTool,
                       cr::CreativeInputKey::GamepadCancel};
  cancel.actionCount = 1U;
  const app::CreativeEditorAssetReplacementFrameResult cancelled =
      app::processCreativeEditorAssetReplacementFrame(
          {appState, state, cancel});
  ok = expect(restart.accepted && cancelled.blockWorldActions &&
                  cancelled.finished && !state.active,
              "cancel action closes active replacement preview") &&
       expect(appState.facade.document().revision() == revisionBeforeCancel &&
                  appState.facade.document()
                          .findObject(created.objectId)
                          ->assetId == "asset_a" &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "cancel leaves document and history unchanged") &&
       ok;

  cr::CreativeDocument commitRecheck = cr::CreativeDocument::create("Recheck");
  static_cast<void>(commitRecheck.assignId(83U));
  const cr::CreativeDocumentCreateReceipt recheckObject =
      createRichAssetObject(commitRecheck, catalog.entries[0], {}, "Recheck",
                            1U);
  const std::array recheckSelection{recheckObject.objectId};
  app::CreativeEditorAssetReplacementState recheckState;
  recheckState.plan = app::planCreativeAssetReplacement(
      commitRecheck, recheckSelection, catalog, cr::CreativeObjectKind::Rock,
      "asset_b");
  commitRecheck.findObject(recheckObject.objectId)
      ->tags.push_back("creative_world_layout:commit_recheck");
  cr::CreativeAppState recheckAppState;
  const cr::CreativeFacadeDocumentInstallReceipt recheckInstalled =
      recheckAppState.facade.installDocument(std::move(commitRecheck));
  recheckState.active = recheckState.plan.accepted;
  const std::uint64_t recheckRevision =
      recheckAppState.facade.document().revision();
  const app::CreativeAssetReplacementCommitReceipt sourceOwnedCommit =
      app::commitCreativeEditorAssetReplacement(recheckAppState, recheckState);
  const cr::CreativeObject* rechecked =
      recheckAppState.facade.findObject(recheckObject.objectId);
  return expect(recheckObject.accepted && recheckInstalled.accepted &&
                    !sourceOwnedCommit.accepted &&
                    sourceOwnedCommit.status ==
                        app::CreativeAssetReplacementStatus::SourceOwned &&
                    sourceOwnedCommit.reasonCode ==
                        "creative_semantic_action_world_layout_owned" &&
                    !recheckState.active,
                "commit rechecks generated ownership before mutation") &&
         expect(rechecked != nullptr && rechecked->assetId == "asset_a" &&
                    recheckAppState.facade.document().revision() ==
                        recheckRevision &&
                    cr::creativeUndoDepth(recheckAppState.history) == 0U,
                "source-owned commit creates no mutation or history") &&
         ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = discoveryReportsValidBrokenAndFatalRoots() && ok;
  ok = deletedAssetsRemainExplicitFailures() && ok;
  ok = boundsRefreshUpdatesOnlyNaturalAssetBounds() && ok;
  ok = boundsRefreshPreservesAttachmentRelationshipAndWorldPose() && ok;
  ok = worldLayoutBoundsRefreshIsAtomicAndPreservesCustomSources() && ok;
  ok = catalogAndHotbarPreserveStableAssetIdentity() && ok;
  ok = replacementPreviewCommitAndUndoAreAtomic() && ok;
  ok = replacementPlansFailClosedAndCancelCleanly() && ok;
  return ok ? 0 : 1;
}
