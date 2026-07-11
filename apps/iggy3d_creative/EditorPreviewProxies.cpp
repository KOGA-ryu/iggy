#include "EditorPreviewProxies.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

namespace iggy3d_creative_app {

namespace {

[[nodiscard]] bool finiteCreativeVec3(const cr::CreativeVec3& value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool finitePositiveCreativeVec3(const cr::CreativeVec3& value) {
  return finiteCreativeVec3(value) && value.x > 0.0 && value.y > 0.0 &&
         value.z > 0.0;
}

[[nodiscard]] VisualBounds visualBoundsFromAabb(const iggy3d::Aabb3& bounds) {
  return {bounds.min, bounds.max};
}

enum class VisualMajorAxis { X, Y, Z };

[[nodiscard]] VisualMajorAxis majorAxisForBounds(VisualBounds bounds) {
  const float extentX = bounds.max.x - bounds.min.x;
  const float extentY = bounds.max.y - bounds.min.y;
  const float extentZ = bounds.max.z - bounds.min.z;
  if (extentY > extentX && extentY >= extentZ) {
    return VisualMajorAxis::Y;
  }
  if (extentZ > extentX && extentZ > extentY) {
    return VisualMajorAxis::Z;
  }
  return VisualMajorAxis::X;
}

[[nodiscard]] bool pathSegmentAxisAligned(cr::CreativeVec3 start,
                                          cr::CreativeVec3 end) {
  constexpr double kEps = 1.0e-6;
  const bool sameX = std::fabs(start.x - end.x) < kEps;
  const bool sameY = std::fabs(start.y - end.y) < kEps;
  const bool sameZ = std::fabs(start.z - end.z) < kEps;
  return (sameX && sameY && !sameZ) || (sameX && sameZ && !sameY) ||
         (sameY && sameZ && !sameX);
}

}  // namespace

iggy3d::Vec3 toVec3(const cr::CreativeVec3& value) {
  return {static_cast<float>(value.x), static_cast<float>(value.y),
          static_cast<float>(value.z)};
}

iggy3d::Transform3 toTransform3(const cr::CreativeTransform& value) {
  return {toVec3(value.position), toVec3(value.rotationEulerRadians),
          toVec3(value.scale)};
}

iggy3d::Aabb3 visualBoundsToLocalAabb(VisualBounds bounds,
                                      const cr::CreativeTransform& transform) {
  const iggy3d::Vec3 position = toVec3(transform.position);
  return iggy3d::makeAabb3(
      {bounds.min.x - position.x, bounds.min.y - position.y,
       bounds.min.z - position.z},
      {bounds.max.x - position.x, bounds.max.y - position.y,
       bounds.max.z - position.z});
}

bool objectHasVisualTransform(const cr::CreativeObject& object) {
  constexpr double kRotationEps = 1.0e-8;
  const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(object.kind);
  if (!descriptor.hasBounds || !finiteCreativeVec3(object.transform.position) ||
      !finiteCreativeVec3(object.transform.rotationEulerRadians) ||
      !finitePositiveCreativeVec3(object.transform.scale)) {
    return false;
  }
  return std::fabs(object.transform.rotationEulerRadians.x) > kRotationEps ||
         std::fabs(object.transform.rotationEulerRadians.y) > kRotationEps ||
         std::fabs(object.transform.rotationEulerRadians.z) > kRotationEps ||
         std::fabs(object.transform.scale.x - 1.0) > kRotationEps ||
         std::fabs(object.transform.scale.y - 1.0) > kRotationEps ||
         std::fabs(object.transform.scale.z - 1.0) > kRotationEps;
}

bool validPathPoints(const std::vector<cr::CreativePathPoint>& points) {
  if (points.size() < 2U) {
    return false;
  }
  return std::all_of(points.begin(), points.end(),
                     [](const cr::CreativePathPoint& point) {
                       return finiteCreativeVec3(point.position);
                     });
}

VisualBounds pointMarkerBounds(const cr::CreativeVec3& position) {
  const float half = kPointMarkerSizeMeters * 0.5F;
  const iggy3d::Vec3 center = toVec3(position);
  return {{center.x - half, center.y - half, center.z - half},
          {center.x + half, center.y + half, center.z + half}};
}

VisualBounds pathPointHandleBounds(const cr::CreativeVec3& position) {
  const float half = kPathPointHandleSizeMeters * 0.5F;
  const iggy3d::Vec3 center = toVec3(position);
  return {{center.x - half, center.y - half, center.z - half},
          {center.x + half, center.y + half, center.z + half}};
}

VisualBounds lineProxyBounds(VisualBounds authoredBounds) {
  const VisualMajorAxis majorAxis = majorAxisForBounds(authoredBounds);
  const iggy3d::Vec3 center{
      (authoredBounds.min.x + authoredBounds.max.x) * 0.5F,
      (authoredBounds.min.y + authoredBounds.max.y) * 0.5F,
      (authoredBounds.min.z + authoredBounds.max.z) * 0.5F};
  const float halfThickness = kLineProxyThicknessMeters * 0.5F;
  VisualBounds proxy{{center.x - halfThickness, center.y - halfThickness,
                      center.z - halfThickness},
                     {center.x + halfThickness, center.y + halfThickness,
                      center.z + halfThickness}};
  switch (majorAxis) {
    case VisualMajorAxis::X:
      proxy.min.x = authoredBounds.min.x;
      proxy.max.x = authoredBounds.max.x;
      break;
    case VisualMajorAxis::Y:
      proxy.min.y = authoredBounds.min.y;
      proxy.max.y = authoredBounds.max.y;
      break;
    case VisualMajorAxis::Z:
      proxy.min.z = authoredBounds.min.z;
      proxy.max.z = authoredBounds.max.z;
      break;
  }
  return proxy;
}

VisualBounds pathProxyBounds(
    const std::vector<cr::CreativePathPoint>& pathPoints) {
  if (!validPathPoints(pathPoints)) {
    return {{-0.5F, 0.0F, -0.5F}, {0.5F, kPathProxyThicknessMeters, 0.5F}};
  }

  const float halfThickness = kPathProxyThicknessMeters * 0.5F;
  iggy3d::Vec3 min{static_cast<float>(pathPoints.front().position.x),
                   static_cast<float>(pathPoints.front().position.y),
                   static_cast<float>(pathPoints.front().position.z)};
  iggy3d::Vec3 max = min;
  for (const cr::CreativePathPoint& point : pathPoints) {
    const iggy3d::Vec3 p = toVec3(point.position);
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    min.z = std::min(min.z, p.z);
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
    max.z = std::max(max.z, p.z);
  }
  return {{min.x - halfThickness, min.y - halfThickness,
           min.z - halfThickness},
          {max.x + halfThickness, max.y + halfThickness,
           max.z + halfThickness}};
}

VisualBounds pathSegmentProxyBounds(cr::CreativeVec3 start,
                                    cr::CreativeVec3 end) {
  const float halfThickness = kPathProxyThicknessMeters * 0.5F;
  const iggy3d::Vec3 a = toVec3(start);
  const iggy3d::Vec3 b = toVec3(end);
  return {{std::min(a.x, b.x) - halfThickness,
           std::min(a.y, b.y) - halfThickness,
           std::min(a.z, b.z) - halfThickness},
          {std::max(a.x, b.x) + halfThickness,
           std::max(a.y, b.y) + halfThickness,
           std::max(a.z, b.z) + halfThickness}};
}

VisualBounds axisAlignedVisualBoundsForObject(const cr::CreativeObject& object) {
  const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(object.kind);
  if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Point) {
    return pointMarkerBounds(object.transform.position);
  }
  if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Path) {
    return pathProxyBounds(object.pathPoints);
  }
  const VisualBounds authoredBounds{toVec3(object.bounds.min),
                                    toVec3(object.bounds.max)};
  if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Line) {
    return lineProxyBounds(authoredBounds);
  }
  return authoredBounds;
}

std::optional<iggy3d::OrientedBox> orientedVisualBoxForObject(
    const cr::CreativeObject& object) {
  if (!objectHasVisualTransform(object)) {
    return std::nullopt;
  }
  const VisualBounds axisAlignedBounds = axisAlignedVisualBoundsForObject(object);
  const iggy3d::Transform3 transform = toTransform3(object.transform);
  const iggy3d::Aabb3 localBounds =
      visualBoundsToLocalAabb(axisAlignedBounds, object.transform);
  const iggy3d::OrientedBox box =
      iggy3d::makeOrientedBox(transform, localBounds);
  if (!iggy3d::isFinite(box.transform) || !iggy3d::isValid(box.localBounds)) {
    return std::nullopt;
  }
  return box;
}

VisualBounds visualBoundsForObject(const cr::CreativeObject& object) {
  if (const std::optional<iggy3d::OrientedBox> box =
          orientedVisualBoxForObject(object)) {
    return visualBoundsFromAabb(iggy3d::orientedBoxWorldAabb(*box));
  }
  return axisAlignedVisualBoundsForObject(object);
}

iggy3d::Vec3 visualBoundsCenter(VisualBounds bounds) {
  return {(bounds.min.x + bounds.max.x) * 0.5F,
          (bounds.min.y + bounds.max.y) * 0.5F,
          (bounds.min.z + bounds.max.z) * 0.5F};
}

std::string_view renderRoleForDescriptor(
    const cr::CreativeObjectDescriptor& descriptor) {
  if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Point) {
    return "spell";
  }
  if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Line) {
    return "rail";
  }
  if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Path) {
    return "rail";
  }
  const cr::CreativeBounds& bounds = descriptor.defaults.bounds;
  const float sizeX = static_cast<float>(bounds.max.x - bounds.min.x);
  const float height = static_cast<float>(bounds.max.y - bounds.min.y);
  const float sizeZ = static_cast<float>(bounds.max.z - bounds.min.z);
  const bool finitePositive = std::isfinite(sizeX) && sizeX > 0.0F &&
                              std::isfinite(height) && height > 0.0F &&
                              std::isfinite(sizeZ) && sizeZ > 0.0F;
  if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Surface &&
      descriptor.occupancyKind == cr::CreativeSpatialOccupancyKind::Structural &&
      finitePositive) {
    if (height <= std::min(sizeX, sizeZ)) {
      return "floor";
    }
    if (height > std::min(sizeX, sizeZ)) {
      return "wall";
    }
  }
  return "prop";
}

void appendPathProxyMeshesToScene(const cr::CreativeObject& object,
                                  iggy3d::SceneProjectionResult& scene,
                                  std::string_view role) {
  if (!validPathPoints(object.pathPoints)) {
    return;
  }
  for (std::size_t index = 0; index < object.pathPoints.size() - 1U; ++index) {
    const cr::CreativeVec3 start = object.pathPoints[index].position;
    const cr::CreativeVec3 end = object.pathPoints[index + 1U].position;
    if (!pathSegmentAxisAligned(start, end)) {
      continue;
    }
    const VisualBounds segmentBounds = pathSegmentProxyBounds(start, end);
    iggy3d::SceneRoomMeshItem mesh;
    mesh.id = "creative.object_" + std::to_string(object.id) + ".path_" +
              std::to_string(index);
    mesh.role = std::string(role);
    mesh.materialId = "creative_object";
    mesh.position = visualBoundsCenter(segmentBounds);
    mesh.size = {segmentBounds.max.x - segmentBounds.min.x,
                 segmentBounds.max.y - segmentBounds.min.y,
                 segmentBounds.max.z - segmentBounds.min.z};
    scene.room.meshes.push_back(std::move(mesh));
  }
}

bool objectHasBakedStaticMeshSource(
    const cr::CreativeObject& object,
    const std::vector<cr::CreativeRoomBakeStaticMeshSource>&
        bakedStaticMeshSources) {
  return std::any_of(
      bakedStaticMeshSources.begin(), bakedStaticMeshSources.end(),
      [&](const cr::CreativeRoomBakeStaticMeshSource& source) {
        return source.objectId == object.id;
      });
}

std::size_t appendStandalonePreviewProxiesToScene(
    const cr::CreativeDocument& document,
    const std::vector<cr::CreativeRoomBakeStaticMeshSource>&
        bakedStaticMeshSources,
    iggy3d::SceneProjectionResult& scene) {
  std::size_t appended = 0;
  for (const cr::CreativeObject& obj : document.objects()) {
    if (!obj.visible) {
      continue;
    }
    const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(obj.kind);
    const std::string_view role = renderRoleForDescriptor(descriptor);
    if (objectHasBakedStaticMeshSource(obj, bakedStaticMeshSources)) {
      continue;
    }
    if (descriptor.shapeKind == cr::CreativeObjectShapeKind::Path) {
      const std::size_t before = scene.room.meshes.size();
      appendPathProxyMeshesToScene(obj, scene, role);
      appended += scene.room.meshes.size() - before;
      continue;
    }
    if (descriptor.shapeKind != cr::CreativeObjectShapeKind::Point &&
        descriptor.shapeKind != cr::CreativeObjectShapeKind::Line) {
      continue;
    }

    const VisualBounds visualBounds = visualBoundsForObject(obj);
    const iggy3d::Vec3 boxMin = visualBounds.min;
    const iggy3d::Vec3 boxMax = visualBounds.max;
    iggy3d::SceneRoomMeshItem mesh;
    mesh.id = "creative.preview_object_" + std::to_string(obj.id);
    mesh.role = std::string(role);
    mesh.materialId = "creative_preview_object";
    mesh.position = {(boxMin.x + boxMax.x) * 0.5F,
                     (boxMin.y + boxMax.y) * 0.5F,
                     (boxMin.z + boxMax.z) * 0.5F};
    mesh.size = {boxMax.x - boxMin.x, boxMax.y - boxMin.y,
                 boxMax.z - boxMin.z};
    scene.room.meshes.push_back(std::move(mesh));
    ++appended;
  }
  if (appended > 0U) {
    scene.room.staticMeshCount = scene.room.meshes.size();
    scene.room.loaded = true;
  }
  return appended;
}

void appendStandaloneWireframeBoxEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& out,
    iggy3d::Vec3 boxMin,
    iggy3d::Vec3 boxMax,
    iggy3d::RenderLineColor color,
    float thickness) {
  // 8 corners indexed by (x bit0, y bit1, z bit2).
  const auto corner = [&](int c) -> iggy3d::Vec3 {
    return {(c & 1) ? boxMax.x : boxMin.x, (c & 2) ? boxMax.y : boxMin.y,
            (c & 4) ? boxMax.z : boxMin.z};
  };
  // 12 edges: pairs of corner indices differing in exactly one axis bit.
  static constexpr int kEdges[12][2] = {
      {0, 1}, {2, 3}, {4, 5}, {6, 7},  // along X
      {0, 2}, {1, 3}, {4, 6}, {5, 7},  // along Y
      {0, 4}, {1, 5}, {2, 6}, {3, 7},  // along Z
  };
  out.reserve(out.size() + 12);
  for (const auto& e : kEdges) {
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = corner(e[0]);
    line.end = corner(e[1]);
    line.color = color;
    line.objectId = 0;  // Ghost is not a document object.
    line.thickness = thickness;
    out.push_back(line);
  }
}

}  // namespace iggy3d_creative_app
