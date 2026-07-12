#pragma once

#include "app/iggy3d/creative/tools/Group.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

inline constexpr std::size_t kCreativeEditorGroupFocusDepth = 8U;

enum class CreativeEditorGroupFocusStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  MissingGroup,
  NotGroup,
  OutsideActiveGroup,
  DepthExceeded,
  Entered,
  Exited,
  NoFocus,
};

struct CreativeEditorGroupFocusState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::array<iggy3d::creative::CreativeObjectId,
             kCreativeEditorGroupFocusDepth>
      groupIds{};
  std::uint8_t depth = 0U;
};

struct CreativeEditorGroupFocusReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorGroupFocusStatus status =
      CreativeEditorGroupFocusStatus::NotRequested;
  iggy3d::creative::CreativeObjectId groupObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::uint8_t depthBefore = 0U;
  std::uint8_t depthAfter = 0U;
  std::string_view reasonCode = "creative_group_focus_not_requested";
};

[[nodiscard]] bool creativeEditorGroupFocusActive(
    const CreativeEditorGroupFocusState& state) noexcept;
[[nodiscard]] iggy3d::creative::CreativeObjectId
activeCreativeEditorGroupFocusId(
    const CreativeEditorGroupFocusState& state) noexcept;

// Truncates stale focus levels after document replacement, undo, or deletion.
[[nodiscard]] bool syncCreativeEditorGroupFocus(
    CreativeEditorGroupFocusState& state,
    const iggy3d::creative::CreativeDocument& document) noexcept;

// Outside focus, a visible child resolves to its outermost Group. Inside
// focus, it resolves to the active Group's immediate child root. Objects
// outside the active hierarchy resolve to kInvalidObjectId. Complexity O(h).
[[nodiscard]] iggy3d::creative::CreativeObjectId
resolveCreativeEditorGroupSelectionTarget(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorGroupFocusState& state,
    iggy3d::creative::CreativeObjectId hitObjectId) noexcept;
[[nodiscard]] bool creativeEditorObjectInsideActiveGroup(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorGroupFocusState& state,
    iggy3d::creative::CreativeObjectId objectId) noexcept;

[[nodiscard]] CreativeEditorGroupFocusReceipt enterCreativeEditorGroupFocus(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorGroupFocusState& state,
    iggy3d::creative::CreativeObjectId groupObjectId);
[[nodiscard]] CreativeEditorGroupFocusReceipt exitCreativeEditorGroupFocus(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorGroupFocusState& state);

[[nodiscard]] iggy3d::creative::CreativeGroupCommandReceipt
applyCreativeEditorGroupCommandWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    std::string_view source);

}  // namespace iggy3d_creative_app
