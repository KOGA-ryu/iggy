#include "EditorFrustumCull.hpp"

#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using iggy3d::Mat4;
using iggy3d::SceneProjectionResult;
using iggy3d::SceneRoomMeshItem;
using iggy3d::Vec3;
using iggy3d_creative_app::StandaloneFrustumCullResult;
using iggy3d_creative_app::cullStandaloneSceneRoomMeshesByFrustum;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

Mat4 identityClip() {
  Mat4 out{};
  out.m[0] = 1.0F;
  out.m[5] = 1.0F;
  out.m[10] = 1.0F;
  out.m[15] = 1.0F;
  return out;
}

SceneRoomMeshItem mesh(std::string id,
                       Vec3 position,
                       Vec3 size,
                       std::string role = "prop") {
  SceneRoomMeshItem item;
  item.id = std::move(id);
  item.role = std::move(role);
  item.materialId = "test_material";
  item.position = position;
  item.size = size;
  return item;
}

SceneProjectionResult sceneWithMeshes(std::vector<SceneRoomMeshItem> meshes) {
  SceneProjectionResult scene;
  scene.room.loaded = true;
  scene.room.assetId = "test_room";
  scene.room.meshes = std::move(meshes);
  scene.room.staticMeshCount = scene.room.meshes.size();
  return scene;
}

bool idsEqual(const std::vector<SceneRoomMeshItem>& meshes,
              const std::vector<std::string_view>& expected) {
  if (meshes.size() != expected.size()) {
    return false;
  }
  for (std::size_t i = 0; i < expected.size(); ++i) {
    if (meshes[i].id != expected[i]) {
      return false;
    }
  }
  return true;
}

bool cullsOnlyWhollyOffscreenMeshes() {
  const float nan = std::numeric_limits<float>::quiet_NaN();
  SceneProjectionResult scene = sceneWithMeshes({
      mesh("visible", {0.0F, 0.0F, 0.5F}, {0.25F, 0.25F, 0.25F}),
      mesh("offscreen_x", {3.0F, 0.0F, 0.5F}, {0.25F, 0.25F, 0.25F}),
      mesh("nonfinite", {0.0F, 0.0F, 0.5F}, {nan, 0.25F, 0.25F}),
      mesh("degenerate", {0.0F, 0.0F, 0.5F}, {0.0F, 0.25F, 0.25F}),
  });

  const StandaloneFrustumCullResult result =
      cullStandaloneSceneRoomMeshesByFrustum(scene, identityClip());

  return expect(result.receipt.inputRoomMeshCount == 4U,
                "records input mesh count") &&
         expect(result.receipt.keptRoomMeshCount == 3U,
                "keeps visible and invalid meshes") &&
         expect(result.receipt.culledRoomMeshCount == 1U,
                "culls one offscreen mesh") &&
         expect(result.receipt.conservativelyKeptMeshCount == 2U,
                "records non-finite/degenerate conservative keeps") &&
         expect(result.scene.room.staticMeshCount == 3U,
                "updates static mesh count to kept meshes") &&
         expect(idsEqual(result.scene.room.meshes,
                         {"visible", "nonfinite", "degenerate"}),
                "kept mesh order remains stable");
}

bool intersectingBoundaryMeshStaysVisible() {
  SceneProjectionResult scene = sceneWithMeshes({
      mesh("intersecting_right_plane",
           {1.08F, 0.0F, 0.5F},
           {0.25F, 0.25F, 0.25F},
           "wall"),
  });

  const StandaloneFrustumCullResult result =
      cullStandaloneSceneRoomMeshesByFrustum(scene, identityClip());

  return expect(result.receipt.keptRoomMeshCount == 1U,
                "frustum-intersecting mesh remains visible") &&
         expect(result.receipt.culledRoomMeshCount == 0U,
                "intersecting mesh is not culled") &&
         expect(result.scene.room.wallVisible,
                "role visibility is refreshed from kept wall mesh");
}

bool largeOffscreenSetDropsBeforeSubmit() {
  std::vector<SceneRoomMeshItem> meshes;
  meshes.push_back(mesh("visible_center", {0.0F, 0.0F, 0.5F},
                        {0.25F, 0.25F, 0.25F}, "floor"));
  for (int i = 0; i < 12; ++i) {
    meshes.push_back(mesh("offscreen_" + std::to_string(i),
                          {4.0F + static_cast<float>(i), 0.0F, 0.5F},
                          {0.25F, 0.25F, 0.25F}));
  }

  const StandaloneFrustumCullResult result =
      cullStandaloneSceneRoomMeshesByFrustum(sceneWithMeshes(std::move(meshes)),
                                             identityClip());

  return expect(result.receipt.inputRoomMeshCount == 13U,
                "large set input count recorded") &&
         expect(result.receipt.keptRoomMeshCount == 1U,
                "only visible center mesh remains") &&
         expect(result.receipt.culledRoomMeshCount == 12U,
                "offscreen room meshes are dropped before submit") &&
         expect(result.scene.room.floorVisible,
                "floor visibility remains true for kept floor mesh") &&
         expect(!result.scene.room.propVisible,
                "prop visibility clears when all props are culled");
}

bool allCulledMeshesUnloadRoomForSubmitSafety() {
  SceneProjectionResult scene = sceneWithMeshes({
      mesh("offscreen_a", {3.0F, 0.0F, 0.5F}, {0.25F, 0.25F, 0.25F}, "floor"),
      mesh("offscreen_b", {-3.0F, 0.0F, 0.5F}, {0.25F, 0.25F, 0.25F}, "wall"),
  });

  const StandaloneFrustumCullResult result =
      cullStandaloneSceneRoomMeshesByFrustum(scene, identityClip());

  return expect(result.receipt.inputRoomMeshCount == 2U,
                "all-cull input count recorded") &&
         expect(result.receipt.keptRoomMeshCount == 0U,
                "all-cull keeps no meshes") &&
         expect(result.receipt.culledRoomMeshCount == 2U,
                "all-cull culls both meshes") &&
         expect(result.scene.room.meshes.empty(), "all-cull scene has no meshes") &&
         expect(result.scene.room.staticMeshCount == 0U,
                "all-cull static mesh count clears") &&
         expect(!result.scene.room.loaded,
                "all-cull room unloads to avoid empty package-room submit") &&
         expect(!result.scene.room.floorVisible && !result.scene.room.wallVisible,
                "all-cull role visibility clears");
}

}  // namespace

int main() {
  const bool ok = cullsOnlyWhollyOffscreenMeshes() &&
                  intersectingBoundaryMeshStaysVisible() &&
                  largeOffscreenSetDropsBeforeSubmit() &&
                  allCulledMeshesUnloadRoomForSubmitSafety();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
