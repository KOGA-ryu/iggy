#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::vulkan {

using room_mesh_detail::appendCreativeGeneratedPreviewShape;
using room_mesh_detail::appendCreativePathWireframe;
using room_mesh_detail::appendCreativeTargetWireframe;
using room_mesh_detail::colorForRoomRole;
using room_mesh_detail::componentProduct;
using room_mesh_detail::finiteVec3;

namespace {

bool appendStaticMeshPreviewRole(
    std::vector<FirstRoomVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    std::vector<IndexedDrawRange>& componentDraws,
    const StaticMeshAsset& asset,
    RenderCreativePreviewRole role,
    Vec3 color,
    IndexedDrawRange& range) {
  constexpr std::size_t kTargetWireframeVertexCount = 12U * 8U;
  const Vec3 size = asset.boundsMax - asset.boundsMin;
  const bool target = role != RenderCreativePreviewRole::Held;
  const std::size_t extraVertices =
      asset.vertices.size() + (target ? kTargetWireframeVertexCount : 0U);
  if (!asset.hasBounds || !finiteVec3(size) || size.x <= 0.0F ||
      size.y <= 0.0F || size.z <= 0.0F || asset.vertices.empty() ||
      vertices.size() + extraVertices >
          static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
    return false;
  }
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    if (primitive.firstIndex > asset.indices.size() ||
        primitive.indexCount > asset.indices.size() - primitive.firstIndex) {
      return false;
    }
    for (std::uint32_t offset = 0; offset < primitive.indexCount; ++offset) {
      if (asset.indices[primitive.firstIndex + offset] >=
          asset.vertices.size()) {
        return false;
      }
    }
  }

  const float inset = target ? 0.96F : 1.0F;
  const Vec3 center = (asset.boundsMin + asset.boundsMax) * 0.5F;
  const Vec3 scale{inset / size.x, inset / size.y, inset / size.z};
  const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
  for (const StaticMeshVertex& source : asset.vertices) {
    const Vec3 position = componentProduct(source.position - center, scale);
    if (!finiteVec3(position)) {
      return false;
    }
    vertices.push_back({{position.x, position.y, position.z},
                        {color.x, color.y, color.z}});
  }
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    for (std::uint32_t offset = 0; offset < primitive.indexCount; ++offset) {
      indices.push_back(base + asset.indices[primitive.firstIndex + offset]);
    }
  }
  if (target) {
    appendCreativeTargetWireframe(vertices, indices, componentDraws, color);
  }
  range.indexCount =
      static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  return range.indexCount > 0U;
}

}  // namespace

CreativePreviewCpuGeometry buildCreativePreviewCpuGeometry(
    StaticMeshAssetCache* staticMeshAssets) {
  CreativePreviewCpuGeometry result;
  result.indexedDraws.resize(kCreativePreviewGeometryDrawRangeCount);
  constexpr std::array<std::string_view, kRenderCreativePreviewRoleCount>
      roles{
          "editor_ghost_select",
          "editor_ghost_valid",
          "editor_ghost_invalid",
          "editor_ghost_route",
      };
  for (std::size_t profileSlot = 0;
       profileSlot < kCreativePreviewGeometryProfileSlotCount; ++profileSlot) {
    RenderCreativePreviewGeometryProfile profile =
        RenderCreativePreviewGeometryProfile::Box;
    std::uint16_t proceduralSegmentCount = 0U;
    if (profileSlot == 1U) {
      profile = RenderCreativePreviewGeometryProfile::RampWedge;
    } else if (profileSlot == 2U) {
      profile = RenderCreativePreviewGeometryProfile::OpenFrame;
    } else if (profileSlot >= 3U) {
      profile = RenderCreativePreviewGeometryProfile::StairSteps;
      proceduralSegmentCount =
          static_cast<std::uint16_t>(profileSlot - 2U);
    }

    for (std::size_t roleIndex = 0; roleIndex < roles.size(); ++roleIndex) {
      const RenderCreativePreviewRole role =
          static_cast<RenderCreativePreviewRole>(roleIndex);
      const std::uint32_t firstIndex =
          static_cast<std::uint32_t>(result.indices.size());
      std::vector<IndexedDrawRange> componentDraws;
      const Vec3 color = colorForRoomRole(std::string(roles[roleIndex]));
      const Vec3 solidSize =
          role == RenderCreativePreviewRole::Held
              ? Vec3{1.0F, 1.0F, 1.0F}
              : Vec3{0.96F, 0.96F, 0.96F};
      if (!appendCreativeGeneratedPreviewShape(
              result.vertices, result.indices, componentDraws, profile,
              proceduralSegmentCount, solidSize, color)) {
        return result;
      }
      if (role == RenderCreativePreviewRole::Held) {
        result.indexedDraws[creativePreviewGeometryDrawIndex(
            role, false, profile, proceduralSegmentCount)] = {
            firstIndex,
            static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
        continue;
      }

      appendCreativeTargetWireframe(result.vertices, result.indices,
                                    componentDraws, color);
      result.indexedDraws[creativePreviewGeometryDrawIndex(
          role, false, profile, proceduralSegmentCount)] = {
          firstIndex,
          static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
      appendCreativePathWireframe(result.vertices, result.indices,
                                  componentDraws, color);
      result.indexedDraws[creativePreviewGeometryDrawIndex(
          role, true, profile, proceduralSegmentCount)] = {
          firstIndex,
          static_cast<std::uint32_t>(result.indices.size()) - firstIndex};
    }
  }

  if (staticMeshAssets != nullptr) {
    const StaticMeshAssetCatalog catalog =
        discoverStaticMeshAssetCatalog(staticMeshAssets->root());
    result.assetDraws.reserve(catalog.entries.size());
    for (const StaticMeshAssetCatalogEntry& entry : catalog.entries) {
      const StaticMeshAsset* asset = staticMeshAssets->find(entry.assetId);
      if (asset == nullptr) {
        continue;
      }
      const std::size_t vertexStart = result.vertices.size();
      const std::size_t indexStart = result.indices.size();
      const std::size_t drawStart = result.indexedDraws.size();
      CreativePreviewGeometryResources::AssetDrawRanges assetDraw;
      assetDraw.assetId = entry.assetId;
      bool complete = true;
      for (std::size_t roleIndex = 0;
           roleIndex < kRenderCreativePreviewRoleCount; ++roleIndex) {
        const RenderCreativePreviewRole role =
            static_cast<RenderCreativePreviewRole>(roleIndex);
        const std::string_view roleName = roles[roleIndex];
        IndexedDrawRange draw;
        std::vector<IndexedDrawRange> componentDraws;
        if (!appendStaticMeshPreviewRole(
                result.vertices, result.indices, componentDraws, *asset, role,
                colorForRoomRole(std::string(roleName)), draw)) {
          complete = false;
          break;
        }
        assetDraw.geometryDrawIndices[roleIndex] =
            static_cast<std::uint32_t>(result.indexedDraws.size());
        result.indexedDraws.push_back(draw);
      }
      if (!complete) {
        result.vertices.resize(vertexStart);
        result.indices.resize(indexStart);
        result.indexedDraws.resize(drawStart);
        continue;
      }
      result.assetDraws.push_back(std::move(assetDraw));
    }
  }
  result.ready = !result.vertices.empty() && !result.indices.empty();
  return result;
}

CreativePreviewCpuGeometry buildCreativePreviewCpuGeometry() {
  return buildCreativePreviewCpuGeometry(nullptr);
}

std::uint32_t resolveCreativePreviewGeometryDrawIndex(
    const CreativePreviewGeometryResources& geometry,
    const RenderCreativePreviewItem& item) noexcept {
  const std::string_view assetId = renderCreativePreviewAssetId(item);
  if (!assetId.empty()) {
    const auto found = std::find_if(
        geometry.assetDraws.begin(), geometry.assetDraws.end(),
        [assetId](const CreativePreviewGeometryResources::AssetDrawRanges& draw) {
          return draw.assetId == assetId;
        });
    const std::size_t roleIndex = static_cast<std::size_t>(item.role);
    if (found != geometry.assetDraws.end() &&
        roleIndex < found->geometryDrawIndices.size()) {
      return found->geometryDrawIndices[roleIndex];
    }
  }
  return creativePreviewGeometryDrawIndex(item.role,
                                          item.includePathWireframe,
                                          item.geometryProfile,
                                          item.proceduralSegmentCount);
}

}  // namespace iggy3d::vulkan
