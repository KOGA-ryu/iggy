#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/document/LogicLink.hpp"

namespace iggy3d::creative {
class CreativeDocument;
struct CreativeAppState;
}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

enum class CreativeEditorLogicLinkStatus : std::uint8_t {
  Idle,
  SourceSelected,
  SourceCleared,
  InvalidSource,
  InvalidTarget,
  Added,
  Updated,
  Removed,
};

struct CreativeEditorLogicLinkState {
  std::uint64_t documentId = 0U;
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeLogicLinkAction action =
      iggy3d::creative::CreativeLogicLinkAction::Toggle;
  CreativeEditorLogicLinkStatus status =
      CreativeEditorLogicLinkStatus::Idle;
  iggy3d::creative::CreativeLogicLinkMutationReceipt lastMutation;
};

struct CreativeEditorLogicLinkReceipt {
  bool accepted = false;
  bool changed = false;
  CreativeEditorLogicLinkStatus status =
      CreativeEditorLogicLinkStatus::Idle;
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId targetObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeLogicLinkAction action =
      iggy3d::creative::CreativeLogicLinkAction::Toggle;
  std::string_view reasonCode = "creative_logic_link_not_requested";
};

void syncCreativeEditorLogicLinkState(
    CreativeEditorLogicLinkState& state,
    const iggy3d::creative::CreativeDocument& document) noexcept;
[[nodiscard]] bool clearCreativeEditorLogicLinkSource(
    CreativeEditorLogicLinkState& state) noexcept;
[[nodiscard]] bool cycleCreativeEditorLogicLinkAction(
    CreativeEditorLogicLinkState& state,
    std::int32_t direction) noexcept;
[[nodiscard]] std::size_t creativeEditorOutgoingLogicLinkCount(
    const iggy3d::creative::CreativeDocument& document,
    iggy3d::creative::CreativeObjectId sourceObjectId) noexcept;
[[nodiscard]] CreativeEditorLogicLinkReceipt
advanceCreativeEditorLogicLink(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorLogicLinkState& state,
    iggy3d::creative::CreativeObjectId targetObjectId,
    std::string_view source);

}  // namespace iggy3d_creative_app
