#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"

#include <cmath>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "core/math/Mat4.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace iggy3d {
namespace {

Vec3 cameraBasisNormalizedOr(Vec3 value, Vec3 fallback) {
  const float length2 = lengthSquared(value);
  if (!std::isfinite(length2) || length2 <= 0.000001F) {
    return fallback;
  }
  return value / std::sqrt(length2);
}

Mat4 perspectiveMat4(float verticalFovRadians,
                     float aspect,
                     float nearPlane,
                     float farPlane) {
  const float f = 1.0F / std::tan(verticalFovRadians * 0.5F);
  Mat4 result{{{}}};
  result.m[0] = f / aspect;
  result.m[5] = -f;
  result.m[10] = farPlane / (nearPlane - farPlane);
  result.m[11] = -(farPlane * nearPlane) / (farPlane - nearPlane);
  result.m[14] = -1.0F;
  return result;
}

Mat4 viewFromCamera(Vec3 eye, Vec3 forward, Vec3 up) {
  const Vec3 f = cameraBasisNormalizedOr(forward, {0.0F, 0.0F, -1.0F});
  const Vec3 r = cameraBasisNormalizedOr(cross(f, up), {1.0F, 0.0F, 0.0F});
  const Vec3 u = cross(r, f);
  Mat4 result = identityMat4();
  result.m[0] = r.x;
  result.m[1] = r.y;
  result.m[2] = r.z;
  result.m[3] = -dot(r, eye);
  result.m[4] = u.x;
  result.m[5] = u.y;
  result.m[6] = u.z;
  result.m[7] = -dot(u, eye);
  result.m[8] = -f.x;
  result.m[9] = -f.y;
  result.m[10] = -f.z;
  result.m[11] = dot(f, eye);
  return result;
}

}  // namespace

FrameInput makeCreativeVulkanFrame(const SceneProjectionResult& scene,
                                   const DebugProjectionResult& debug,
                                   std::uint64_t frameIndex,
                                   std::uint32_t viewportWidth,
                                   std::uint32_t viewportHeight,
                                   float cameraYawDegrees,
                                   float cameraPitchDegrees,
                                   bool cameraAnchorOverrideAvailable,
                                   Vec3 cameraAnchorOverrideMeters,
                                   RenderContentViewport contentViewport) {
  constexpr float kPi = 3.14159265358979323846F;
  FrameInput frame;
  frame.viewport = {viewportWidth, viewportHeight,
                    static_cast<float>(viewportWidth) /
                        static_cast<float>(viewportHeight)};
  frame.contentViewport = contentViewport;
  // The camera aspect follows the sub-rectangle the scene actually occupies.
  // The sentinel resolves to the full viewport, so this is identical to the
  // swapchain aspect until a panel layout shrinks the content rect.
  const RenderContentViewport effectiveContent = effectiveContentViewport(frame);
  const float contentAspect = static_cast<float>(effectiveContent.width) /
                              static_cast<float>(effectiveContent.height);
  frame.clock = {scene.sourceTick, frameIndex, 0.0F, 1.0F / 60.0F};
  frame.camera.mode = RenderCameraMode::FirstPerson;
  Vec3 eye{0.0F, kProductCreativeCameraEyeHeightMeters, 0.0F};
  for (const SceneItem& item : scene.items) {
    if (item.kind == SceneItemKind::Player || item.stableName == "player") {
      eye = item.transform.position +
            Vec3{0.0F, kProductCreativeCameraEyeHeightMeters, 0.0F};
      break;
    }
  }
  if (cameraAnchorOverrideAvailable) {
    eye = cameraAnchorOverrideMeters +
          Vec3{0.0F, kProductCreativeCameraEyeHeightMeters, 0.0F};
  }
  const float yaw = cameraYawDegrees * kPi / 180.0F;
  const float pitch = cameraPitchDegrees * kPi / 180.0F;
  const float cosPitch = std::cos(pitch);
  frame.camera.worldEye = eye;
  frame.camera.worldForward = {std::sin(yaw) * cosPitch, std::sin(pitch),
                               -std::cos(yaw) * cosPitch};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.camera.viewFromWorld =
      viewFromCamera(frame.camera.worldEye, frame.camera.worldForward,
                     frame.camera.worldUp);
  frame.camera.clipFromView = perspectiveMat4(
      kProductCreativeCameraVerticalFovDegrees * kPi / 180.0F, contentAspect,
      frame.camera.nearPlane, frame.camera.farPlane);
  frame.camera.clipFromWorld =
      frame.camera.clipFromView * frame.camera.viewFromWorld;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

}  // namespace iggy3d
