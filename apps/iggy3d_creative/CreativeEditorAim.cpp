#include "CreativeEditorAim.hpp"

#include <cmath>

#include "StandalonePlacement.hpp"

namespace iggy3d_creative_app {

iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize) {
  const iggy3d::Vec3 aimEye = camera.worldEye;
  const iggy3d::Vec3 aimFwd = camera.worldForward;
  double aimGroundX = static_cast<double>(aimEye.x);
  double aimGroundZ = static_cast<double>(aimEye.z);
  if (std::fabs(aimFwd.y) > 1.0e-4F) {
    const float t = -aimEye.y / aimFwd.y;  // eye.y + t*fwd.y == 0
    if (t > 0.0F) {
      aimGroundX = static_cast<double>(aimEye.x + aimFwd.x * t);
      aimGroundZ = static_cast<double>(aimEye.z + aimFwd.z * t);
    }
  }
  return snapGroundToCellCenter(aimGroundX, aimGroundZ, placeCellSize);
}

}  // namespace iggy3d_creative_app
