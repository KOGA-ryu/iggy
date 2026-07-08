#include "CreativeEditorAim.hpp"

#include <cmath>

#include "StandalonePlacement.hpp"

namespace iggy3d_creative_app {

iggy3d::creative::CreativeToolWorldPoint resolveCreativeEditorGroundPoint(
    const iggy3d::RenderCameraFrame& camera) {
  const iggy3d::Vec3 eye = camera.worldEye;
  const iggy3d::Vec3 fwd = camera.worldForward;
  iggy3d::creative::CreativeToolWorldPoint ground{eye.x, 0.0, eye.z};
  if (std::fabs(fwd.y) > 1.0e-4F) {
    const float t = -eye.y / fwd.y;  // eye.y + t*fwd.y == 0
    if (t > 0.0F) {
      ground.x = static_cast<double>(eye.x + fwd.x * t);
      ground.z = static_cast<double>(eye.z + fwd.z * t);
    }
  }
  return ground;
}

iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize) {
  const iggy3d::creative::CreativeToolWorldPoint ground =
      resolveCreativeEditorGroundPoint(camera);
  return snapGroundToCellCenter(ground.x, ground.z, placeCellSize);
}

}  // namespace iggy3d_creative_app
