#pragma once

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/OrientedBox.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

constexpr float kPointMarkerSizeMeters = 0.35F;
constexpr float kLineProxyThicknessMeters = 0.16F;
constexpr float kPathProxyThicknessMeters = 0.16F;
constexpr float kPathPointHandleSizeMeters = 0.30F;

struct VisualBounds {
  iggy3d::Vec3 min;
  iggy3d::Vec3 max;
};

[[nodiscard]] iggy3d::Vec3 toVec3(const cr::CreativeVec3& value);
[[nodiscard]] iggy3d::Transform3 toTransform3(
    const cr::CreativeTransform& value);
[[nodiscard]] iggy3d::Aabb3 visualBoundsToLocalAabb(
    VisualBounds bounds,
    const cr::CreativeTransform& transform);
[[nodiscard]] bool objectHasVisualRotation(const cr::CreativeObject& object);
[[nodiscard]] bool validPathPoints(
    const std::vector<cr::CreativePathPoint>& points);
[[nodiscard]] VisualBounds pointMarkerBounds(
    const cr::CreativeVec3& position);
[[nodiscard]] VisualBounds pathPointHandleBounds(
    const cr::CreativeVec3& position);
[[nodiscard]] VisualBounds lineProxyBounds(VisualBounds authoredBounds);
[[nodiscard]] VisualBounds pathProxyBounds(
    const std::vector<cr::CreativePathPoint>& pathPoints);
[[nodiscard]] VisualBounds pathSegmentProxyBounds(
    cr::CreativeVec3 start,
    cr::CreativeVec3 end);
[[nodiscard]] VisualBounds axisAlignedVisualBoundsForObject(
    const cr::CreativeObject& object);
[[nodiscard]] std::optional<iggy3d::OrientedBox> orientedVisualBoxForObject(
    const cr::CreativeObject& object);
[[nodiscard]] VisualBounds visualBoundsForObject(const cr::CreativeObject& object);
[[nodiscard]] iggy3d::Vec3 visualBoundsCenter(VisualBounds bounds);
[[nodiscard]] std::string_view renderRoleForDescriptor(
    const cr::CreativeObjectDescriptor& descriptor);

void appendPathProxyMeshesToScene(
    const cr::CreativeObject& object,
    iggy3d::SceneProjectionResult& scene,
    std::string_view role);

[[nodiscard]] bool objectHasBakedStaticMeshSource(
    const cr::CreativeObject& object,
    const std::vector<cr::CreativeRoomBakeStaticMeshSource>&
        bakedStaticMeshSources);

[[nodiscard]] std::size_t appendStandalonePreviewProxiesToScene(
    const cr::CreativeDocument& document,
    const std::vector<cr::CreativeRoomBakeStaticMeshSource>&
        bakedStaticMeshSources,
    iggy3d::SceneProjectionResult& scene);

void appendPathPolylineLines(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& out,
    const std::vector<cr::CreativePathPoint>& pathPoints,
    iggy3d::RenderLineColor color,
    float thickness,
    cr::CreativeObjectId objectId = cr::kInvalidObjectId);

}  // namespace iggy3d_creative_app
