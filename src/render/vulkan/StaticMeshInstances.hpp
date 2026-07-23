#pragma once

#include <string>
#include <vector>

#include "content/assets/StaticMeshAsset.hpp"
#include "projection/scene/SceneItem.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
#include "render/vulkan/StaticMeshMaterialTextures.hpp"

namespace iggy3d::vulkan {

struct StaticMeshMaterialVariantDrawRanges {
  std::string name;
  std::vector<IndexedDrawRange> indexedDraws;
};

struct StaticMeshAssetDrawRanges {
  std::string assetId;
  Vec3 boundsMin{};
  Vec3 boundsMax{};
  std::vector<IndexedDrawRange> indexedDraws;
  std::vector<StaticMeshMaterialVariantDrawRanges> materialVariants;
};

struct StaticMeshAssetAtlasCpuGeometry {
  std::vector<StaticMeshInstanceVertex> vertices;
  std::vector<std::uint32_t> indices;
  std::vector<StaticMeshAssetDrawRanges> assetDraws;
  bool valid = false;
};

[[nodiscard]] StaticMeshAssetAtlasCpuGeometry
buildStaticMeshAssetAtlasCpuGeometry(
    StaticMeshAssetCache* staticMeshAssets,
    const StaticMeshMaterialTextureResources* materialTextures = nullptr);

[[nodiscard]] bool buildStaticMeshInstanceTransform(
    const SceneRoomMeshItem& item,
    Vec3 assetBoundsMin,
    Vec3 assetBoundsMax,
    StaticMeshInstanceTransform& output) noexcept;

}  // namespace iggy3d::vulkan
