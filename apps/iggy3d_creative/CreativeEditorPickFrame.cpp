#include "CreativeEditorPickFrame.hpp"

#include <SDL3/SDL.h>

#include "StandalonePlacement.hpp"

namespace iggy3d_creative_app {

CreativeEditorPickFrame buildCreativeEditorPickFrame(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    iggy3d::creative::CreativeObjectId floorObjectId,
    StandaloneCaptureScript& captureScript,
    bool captureMode) {
  CreativeEditorPickFrame frame;
  for (const iggy3d::creative::CreativeObject& obj : document.objects()) {
    if (!obj.visible) {
      continue;
    }
    const ObjectVisualPickBounds hit = buildObjectVisualPickBounds(
        obj, camera.clipFromWorld, drawableWidth, drawableHeight);
    const iggy3d::Vec3 boxMin = hit.bounds.min;
    const iggy3d::Vec3 boxMax = hit.bounds.max;
    frame.objectPickCandidates.push_back(hit);
    if (obj.id == floorObjectId) {
      frame.haveFloorBounds = true;
      frame.floorBoxMin = boxMin;
      frame.floorBoxMax = boxMax;
    }
    if (captureMode && !captureScript.pointHitProxyLogged &&
        obj.id == captureScript.pointTargetId) {
      SDL_Log("iggy3d_creative: POINT hit proxy objectId=%llu "
              "aabbValid=%d marker=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
              static_cast<unsigned long long>(captureScript.pointTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY);
      captureScript.pointHitProxyLogged = true;
    }
    if (captureMode && !captureScript.lineHitProxyLogged &&
        obj.id == captureScript.lineTargetId) {
      SDL_Log("iggy3d_creative: LINE hit proxy objectId=%llu "
              "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
              static_cast<unsigned long long>(captureScript.lineTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY);
      captureScript.lineHitProxyLogged = true;
    }
    if (captureMode && !captureScript.pathHitProxyLogged &&
        obj.id == captureScript.pathTargetId) {
      SDL_Log("iggy3d_creative: PATH hit proxy objectId=%llu "
              "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f] "
              "pathPointCount=%zu pathPoints='%s'",
              static_cast<unsigned long long>(captureScript.pathTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY,
              obj.pathPoints.size(), pathPointsSummary(obj.pathPoints).c_str());
      captureScript.pathHitProxyLogged = true;
    }
  }
  return frame;
}

}  // namespace iggy3d_creative_app
