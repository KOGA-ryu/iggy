#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <algorithm>
#include <array>

namespace iggy3d::creative {
namespace {

[[nodiscard]] const CreativeObject* findObject(
    std::span<const CreativeObject> objects,
    CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      objects.begin(), objects.end(),
      [objectId](const CreativeObject& object) { return object.id == objectId; });
  return found == objects.end() ? nullptr : &*found;
}

template <typename FindObject>
[[nodiscard]] CreativeObjectHierarchyState resolveHierarchyState(
    CreativeObjectId objectId, FindObject&& findObjectById) noexcept {
  CreativeObjectHierarchyState result;
  result.requested = true;
  result.objectId = objectId;
  const CreativeObject* object = findObjectById(objectId);
  if (object == nullptr) {
    result.status = CreativeObjectHierarchyStateStatus::MissingObject;
    result.reasonCode = "creative_hierarchy_state_object_missing";
    return result;
  }

  result.effectivelyVisible = true;
  result.effectivelyLocked = false;
  std::array<CreativeObjectId, kCreativeHierarchyDepthCapacity + 1U> visited{};
  std::size_t visitedCount = 0U;
  const CreativeObject* current = object;
  while (current != nullptr) {
    if (std::find(visited.begin(), visited.begin() + visitedCount,
                  current->id) != visited.begin() + visitedCount) {
      result.status = CreativeObjectHierarchyStateStatus::Cycle;
      result.effectivelyVisible = false;
      result.effectivelyLocked = true;
      result.reasonCode = "creative_hierarchy_state_cycle";
      return result;
    }
    if (visitedCount >= visited.size()) {
      result.status = CreativeObjectHierarchyStateStatus::DepthExceeded;
      result.effectivelyVisible = false;
      result.effectivelyLocked = true;
      result.reasonCode = "creative_hierarchy_state_depth_exceeded";
      return result;
    }
    visited[visitedCount++] = current->id;

    if (!current->visible && result.hiddenByObjectId == kInvalidObjectId) {
      result.effectivelyVisible = false;
      result.hiddenByObjectId = current->id;
    }
    if (current->locked && result.lockedByObjectId == kInvalidObjectId) {
      result.effectivelyLocked = true;
      result.lockedByObjectId = current->id;
    }
    if (!current->parentId.has_value()) {
      result.resolved = true;
      result.status = CreativeObjectHierarchyStateStatus::Resolved;
      result.depth = visitedCount - 1U;
      result.reasonCode = "creative_hierarchy_state_resolved";
      return result;
    }
    current = findObjectById(*current->parentId);
    if (current == nullptr) {
      result.status = CreativeObjectHierarchyStateStatus::MissingParent;
      result.effectivelyVisible = false;
      result.effectivelyLocked = true;
      result.reasonCode = "creative_hierarchy_state_parent_missing";
      return result;
    }
  }

  result.status = CreativeObjectHierarchyStateStatus::MissingParent;
  result.effectivelyVisible = false;
  result.effectivelyLocked = true;
  result.reasonCode = "creative_hierarchy_state_parent_missing";
  return result;
}

}  // namespace

std::string_view toString(CreativeObjectHierarchyStateStatus status) noexcept {
  switch (status) {
    case CreativeObjectHierarchyStateStatus::NotRequested:
      return "NotRequested";
    case CreativeObjectHierarchyStateStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeObjectHierarchyStateStatus::MissingObject:
      return "MissingObject";
    case CreativeObjectHierarchyStateStatus::MissingParent:
      return "MissingParent";
    case CreativeObjectHierarchyStateStatus::Cycle: return "Cycle";
    case CreativeObjectHierarchyStateStatus::DepthExceeded:
      return "DepthExceeded";
    case CreativeObjectHierarchyStateStatus::Resolved: return "Resolved";
  }
  return "Unknown";
}

CreativeObjectHierarchyState resolveCreativeObjectHierarchyState(
    std::span<const CreativeObject> objects,
    CreativeObjectId objectId) noexcept {
  return resolveHierarchyState(objectId, [objects](CreativeObjectId id) {
    return findObject(objects, id);
  });
}

CreativeObjectHierarchyState resolveCreativeObjectHierarchyState(
    const CreativeDocument& document,
    CreativeObjectId objectId) noexcept {
  if (!document.isValid()) {
    CreativeObjectHierarchyState result;
    result.requested = true;
    result.objectId = objectId;
    result.status = CreativeObjectHierarchyStateStatus::InvalidDocument;
    result.reasonCode = "creative_hierarchy_state_document_invalid";
    return result;
  }
  return resolveHierarchyState(objectId, [&document](CreativeObjectId id) {
    return document.findObject(id);
  });
}

bool creativeObjectEffectivelyVisible(const CreativeDocument& document,
                                      CreativeObjectId objectId) noexcept {
  const CreativeObjectHierarchyState state =
      resolveCreativeObjectHierarchyState(document, objectId);
  return state.resolved && state.effectivelyVisible;
}

bool creativeObjectEffectivelyLocked(const CreativeDocument& document,
                                     CreativeObjectId objectId) noexcept {
  const CreativeObjectHierarchyState state =
      resolveCreativeObjectHierarchyState(document, objectId);
  return !state.resolved || state.effectivelyLocked;
}

}  // namespace iggy3d::creative
