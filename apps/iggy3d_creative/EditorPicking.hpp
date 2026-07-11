#pragma once

#include "EditorPreviewProxies.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/OrientedBox.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

struct ScreenAabb {
  bool valid = false;
  float minX = 0.0F;
  float minY = 0.0F;
  float maxX = 0.0F;
  float maxY = 0.0F;
};

struct ScreenPoint {
  bool valid = false;
  float x = 0.0F;
  float y = 0.0F;
};

struct WorldRay {
  bool valid = false;
  iggy3d::Vec3 origin{};
  iggy3d::Vec3 direction{0.0F, 0.0F, -1.0F};
};

struct ObjectVisualPickBounds {
  cr::CreativeObjectId id = cr::kInvalidObjectId;
  VisualBounds bounds{};
  ScreenAabb screenAabb{};
  std::optional<iggy3d::OrientedBox> orientedBounds{};
};

struct ObjectVisualPickResult {
  bool rayValid = false;
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  float entryDistance = std::numeric_limits<float>::max();
  std::uint64_t testedCount = 0;
  std::uint64_t hitCount = 0;
};

struct PathPointHandleHit {
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  std::size_t pointIndex = 0U;
  ScreenAabb aabb{};
  cr::CreativeVec3 position{};
};

[[nodiscard]] ScreenAabb projectBoxToScreen(
    const iggy3d::Mat4& clipFromWorld,
    iggy3d::Vec3 boxMin,
    iggy3d::Vec3 boxMax,
    std::uint32_t widthPx,
    std::uint32_t heightPx);
[[nodiscard]] ScreenPoint projectPointToScreen(
    const iggy3d::Mat4& clipFromWorld,
    iggy3d::Vec3 world,
    std::uint32_t widthPx,
    std::uint32_t heightPx);
[[nodiscard]] WorldRay worldRayFromPixel(const iggy3d::RenderCameraFrame& camera,
                                         float pixelX,
                                         float pixelY,
                                         std::uint32_t widthPx,
                                         std::uint32_t heightPx);
[[nodiscard]] bool rayEntryDistanceForAabb(WorldRay ray,
                                           VisualBounds bounds,
                                           float& outT);
[[nodiscard]] ObjectVisualPickBounds buildObjectVisualPickBounds(
    const cr::CreativeObject& object,
    const iggy3d::Mat4& clipFromWorld,
    std::uint32_t widthPx,
    std::uint32_t heightPx);
[[nodiscard]] ObjectVisualPickResult pickNearestVisualBoundsObject(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray);
[[nodiscard]] ObjectVisualPickResult pickNearestVisualBoundsObjectBruteForce(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray);
[[nodiscard]] std::vector<PathPointHandleHit> buildPathPointHandleHits(
    const cr::CreativeObject& object,
    const iggy3d::Mat4& clipFromWorld,
    std::uint32_t widthPx,
    std::uint32_t heightPx);

}  // namespace iggy3d_creative_app
