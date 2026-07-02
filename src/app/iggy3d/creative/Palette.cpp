#include "app/iggy3d/creative/Palette.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace iggy3d {
namespace {

struct ProductCreativePaletteGroupDescriptor {
  ProductCreativePaletteGroup group = ProductCreativePaletteGroup::Unknown;
  std::string_view name = "unknown";
};

struct ProductCreativeAssetGroupDescriptor {
  std::string_view assetId;
  ProductCreativePaletteGroup group = ProductCreativePaletteGroup::Unknown;
};

struct ProductCreativeEditorGroupDescriptor {
  std::string_view editorGroup;
  ProductCreativePaletteGroup group = ProductCreativePaletteGroup::Unknown;
};

constexpr std::array kProductCreativePaletteGroupDescriptors{
    ProductCreativePaletteGroupDescriptor{ProductCreativePaletteGroup::Structure,
                                          "structure"},
    ProductCreativePaletteGroupDescriptor{ProductCreativePaletteGroup::Movement,
                                          "movement"},
    ProductCreativePaletteGroupDescriptor{ProductCreativePaletteGroup::Physics,
                                          "physics"},
    ProductCreativePaletteGroupDescriptor{ProductCreativePaletteGroup::Markers,
                                          "markers"},
    ProductCreativePaletteGroupDescriptor{ProductCreativePaletteGroup::TestLab,
                                          "test_lab"},
    ProductCreativePaletteGroupDescriptor{ProductCreativePaletteGroup::Unknown,
                                          "unknown"},
};

constexpr std::array kProductCreativeAssetGroupDescriptors{
    ProductCreativeAssetGroupDescriptor{"stone_block_proxy",
                                        ProductCreativePaletteGroup::Structure},
    ProductCreativeAssetGroupDescriptor{"stone_floor_slab",
                                        ProductCreativePaletteGroup::Structure},
    ProductCreativeAssetGroupDescriptor{"stone_wall_panel",
                                        ProductCreativePaletteGroup::Structure},
    ProductCreativeAssetGroupDescriptor{"wood_crate_proxy",
                                        ProductCreativePaletteGroup::Physics},
};

constexpr std::array kProductCreativeEditorGroupDescriptors{
    ProductCreativeEditorGroupDescriptor{"structure",
                                         ProductCreativePaletteGroup::Structure},
    ProductCreativeEditorGroupDescriptor{"movement",
                                         ProductCreativePaletteGroup::Movement},
    ProductCreativeEditorGroupDescriptor{"physics",
                                         ProductCreativePaletteGroup::Physics},
    ProductCreativeEditorGroupDescriptor{"markers",
                                         ProductCreativePaletteGroup::Markers},
    ProductCreativeEditorGroupDescriptor{"test_lab",
                                         ProductCreativePaletteGroup::TestLab},
    ProductCreativeEditorGroupDescriptor{"shapes",
                                         ProductCreativePaletteGroup::Structure},
};

ProductCreativePaletteGroup groupForEditorGroup(std::string_view editorGroup) {
  for (const ProductCreativeEditorGroupDescriptor& descriptor :
       kProductCreativeEditorGroupDescriptors) {
    // branch-gate: BG-1221
    if (descriptor.editorGroup == editorGroup) {
      return descriptor.group;
    }
  }
  return ProductCreativePaletteGroup::Unknown;
}

ProductCreativePaletteGroup groupForAsset(const ObjectAssetDefinition& asset) {
  for (const ProductCreativeAssetGroupDescriptor& descriptor :
       kProductCreativeAssetGroupDescriptors) {
    // branch-gate: BG-1221
    if (descriptor.assetId == asset.id.value) {
      return descriptor.group;
    }
  }
  return groupForEditorGroup(asset.editor.paletteGroup);
}

bool isKnownCreativePaletteGroup(ProductCreativePaletteGroup group) {
  return group != ProductCreativePaletteGroup::Unknown;
}

std::uint64_t& groupCount(ProductCreativePalette& palette,
                          ProductCreativePaletteGroup group) {
  // branch-gate: BG-1221
  switch (group) {
    case ProductCreativePaletteGroup::Structure:
      return palette.structureCount;
    case ProductCreativePaletteGroup::Movement:
      return palette.movementCount;
    case ProductCreativePaletteGroup::Physics:
      return palette.physicsCount;
    case ProductCreativePaletteGroup::Markers:
      return palette.markerCount;
    case ProductCreativePaletteGroup::TestLab:
      return palette.testLabCount;
    case ProductCreativePaletteGroup::Unknown:
      return palette.unknownCount;
  }
  return palette.unknownCount;
}

ProductCreativePaletteSlot slotFromAsset(const ObjectAssetDefinition& asset,
                                         std::uint32_t slotIndex) {
  ProductCreativePaletteSlot slot;
  slot.group = groupForAsset(asset);
  slot.slotIndex = slotIndex;
  slot.assetId = asset.id.value;
  slot.displayName = asset.editor.displayName;
  slot.defaultSizeMeters = asset.primitiveShape.sizeMeters;
  slot.placeable = asset.editor.placeable;
  slot.rotatable = asset.editor.rotatable;
  slot.scalable = asset.editor.scalable;
  slot.blocksActor = asset.collision.blocksMovement;
  slot.blocksVision = asset.collision.blocksVision;

  const ObjectValidationResult validation = validateObjectAssetDefinition(&asset);
  slot.enabled = validation.ok && slot.placeable &&
                 isKnownCreativePaletteGroup(slot.group);
  // branch-gate: BG-1221
  if (!validation.ok) {
    slot.reasonCode = std::string(validation.reasonCode);
  } else if (!slot.placeable) {  // branch-gate: BG-1221
    slot.reasonCode = "creative_palette_not_placeable";
  } else if (!isKnownCreativePaletteGroup(slot.group)) {  // branch-gate: BG-1221
    slot.reasonCode = "creative_palette_unknown_group";
  } else {
    slot.reasonCode = "creative_palette_slot_ready";
  }
  return slot;
}

}  // namespace

std::string_view productCreativePaletteGroupName(
    ProductCreativePaletteGroup group) {
  for (const ProductCreativePaletteGroupDescriptor& descriptor :
       kProductCreativePaletteGroupDescriptors) {
    // branch-gate: BG-1221
    if (descriptor.group == group) {
      return descriptor.name;
    }
  }
  return "unknown";
}

ProductCreativePalette buildProductCreativePaletteFromCatalog(
    const ObjectAssetCatalog& catalog) {
  ProductCreativePalette palette;
  palette.slots.reserve(catalog.assets.size());
  for (const ObjectAssetDefinition& asset : catalog.assets) {
    std::uint64_t& count = groupCount(palette, groupForAsset(asset));
    const auto slotIndex = static_cast<std::uint32_t>(count);
    ProductCreativePaletteSlot slot = slotFromAsset(asset, slotIndex);
    ++count;
    // branch-gate: BG-1221
    if (!slot.enabled) {
      ++palette.disabledCount;
    }
    palette.slots.push_back(std::move(slot));
  }
  palette.ok = true;
  palette.status = "creative_palette_ready";
  palette.reasonCode = "creative_palette_ready";
  return palette;
}

ProductCreativePalette buildProductCreativePalette() {
  return buildProductCreativePaletteFromCatalog(makeBuiltInObjectAssetCatalog());
}

const ProductCreativePaletteSlot* findProductCreativePaletteSlot(
    const ProductCreativePalette& palette,
    std::string_view assetId) {
  for (const ProductCreativePaletteSlot& slot : palette.slots) {
    // branch-gate: BG-1221
    if (slot.assetId == assetId) {
      return &slot;
    }
  }
  return nullptr;
}

}  // namespace iggy3d
