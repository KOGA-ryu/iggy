#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include "render/mesh/BeanMesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <span>
#include <vector>

namespace iggy3d::vulkan {

using room_mesh_detail::appendBoxIfFits;
using room_mesh_detail::appendCreativeWireframeDebugGeometry;
using room_mesh_detail::appendFloorPlaneIfFits;
using room_mesh_detail::appendOpenFrameIfFits;
using room_mesh_detail::appendRampWedgeIfFits;
using room_mesh_detail::appendHipRoofPanelIfFits;
using room_mesh_detail::appendStaticMeshAsset;
using room_mesh_detail::appendStairStepsIfFits;
using room_mesh_detail::appendSurfacePatches;
using room_mesh_detail::buildOptimizedFloorDraws;
using room_mesh_detail::buildOptimizedWallDraws;
using room_mesh_detail::canEmitFloorDraw;
using room_mesh_detail::canAppendSurfacePatches;
using room_mesh_detail::colorForRoomRole;
using room_mesh_detail::colorForRoomMaterial;
using room_mesh_detail::externalStaticMeshId;
using room_mesh_detail::finiteVec3;
using room_mesh_detail::FloorDraw;
using room_mesh_detail::hasRotation;
using room_mesh_detail::roomGeometrySignature;
using room_mesh_detail::wallBoxForMesh;
using room_mesh_detail::WallBoxDraw;

namespace {

bool appendFloorGrid(std::vector<FirstRoomVertex>& vertices,
                     std::vector<std::uint16_t>& indices,
                     std::vector<IndexedDrawRange>& draws,
                     Vec3 center,
                     Vec3 size,
                     std::size_t& lineCount) {
  constexpr float kGridThickness = 0.035F;
  constexpr float kGridHeight = 0.012F;
  constexpr float kGridLift = 0.008F;
  const float halfX = std::max(size.x * 0.5F, 0.001F);
  const float halfZ = std::max(size.z * 0.5F, 0.001F);
  const float y = center.y + std::max(size.y * 0.5F, 0.001F) + kGridLift;
  const Vec3 color = colorForRoomRole("grid");
  const Vec3 xLineSize{halfX * 2.0F + kGridThickness, kGridHeight, kGridThickness};
  const Vec3 zLineSize{kGridThickness, kGridHeight, halfZ * 2.0F + kGridThickness};
  const Vec3 lineCenters[4] = {
      {center.x, y, center.z - halfZ},
      {center.x, y, center.z + halfZ},
      {center.x - halfX, y, center.z},
      {center.x + halfX, y, center.z},
  };
  const Vec3 lineSizes[4] = {xLineSize, xLineSize, zLineSize, zLineSize};
  for (std::size_t i = 0; i < 4U; ++i) {
    if (!appendBoxIfFits(vertices, indices, draws, lineCenters[i], lineSizes[i], color)) {
      return false;
    }
    ++lineCount;
  }
  return true;
}

bool appendWallGrid(std::vector<FirstRoomVertex>& vertices,
                    std::vector<std::uint16_t>& indices,
                    std::vector<IndexedDrawRange>& draws,
                    Vec3 center,
                    Vec3 size,
                    std::size_t& lineCount) {
  constexpr float kGridThickness = 0.035F;
  constexpr float kGridLift = 0.010F;
  const float halfX = std::max(size.x * 0.5F, 0.001F);
  const float halfY = std::max(size.y * 0.5F, 0.001F);
  const float halfZ = std::max(size.z * 0.5F, 0.001F);
  const Vec3 color = colorForRoomRole("grid");
  const float topY = center.y + halfY + kGridLift;
  const Vec3 topXLineSize{halfX * 2.0F + kGridThickness, kGridThickness,
                          kGridThickness};
  const Vec3 topZLineSize{kGridThickness, kGridThickness,
                          halfZ * 2.0F + kGridThickness};
  const Vec3 verticalSize{kGridThickness, halfY * 2.0F + kGridThickness,
                          kGridThickness};
  const Vec3 lineCenters[8] = {
      {center.x, topY, center.z - halfZ},
      {center.x, topY, center.z + halfZ},
      {center.x - halfX, topY, center.z},
      {center.x + halfX, topY, center.z},
      {center.x - halfX, center.y, center.z - halfZ},
      {center.x + halfX, center.y, center.z - halfZ},
      {center.x - halfX, center.y, center.z + halfZ},
      {center.x + halfX, center.y, center.z + halfZ},
  };
  const Vec3 lineSizes[8] = {topXLineSize, topXLineSize, topZLineSize, topZLineSize,
                             verticalSize, verticalSize, verticalSize, verticalSize};
  for (std::size_t i = 0; i < 8U; ++i) {
    if (!appendBoxIfFits(vertices, indices, draws, lineCenters[i], lineSizes[i], color)) {
      return false;
    }
    ++lineCount;
  }
  return true;
}

bool appendBean(std::vector<FirstRoomVertex>& vertices,
                std::vector<std::uint16_t>& indices,
                std::vector<IndexedDrawRange>& draws,
                Vec3 center,
                Vec3 size,
                Vec3 color,
                SceneModelKind kind) {
  const BeanMesh bean = buildBeanMesh(kind);
  if (bean.vertices.empty() || bean.indices.empty() ||
      vertices.size() + bean.vertices.size() >
          static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }
  const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
  const float sx = std::max(size.x, 0.001F);
  const float sy = std::max(size.y, 0.001F);
  const float sz = std::max(size.z, 0.001F);
  for (const BeanMeshVertex& vertex : bean.vertices) {
    const Vec3 world{
        center.x + vertex.position.x * sx,
        center.y + vertex.position.y * sy,
        center.z + vertex.position.z * sz,
    };
    vertices.push_back({{world.x, world.y, world.z}, {color.x, color.y, color.z}});
  }

  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  for (const std::uint16_t index : bean.indices) {
    indices.push_back(static_cast<std::uint16_t>(base + index));
  }
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
  return true;
}



// A thin, double-sided horizontal triangle from `start` to `end` (a debug
// "gaze blade" showing an NPC's vision direction and range). Base half-width
// is `halfWidth`. Returns false on a degenerate segment or vertex overflow.
bool appendGazeBlade(std::vector<FirstRoomVertex>& vertices,
                     std::vector<std::uint16_t>& indices,
                     std::vector<IndexedDrawRange>& draws,
                     Vec3 start,
                     Vec3 end,
                     float halfWidth,
                     Vec3 color) {
  const float dx = end.x - start.x;
  const float dz = end.z - start.z;
  const float lengthSq = dx * dx + dz * dz;
  if (!std::isfinite(lengthSq) || lengthSq < 1.0e-6F ||
      vertices.size() + 3 >
          static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }
  const float width = std::max(halfWidth, 0.001F);
  const float inv = 1.0F / std::sqrt(lengthSq);
  const float perpX = -dz * inv * width;
  const float perpZ = dx * inv * width;
  const std::uint16_t base = static_cast<std::uint16_t>(vertices.size());
  vertices.push_back({{start.x, start.y, start.z}, {color.x, color.y, color.z}});
  vertices.push_back({{end.x + perpX, end.y, end.z + perpZ},
                      {color.x, color.y, color.z}});
  vertices.push_back({{end.x - perpX, end.y, end.z - perpZ},
                      {color.x, color.y, color.z}});
  IndexedDrawRange range;
  range.firstIndex = static_cast<std::uint32_t>(indices.size());
  const std::uint16_t winding[6] = {
      base, static_cast<std::uint16_t>(base + 1),
      static_cast<std::uint16_t>(base + 2), base,
      static_cast<std::uint16_t>(base + 2),
      static_cast<std::uint16_t>(base + 1)};
  for (const std::uint16_t index : winding) {
    indices.push_back(index);
  }
  range.indexCount = static_cast<std::uint32_t>(indices.size()) - range.firstIndex;
  draws.push_back(range);
  return true;
}

}  // namespace

std::vector<FirstRoomVertex> firstRoomBootstrapVertices() {
  return {{{-1.5F, 0.0F, -1.5F}, {0.35F, 0.38F, 0.42F}},
          {{1.5F, 0.0F, -1.5F}, {0.35F, 0.38F, 0.42F}},
          {{1.5F, 0.0F, 1.5F}, {0.48F, 0.52F, 0.56F}},
          {{-1.5F, 0.0F, 1.5F}, {0.48F, 0.52F, 0.56F}}};
}

std::vector<std::uint16_t> firstRoomBootstrapIndices() {
  return {0U, 1U, 2U, 2U, 3U, 0U};
}

namespace {

const StaticMeshAssetDrawRanges* findStaticMeshAssetDrawRanges(
    std::span<const StaticMeshAssetDrawRanges> assets,
    std::string_view assetId,
    std::size_t& index) noexcept {
  const auto found = std::lower_bound(
      assets.begin(), assets.end(), assetId,
      [](const StaticMeshAssetDrawRanges& asset, std::string_view id) {
        return asset.assetId < id;
      });
  if (found == assets.end() || found->assetId != assetId) {
    return nullptr;
  }
  index = static_cast<std::size_t>(found - assets.begin());
  return &*found;
}

struct StaticMeshAssetInstanceGroups {
  std::vector<StaticMeshInstanceTransform> defaultInstances;
  std::vector<std::vector<StaticMeshInstanceTransform>> materialVariants;
};

RoomMeshCpuGeometry buildRoomMeshCpuGeometryImpl(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    StaticMeshAssetCache* staticMeshAssets,
    const StaticMeshMaterialTextureResources* materialTextures,
    const std::span<const StaticMeshAssetDrawRanges>* staticMeshAssetDraws) {
  RoomMeshCpuGeometry result;
  result.sourceRoomAssetId = room.assetId;
  result.sourceRoomStaticMeshCount = room.meshes.size();
  result.sourceRoomGeometrySignature = roomGeometrySignature(room);
  if (room.meshes.empty() && room.surfacePatches.empty()) {
    return result;
  }

  result.vertices.reserve(room.meshes.size() * 16U +
                          room.surfacePatches.size() * 5U);
  result.indices.reserve(room.meshes.size() * 144U +
                         room.surfacePatches.size() * 24U);
  std::vector<StaticMeshAssetInstanceGroups> instanceGroups;
  if (staticMeshAssetDraws != nullptr) {
    instanceGroups.resize(staticMeshAssetDraws->size());
    for (std::size_t assetIndex = 0U;
         assetIndex < staticMeshAssetDraws->size(); ++assetIndex) {
      instanceGroups[assetIndex].materialVariants.resize(
          (*staticMeshAssetDraws)[assetIndex].materialVariants.size());
    }
  }

  const std::vector<FloorDraw> floorDraws = buildOptimizedFloorDraws(room);
  std::vector<WallBoxDraw> wallDraws;
  if (!buildOptimizedWallDraws(room, wallDraws)) {
    result.vertices.clear();
    result.indices.clear();
    result.indexedDraws.clear();
    return result;
  }
  for (const FloorDraw& floor : floorDraws) {
    if (!canEmitFloorDraw(floor)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    const bool appended = appendBoxIfFits(
        result.vertices, result.indices, result.indexedDraws, floor.position,
        floor.size,
        colorForRoomMaterial("floor", floor.semanticRole, floor.materialId),
        floor.rotationEulerRadians);
    if (!appended) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    ++result.roomFloorDrawCount;
  }
  for (const WallBoxDraw& wall : wallDraws) {
    if (!appendBoxIfFits(result.vertices, result.indices, result.indexedDraws,
                         wall.position, wall.size,
                         colorForRoomMaterial("wall", wall.semanticRole,
                                              wall.materialId),
                         wall.rotationEulerRadians)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    ++result.roomWallDrawCount;
  }

  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (!finiteVec3(mesh.rotationEulerRadians)) {
      result.vertices.clear();
      result.indices.clear();
      result.indexedDraws.clear();
      return result;
    }
    std::string_view externalAssetId;
    if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
      if (staticMeshAssetDraws != nullptr) {
        std::size_t assetIndex = 0U;
        const StaticMeshAssetDrawRanges* asset =
            findStaticMeshAssetDrawRanges(*staticMeshAssetDraws,
                                          externalAssetId, assetIndex);
        StaticMeshInstanceTransform transform;
        if (asset != nullptr && buildStaticMeshInstanceTransform(
                                    mesh, asset->boundsMin, asset->boundsMax,
                                    transform)) {
          const auto variant = std::find_if(
              asset->materialVariants.begin(), asset->materialVariants.end(),
              [&mesh](const StaticMeshMaterialVariantDrawRanges& candidate) {
                return candidate.name == mesh.materialVariant;
              });
          if (!mesh.materialVariant.empty() &&
              variant != asset->materialVariants.end()) {
            const std::size_t variantIndex = static_cast<std::size_t>(
                variant - asset->materialVariants.begin());
            instanceGroups[assetIndex].materialVariants[variantIndex].push_back(
                transform);
          } else {
            instanceGroups[assetIndex].defaultInstances.push_back(transform);
          }
          continue;
        }
        if (!appendBoxIfFits(result.vertices, result.indices,
                             result.indexedDraws, mesh.position, mesh.size,
                             {1.0F, 0.0F, 1.0F},
                             mesh.rotationEulerRadians)) {
          result.vertices.clear();
          result.indices.clear();
          result.indexedDraws.clear();
          return result;
        }
        continue;
      }
      const StaticMeshAsset* asset =
          staticMeshAssets != nullptr
              ? staticMeshAssets->find(externalAssetId)
              : nullptr;
      if (asset != nullptr &&
          appendStaticMeshAsset(result.vertices, result.indices,
                                result.indexedDraws, mesh, *asset,
                                materialTextures)) {
        continue;
      }
      if (!appendBoxIfFits(result.vertices, result.indices,
                           result.indexedDraws, mesh.position, mesh.size,
                           {1.0F, 0.0F, 1.0F},
                           mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }
    if (mesh.role == "terrain") {
      continue;
    }
    if (mesh.role == "floor") {
      if (!mesh.semanticRole.empty() && mesh.semanticRole != "Floor") {
        continue;
      }
      if (hasRotation(mesh.rotationEulerRadians)) {
        continue;
      }
      std::size_t gridLines = 0;
      if (!appendFloorGrid(result.vertices, result.indices, result.indexedDraws,
                           mesh.position, mesh.size, gridLines)) {
        result.roomGridTruncated = true;
      }
      result.roomGridLineDrawCount += gridLines;
      result.roomGridVisible = result.roomGridVisible || gridLines > 0U;
      continue;
    }
    if (mesh.role == "wall") {
      WallBoxDraw wallDraw;
      if (!wallBoxForMesh(mesh, wallDraw)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      if (hasRotation(wallDraw.rotationEulerRadians)) {
        continue;
      }
      std::size_t gridLines = 0;
      if (!appendWallGrid(result.vertices, result.indices, result.indexedDraws,
                          wallDraw.position, wallDraw.size, gridLines)) {
        result.roomGridTruncated = true;
      }
      result.roomGridLineDrawCount += gridLines;
      result.roomGridVisible = result.roomGridVisible || gridLines > 0U;
      continue;
    }

    if (mesh.role == "npc_gaze_perceived" || mesh.role == "npc_gaze_blocked" ||
        mesh.role == "npc_gaze_scan") {
      // Debug aid: a failed blade is skipped, never nukes the frame.
      (void)appendGazeBlade(result.vertices, result.indices, result.indexedDraws,
                            mesh.wallStartMeters, mesh.wallEndMeters,
                            mesh.wallThicknessMeters, colorForRoomRole(mesh.role));
      continue;
    }

    if (mesh.meshId == "creative_ramp_wedge") {
      if (!appendRampWedgeIfFits(result.vertices, result.indices,
                                 result.indexedDraws, mesh.position, mesh.size,
                                 colorForRoomMaterial(mesh.role,
                                                      mesh.semanticRole,
                                                      mesh.materialId),
                                 mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }
    if (mesh.meshId == "creative_hip_roof_panel") {
      if (!appendHipRoofPanelIfFits(
              result.vertices, result.indices, result.indexedDraws,
              mesh.position, mesh.size,
              colorForRoomMaterial(mesh.role, mesh.semanticRole,
                                   mesh.materialId),
              mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }
    if (mesh.meshId == "creative_open_frame") {
      if (!appendOpenFrameIfFits(
              result.vertices, result.indices, result.indexedDraws,
              mesh.position, mesh.size,
              colorForRoomMaterial(mesh.role, mesh.semanticRole,
                                   mesh.materialId),
              mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }
    if (mesh.meshId == "creative_stair_steps") {
      if (!appendStairStepsIfFits(
              result.vertices, result.indices, result.indexedDraws,
              mesh.position, mesh.size,
              colorForRoomMaterial(mesh.role, mesh.semanticRole,
                                   mesh.materialId),
              mesh.rotationEulerRadians, mesh.proceduralSegmentCount)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      continue;
    }

    SceneModelKind beanKind = SceneModelKind::PlayerBean;
    if (parseSceneModelId(mesh.role, beanKind)) {
      if (!appendBean(result.vertices, result.indices, result.indexedDraws,
                      mesh.position, mesh.size,
                      colorForRoomMaterial(mesh.role, mesh.semanticRole,
                                           mesh.materialId),
                      beanKind)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
    } else {
      if (!appendBoxIfFits(result.vertices, result.indices,
                           result.indexedDraws, mesh.position, mesh.size,
                           colorForRoomMaterial(mesh.role, mesh.semanticRole,
                                                mesh.materialId),
                           mesh.rotationEulerRadians)) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      if (mesh.role == "grid") {
        ++result.roomGridLineDrawCount;
        result.roomGridVisible = true;
      }
    }
  }

  const bool useSurfacePatches =
      canAppendSurfacePatches(result.vertices, room.surfacePatches);
  if (useSurfacePatches) {
    static_cast<void>(appendSurfacePatches(
        result.vertices, result.indices, result.indexedDraws,
        room.surfacePatches));
    ++result.roomFloorDrawCount;
  } else {
    for (const SceneRoomMeshItem& mesh : room.meshes) {
      if (mesh.role != "terrain") {
        continue;
      }
      std::string_view externalAssetId;
      if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
        continue;
      }
      const FloorDraw terrain{mesh.position, mesh.size,
                              mesh.rotationEulerRadians, mesh.materialId,
                              mesh.semanticRole};
      if (!canEmitFloorDraw(terrain) ||
          !appendFloorPlaneIfFits(result.vertices, result.indices,
                                  result.indexedDraws, terrain.position,
                                  terrain.size, colorForRoomRole("terrain"))) {
        result.vertices.clear();
        result.indices.clear();
        result.indexedDraws.clear();
        return result;
      }
      ++result.roomFloorDrawCount;
    }
  }

  if (staticMeshAssetDraws != nullptr) {
    const auto appendInstanceFamily =
        [&result](std::span<const StaticMeshInstanceTransform> instances,
                  std::span<const IndexedDrawRange> draws) {
          if (instances.empty()) {
            return true;
          }
          if (instances.size() > std::numeric_limits<std::uint32_t>::max() ||
              result.staticMeshInstances.size() >
                  std::numeric_limits<std::uint32_t>::max() -
                      instances.size()) {
            return false;
          }
          const std::uint32_t firstInstance =
              static_cast<std::uint32_t>(result.staticMeshInstances.size());
          const std::uint32_t instanceCount =
              static_cast<std::uint32_t>(instances.size());
          result.staticMeshInstances.insert(result.staticMeshInstances.end(),
                                            instances.begin(), instances.end());
          for (const IndexedDrawRange& draw : draws) {
            if (draw.indexCount == 0U) {
              continue;
            }
            result.staticMeshInstanceBatches.push_back(
                {draw.firstIndex, draw.indexCount, draw.materialTextureIndex,
                 firstInstance, instanceCount});
          }
          return true;
        };
    for (std::size_t assetIndex = 0U; assetIndex < instanceGroups.size();
         ++assetIndex) {
      const StaticMeshAssetDrawRanges& asset =
          (*staticMeshAssetDraws)[assetIndex];
      const StaticMeshAssetInstanceGroups& groups = instanceGroups[assetIndex];
      bool appended = appendInstanceFamily(groups.defaultInstances,
                                           asset.indexedDraws);
      for (std::size_t variantIndex = 0U;
           appended && variantIndex < groups.materialVariants.size();
           ++variantIndex) {
        appended = appendInstanceFamily(
            groups.materialVariants[variantIndex],
            asset.materialVariants[variantIndex].indexedDraws);
      }
      if (!appended) {
        result.staticMeshInstances.clear();
        result.staticMeshInstanceBatches.clear();
        return result;
      }
    }
  }

  appendCreativeWireframeDebugGeometry(result, creativeWireframeDebug);
  const bool baseGeometryReady = !result.vertices.empty() &&
                                 !result.indices.empty() &&
                                 !result.indexedDraws.empty();
  const bool instanceGeometryReady = !result.staticMeshInstances.empty() &&
                                     !result.staticMeshInstanceBatches.empty();
  result.ready = baseGeometryReady || instanceGeometryReady;
  return result;
}

}  // namespace

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    StaticMeshAssetCache* staticMeshAssets,
    const StaticMeshMaterialTextureResources* materialTextures) {
  return buildRoomMeshCpuGeometryImpl(room, creativeWireframeDebug,
                                      staticMeshAssets, materialTextures,
                                      nullptr);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    std::span<const StaticMeshAssetDrawRanges> staticMeshAssetDraws) {
  return buildRoomMeshCpuGeometryImpl(room, creativeWireframeDebug, nullptr,
                                      nullptr, &staticMeshAssetDraws);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(const SceneRoomProjection& room) {
  return buildRoomMeshCpuGeometry(room, nullptr, nullptr, nullptr);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  return buildRoomMeshCpuGeometry(room, creativeWireframeDebug, nullptr);
}

RoomMeshCpuGeometry buildRoomMeshCpuGeometry(
    const SceneRoomProjection& room,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug,
    StaticMeshAssetCache* staticMeshAssets) {
  return buildRoomMeshCpuGeometry(room, creativeWireframeDebug,
                                  staticMeshAssets, nullptr);
}

}  // namespace iggy3d::vulkan
