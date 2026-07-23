#include "EditorFrustumCull.hpp"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include "core/math/Aabb3.hpp"
#include "core/math/Frustum.hpp"
#include "core/math/Vec3.hpp"
#include "projection/scene/SceneItem.hpp"

namespace iggy3d_creative_app {
namespace {

bool validCullMeshBounds(const iggy3d::SceneRoomMeshItem& mesh) {
  return iggy3d::isFinite(mesh.position) && iggy3d::isFinite(mesh.size) &&
         mesh.size.x > 0.0F && mesh.size.y > 0.0F && mesh.size.z > 0.0F;
}

iggy3d::Aabb3 meshAabb(const iggy3d::SceneRoomMeshItem& mesh) {
  const iggy3d::Vec3 halfSize = mesh.size * 0.5F;
  return iggy3d::makeAabb3(mesh.position - halfSize,
                           mesh.position + halfSize);
}

bool surfacePatchAabb(const iggy3d::SceneRoomSurfacePatchItem& patch,
                      iggy3d::Aabb3& bounds) {
  iggy3d::Vec3 minimum = patch.center;
  iggy3d::Vec3 maximum = patch.center;
  if (!iggy3d::isFinite(minimum)) {
    return false;
  }
  for (const iggy3d::Vec3 corner : patch.corners) {
    if (!iggy3d::isFinite(corner)) {
      return false;
    }
    minimum.x = std::min(minimum.x, corner.x);
    minimum.y = std::min(minimum.y, corner.y);
    minimum.z = std::min(minimum.z, corner.z);
    maximum.x = std::max(maximum.x, corner.x);
    maximum.y = std::max(maximum.y, corner.y);
    maximum.z = std::max(maximum.z, corner.z);
  }
  bounds = iggy3d::makeAabb3(minimum, maximum);
  return iggy3d::isValid(bounds);
}

bool isOpeningSemanticRole(std::string_view role) {
  return role == "Door" || role == "Window" || role == "Arch" ||
         role == "CaveOpening";
}

void refreshRoomRoleVisibility(iggy3d::SceneRoomProjection& room) {
  room.floorVisible = false;
  room.wallVisible = false;
  room.openingVisible = false;
  room.propVisible = false;
  for (const iggy3d::SceneRoomMeshItem& mesh : room.meshes) {
    room.floorVisible = room.floorVisible || mesh.role == "floor" ||
                        mesh.role == "terrain";
    room.wallVisible = room.wallVisible || mesh.role == "wall";
    room.openingVisible =
        room.openingVisible || mesh.role == "opening" ||
        isOpeningSemanticRole(mesh.semanticRole);
    room.propVisible = room.propVisible || mesh.role == "prop";
  }
  room.floorVisible = room.floorVisible || !room.surfacePatches.empty();
}

}  // namespace

StandaloneFrustumCullResult cullStandaloneSceneRoomMeshesByFrustum(
    const iggy3d::SceneProjectionResult& scene,
    const iggy3d::Mat4& clipFromWorld) {
  StandaloneFrustumCullResult result;
  result.scene = scene;
  result.receipt.requested = true;
  result.receipt.inputRoomMeshCount = scene.room.meshes.size();
  result.receipt.inputSurfacePatchCount = scene.room.surfacePatches.size();

  const iggy3d::FrustumPlanes planes = iggy3d::frustumPlanesFromClip(
      clipFromWorld, iggy3d::ClipDepthRange::ZeroToOne);

  std::vector<iggy3d::SceneRoomMeshItem> keptMeshes;
  keptMeshes.reserve(scene.room.meshes.size());
  for (const iggy3d::SceneRoomMeshItem& mesh : scene.room.meshes) {
    if (!validCullMeshBounds(mesh)) {
      keptMeshes.push_back(mesh);
      ++result.receipt.keptRoomMeshCount;
      ++result.receipt.conservativelyKeptMeshCount;
      continue;
    }

    if (iggy3d::aabbInFrustum(planes, meshAabb(mesh))) {
      keptMeshes.push_back(mesh);
      ++result.receipt.keptRoomMeshCount;
      continue;
    }

    ++result.receipt.culledRoomMeshCount;
  }

  result.scene.room.meshes = std::move(keptMeshes);
  std::vector<iggy3d::SceneRoomSurfacePatchItem> keptPatches;
  keptPatches.reserve(scene.room.surfacePatches.size());
  for (const iggy3d::SceneRoomSurfacePatchItem& patch :
       scene.room.surfacePatches) {
    iggy3d::Aabb3 bounds;
    if (!surfacePatchAabb(patch, bounds)) {
      keptPatches.push_back(patch);
      ++result.receipt.keptSurfacePatchCount;
      ++result.receipt.conservativelyKeptSurfacePatchCount;
      continue;
    }
    if (iggy3d::aabbInFrustum(planes, bounds)) {
      keptPatches.push_back(patch);
      ++result.receipt.keptSurfacePatchCount;
      continue;
    }
    ++result.receipt.culledSurfacePatchCount;
  }
  result.scene.room.surfacePatches = std::move(keptPatches);
  result.scene.room.staticMeshCount = result.scene.room.meshes.size();
  result.scene.room.loaded =
      result.scene.room.loaded &&
      (!result.scene.room.meshes.empty() ||
       !result.scene.room.surfacePatches.empty());
  refreshRoomRoleVisibility(result.scene.room);
  result.receipt.applied = true;
  return result;
}

}  // namespace iggy3d_creative_app
