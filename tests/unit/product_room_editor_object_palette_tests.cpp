#include "app/iggy3d/room_editor/ObjectPalette.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const iggy3d::ProductRoomEditorObjectPaletteEntry* findEntry(
    const iggy3d::ProductRoomEditorObjectPalette& palette,
    std::string_view assetId) {
  for (const iggy3d::ProductRoomEditorObjectPaletteEntry& entry :
       palette.entries) {
    if (entry.assetId == assetId) {
      return &entry;
    }
  }
  return nullptr;
}

iggy3d::ObjectAssetDefinition validAsset(std::string_view id) {
  iggy3d::ObjectAssetDefinition asset;
  asset.id.value = std::string(id);
  asset.primitiveShape.kind = iggy3d::ObjectPrimitiveShapeKind::Box;
  asset.primitiveShape.sizeMeters = {1.0F, 1.0F, 1.0F};
  asset.render.mesh.value = "generated_box";
  asset.render.material.value = "stone_proxy";
  asset.collision.kind = iggy3d::ObjectCollisionShapeKind::Box;
  asset.collision.sizeMeters = {1.0F, 1.0F, 1.0F};
  asset.physics.motion = iggy3d::ObjectPhysicsMotionKind::Static;
  asset.physics.massKilograms = 0.0F;
  asset.material.kind = iggy3d::ObjectMaterialKind::Stone;
  asset.interaction.verbs = {iggy3d::ObjectInteractionVerb::Inspect};
  asset.editor.displayName = "Palette Test";
  return asset;
}

bool builtInPaletteUsesCatalogOrderAndDefaultSelection() {
  const iggy3d::ProductRoomEditorObjectPalette palette =
      iggy3d::makeBuiltInProductRoomEditorObjectPalette();

  return expect(palette.ready, "built-in ready") &&
         expect(palette.status == "object_palette_ready", "ready status") &&
         expect(palette.reasonCode == "object_palette_ready",
                "ready reason") &&
         expect(palette.entries.size() == 4U, "entry count") &&
         expect(palette.enabledCount == 4U, "enabled count") &&
         expect(palette.disabledCount == 0U, "disabled count") &&
         expect(palette.entries[0].assetId == "stone_block_proxy",
                "stone block first") &&
         expect(palette.entries[1].assetId == "stone_floor_slab",
                "floor slab second") &&
         expect(palette.entries[2].assetId == "stone_wall_panel",
                "wall panel third") &&
         expect(palette.entries[3].assetId == "wood_crate_proxy",
                "crate fourth") &&
         expect(palette.selectedIndex == 0U, "default selected index") &&
         expect(palette.selectedAssetId == "stone_block_proxy",
                "default selected asset");
}

bool selectedAssetChoosesMatchingEnabledEntry() {
  const iggy3d::ProductRoomEditorObjectPalette palette =
      iggy3d::makeBuiltInProductRoomEditorObjectPalette("wood_crate_proxy");

  return expect(palette.ready, "selected ready") &&
         expect(palette.selectedIndex == 3U, "crate selected index") &&
         expect(palette.selectedAssetId == "wood_crate_proxy",
                "crate selected asset");
}

bool missingSelectionFallsBackToFirstEnabledEntry() {
  const iggy3d::ProductRoomEditorObjectPalette palette =
      iggy3d::makeBuiltInProductRoomEditorObjectPalette("missing_proxy");

  return expect(palette.ready, "fallback ready") &&
         expect(palette.selectedIndex == 0U, "fallback index") &&
         expect(palette.selectedAssetId == "stone_block_proxy",
                "fallback selected asset");
}

bool missingAndEmptyCatalogsRejectWithStableStatus() {
  const iggy3d::ObjectAssetCatalog empty;
  const iggy3d::ProductRoomEditorObjectPalette missing =
      iggy3d::buildProductRoomEditorObjectPalette({nullptr, {}});
  const iggy3d::ProductRoomEditorObjectPalette emptyResult =
      iggy3d::buildProductRoomEditorObjectPalette({&empty, {}});

  return expect(!missing.ready, "missing not ready") &&
         expect(missing.status == "object_palette_catalog_missing",
                "missing status") &&
         expect(missing.reasonCode == "object_palette_catalog_missing",
                "missing reason") &&
         expect(!emptyResult.ready, "empty not ready") &&
         expect(emptyResult.status == "object_palette_catalog_empty",
                "empty status") &&
         expect(emptyResult.reasonCode == "object_palette_catalog_empty",
                "empty reason");
}

bool invalidCatalogRejectsWithUnderlyingReason() {
  iggy3d::ObjectAssetCatalog catalog;
  catalog.assets.push_back(validAsset("bad_asset"));
  catalog.assets[0].render.mesh.value.clear();

  const iggy3d::ProductRoomEditorObjectPalette palette =
      iggy3d::buildProductRoomEditorObjectPalette({&catalog, {}});

  return expect(!palette.ready, "invalid not ready") &&
         expect(palette.status == "object_palette_catalog_invalid",
                "invalid status") &&
         expect(palette.reasonCode == "object_missing_render_asset",
                "invalid reason");
}

bool entryFactsExposeObjectTruthForEditorUse() {
  const iggy3d::ProductRoomEditorObjectPalette palette =
      iggy3d::makeBuiltInProductRoomEditorObjectPalette();
  const iggy3d::ProductRoomEditorObjectPaletteEntry* floor =
      findEntry(palette, "stone_floor_slab");
  const iggy3d::ProductRoomEditorObjectPaletteEntry* crate =
      findEntry(palette, "wood_crate_proxy");

  return expect(floor != nullptr, "floor entry exists") &&
         expect(crate != nullptr, "crate entry exists") &&
         expect(floor->label == "Stone Floor Slab", "floor label") &&
         expect(floor->group == "shapes", "floor group") &&
         expect(floor->material == "stone", "floor material") &&
         expect(floor->shape == "slab", "floor shape") &&
         expect(floor->blocksMovement, "floor blocks movement") &&
         expect(!floor->blocksVision, "floor does not block vision") &&
         expect(!floor->dynamicPhysics, "floor static") &&
         expect(!floor->flammable, "floor not flammable") &&
         expect(crate->label == "Wood Crate Proxy", "crate label") &&
         expect(crate->material == "wood", "crate material") &&
         expect(crate->shape == "box", "crate shape") &&
         expect(crate->dynamicPhysics, "crate dynamic") &&
         expect(crate->flammable, "crate flammable");
}

bool disabledAssetsRemainVisibleButUnselected() {
  iggy3d::ObjectAssetCatalog catalog;
  catalog.assets.push_back(validAsset("disabled_asset"));
  catalog.assets[0].editor.placeable = false;

  const iggy3d::ProductRoomEditorObjectPalette palette =
      iggy3d::buildProductRoomEditorObjectPalette({&catalog, "disabled_asset"});

  return expect(!palette.ready, "disabled-only not ready") &&
         expect(palette.status == "object_palette_no_placeable_assets",
                "disabled-only status") &&
         expect(palette.entries.size() == 1U, "disabled entry visible") &&
         expect(!palette.entries[0].enabled, "disabled entry disabled") &&
         expect(palette.enabledCount == 0U, "disabled enabled count") &&
         expect(palette.disabledCount == 1U, "disabled disabled count") &&
         expect(palette.selectedAssetId == "none", "disabled not selected");
}

}  // namespace

int main() {
  const bool ok = builtInPaletteUsesCatalogOrderAndDefaultSelection() &&
                  selectedAssetChoosesMatchingEnabledEntry() &&
                  missingSelectionFallsBackToFirstEnabledEntry() &&
                  missingAndEmptyCatalogsRejectWithStableStatus() &&
                  invalidCatalogRejectsWithUnderlyingReason() &&
                  entryFactsExposeObjectTruthForEditorUse() &&
                  disabledAssetsRemainVisibleButUnselected();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
