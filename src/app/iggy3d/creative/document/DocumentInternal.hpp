#pragma once

#include "app/iggy3d/creative/document/Document.hpp"

namespace iggy3d::creative::document_internal {

constexpr CreativeObjectDirtyFlags dirtyFlagValue(
    CreativeObjectDirtyFlag flag) noexcept {
  return static_cast<CreativeObjectDirtyFlags>(flag);
}

constexpr CreativeObjectDirtyFlags documentIdentityDirtyFlags() noexcept {
  return dirtyFlagValue(CreativeObjectDirtyFlag::Identity) |
         dirtyFlagValue(CreativeObjectDirtyFlag::Preview) |
         dirtyFlagValue(CreativeObjectDirtyFlag::Serialization);
}

constexpr CreativeObjectDirtyFlags documentSettingsDirtyFlags() noexcept {
  return dirtyFlagValue(CreativeObjectDirtyFlag::Preview) |
         dirtyFlagValue(CreativeObjectDirtyFlag::Serialization);
}

// Facade replacement may only advance the transient live revision. Durable
// content and dirty state remain owned by the document's normal mutations.
struct CreativeDocumentRevisionAccess {
  [[nodiscard]] static bool rebaseForLiveInstall(
      CreativeDocument& document,
      std::uint64_t revision) noexcept {
    if (!document.isValid() || document.id() == kInvalidDocumentId ||
        revision <= document.revision()) {
      return false;
    }

    document.revision_ = revision;
    return true;
  }
};

[[nodiscard]] std::string_view validateCreatePathPayload(
    const CreativeObjectDescriptor& descriptor,
    const CreativeDocumentCreateRequest& request) noexcept;
[[nodiscard]] std::string_view validateRestoredPathPayload(
    const CreativeObjectDescriptor& descriptor,
    const CreativeObject& object) noexcept;
[[nodiscard]] bool isValidUnits(CreativeUnits units) noexcept;
[[nodiscard]] bool isValidGridSettings(
    CreativeGridSettings settings) noexcept;
[[nodiscard]] bool isValidWorldBounds(CreativeBounds bounds) noexcept;
[[nodiscard]] bool isValidRestoreObject(
    const CreativeObject& object) noexcept;

}  // namespace iggy3d::creative::document_internal
