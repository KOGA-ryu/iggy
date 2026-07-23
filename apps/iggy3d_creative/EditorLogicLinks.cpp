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

void applySelectedSource(cr::CreativeAppState& appState,
                         CreativeEditorLogicLinkState& state,
                         cr::CreativeObjectId objectId) {
  state.sourceObjectId = objectId;
  state.status = CreativeEditorLogicLinkStatus::SourceSelected;
  state.lastMutation = {};
  const std::array ids{objectId};
  static_cast<void>(appState.facade.selectTargets(ids, objectId));
}

[[nodiscard]] CreativeEditorLogicLinkStatus statusForMutation(
    cr::CreativeLogicLinkMutationStatus status) noexcept {
  switch (status) {
    case cr::CreativeLogicLinkMutationStatus::Added:
      return CreativeEditorLogicLinkStatus::Added;
    case cr::CreativeLogicLinkMutationStatus::Updated:
      return CreativeEditorLogicLinkStatus::Updated;
    case cr::CreativeLogicLinkMutationStatus::Removed:
      return CreativeEditorLogicLinkStatus::Removed;
    case cr::CreativeLogicLinkMutationStatus::NoChange:
      return CreativeEditorLogicLinkStatus::Unchanged;
    case cr::CreativeLogicLinkMutationStatus::MissingSource:
    case cr::CreativeLogicLinkMutationStatus::UnsupportedSource:
      return CreativeEditorLogicLinkStatus::InvalidSource;
    case cr::CreativeLogicLinkMutationStatus::NotRequested:
    case cr::CreativeLogicLinkMutationStatus::InvalidDocument:
    case cr::CreativeLogicLinkMutationStatus::InvalidAction:
    case cr::CreativeLogicLinkMutationStatus::MissingTarget:
    case cr::CreativeLogicLinkMutationStatus::UnsupportedTarget:
    case cr::CreativeLogicLinkMutationStatus::MissingLink:
      return CreativeEditorLogicLinkStatus::InvalidTarget;
  }
  return CreativeEditorLogicLinkStatus::InvalidTarget;
}

[[nodiscard]] CreativeEditorLogicLinkReceipt mutationReceipt(
    CreativeEditorLogicLinkState& state,
    cr::CreativeObjectId sourceObjectId,
    cr::CreativeObjectId targetObjectId,
    cr::CreativeLogicLinkAction action) noexcept {
  state.status = statusForMutation(state.lastMutation.status);
  if (state.lastMutation.accepted) {
    state.sourceObjectId = sourceObjectId;
    state.action = action;
  }
  CreativeEditorLogicLinkReceipt receipt;
  receipt.accepted = state.lastMutation.accepted;
  receipt.changed = state.lastMutation.changed;
  receipt.status = state.status;
  receipt.sourceObjectId = sourceObjectId;
  receipt.targetObjectId = targetObjectId;
  receipt.action = action;
  receipt.reasonCode = state.lastMutation.reasonCode;
  return receipt;
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
  const std::span<const cr::CreativeLogicLinkAction> actions =
      cr::creativeLogicLinkActions();
  if (actions.empty()) {
    return false;
  }
  std::size_t index = 0U;
  for (std::size_t candidate = 0U; candidate < actions.size();
       ++candidate) {
    if (actions[candidate] == state.action) {
      index = candidate;
      break;
    }
  }
  const std::int32_t count = static_cast<std::int32_t>(actions.size());
  const std::int32_t stepped =
      (static_cast<std::int32_t>(index) + (direction < 0 ? -1 : 1) + count) %
      count;
  const cr::CreativeLogicLinkAction before = state.action;
  state.action = actions[static_cast<std::size_t>(stepped)];
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

CreativeEditorLogicLinkReceipt selectCreativeEditorLogicLinkSource(
    cr::CreativeAppState& appState,
    CreativeEditorLogicLinkState& state,
    cr::CreativeObjectId sourceObjectId) {
  const cr::CreativeDocument& document = appState.facade.document();
  syncCreativeEditorLogicLinkState(state, document);
  CreativeEditorLogicLinkReceipt receipt;
  receipt.sourceObjectId = sourceObjectId;
  const cr::CreativeObject* source = document.findObject(sourceObjectId);
  if (source == nullptr || !cr::creativeObjectCanSourceLogicLink(source->kind)) {
    state.status = CreativeEditorLogicLinkStatus::InvalidSource;
    state.lastMutation = {};
    receipt.status = state.status;
    receipt.reasonCode = source == nullptr
                             ? "creative_logic_link_source_missing"
                             : "creative_logic_link_source_unsupported";
    return receipt;
  }
  applySelectedSource(appState, state, sourceObjectId);
  receipt.accepted = true;
  receipt.status = state.status;
  receipt.reasonCode = "creative_logic_link_source_selected";
  return receipt;
}

CreativeEditorLogicLinkReceipt setCreativeEditorLogicLink(
    cr::CreativeAppState& appState,
    CreativeEditorLogicLinkState& state,
    cr::CreativeObjectId sourceObjectId,
    cr::CreativeObjectId targetObjectId,
    cr::CreativeLogicLinkAction action,
    std::string_view source) {
  syncCreativeEditorLogicLinkState(state, appState.facade.document());
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  state.lastMutation = appState.facade.setLogicLink(
      {sourceObjectId, targetObjectId, action});
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastMutation.accepted && state.lastMutation.changed,
      state.lastMutation.reasonCode));
  return mutationReceipt(state, sourceObjectId, targetObjectId, action);
}

CreativeEditorLogicLinkReceipt removeCreativeEditorLogicLink(
    cr::CreativeAppState& appState,
    CreativeEditorLogicLinkState& state,
    cr::CreativeObjectId sourceObjectId,
    cr::CreativeObjectId targetObjectId,
    std::string_view source) {
  const cr::CreativeDocument& document = appState.facade.document();
  syncCreativeEditorLogicLinkState(state, document);
  const cr::CreativeLogicLink* existing =
      document.findLogicLink(sourceObjectId, targetObjectId);
  const cr::CreativeLogicLinkAction action =
      existing != nullptr ? existing->action : state.action;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  state.lastMutation =
      appState.facade.removeLogicLink(sourceObjectId, targetObjectId);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      state.lastMutation.accepted && state.lastMutation.changed,
      state.lastMutation.reasonCode));
  return mutationReceipt(state, sourceObjectId, targetObjectId, action);
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
    return selectCreativeEditorLogicLinkSource(appState, state,
                                               targetObjectId);
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

  const cr::CreativeLogicLink* existing =
      document.findLogicLink(state.sourceObjectId, targetObjectId);
  return existing != nullptr && existing->action == state.action
             ? removeCreativeEditorLogicLink(
                   appState, state, state.sourceObjectId, targetObjectId,
                   source)
             : setCreativeEditorLogicLink(
                   appState, state, state.sourceObjectId, targetObjectId,
                   state.action, source);
}

}  // namespace iggy3d_creative_app
