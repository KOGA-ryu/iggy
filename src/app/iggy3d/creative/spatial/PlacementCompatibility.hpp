#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

namespace iggy3d::creative {

enum class CreativePlacementTargetSource : std::uint8_t {
  Unknown,
  EmptyPlane,
  Terrain,
  Voxel,
  AuthoredObject,
  Count,
};

struct CreativePlacementTargetFacts {
  CreativePlacementTargetSource source =
      CreativePlacementTargetSource::Unknown;
  CreativeObjectKind hostKind = CreativeObjectKind::Unknown;
  CreativeObjectId hostObjectId = kInvalidObjectId;
  bool valid = false;
};

enum class CreativePlacementCompatibilityStatus : std::uint8_t {
  InvalidRequest,
  UnknownTarget,
  SourceDisallowed,
  HostDisallowed,
  SurfaceDirectionDisallowed,
  Ready,
};

struct CreativePlacementCompatibilityRequest {
  CreativePlacementHostPolicy hostPolicy =
      CreativePlacementHostPolicy::AnyKnownTarget;
  CreativePlacementTargetFacts target{};
  CreativeVec3 surfaceNormal{};
};

struct CreativePlacementCompatibilityResult {
  CreativePlacementCompatibilityStatus status =
      CreativePlacementCompatibilityStatus::InvalidRequest;
  CreativePlacementHostPolicy hostPolicy =
      CreativePlacementHostPolicy::AnyKnownTarget;
  CreativePlacementTargetSource source =
      CreativePlacementTargetSource::Unknown;
  CreativeObjectKind hostKind = CreativeObjectKind::Unknown;
  CreativeObjectId hostObjectId = kInvalidObjectId;
  bool evaluated = false;
  bool allowed = false;
};

static_assert(std::is_trivially_copyable_v<CreativePlacementTargetFacts>);
static_assert(std::is_standard_layout_v<CreativePlacementTargetFacts>);
static_assert(
    std::is_trivially_copyable_v<CreativePlacementCompatibilityRequest>);
static_assert(std::is_standard_layout_v<CreativePlacementCompatibilityRequest>);
static_assert(
    std::is_trivially_copyable_v<CreativePlacementCompatibilityResult>);
static_assert(std::is_standard_layout_v<CreativePlacementCompatibilityResult>);

[[nodiscard]] CreativePlacementTargetFacts makeCreativePlacementTargetFacts(
    CreativePlacementTargetSource source,
    CreativeObjectKind hostKind = CreativeObjectKind::Unknown,
    CreativeObjectId hostObjectId = kInvalidObjectId) noexcept;
[[nodiscard]] CreativePlacementCompatibilityResult
resolveCreativePlacementCompatibility(
    const CreativePlacementCompatibilityRequest& request) noexcept;
[[nodiscard]] std::string_view toString(
    CreativePlacementCompatibilityStatus status) noexcept;

}  // namespace iggy3d::creative
