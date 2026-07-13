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
