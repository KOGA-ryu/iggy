#pragma once

#include "EditorPreviewProxies.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/OrientedBox.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

#include <cstddef>
#include <cstdint>
#include <array>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

struct WorldRay {
  bool valid = false;
  iggy3d::Vec3 origin{};
  iggy3d::Vec3 direction{0.0F, 0.0F, -1.0F};
};

struct ObjectVisualPickBounds {
  cr::CreativeObjectId id = cr::kInvalidObjectId;
  VisualBounds bounds{};
  cr::CreativeScreenBounds screenAabb{};
  std::optional<iggy3d::OrientedBox> orientedBounds{};
  bool visible = true;
  bool locked = false;
};

inline constexpr std::size_t kObjectVisualPickHitCapacity = 64U;

struct ObjectVisualPickHit {
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  float entryDistance = std::numeric_limits<float>::max();
  bool locked = false;
};

struct ObjectVisualPickStack {
  bool rayValid = false;
  std::array<ObjectVisualPickHit, kObjectVisualPickHitCapacity> items{};
  std::size_t count = 0U;
  std::uint64_t testedCount = 0U;
  std::uint64_t totalHitCount = 0U;
  std::uint64_t hiddenExcludedCount = 0U;
  std::uint64_t lockedHitCount = 0U;
  bool truncated = false;

  [[nodiscard]] std::span<const ObjectVisualPickHit> hits() const noexcept {
    return {items.data(), count};
  }
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
  cr::CreativeScreenBounds aabb{};
  cr::CreativeVec3 position{};
};

struct PathPointHandlePickResult {
  bool hit = false;
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  std::size_t pointIndex = 0U;
  float centerDistanceSquared = std::numeric_limits<float>::max();
};
// pixelX/pixelY are drawable-pixel coordinates; region is the sub-rectangle
// the 3D scene occupies (the ImGui central node). NDC is computed relative to
// the region so the ray is correct when panels shrink the viewport. A region
// spanning the full drawable is identical to the historical full-window ray.
[[nodiscard]] WorldRay worldRayFromPixel(const iggy3d::RenderCameraFrame& camera,
                                         float pixelX,
                                         float pixelY,
                                         const iggy3d::RenderContentViewport& region);
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
[[nodiscard]] ObjectVisualPickStack pickVisualBoundsObjectStack(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray);
[[nodiscard]] ObjectVisualPickStack pickVisualBoundsObjectStackBruteForce(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray);

// Repeating a plain selection over the same overlap stack advances to the next
// visible hit. A current selection outside the stack restarts at the nearest.
[[nodiscard]] cr::CreativeObjectId cycleObjectVisualPick(
    const ObjectVisualPickStack& stack,
    cr::CreativeObjectId currentObjectId) noexcept;
[[nodiscard]] std::vector<PathPointHandleHit> buildPathPointHandleHits(
    const cr::CreativeObject& object,
    const iggy3d::Mat4& clipFromWorld,
    std::uint32_t widthPx,
    std::uint32_t heightPx);
[[nodiscard]] PathPointHandlePickResult pickPathPointHandleAtPixel(
    std::span<const PathPointHandleHit> handles,
    float pixelX,
    float pixelY,
    float paddingPixels = 6.0F) noexcept;

}  // namespace iggy3d_creative_app
