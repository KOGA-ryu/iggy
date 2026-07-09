#include "EditorFrustumCull.hpp"

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

void refreshRoomRoleVisibility(iggy3d::SceneRoomProjection& room) {
  room.floorVisible = false;
  room.wallVisible = false;
  room.openingVisible = false;
  room.propVisible = false;
  for (const iggy3d::SceneRoomMeshItem& mesh : room.meshes) {
    room.floorVisible = room.floorVisible || mesh.role == "floor";
    room.wallVisible = room.wallVisible || mesh.role == "wall";
    room.openingVisible = room.openingVisible || mesh.role == "opening";
    room.propVisible = room.propVisible || mesh.role == "prop";
  }
}

}  // namespace

StandaloneFrustumCullResult cullStandaloneSceneRoomMeshesByFrustum(
    const iggy3d::SceneProjectionResult& scene,
    const iggy3d::Mat4& clipFromWorld) {
  StandaloneFrustumCullResult result;
  result.scene = scene;
  result.receipt.requested = true;
  result.receipt.inputRoomMeshCount = scene.room.meshes.size();

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
  result.scene.room.staticMeshCount = result.scene.room.meshes.size();
  result.scene.room.loaded =
      result.scene.room.loaded && !result.scene.room.meshes.empty();
  refreshRoomRoleVisibility(result.scene.room);
  result.receipt.applied = true;
  return result;
}

}  // namespace iggy3d_creative_app
