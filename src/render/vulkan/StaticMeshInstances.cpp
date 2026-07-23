#include "render/vulkan/StaticMeshInstances.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "core/math/EulerRotation.hpp"

namespace iggy3d::vulkan {
namespace {

[[nodiscard]] Vec3 componentProduct(Vec3 lhs, Vec3 rhs) noexcept {
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[nodiscard]] Vec3 importedMaterialBaseColor(
    const StaticMeshAsset& asset,
    std::uint32_t materialIndex) noexcept {
  Vec3 base{0.48F, 0.44F, 0.38F};
  if (materialIndex >= asset.materials.size()) {
    return base;
  }
  const StaticMeshMaterial& material = asset.materials[materialIndex];
  if (std::isfinite(material.baseColorFactor[0]) &&
      std::isfinite(material.baseColorFactor[1]) &&
      std::isfinite(material.baseColorFactor[2])) {
    base = {std::clamp(material.baseColorFactor[0], 0.0F, 1.0F),
            std::clamp(material.baseColorFactor[1], 0.0F, 1.0F),
            std::clamp(material.baseColorFactor[2], 0.0F, 1.0F)};
  }
  return base;
}

[[nodiscard]] bool variantUsesDefaultMaterials(
    const StaticMeshAsset& asset,
    std::size_t variantIndex) noexcept {
  return std::all_of(
      asset.primitives.begin(), asset.primitives.end(),
      [variantIndex](const StaticMeshPrimitive& primitive) {
        return resolveStaticMeshPrimitiveMaterialIndex(primitive, variantIndex) ==
               primitive.materialIndex;
      });
}

[[nodiscard]] bool appendAssetDrawFamily(
    const StaticMeshAsset& asset,
    std::optional<std::size_t> variantIndex,
    const StaticMeshMaterialTextureResources* materialTextures,
    StaticMeshAssetAtlasCpuGeometry& geometry,
    std::vector<IndexedDrawRange>& draws) {
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    if (primitive.indexCount == 0U || primitive.indexCount % 3U != 0U ||
        primitive.firstIndex > asset.indices.size() ||
        primitive.indexCount > asset.indices.size() - primitive.firstIndex ||
        geometry.indices.size() >
            std::numeric_limits<std::uint32_t>::max() - primitive.indexCount ||
        geometry.vertices.size() >
            std::numeric_limits<std::uint32_t>::max() - primitive.indexCount) {
      return false;
    }

    const std::uint32_t materialIndex =
        resolveStaticMeshPrimitiveMaterialIndex(primitive, variantIndex);
    IndexedDrawRange draw;
    draw.firstIndex = static_cast<std::uint32_t>(geometry.indices.size());
    const StaticMeshMaterial* material =
        materialIndex < asset.materials.size()
            ? &asset.materials[materialIndex]
            : nullptr;
    if (primitive.hasTexcoord0 && material != nullptr &&
        material->baseColorImageIndex != kInvalidStaticMeshImageIndex &&
        materialTextures != nullptr) {
      draw.materialTextureIndex = findStaticMeshMaterialTextureIndex(
          materialTextures->materialBindings, asset.id, materialIndex);
    }
    const Vec3 baseColor = importedMaterialBaseColor(asset, materialIndex);
    for (std::uint32_t triangle = 0U; triangle < primitive.indexCount;
         triangle += 3U) {
      std::uint32_t sourceIndices[3]{};
      Vec3 positions[3]{};
      for (std::uint32_t corner = 0U; corner < 3U; ++corner) {
        sourceIndices[corner] =
            asset.indices[primitive.firstIndex + triangle + corner];
        if (sourceIndices[corner] >= asset.vertices.size()) {
          return false;
        }
        positions[corner] = asset.vertices[sourceIndices[corner]].position;
      }
      const Vec3 faceNormal = normalized(
          cross(positions[1] - positions[0], positions[2] - positions[0]));
      if (!isFinite(faceNormal)) {
        return false;
      }
      for (std::uint32_t corner = 0U; corner < 3U; ++corner) {
        float u = 0.0F;
        float v = 0.0F;
        if (draw.materialTextureIndex != kInvalidMaterialTextureIndex) {
          const StaticMeshVertex& source =
              asset.vertices[sourceIndices[corner]];
          const float scaledU = source.uv[0] * material->baseColorUvScale[0];
          const float scaledV = source.uv[1] * material->baseColorUvScale[1];
          const float cosine =
              std::cos(material->baseColorUvRotationRadians);
          const float sine = std::sin(material->baseColorUvRotationRadians);
          u = material->baseColorUvOffset[0] + cosine * scaledU - sine * scaledV;
          v = material->baseColorUvOffset[1] + sine * scaledU + cosine * scaledV;
          if (!std::isfinite(u) || !std::isfinite(v)) {
            return false;
          }
        }
        const Vec3 position = positions[corner];
        const std::uint32_t vertexIndex =
            static_cast<std::uint32_t>(geometry.vertices.size());
        geometry.vertices.push_back(
            {{position.x, position.y, position.z},
             {u, v},
             {baseColor.x, baseColor.y, baseColor.z},
             {faceNormal.x, faceNormal.y, faceNormal.z}});
        geometry.indices.push_back(vertexIndex);
      }
    }
    draw.indexCount =
        static_cast<std::uint32_t>(geometry.indices.size()) - draw.firstIndex;
    draws.push_back(draw);
  }
  return true;
}

}  // namespace

bool buildStaticMeshInstanceTransform(
    const SceneRoomMeshItem& item,
    Vec3 assetBoundsMin,
    Vec3 assetBoundsMax,
    StaticMeshInstanceTransform& output) noexcept {
  output = {};
  const Vec3 assetSize = assetBoundsMax - assetBoundsMin;
  if (!isFinite(assetBoundsMin) || !isFinite(assetBoundsMax) ||
      !isFinite(assetSize) || assetSize.x <= 0.0F ||
      assetSize.y <= 0.0F || assetSize.z <= 0.0F ||
      !isFinite(item.position) || !isFinite(item.size) ||
      !isFinite(item.rotationEulerRadians) || item.size.x <= 0.0F ||
      item.size.y <= 0.0F || item.size.z <= 0.0F) {
    return false;
  }

  const Vec3 scale{item.size.x / assetSize.x, item.size.y / assetSize.y,
                   item.size.z / assetSize.z};
  if (!isFinite(scale) || scale.x <= 0.0F || scale.y <= 0.0F ||
      scale.z <= 0.0F) {
    return false;
  }
  const Vec3 center = (assetBoundsMin + assetBoundsMax) * 0.5F;
  const Vec3 modelX = rotateEulerXyz({scale.x, 0.0F, 0.0F},
                                     item.rotationEulerRadians);
  const Vec3 modelY = rotateEulerXyz({0.0F, scale.y, 0.0F},
                                     item.rotationEulerRadians);
  const Vec3 modelZ = rotateEulerXyz({0.0F, 0.0F, scale.z},
                                     item.rotationEulerRadians);
  const Vec3 translation =
      item.position -
      rotateEulerXyz(componentProduct(center, scale),
                     item.rotationEulerRadians);
  const Vec3 normalX = rotateEulerXyz({1.0F / scale.x, 0.0F, 0.0F},
                                      item.rotationEulerRadians);
  const Vec3 normalY = rotateEulerXyz({0.0F, 1.0F / scale.y, 0.0F},
                                      item.rotationEulerRadians);
  const Vec3 normalZ = rotateEulerXyz({0.0F, 0.0F, 1.0F / scale.z},
                                      item.rotationEulerRadians);
  if (!isFinite(modelX) || !isFinite(modelY) || !isFinite(modelZ) ||
      !isFinite(translation) || !isFinite(normalX) || !isFinite(normalY) ||
      !isFinite(normalZ)) {
    return false;
  }

  output.modelColumn0 = {modelX.x, modelX.y, modelX.z, 0.0F};
  output.modelColumn1 = {modelY.x, modelY.y, modelY.z, 0.0F};
  output.modelColumn2 = {modelZ.x, modelZ.y, modelZ.z, 0.0F};
  output.modelColumn3 = {translation.x, translation.y, translation.z, 1.0F};
  output.normalColumn0 = {normalX.x, normalX.y, normalX.z, 0.0F};
  output.normalColumn1 = {normalY.x, normalY.y, normalY.z, 0.0F};
  output.normalColumn2 = {normalZ.x, normalZ.y, normalZ.z, 0.0F};
  return true;
}

StaticMeshAssetAtlasCpuGeometry buildStaticMeshAssetAtlasCpuGeometry(
    StaticMeshAssetCache* staticMeshAssets,
    const StaticMeshMaterialTextureResources* materialTextures) {
  StaticMeshAssetAtlasCpuGeometry result;
  if (staticMeshAssets == nullptr) {
    return result;
  }

  const StaticMeshAssetCatalog catalog =
      discoverStaticMeshAssetCatalog(staticMeshAssets->root());
  for (const StaticMeshAssetCatalogEntry& entry : catalog.entries) {
    const StaticMeshAsset* asset = staticMeshAssets->find(entry.assetId);
    if (asset == nullptr || !asset->hasBounds ||
        asset->primitives.empty()) {
      continue;
    }
    StaticMeshAssetDrawRanges assetDraw;
    assetDraw.assetId = asset->id;
    assetDraw.boundsMin = asset->boundsMin;
    assetDraw.boundsMax = asset->boundsMax;
    if (!appendAssetDrawFamily(*asset, std::nullopt, materialTextures, result,
                               assetDraw.indexedDraws)) {
      return {};
    }
    assetDraw.materialVariants.reserve(asset->materialVariants.size());
    for (std::size_t variantIndex = 0U;
         variantIndex < asset->materialVariants.size(); ++variantIndex) {
      StaticMeshMaterialVariantDrawRanges variantDraw;
      variantDraw.name = asset->materialVariants[variantIndex].name;
      if (variantUsesDefaultMaterials(*asset, variantIndex)) {
        variantDraw.indexedDraws = assetDraw.indexedDraws;
      } else if (!appendAssetDrawFamily(*asset, variantIndex, materialTextures,
                                        result, variantDraw.indexedDraws)) {
        return {};
      }
      assetDraw.materialVariants.push_back(std::move(variantDraw));
    }
    if (!assetDraw.indexedDraws.empty()) {
      result.assetDraws.push_back(std::move(assetDraw));
    }
  }
  result.valid = true;
  return result;
}

}  // namespace iggy3d::vulkan
