#pragma once

#include <cstddef>

#include "core/math/Mat4.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace iggy3d_creative_app {

struct StandaloneFrustumCullReceipt {
  bool requested = false;
  bool applied = false;
  std::size_t inputRoomMeshCount = 0;
  std::size_t keptRoomMeshCount = 0;
  std::size_t culledRoomMeshCount = 0;
  std::size_t conservativelyKeptMeshCount = 0;
};

struct StandaloneFrustumCullResult {
  iggy3d::SceneProjectionResult scene;
  StandaloneFrustumCullReceipt receipt;
};

[[nodiscard]] StandaloneFrustumCullResult cullStandaloneSceneRoomMeshesByFrustum(
    const iggy3d::SceneProjectionResult& scene,
    const iggy3d::Mat4& clipFromWorld);

}  // namespace iggy3d_creative_app
