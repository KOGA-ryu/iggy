#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include "core/math/EulerRotation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace iggy3d::vulkan::room_mesh_detail {

[[nodiscard]] Vec3 componentProduct(Vec3 lhs, Vec3 rhs) noexcept {
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

namespace {

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

[[nodiscard]] Vec3 importedTriangleColor(
    const StaticMeshAsset& asset,
    std::uint32_t materialIndex,
    Vec3 worldNormal) noexcept {
  const Vec3 base = importedMaterialBaseColor(asset, materialIndex);
  const Vec3 light = normalized(Vec3{-0.35F, 0.85F, 0.40F});
  const float brightness =
      0.52F + 0.48F * std::max(0.0F, dot(worldNormal, light));
  return base * brightness;
}

}  // namespace

bool appendStaticMeshAsset(std::vector<FirstRoomVertex>& vertices,
                           std::vector<std::uint16_t>& indices,
                           std::vector<IndexedDrawRange>& draws,
                           const SceneRoomMeshItem& item,
                           const StaticMeshAsset& asset,
                           const StaticMeshMaterialTextureResources*
                               materialTextures) {
  const Vec3 inputSize = asset.boundsMax - asset.boundsMin;
  if (!asset.hasBounds || !finiteVec3(inputSize) ||
      inputSize.x <= 0.0F || inputSize.y <= 0.0F || inputSize.z <= 0.0F ||
      !finiteVec3(item.position) || !finiteVec3(item.size) ||
      item.size.x <= 0.0F || item.size.y <= 0.0F || item.size.z <= 0.0F) {
    return false;
  }
  const Vec3 inputCenter = (asset.boundsMin + asset.boundsMax) * 0.5F;
  const Vec3 scale{item.size.x / inputSize.x, item.size.y / inputSize.y,
                   item.size.z / inputSize.z};
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    if (primitive.indexCount == 0U || primitive.indexCount % 3U != 0U ||
        primitive.firstIndex > asset.indices.size() ||
        primitive.indexCount > asset.indices.size() - primitive.firstIndex ||
        vertices.size() + primitive.indexCount >
            static_cast<std::size_t>(
                std::numeric_limits<std::uint16_t>::max())) {
      return false;
    }
    IndexedDrawRange draw;
    draw.firstIndex = static_cast<std::uint32_t>(indices.size());
    const StaticMeshMaterial* material =
        primitive.materialIndex < asset.materials.size()
            ? &asset.materials[primitive.materialIndex]
            : nullptr;
    if (primitive.hasTexcoord0 && material != nullptr &&
        material->baseColorImageIndex != kInvalidStaticMeshImageIndex &&
        materialTextures != nullptr) {
      draw.materialTextureIndex = findStaticMeshMaterialTextureIndex(
          materialTextures->materialBindings, asset.id,
          primitive.materialIndex);
    }
    for (std::uint32_t triangle = 0U; triangle < primitive.indexCount;
         triangle += 3U) {
      Vec3 world[3]{};
      float uv[3][2]{};
      for (std::uint32_t corner = 0U; corner < 3U; ++corner) {
        const std::uint32_t sourceIndex =
            asset.indices[primitive.firstIndex + triangle + corner];
        if (sourceIndex >= asset.vertices.size()) {
          return false;
        }
        const Vec3 local = componentProduct(
            asset.vertices[sourceIndex].position - inputCenter, scale);
        world[corner] = item.position +
                        rotateEulerXyz(local, item.rotationEulerRadians);
        if (!finiteVec3(world[corner])) {
          return false;
        }
        if (draw.materialTextureIndex != kInvalidMaterialTextureIndex) {
          const StaticMeshVertex& sourceVertex = asset.vertices[sourceIndex];
          const float scaledU =
              sourceVertex.uv[0] * material->baseColorUvScale[0];
          const float scaledV =
              sourceVertex.uv[1] * material->baseColorUvScale[1];
          const float cosine =
              std::cos(material->baseColorUvRotationRadians);
          const float sine = std::sin(material->baseColorUvRotationRadians);
          uv[corner][0] = material->baseColorUvOffset[0] +
                          cosine * scaledU - sine * scaledV;
          uv[corner][1] = material->baseColorUvOffset[1] +
                          sine * scaledU + cosine * scaledV;
          if (!std::isfinite(uv[corner][0]) ||
              !std::isfinite(uv[corner][1])) {
            return false;
          }
        }
      }
      const Vec3 faceNormal =
          normalized(cross(world[1] - world[0], world[2] - world[0]));
      const Vec3 color =
          importedTriangleColor(asset, primitive.materialIndex, faceNormal);
      for (std::size_t corner = 0U; corner < 3U; ++corner) {
        const Vec3 position = world[corner];
        const std::uint16_t index =
            static_cast<std::uint16_t>(vertices.size());
        vertices.push_back({{position.x, position.y, position.z},
                            {color.x, color.y, color.z},
                            {uv[corner][0], uv[corner][1]}});
        indices.push_back(index);
      }
    }
    draw.indexCount =
        static_cast<std::uint32_t>(indices.size()) - draw.firstIndex;
    draws.push_back(draw);
  }
  return true;
}

}  // namespace iggy3d::vulkan::room_mesh_detail
