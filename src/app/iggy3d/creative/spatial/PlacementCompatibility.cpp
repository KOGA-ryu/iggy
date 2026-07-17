#include "app/iggy3d/creative/spatial/PlacementCompatibility.hpp"

#include <cmath>

namespace iggy3d::creative {
namespace {

constexpr double kMinimumNormalLengthSquared = 1.0e-12;
constexpr double kSupportingSurfaceMinimumY = 0.5;
constexpr double kVerticalSurfaceMaximumAbsY = 0.25;

[[nodiscard]] bool knownObjectKind(CreativeObjectKind kind) noexcept {
  return kind > CreativeObjectKind::Unknown && kind < CreativeObjectKind::Count;
}

[[nodiscard]] bool knownTargetSource(
    CreativePlacementTargetSource source) noexcept {
  return source > CreativePlacementTargetSource::Unknown &&
         source < CreativePlacementTargetSource::Count;
}

[[nodiscard]] bool validTargetFacts(
    const CreativePlacementTargetFacts& facts) noexcept {
  if (!facts.valid || !knownTargetSource(facts.source)) {
    return false;
  }
  switch (facts.source) {
    case CreativePlacementTargetSource::EmptyPlane:
      return facts.hostKind == CreativeObjectKind::Unknown &&
             facts.hostObjectId == kInvalidObjectId;
    case CreativePlacementTargetSource::Terrain:
      return facts.hostKind == CreativeObjectKind::TerrainPatch &&
             facts.hostObjectId == kInvalidObjectId;
    case CreativePlacementTargetSource::Voxel:
      return knownObjectKind(facts.hostKind) &&
             facts.hostObjectId == kInvalidObjectId;
    case CreativePlacementTargetSource::AuthoredObject:
      return knownObjectKind(facts.hostKind) &&
             facts.hostObjectId != kInvalidObjectId;
    case CreativePlacementTargetSource::Unknown:
    case CreativePlacementTargetSource::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool hostedTarget(
    CreativePlacementTargetSource source) noexcept {
  return source == CreativePlacementTargetSource::Voxel ||
         source == CreativePlacementTargetSource::AuthoredObject;
}

[[nodiscard]] bool solidHost(const CreativeObjectDescriptor& host) noexcept {
  if (!host.hasBounds) {
    return false;
  }
  switch (host.occupancyKind) {
    case CreativeSpatialOccupancyKind::Structural:
    case CreativeSpatialOccupancyKind::Collision:
    case CreativeSpatialOccupancyKind::Navigation:
      return true;
    case CreativeSpatialOccupancyKind::Unknown:
    case CreativeSpatialOccupancyKind::Trigger:
    case CreativeSpatialOccupancyKind::Gameplay:
    case CreativeSpatialOccupancyKind::Light:
    case CreativeSpatialOccupancyKind::Audio:
    case CreativeSpatialOccupancyKind::Camera:
    case CreativeSpatialOccupancyKind::Testing:
    case CreativeSpatialOccupancyKind::Authoring:
      return false;
  }
  return false;
}

[[nodiscard]] CreativePlacementCompatibilityResult reject(
    CreativePlacementCompatibilityResult result,
    CreativePlacementCompatibilityStatus status) noexcept {
  result.status = status;
  return result;
}

[[nodiscard]] CreativePlacementCompatibilityResult allow(
    CreativePlacementCompatibilityResult result) noexcept {
  result.status = CreativePlacementCompatibilityStatus::Ready;
  result.allowed = true;
  return result;
}

}  // namespace

CreativePlacementTargetFacts makeCreativePlacementTargetFacts(
    CreativePlacementTargetSource source,
    CreativeObjectKind hostKind,
    CreativeObjectId hostObjectId) noexcept {
  CreativePlacementTargetFacts facts;
  facts.source = source;
  facts.hostKind = hostKind;
  facts.hostObjectId = hostObjectId;
  switch (source) {
    case CreativePlacementTargetSource::EmptyPlane:
      facts.valid = hostKind == CreativeObjectKind::Unknown &&
                    hostObjectId == kInvalidObjectId;
      break;
    case CreativePlacementTargetSource::Terrain:
      facts.valid = hostKind == CreativeObjectKind::TerrainPatch &&
                    hostObjectId == kInvalidObjectId;
      break;
    case CreativePlacementTargetSource::Voxel:
      facts.valid = knownObjectKind(hostKind) &&
                    hostObjectId == kInvalidObjectId;
      break;
    case CreativePlacementTargetSource::AuthoredObject:
      facts.valid = knownObjectKind(hostKind) &&
                    hostObjectId != kInvalidObjectId;
      break;
    case CreativePlacementTargetSource::Unknown:
    case CreativePlacementTargetSource::Count:
      break;
  }
  return facts;
}

CreativePlacementCompatibilityResult resolveCreativePlacementCompatibility(
    const CreativePlacementCompatibilityRequest& request) noexcept {
  CreativePlacementCompatibilityResult result;
  result.hostPolicy = request.hostPolicy;
  result.source = request.target.source;
  result.hostKind = request.target.hostKind;
  result.hostObjectId = request.target.hostObjectId;
  result.evaluated = true;

  const double lengthSquared =
      request.surfaceNormal.x * request.surfaceNormal.x +
      request.surfaceNormal.y * request.surfaceNormal.y +
      request.surfaceNormal.z * request.surfaceNormal.z;
  if (request.hostPolicy >= CreativePlacementHostPolicy::Count ||
      !std::isfinite(lengthSquared) ||
      lengthSquared <= kMinimumNormalLengthSquared) {
    return result;
  }
  if (!validTargetFacts(request.target)) {
    return reject(result, CreativePlacementCompatibilityStatus::UnknownTarget);
  }

  const double normalY =
      request.surfaceNormal.y / std::sqrt(lengthSquared);
  const bool hosted = hostedTarget(request.target.source);
  const CreativeObjectDescriptor& host = describeObject(request.target.hostKind);
  switch (request.hostPolicy) {
    case CreativePlacementHostPolicy::AnyKnownTarget:
      return allow(result);
    case CreativePlacementHostPolicy::SupportingSurface:
      if (hosted && !solidHost(host)) {
        return reject(result,
                      CreativePlacementCompatibilityStatus::HostDisallowed);
      }
      return normalY >= kSupportingSurfaceMinimumY
                 ? allow(result)
                 : reject(result,
                          CreativePlacementCompatibilityStatus::
                              SurfaceDirectionDisallowed);
    case CreativePlacementHostPolicy::StructuralVerticalSurface:
      if (!hosted) {
        return reject(result,
                      CreativePlacementCompatibilityStatus::SourceDisallowed);
      }
      if (host.category != CreativeObjectCategory::Structural ||
          host.occupancyKind != CreativeSpatialOccupancyKind::Structural) {
        return reject(result,
                      CreativePlacementCompatibilityStatus::HostDisallowed);
      }
      return std::fabs(normalY) <= kVerticalSurfaceMaximumAbsY
                 ? allow(result)
                 : reject(result,
                          CreativePlacementCompatibilityStatus::
                              SurfaceDirectionDisallowed);
    case CreativePlacementHostPolicy::SolidVerticalSurface:
      if (!hosted) {
        return reject(result,
                      CreativePlacementCompatibilityStatus::SourceDisallowed);
      }
      if (!solidHost(host)) {
        return reject(result,
                      CreativePlacementCompatibilityStatus::HostDisallowed);
      }
      return std::fabs(normalY) <= kVerticalSurfaceMaximumAbsY
                 ? allow(result)
                 : reject(result,
                          CreativePlacementCompatibilityStatus::
                              SurfaceDirectionDisallowed);
    case CreativePlacementHostPolicy::SolidSurface:
      if (request.target.source == CreativePlacementTargetSource::EmptyPlane) {
        return reject(result,
                      CreativePlacementCompatibilityStatus::SourceDisallowed);
      }
      if (hosted && !solidHost(host)) {
        return reject(result,
                      CreativePlacementCompatibilityStatus::HostDisallowed);
      }
      return allow(result);
    case CreativePlacementHostPolicy::Count:
      return result;
  }
  return result;
}

std::string_view toString(
    CreativePlacementCompatibilityStatus status) noexcept {
  switch (status) {
    case CreativePlacementCompatibilityStatus::InvalidRequest:
      return "creative_placement_compatibility_invalid";
    case CreativePlacementCompatibilityStatus::UnknownTarget:
      return "creative_placement_target_unknown";
    case CreativePlacementCompatibilityStatus::SourceDisallowed:
      return "creative_placement_target_source_disallowed";
    case CreativePlacementCompatibilityStatus::HostDisallowed:
      return "creative_placement_host_disallowed";
    case CreativePlacementCompatibilityStatus::SurfaceDirectionDisallowed:
      return "creative_placement_surface_direction_disallowed";
    case CreativePlacementCompatibilityStatus::Ready:
      return "creative_placement_compatible";
  }
  return "creative_placement_compatibility_status_invalid";
}

}  // namespace iggy3d::creative
