#include "app/iggy3d/creative/tools/Palette.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

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
  return expect(cr::groupName(
                    cr::Group::Structure) ==
                    "structure",
                "structure group name") &&
         expect(cr::groupName(
                    cr::Group::Movement) ==
                    "movement",
                "movement group name") &&
         expect(cr::groupName(
                    cr::Group::Physics) == "physics",
                "physics group name") &&
         expect(cr::groupName(
                    cr::Group::Markers) == "markers",
                "markers group name") &&
         expect(cr::groupName(
                    cr::Group::TestLab) == "test_lab",
                "test lab group name") &&
         expect(cr::groupName(
                    cr::Group::Unknown) == "unknown",
                "unknown group name");
}

bool paletteBuildsCurrentCatalogInDeterministicOrder() {
  const cr::PalView palette =
      cr::buildPalView();
  return expect(palette.ok, "palette ok") &&
         expect(palette.status == "palette_ready", "palette status") &&
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
  const cr::PalView palette =
      cr::buildPalView();
  bool ok = true;
  for (const cr::Slot& slot : palette.slots) {
    ok = ok && expect(slot.enabled, "current built-in slot enabled");
    ok = ok && expect(!slot.assetId.empty(), "asset id nonempty");
    ok = ok && expect(!slot.displayName.empty(), "display name nonempty");
    ok = ok && expect(slot.group != cr::Group::Unknown,
                     "known group");
    ok = ok && expect(finitePositive(slot.defaultSizeMeters),
                      "positive default size");
    ok = ok && expect(slot.reasonCode == "slot_ready",
                      "ready reason");
  }
  return ok;
}

bool representativeSlotsMirrorObjectTraits() {
  const iggy3d::ObjectAssetCatalog catalog =
      iggy3d::makeBuiltInObjectAssetCatalog();
  const cr::PalView palette =
      cr::buildPalViewFromCatalog(catalog);
  const cr::Slot* crate =
      cr::findSlot(palette, "wood_crate_proxy");
  const cr::Slot* wall =
      cr::findSlot(palette, "stone_wall_panel");
  const iggy3d::ObjectAssetDefinition* crateAsset =
      catalogAsset(catalog, "wood_crate_proxy");
  const iggy3d::ObjectAssetDefinition* wallAsset =
      catalogAsset(catalog, "stone_wall_panel");

  return expect(crate != nullptr, "crate slot") &&
         expect(wall != nullptr, "wall slot") &&
         expect(crateAsset != nullptr, "crate asset") &&
         expect(wallAsset != nullptr, "wall asset") &&
         expect(crate->group == cr::Group::Physics,
                "crate physics group") &&
         expect(wall->group == cr::Group::Structure,
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
  const cr::PalView palette =
      cr::buildPalView();
  std::uint64_t structure = 0;
  std::uint64_t movement = 0;
  std::uint64_t physics = 0;
  std::uint64_t markers = 0;
  std::uint64_t testLab = 0;
  std::uint64_t unknown = 0;
  std::uint64_t disabled = 0;
  for (const cr::Slot& slot : palette.slots) {
    switch (slot.group) {
      case cr::Group::Structure:
        ++structure;
        break;
      case cr::Group::Movement:
        ++movement;
        break;
      case cr::Group::Physics:
        ++physics;
        break;
      case cr::Group::Markers:
        ++markers;
        break;
      case cr::Group::TestLab:
        ++testLab;
        break;
      case cr::Group::Unknown:
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
  const cr::PalView palette =
      cr::buildPalView();
  const cr::Slot* crate =
      cr::findSlot(palette, "wood_crate_proxy");
  const cr::Slot* missing =
      cr::findSlot(palette, "missing_asset");
  std::set<std::string> enabledIds;
  bool ok = expect(crate != nullptr, "crate found") &&
            expect(missing == nullptr, "missing not found");
  for (const cr::Slot& slot : palette.slots) {
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
  const cr::PalView palette =
      cr::buildPalViewFromCatalog(catalog);
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
