#include "render/vulkan/ProjectileOverlayProjection.hpp"

#include "core/math/Mat4.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::Mat4 clipMatrix(float scaleX = 1.0F,
                        float scaleY = 1.0F,
                        float scaleZ = 1.0F,
                        float w = 1.0F) {
  iggy3d::Mat4 out{};
  out.m[0] = scaleX;
  out.m[5] = scaleY;
  out.m[10] = scaleZ;
  out.m[15] = w;
  return out;
}

iggy3d::SceneProjectileItem projectileAt(iggy3d::Vec3 point) {
  iggy3d::SceneProjectileItem projectile;
  projectile.id = "test_projectile";
  projectile.positionMeters = point;
  projectile.previousPositionMeters = point;
  projectile.active = true;
  return projectile;
}

iggy3d::FrameInput frameFor(iggy3d::SceneProjectionResult& scene,
                            iggy3d::Mat4 clipFromWorld,
                            std::uint32_t width = 200U,
                            std::uint32_t height = 100U) {
  iggy3d::FrameInput frame;
  frame.viewport = {width, height, height == 0U ? 1.0F
                                                : static_cast<float>(width) /
                                                      static_cast<float>(height)};
  frame.clock = {1U, 1U, 0.0F, 0.0F};
  frame.camera.mode = iggy3d::RenderCameraMode::FirstPerson;
  frame.camera.worldEye = {0.0F, 0.0F, 0.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.camera.clipFromWorld = clipFromWorld;
  frame.projections.scene = &scene;
  return frame;
}

bool visibleProjectileProducesMarkerAndTrailRects() {
  iggy3d::SceneProjectionResult scene;
  scene.projectiles.push_back(projectileAt({0.0F, 0.0F, 0.5F}));
  const iggy3d::FrameInput frame = frameFor(scene, clipMatrix());

  const iggy3d::vulkan::ProjectileOverlayLayout layout =
      iggy3d::vulkan::projectileOverlayLayoutFor(frame);

  bool ok = true;
  ok = expect(layout.projectileCount == 1U, "visible projectile counted") && ok;
  ok = expect(layout.projected, "visible projectile projected") && ok;
  ok = expect(layout.markerCount == 1U, "visible projectile marker count") && ok;
  ok = expect(layout.trailRectCount == 4U, "visible projectile trail samples") && ok;
  ok = expect(layout.rects.size() == 5U, "visible projectile overlay rects") && ok;
  ok = expect(layout.rects.front().width == 12U &&
                  layout.rects.front().height == 12U,
              "visible projectile marker size") &&
       ok;
  ok = expect(layout.rects.front().x == 94 && layout.rects.front().y == 44,
              "visible projectile marker centered on viewport") &&
       ok;
  return ok;
}

bool outOfRangeNdcXyRejectsProjection() {
  iggy3d::SceneProjectionResult scene;
  scene.projectiles.push_back(projectileAt({1.21F, 0.0F, 0.5F}));
  scene.projectiles.push_back(projectileAt({0.0F, -1.21F, 0.5F}));
  const iggy3d::FrameInput frame = frameFor(scene, clipMatrix());

  const iggy3d::vulkan::ProjectileOverlayLayout layout =
      iggy3d::vulkan::projectileOverlayLayoutFor(frame);

  return expect(layout.projectileCount == 2U, "out-of-range xy projectiles counted") &&
         expect(!layout.projected, "out-of-range xy projectiles not projected") &&
         expect(layout.markerCount == 0U, "out-of-range xy markers rejected") &&
         expect(layout.trailRectCount == 0U, "out-of-range xy trails rejected") &&
         expect(layout.rects.empty(), "out-of-range xy emits no rects");
}

bool outOfRangeNdcZRejectsProjection() {
  iggy3d::SceneProjectionResult scene;
  scene.projectiles.push_back(projectileAt({0.0F, 0.0F, 1.06F}));
  const iggy3d::FrameInput frame = frameFor(scene, clipMatrix());

  const iggy3d::vulkan::ProjectileOverlayLayout layout =
      iggy3d::vulkan::projectileOverlayLayoutFor(frame);

  return expect(layout.projectileCount == 1U, "out-of-range z projectile counted") &&
         expect(!layout.projected, "out-of-range z projectile not projected") &&
         expect(layout.rects.empty(), "out-of-range z emits no rects");
}

bool negativeFiniteRawWStillProjectsByCurrentPolicy() {
  iggy3d::SceneProjectionResult scene;
  scene.projectiles.push_back(projectileAt({0.0F, 0.0F, 0.0F}));
  const iggy3d::FrameInput frame = frameFor(scene, clipMatrix(1.0F, 1.0F, 1.0F, -1.0F));

  const iggy3d::vulkan::ProjectileOverlayLayout layout =
      iggy3d::vulkan::projectileOverlayLayoutFor(frame);

  return expect(layout.projectileCount == 1U, "negative-w projectile counted") &&
         expect(layout.projected, "negative finite raw w still projects") &&
         expect(layout.markerCount == 1U, "negative-w marker projected") &&
         expect(layout.trailRectCount == 4U, "negative-w trail projected") &&
         expect(layout.rects.size() == 5U, "negative-w overlay rect count");
}

bool zeroViewportRejectsProjectionLayout() {
  iggy3d::SceneProjectionResult scene;
  scene.projectiles.push_back(projectileAt({0.0F, 0.0F, 0.5F}));
  const iggy3d::FrameInput frame = frameFor(scene, clipMatrix(), 0U, 100U);

  const iggy3d::vulkan::ProjectileOverlayLayout layout =
      iggy3d::vulkan::projectileOverlayLayoutFor(frame);

  return expect(layout.projectileCount == 1U, "zero viewport projectile counted") &&
         expect(!layout.projected, "zero viewport not projected") &&
         expect(layout.markerCount == 0U, "zero viewport marker rejected") &&
         expect(layout.trailRectCount == 0U, "zero viewport trail rejected") &&
         expect(layout.rects.empty(), "zero viewport emits no rects");
}

bool nonFiniteNdcRejectsProjection() {
  iggy3d::SceneProjectionResult scene;
  scene.projectiles.push_back(projectileAt({1.0F, 0.0F, 0.5F}));
  const iggy3d::FrameInput frame =
      frameFor(scene, clipMatrix(std::numeric_limits<float>::infinity()));

  const iggy3d::vulkan::ProjectileOverlayLayout layout =
      iggy3d::vulkan::projectileOverlayLayoutFor(frame);

  return expect(layout.projectileCount == 1U, "non-finite ndc projectile counted") &&
         expect(!layout.projected, "non-finite ndc not projected") &&
         expect(layout.rects.empty(), "non-finite ndc emits no rects");
}

}  // namespace

int main() {
  bool ok = true;
  ok = visibleProjectileProducesMarkerAndTrailRects() && ok;
  ok = outOfRangeNdcXyRejectsProjection() && ok;
  ok = outOfRangeNdcZRejectsProjection() && ok;
  ok = negativeFiniteRawWStillProjectsByCurrentPolicy() && ok;
  ok = zeroViewportRejectsProjectionLayout() && ok;
  ok = nonFiniteNdcRejectsProjection() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
