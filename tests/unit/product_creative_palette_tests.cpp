#include "app/iggy3d/creative/Palette.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool finitePositive(iggy3d::Vec3 value) {
  return std::isfinite(value.x) && value.x > 0.0F &&
         std::isfinite(value.y) && value.y > 0.0F &&
         std::isfinite(value.z) && value.z > 0.0F;
}

const iggy3d::ObjectAssetDefinition* catalogAsset(
    const iggy3d::ObjectAssetCatalog& catalog,
    std::string_view assetId) {
  for (const iggy3d::ObjectAssetDefinition& asset : catalog.assets) {
    if (asset.id.value == assetId) {
      return &asset;
    }
  }
  return nullptr;
}

bool groupNamesAreStable() {
  return expect(iggy3d::productCreativePaletteGroupName(
                    iggy3d::ProductCreativePaletteGroup::Structure) ==
                    "structure",
                "structure group name") &&
         expect(iggy3d::productCreativePaletteGroupName(
                    iggy3d::ProductCreativePaletteGroup::Movement) ==
                    "movement",
                "movement group name") &&
         expect(iggy3d::productCreativePaletteGroupName(
                    iggy3d::ProductCreativePaletteGroup::Physics) == "physics",
                "physics group name") &&
         expect(iggy3d::productCreativePaletteGroupName(
                    iggy3d::ProductCreativePaletteGroup::Markers) == "markers",
                "markers group name") &&
         expect(iggy3d::productCreativePaletteGroupName(
                    iggy3d::ProductCreativePaletteGroup::TestLab) == "test_lab",
                "test lab group name") &&
         expect(iggy3d::productCreativePaletteGroupName(
                    iggy3d::ProductCreativePaletteGroup::Unknown) == "unknown",
                "unknown group name");
}

bool paletteBuildsCurrentCatalogInDeterministicOrder() {
  const iggy3d::ProductCreativePalette palette =
      iggy3d::buildProductCreativePalette();
  return expect(palette.ok, "palette ok") &&
         expect(palette.status == "creative_palette_ready", "palette status") &&
         expect(palette.slots.size() == 4U, "slot count") &&
         expect(palette.slots[0].assetId == "stone_block_proxy",
                "stone block first") &&
         expect(palette.slots[1].assetId == "stone_floor_slab",
                "floor second") &&
         expect(palette.slots[2].assetId == "stone_wall_panel",
                "wall third") &&
         expect(palette.slots[3].assetId == "wood_crate_proxy",
                "crate fourth");
}

bool enabledSlotsHaveScanFriendlyFacts() {
  const iggy3d::ProductCreativePalette palette =
      iggy3d::buildProductCreativePalette();
  bool ok = true;
  for (const iggy3d::ProductCreativePaletteSlot& slot : palette.slots) {
    ok = ok && expect(slot.enabled, "current built-in slot enabled");
    ok = ok && expect(!slot.assetId.empty(), "asset id nonempty");
    ok = ok && expect(!slot.displayName.empty(), "display name nonempty");
    ok = ok && expect(slot.group != iggy3d::ProductCreativePaletteGroup::Unknown,
                     "known group");
    ok = ok && expect(finitePositive(slot.defaultSizeMeters),
                      "positive default size");
    ok = ok && expect(slot.reasonCode == "creative_palette_slot_ready",
                      "ready reason");
  }
  return ok;
}

bool representativeSlotsMirrorObjectTraits() {
  const iggy3d::ObjectAssetCatalog catalog =
      iggy3d::makeBuiltInObjectAssetCatalog();
  const iggy3d::ProductCreativePalette palette =
      iggy3d::buildProductCreativePaletteFromCatalog(catalog);
  const iggy3d::ProductCreativePaletteSlot* crate =
      iggy3d::findProductCreativePaletteSlot(palette, "wood_crate_proxy");
  const iggy3d::ProductCreativePaletteSlot* wall =
      iggy3d::findProductCreativePaletteSlot(palette, "stone_wall_panel");
  const iggy3d::ObjectAssetDefinition* crateAsset =
      catalogAsset(catalog, "wood_crate_proxy");
  const iggy3d::ObjectAssetDefinition* wallAsset =
      catalogAsset(catalog, "stone_wall_panel");

  return expect(crate != nullptr, "crate slot") &&
         expect(wall != nullptr, "wall slot") &&
         expect(crateAsset != nullptr, "crate asset") &&
         expect(wallAsset != nullptr, "wall asset") &&
         expect(crate->group == iggy3d::ProductCreativePaletteGroup::Physics,
                "crate physics group") &&
         expect(wall->group == iggy3d::ProductCreativePaletteGroup::Structure,
                "wall structure group") &&
         expect(crate->placeable == crateAsset->editor.placeable,
                "crate placeable mirrors") &&
         expect(crate->rotatable == crateAsset->editor.rotatable,
                "crate rotatable mirrors") &&
         expect(crate->scalable == crateAsset->editor.scalable,
                "crate scalable mirrors") &&
         expect(crate->blocksActor == crateAsset->collision.blocksMovement,
                "crate actor collision mirrors") &&
         expect(crate->blocksVision == crateAsset->collision.blocksVision,
                "crate vision collision mirrors") &&
         expect(wall->blocksVision == wallAsset->collision.blocksVision,
                "wall vision collision mirrors") &&
         expect(crate->defaultSizeMeters.x == crateAsset->primitiveShape.sizeMeters.x,
                "crate size x mirrors") &&
         expect(crate->defaultSizeMeters.y == crateAsset->primitiveShape.sizeMeters.y,
                "crate size y mirrors") &&
         expect(crate->defaultSizeMeters.z == crateAsset->primitiveShape.sizeMeters.z,
                "crate size z mirrors");
}

bool countsMatchGroupedSlots() {
  const iggy3d::ProductCreativePalette palette =
      iggy3d::buildProductCreativePalette();
  std::uint64_t structure = 0;
  std::uint64_t movement = 0;
  std::uint64_t physics = 0;
  std::uint64_t markers = 0;
  std::uint64_t testLab = 0;
  std::uint64_t unknown = 0;
  std::uint64_t disabled = 0;
  for (const iggy3d::ProductCreativePaletteSlot& slot : palette.slots) {
    switch (slot.group) {
      case iggy3d::ProductCreativePaletteGroup::Structure:
        ++structure;
        break;
      case iggy3d::ProductCreativePaletteGroup::Movement:
        ++movement;
        break;
      case iggy3d::ProductCreativePaletteGroup::Physics:
        ++physics;
        break;
      case iggy3d::ProductCreativePaletteGroup::Markers:
        ++markers;
        break;
      case iggy3d::ProductCreativePaletteGroup::TestLab:
        ++testLab;
        break;
      case iggy3d::ProductCreativePaletteGroup::Unknown:
        ++unknown;
        break;
    }
    if (!slot.enabled) {
      ++disabled;
    }
  }
  return expect(palette.structureCount == structure, "structure count") &&
         expect(palette.movementCount == movement, "movement count") &&
         expect(palette.physicsCount == physics, "physics count") &&
         expect(palette.markerCount == markers, "marker count") &&
         expect(palette.testLabCount == testLab, "test lab count") &&
         expect(palette.unknownCount == unknown, "unknown count") &&
         expect(palette.disabledCount == disabled, "disabled count") &&
         expect(palette.structureCount == 3U, "current structure count") &&
         expect(palette.physicsCount == 1U, "current physics count") &&
         expect(palette.movementCount == 0U, "current movement gap") &&
         expect(palette.markerCount == 0U, "current marker gap") &&
         expect(palette.testLabCount == 0U, "current test lab gap");
}

bool findSlotAndDuplicatePolicyWork() {
  const iggy3d::ProductCreativePalette palette =
      iggy3d::buildProductCreativePalette();
  const iggy3d::ProductCreativePaletteSlot* crate =
      iggy3d::findProductCreativePaletteSlot(palette, "wood_crate_proxy");
  const iggy3d::ProductCreativePaletteSlot* missing =
      iggy3d::findProductCreativePaletteSlot(palette, "missing_asset");
  std::set<std::string> enabledIds;
  bool ok = expect(crate != nullptr, "crate found") &&
            expect(missing == nullptr, "missing not found");
  for (const iggy3d::ProductCreativePaletteSlot& slot : palette.slots) {
    if (slot.enabled) {
      ok = ok && expect(enabledIds.insert(slot.assetId).second,
                        "enabled asset id unique");
    }
  }
  return ok;
}

bool sourceCatalogIsNotMutated() {
  iggy3d::ObjectAssetCatalog catalog = iggy3d::makeBuiltInObjectAssetCatalog();
  const std::string originalFirstId = catalog.assets.front().id.value;
  const std::string originalLastId = catalog.assets.back().id.value;
  const std::size_t originalCount = catalog.assets.size();
  const iggy3d::ProductCreativePalette palette =
      iggy3d::buildProductCreativePaletteFromCatalog(catalog);
  return expect(palette.ok, "palette built from catalog") &&
         expect(catalog.assets.size() == originalCount, "catalog count unchanged") &&
         expect(catalog.assets.front().id.value == originalFirstId,
                "first id unchanged") &&
         expect(catalog.assets.back().id.value == originalLastId,
                "last id unchanged");
}

}  // namespace

int main() {
  const bool ok = groupNamesAreStable() &&
                  paletteBuildsCurrentCatalogInDeterministicOrder() &&
                  enabledSlotsHaveScanFriendlyFacts() &&
                  representativeSlotsMirrorObjectTraits() &&
                  countsMatchGroupedSlots() &&
                  findSlotAndDuplicatePolicyWork() &&
                  sourceCatalogIsNotMutated();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
