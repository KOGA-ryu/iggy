#include "CreativeEditorPickFrame.hpp"

#include <SDL3/SDL.h>

#include "StandalonePlacement.hpp"
#include "StandalonePreviewProxies.hpp"

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

void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode) {
  if (!captureMode) {
    return;
  }

  auto logWorldPickProof =
      [&](const char* label, iggy3d::creative::CreativeObjectId expectedId,
          iggy3d::Vec3 worldPoint, bool& logged) {
        if (logged || expectedId == iggy3d::creative::kInvalidObjectId) {
          return;
        }
        const ScreenPoint screenPoint = projectPointToScreen(
            camera.clipFromWorld, worldPoint, drawableWidth, drawableHeight);
        if (!screenPoint.valid) {
          return;
        }
        const WorldRay ray =
            worldRayFromPixel(camera, screenPoint.x, screenPoint.y,
                              drawableWidth, drawableHeight);
        const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
            pickFrame.objectPickCandidates, ray);
        SDL_Log("iggy3d_creative: WORLD_PICK_PROOF label='%s' "
                "click=(%.1f, %.1f) rayValid=%d expectedObjectId=%llu "
                "pickedObjectId=%llu matched=%d entryDistance=%.3f "
                "tested=%llu hits=%llu",
                label, screenPoint.x, screenPoint.y, pick.rayValid ? 1 : 0,
                static_cast<unsigned long long>(expectedId),
                static_cast<unsigned long long>(pick.objectId),
                pick.objectId == expectedId ? 1 : 0, pick.entryDistance,
                static_cast<unsigned long long>(pick.testedCount),
                static_cast<unsigned long long>(pick.hitCount));
        logged = true;
      };

  if (!editor.captureWorldPickFloorLogged && pickFrame.haveFloorBounds) {
    const iggy3d::Vec3 floorTopCorner{
        pickFrame.floorBoxMin.x +
            (pickFrame.floorBoxMax.x - pickFrame.floorBoxMin.x) * 0.85F,
        pickFrame.floorBoxMax.y,
        pickFrame.floorBoxMin.z +
            (pickFrame.floorBoxMax.z - pickFrame.floorBoxMin.z) * 0.85F};
    logWorldPickProof("floor_overlap", floorObjectId, floorTopCorner,
                      editor.captureWorldPickFloorLogged);
  }

  auto logObjectCenterPick =
      [&](const char* label, iggy3d::creative::CreativeObjectId expectedId,
          bool& logged) {
        const iggy3d::creative::CreativeObject* object =
            facade.findObject(expectedId);
        if (object == nullptr) {
          return;
        }
        logWorldPickProof(label,
                          expectedId,
                          visualBoundsCenter(visualBoundsForObject(*object)),
                          logged);
      };
  logObjectCenterPick("point_proxy", editor.captureScript.pointTargetId,
                      editor.captureWorldPickPointLogged);
  logObjectCenterPick("line_proxy", editor.captureScript.lineTargetId,
                      editor.captureWorldPickLineLogged);
  logObjectCenterPick("path_proxy", editor.captureScript.pathTargetId,
                      editor.captureWorldPickPathLogged);
}

}  // namespace iggy3d_creative_app
