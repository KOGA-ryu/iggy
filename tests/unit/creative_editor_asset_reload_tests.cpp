#include "EditorAssets.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorEdits.hpp"
#include "EditorPreviewFrame.hpp"

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
         lhs.pathPoints.size() == rhs.pathPoints.size();
}

iggy3d::StaticMeshAssetCatalog replacementCatalog() {
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "asset_a", {-1.0F, 0.0F, -1.0F}, {1.0F, 2.0F, 1.0F}));
  catalog.entries.push_back(catalogEntry(
      "asset_b", {-0.5F, 0.0F, -2.0F}, {0.5F, 4.0F, 2.0F}));
  return catalog;
}

cr::CreativeDocumentCreateReceipt createRichAssetObject(
    cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalogEntry& source,
    cr::CreativeVec3 pivot,
    std::string name,
    cr::CreativeLayerId layerId,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Prop;
  request.name = std::move(name);
  request.assetId = source.assetId;
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

bool replacementPreviewCommitAndUndoAreAtomic() {
  const iggy3d::StaticMeshAssetCatalog catalog = replacementCatalog();
  cr::CreativeDocument document = cr::CreativeDocument::create("Replace");
  static_cast<void>(document.assignId(81U));
  const cr::CreativeDocumentCreateReceipt first = createRichAssetObject(
      document, catalog.entries[0], {4.0, 1.0, -3.0}, "First", 7U);
  const cr::CreativeDocumentCreateReceipt second = createRichAssetObject(
      document, catalog.entries[0], {-2.0, 0.5, 6.0}, "Second", 9U,
      first.objectId);
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
  const iggy3d::ProductMapMakerGridSnapshot grid;
  const cr::CreativeDocument& renderDocument =
      app::creativeEditorAssetReplacementRenderDocument(
          state, appState.facade.document());
  const bool previewSceneBuilt = app::refreshCreativeEditorSceneCache(
      sceneCache, renderDocument, grid, &catalog);
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
      sceneCache, appState.facade.document(), grid, &catalog);
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
  bool ok = expect(noChange.status ==
                       app::CreativeAssetReplacementStatus::NoChange &&
                       !noChange.accepted,
                   "identical replacement is a no-change") &&
            expect(customPlan.status ==
                       app::CreativeAssetReplacementStatus::CustomBounds &&
                       lockedPlan.status ==
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
                   "invalid selection capacity and target fail closed");

  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectObject(appState.facade, created.objectId, false);
  app::CreativeEditorAssetReplacementState state;
  const app::CreativeAssetReplacementBeginReceipt begin =
      app::beginCreativeEditorAssetReplacement(
          appState, catalog, cr::CreativeObjectKind::Rock, "asset_b", state);
  const cr::CreativeDocumentMutationReceipt externalChange =
      cr::renameDocumentObject(appState.facade.documentForPersistence(),
                               created.objectId, "Changed elsewhere");
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
  return expect(restart.accepted && cancelled.blockWorldActions &&
                    cancelled.finished && !state.active,
                "cancel action closes active replacement preview") &&
         expect(appState.facade.document().revision() == revisionBeforeCancel &&
                    appState.facade.document()
                            .findObject(created.objectId)
                            ->assetId == "asset_a" &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "cancel leaves document and history unchanged") &&
         ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = discoveryReportsValidBrokenAndFatalRoots() && ok;
  ok = deletedAssetsRemainExplicitFailures() && ok;
  ok = boundsRefreshUpdatesOnlyNaturalAssetBounds() && ok;
  ok = catalogAndHotbarPreserveStableAssetIdentity() && ok;
  ok = replacementPreviewCommitAndUndoAreAtomic() && ok;
  ok = replacementPlansFailClosedAndCancelCleanly() && ok;
  return ok ? 0 : 1;
}
