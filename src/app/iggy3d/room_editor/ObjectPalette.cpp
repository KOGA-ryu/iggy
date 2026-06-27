#include "app/iggy3d/room_editor/ObjectPalette.hpp"

#include <utility>

namespace iggy3d {
namespace {

ProductRoomEditorObjectPalette paletteResult(std::string status,
                                             std::string reasonCode) {
  ProductRoomEditorObjectPalette result;
  result.status = std::move(status);
  result.reasonCode = std::move(reasonCode);
  return result;
}

std::string displayNameForAsset(const ObjectAssetDefinition& asset) {
  // branch-gate: BG-1083
  if (asset.editor.displayName.empty()) {
    return asset.id.value;
  }
  return asset.editor.displayName;
}

ProductRoomEditorObjectPaletteEntry paletteEntryForAsset(
    const ObjectAssetDefinition& asset) {
  ProductRoomEditorObjectPaletteEntry entry;
  entry.assetId = asset.id.value;
  entry.label = displayNameForAsset(asset);
  entry.group = asset.editor.paletteGroup;
  entry.material = objectMaterialKindName(asset.material.kind);
  entry.shape = objectPrimitiveShapeKindName(asset.primitiveShape.kind);
  entry.placeable = asset.editor.placeable;
  entry.enabled = asset.editor.placeable;
  entry.blocksMovement = asset.collision.blocksMovement;
  entry.blocksVision = asset.collision.blocksVision;
  entry.dynamicPhysics = asset.physics.motion == ObjectPhysicsMotionKind::Dynamic;
  entry.flammable = asset.material.flammable;
  return entry;
}

}  // namespace

ProductRoomEditorObjectPalette buildProductRoomEditorObjectPalette(
    const ProductRoomEditorObjectPaletteRequest& request) {
  // branch-gate: BG-1083
  if (request.catalog == nullptr) {
    return paletteResult("object_palette_catalog_missing",
                         "object_palette_catalog_missing");
  }

  // branch-gate: BG-1083
  if (request.catalog->assets.empty()) {
    return paletteResult("object_palette_catalog_empty",
                         "object_palette_catalog_empty");
  }

  const ObjectValidationResult validation =
      validateObjectAssetCatalog(*request.catalog);
  // branch-gate: BG-1083
  if (!validation.ok) {
    return paletteResult("object_palette_catalog_invalid",
                         std::string(validation.reasonCode));
  }

  ProductRoomEditorObjectPalette result;
  result.status = "object_palette_ready";
  result.reasonCode = "object_palette_ready";

  for (const ObjectAssetDefinition& asset : request.catalog->assets) {
    ProductRoomEditorObjectPaletteEntry entry = paletteEntryForAsset(asset);
    // branch-gate: BG-1083
    if (entry.enabled) {
      ++result.enabledCount;
    } else {
      ++result.disabledCount;
    }
    result.entries.push_back(std::move(entry));
  }

  for (std::size_t index = 0U; index < result.entries.size(); ++index) {
    const ProductRoomEditorObjectPaletteEntry& entry = result.entries[index];
    // branch-gate: BG-1083
    if (entry.enabled && request.selectedAssetId == entry.assetId) {
      result.ready = true;
      result.selectedIndex = index;
      result.selectedAssetId = entry.assetId;
      return result;
    }
  }

  for (std::size_t index = 0U; index < result.entries.size(); ++index) {
    const ProductRoomEditorObjectPaletteEntry& entry = result.entries[index];
    // branch-gate: BG-1083
    if (entry.enabled) {
      result.ready = true;
      result.selectedIndex = index;
      result.selectedAssetId = entry.assetId;
      return result;
    }
  }

  result.status = "object_palette_no_placeable_assets";
  result.reasonCode = "object_palette_no_placeable_assets";
  return result;
}

ProductRoomEditorObjectPalette makeBuiltInProductRoomEditorObjectPalette(
    std::string_view selectedAssetId) {
  const ObjectAssetCatalog catalog = makeBuiltInObjectAssetCatalog();
  return buildProductRoomEditorObjectPalette({&catalog, selectedAssetId});
}

}  // namespace iggy3d
