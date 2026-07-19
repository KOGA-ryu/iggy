#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include "EditorDesktopModel.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

namespace {

[[nodiscard]] bool validCatalogAsset(
    const creative::CreativeCatalogEntry& entry) noexcept {
  const creative::CreativeBoundsMetrics bounds =
      creative::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
  return entry.category == creative::CreativeCatalogEntryCategory::Asset &&
         entry.hotbarEntry.objectKind > creative::CreativeObjectKind::Unknown &&
         entry.hotbarEntry.objectKind < creative::CreativeObjectKind::Count &&
         !creative::creativeHotbarAssetId(entry.hotbarEntry).empty() &&
         entry.hotbarEntry.hasAssetBounds && bounds.valid &&
         creative::isPositiveCreativeVec3(bounds.size);
}

CreativeEditorWorldLayoutEditReceipt rejectWorldLayoutAssetRepair(
    CreativeEditorWorldLayoutState& state, std::string message,
    std::string reasonCode) {
  state.statusMessage = std::move(message);
  return {false, false, std::move(reasonCode)};
}

}  // namespace

[[nodiscard]] GeneratedSourceScopeResolution resolveGeneratedSourceScope(
    const creative::CreativeWorldLayout& layout,
    const creative::CreativeObject& object,
    creative::CreativeWorldLayoutTable table,
    std::size_t index,
    std::string_view stableKey) {
  const creative::CreativeWorldLayoutObjectProvenance provenance =
      creative::resolveCreativeWorldLayoutObjectProvenance(layout, object);
  const CreativeDesktopGeneratedSourceScopeModel scopes =
      buildCreativeDesktopGeneratedSourceScopeModel(layout, provenance);
  const std::size_t scopeIndex =
      findCreativeDesktopGeneratedSourceScope(scopes, table, index);
  if (scopeIndex >= scopes.count) {
    return {};
  }
  return {true, !stableKey.empty() &&
                    scopes.entries[scopeIndex].stableKey == stableKey};
}

[[nodiscard]] creative::CreativeObjectId findGeneratedSourceScopeObject(
    const creative::CreativeDocument& document,
    const creative::CreativeWorldLayout& layout,
    creative::CreativeWorldLayoutTable table,
    std::size_t index,
    creative::CreativeObjectKind preferredKind) noexcept {
  creative::CreativeObjectId fallback = creative::kInvalidObjectId;
  for (const creative::CreativeObject& object : document.objects()) {
    const creative::CreativeWorldLayoutObjectProvenance provenance =
        creative::resolveCreativeWorldLayoutObjectProvenance(layout, object);
    const CreativeDesktopGeneratedSourceScopeModel scopes =
        buildCreativeDesktopGeneratedSourceScopeModel(layout, provenance);
    if (findCreativeDesktopGeneratedSourceScope(scopes, table, index) >=
        scopes.count) {
      continue;
    }
    if (object.kind == preferredKind) {
      return object.id;
    }
    if (fallback == creative::kInvalidObjectId) {
      fallback = object.id;
    }
  }
  return fallback;
}

[[nodiscard]] const creative::CreativeCatalogEntry* findCatalogAsset(
    const creative::CreativeCatalogState& catalog,
    std::string_view assetId) noexcept {
  const auto found = std::find_if(
      catalog.entries.begin(), catalog.entries.end(),
      [assetId](const creative::CreativeCatalogEntry& entry) {
        return entry.category ==
                   creative::CreativeCatalogEntryCategory::Asset &&
               creative::creativeHotbarAssetId(entry.hotbarEntry) == assetId;
      });
  return found == catalog.entries.end() ? nullptr : &*found;
}

CreativeEditorWorldLayoutEditReceipt repairWorldLayoutAsset(
    CreativeEditorWorldLayoutState& state,
    const creative::CreativeCatalogState& catalog,
    double gridCellSizeMeters,
    const CreativeDesktopWorldLayoutAssetRepairPayload& payload) {
  if (payload.operation >=
          CreativeDesktopWorldLayoutAssetRepairOperation::Count ||
      (payload.table != creative::CreativeWorldLayoutTable::Opening &&
       payload.table != creative::CreativeWorldLayoutTable::Object) ||
      payload.stableKey.empty() || payload.expectedAssetId.empty() ||
      !creativeEditorWorldLayoutSourceStableKeyMatches(
          state, payload.table, payload.index, payload.stableKey)) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair target is stale",
        "creative_editor_world_layout_asset_repair_target_invalid");
  }

  std::string_view currentAssetId;
  if (payload.table == creative::CreativeWorldLayoutTable::Opening) {
    if (payload.index >= state.source.openings.size()) {
      return rejectWorldLayoutAssetRepair(
          state, "asset repair target is stale",
          "creative_editor_world_layout_asset_repair_target_invalid");
    }
    currentAssetId = state.source.openings[payload.index].insertAssetId;
  } else {
    if (payload.index >= state.source.objects.size()) {
      return rejectWorldLayoutAssetRepair(
          state, "asset repair target is stale",
          "creative_editor_world_layout_asset_repair_target_invalid");
    }
    currentAssetId = state.source.objects[payload.index].assetId;
  }
  if (currentAssetId != payload.expectedAssetId) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair identity changed",
        "creative_editor_world_layout_asset_repair_identity_mismatch");
  }

  if (payload.operation ==
      CreativeDesktopWorldLayoutAssetRepairOperation::UseProceduralInsert) {
    if (payload.table != creative::CreativeWorldLayoutTable::Opening) {
      return rejectWorldLayoutAssetRepair(
          state, "procedural fallback is only valid for openings",
          "creative_editor_world_layout_asset_repair_operation_invalid");
    }
    CreativeEditorWorldLayoutOpeningInsertRequest request;
    request.openingIndex = payload.index;
    request.operation =
        CreativeEditorWorldLayoutOpeningInsertOperation::UseProceduralInsert;
    return applyCreativeEditorWorldLayoutOpeningInsert(state, request);
  }

  const std::string_view requestedAssetId =
      payload.operation ==
              CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds
          ? std::string_view(payload.expectedAssetId)
          : std::string_view(payload.replacementAssetId);
  const creative::CreativeCatalogEntry* entry =
      findCatalogAsset(catalog, requestedAssetId);
  if (entry == nullptr || !validCatalogAsset(*entry)) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair catalog entry is unavailable",
        "creative_editor_world_layout_asset_repair_catalog_missing");
  }

  if (payload.table == creative::CreativeWorldLayoutTable::Opening) {
    const creative::CreativeWorldLayoutOpening& opening =
        state.source.openings[payload.index];
    if (!creativeEditorWorldLayoutCatalogAssetMatchesOpening(
            entry->assetAuthoringMetadata.categoryId, opening.kind)) {
      return rejectWorldLayoutAssetRepair(
          state, "choose a compatible door or window asset",
          "creative_editor_world_layout_asset_repair_incompatible");
    }
    CreativeEditorWorldLayoutOpeningInsertRequest request;
    request.openingIndex = payload.index;
    request.operation =
        CreativeEditorWorldLayoutOpeningInsertOperation::FitAssetToOpening;
    request.assetKind = opening.kind;
    request.assetId = creative::creativeHotbarAssetId(entry->hotbarEntry);
    request.assetSourceBoundsMeters = entry->hotbarEntry.assetSourceBounds;
    request.gridCellSizeMeters = gridCellSizeMeters;
    return applyCreativeEditorWorldLayoutOpeningInsert(state, request);
  }

  const creative::CreativeWorldLayoutObject& object =
      state.source.objects[payload.index];
  if (creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
          entry->assetAuthoringMetadata.categoryId) ||
      entry->hotbarEntry.objectKind != object.kind) {
    return rejectWorldLayoutAssetRepair(
        state, "choose a compatible object asset",
        "creative_editor_world_layout_asset_repair_incompatible");
  }
  CreativeEditorWorldLayoutObjectSettings settings;
  if (!readCreativeEditorWorldLayoutObjectSettings(state, payload.index,
                                                   settings)) {
    return rejectWorldLayoutAssetRepair(
        state, "asset repair target is stale",
        "creative_editor_world_layout_asset_repair_target_invalid");
  }
  settings.assetId = creative::creativeHotbarAssetId(entry->hotbarEntry);
  settings.assetSourceBoundsMeters = entry->hotbarEntry.assetSourceBounds;
  settings.hasAssetSourceBounds = true;
  return setCreativeEditorWorldLayoutObjectSettings(state, payload.index,
                                                    std::move(settings));
}


}  // namespace iggy3d_creative_app
