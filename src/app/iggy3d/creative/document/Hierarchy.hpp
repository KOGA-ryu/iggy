#pragma once

#include "app/iggy3d/creative/document/Document.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeHierarchyDepthCapacity = 8U;

enum class CreativeObjectHierarchyStateStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  MissingObject,
  MissingParent,
  Cycle,
  DepthExceeded,
  Resolved,
};

struct CreativeObjectHierarchyState {
  bool requested = false;
  bool resolved = false;
  bool effectivelyVisible = false;
  bool effectivelyLocked = true;
  CreativeObjectHierarchyStateStatus status =
      CreativeObjectHierarchyStateStatus::NotRequested;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectId hiddenByObjectId = kInvalidObjectId;
  CreativeObjectId lockedByObjectId = kInvalidObjectId;
  std::size_t depth = 0U;
  std::string_view reasonCode = "creative_hierarchy_state_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeObjectHierarchyStateStatus status) noexcept;

// Resolves local flags through the complete parent chain. Unresolved hierarchy
// state fails closed: callers must not render or mutate an object whose parent
// contract cannot be proven.
[[nodiscard]] CreativeObjectHierarchyState resolveCreativeObjectHierarchyState(
    const CreativeDocument& document,
    CreativeObjectId objectId) noexcept;
[[nodiscard]] CreativeObjectHierarchyState resolveCreativeObjectHierarchyState(
    std::span<const CreativeObject> objects,
    CreativeObjectId objectId) noexcept;
[[nodiscard]] bool creativeObjectEffectivelyVisible(
    const CreativeDocument& document,
    CreativeObjectId objectId) noexcept;
[[nodiscard]] bool creativeObjectEffectivelyLocked(
    const CreativeDocument& document,
    CreativeObjectId objectId) noexcept;

}  // namespace iggy3d::creative
