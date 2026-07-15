#include "EditorLogicLinks.hpp"

#include <array>
#include <span>
#include <utility>

#include "EditorEdits.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::array kEditableActions{
    cr::CreativeLogicLinkAction::Toggle,
    cr::CreativeLogicLinkAction::Open,
    cr::CreativeLogicLinkAction::Close,
};

void selectSource(cr::CreativeAppState& appState,
                  CreativeEditorLogicLinkState& state,
                  cr::CreativeObjectId objectId) {
  state.sourceObjectId = objectId;
  state.status = CreativeEditorLogicLinkStatus::SourceSelected;
  state.lastMutation = {};
  const std::array ids{objectId};
  static_cast<void>(appState.facade.selectTargets(ids, objectId));
}

}  // namespace

void syncCreativeEditorLogicLinkState(
    CreativeEditorLogicLinkState& state,
    const cr::CreativeDocument& document) noexcept {
  if (state.documentId != document.id()) {
    state = {};
    state.documentId = document.id();
    return;
  }
  if (state.sourceObjectId == cr::kInvalidObjectId) {
    return;
  }
  const cr::CreativeObject* source =
      document.findObject(state.sourceObjectId);
  if (source == nullptr || !cr::creativeObjectCanSourceLogicLink(source->kind)) {
    state.sourceObjectId = cr::kInvalidObjectId;
    state.status = CreativeEditorLogicLinkStatus::SourceCleared;
    state.lastMutation = {};
  }
}

bool clearCreativeEditorLogicLinkSource(
    CreativeEditorLogicLinkState& state) noexcept {
  const bool changed = state.sourceObjectId != cr::kInvalidObjectId;
  state.sourceObjectId = cr::kInvalidObjectId;
  state.status = CreativeEditorLogicLinkStatus::SourceCleared;
  state.lastMutation = {};
  return changed;
}

bool cycleCreativeEditorLogicLinkAction(
    CreativeEditorLogicLinkState& state,
    std::int32_t direction) noexcept {
  std::size_t index = 0U;
  for (std::size_t candidate = 0U; candidate < kEditableActions.size();
       ++candidate) {
    if (kEditableActions[candidate] == state.action) {
      index = candidate;
      break;
    }
  }
  const std::int32_t count = static_cast<std::int32_t>(kEditableActions.size());
  const std::int32_t stepped =
      (static_cast<std::int32_t>(index) + (direction < 0 ? -1 : 1) + count) %
      count;
  const cr::CreativeLogicLinkAction before = state.action;
  state.action = kEditableActions[static_cast<std::size_t>(stepped)];
  return state.action != before;
}

std::size_t creativeEditorOutgoingLogicLinkCount(
    const cr::CreativeDocument& document,
    cr::CreativeObjectId sourceObjectId) noexcept {
  std::size_t count = 0U;
  for (const cr::CreativeLogicLink& link : document.logicLinks()) {
    count += link.sourceObjectId == sourceObjectId ? 1U : 0U;
  }
  return count;
}

CreativeEditorLogicLinkReceipt advanceCreativeEditorLogicLink(
    cr::CreativeAppState& appState,
    CreativeEditorLogicLinkState& state,
    cr::CreativeObjectId targetObjectId,
    std::string_view source) {
  const cr::CreativeDocument& document = appState.facade.document();
  syncCreativeEditorLogicLinkState(state, document);
  CreativeEditorLogicLinkReceipt receipt;
  receipt.sourceObjectId = state.sourceObjectId;
  receipt.targetObjectId = targetObjectId;
  receipt.action = state.action;
  const cr::CreativeObject* target = document.findObject(targetObjectId);
  if (target == nullptr) {
    state.status = CreativeEditorLogicLinkStatus::InvalidTarget;
    receipt.status = state.status;
    receipt.reasonCode = "creative_logic_link_target_missing";
    return receipt;
  }
  if (cr::creativeObjectCanSourceLogicLink(target->kind)) {
    selectSource(appState, state, targetObjectId);
    receipt.accepted = true;
    receipt.status = state.status;
    receipt.sourceObjectId = targetObjectId;
    receipt.reasonCode = "creative_logic_link_source_selected";
    return receipt;
  }
  if (state.sourceObjectId == cr::kInvalidObjectId) {
    state.status = CreativeEditorLogicLinkStatus::InvalidSource;
    receipt.status = state.status;
    receipt.reasonCode = "creative_logic_link_source_not_selected";
    return receipt;
  }
  if (!cr::creativeObjectCanTargetLogicLink(target->kind) ||
      !cr::creativeLogicLinkActionSupported(target->kind, state.action)) {
    state.status = CreativeEditorLogicLinkStatus::InvalidTarget;
    receipt.status = state.status;
    receipt.reasonCode = "creative_logic_link_target_unsupported";
    return receipt;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const cr::CreativeLogicLink* existing =
      document.findLogicLink(state.sourceObjectId, targetObjectId);
  state.lastMutation = existing != nullptr && existing->action == state.action
                           ? appState.facade.removeLogicLink(
                                 state.sourceObjectId, targetObjectId)
                           : appState.facade.setLogicLink(
                                 {state.sourceObjectId, targetObjectId,
                                  state.action});
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastMutation.accepted && state.lastMutation.changed,
      state.lastMutation.reasonCode));
  receipt.accepted = state.lastMutation.accepted;
  receipt.changed = state.lastMutation.changed;
  receipt.reasonCode = state.lastMutation.reasonCode;
  switch (state.lastMutation.status) {
    case cr::CreativeLogicLinkMutationStatus::Added:
      state.status = CreativeEditorLogicLinkStatus::Added;
      break;
    case cr::CreativeLogicLinkMutationStatus::Updated:
      state.status = CreativeEditorLogicLinkStatus::Updated;
      break;
    case cr::CreativeLogicLinkMutationStatus::Removed:
      state.status = CreativeEditorLogicLinkStatus::Removed;
      break;
    default:
      state.status = CreativeEditorLogicLinkStatus::InvalidTarget;
      break;
  }
  receipt.status = state.status;
  return receipt;
}

}  // namespace iggy3d_creative_app
