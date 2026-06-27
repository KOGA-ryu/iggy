#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/object/ObjectTraits.hpp"

namespace iggy3d {

struct ProductRoomEditorObjectPaletteEntry {
  std::string assetId;
  std::string label;
  std::string group;
  std::string material;
  std::string shape;
  bool enabled = false;
  bool placeable = false;
  bool blocksMovement = false;
  bool blocksVision = false;
  bool dynamicPhysics = false;
  bool flammable = false;
};

struct ProductRoomEditorObjectPalette {
  bool ready = false;
  std::string status = "object_palette_not_requested";
  std::string reasonCode = "object_palette_not_requested";
  std::vector<ProductRoomEditorObjectPaletteEntry> entries;
  std::size_t selectedIndex = 0U;
  std::string selectedAssetId = "none";
  std::size_t enabledCount = 0U;
  std::size_t disabledCount = 0U;
};

struct ProductRoomEditorObjectPaletteRequest {
  const ObjectAssetCatalog* catalog = nullptr;
  std::string_view selectedAssetId;
};

ProductRoomEditorObjectPalette buildProductRoomEditorObjectPalette(
    const ProductRoomEditorObjectPaletteRequest& request);

ProductRoomEditorObjectPalette makeBuiltInProductRoomEditorObjectPalette(
    std::string_view selectedAssetId = {});

}  // namespace iggy3d
