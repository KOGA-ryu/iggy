#include "EditorObjectActionOutcome.hpp"

#include <array>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

namespace {

[[nodiscard]] std::string rejectedMessage(
    const CreativeEditorObjectActionOutcome& outcome) {
  struct RejectionMessage {
    std::string_view reasonCode;
    std::string_view message;
  };
  static constexpr std::array kMessages{
      RejectionMessage{"creative_editor_object_action_selection_locked",
                       "selection is locked"},
      RejectionMessage{
          "creative_editor_object_action_world_layout_unsynchronized",
          "regenerate World Layout before editing"},
      RejectionMessage{"creative_selection_set_empty",
                       "no objects selected"},
      RejectionMessage{"creative_selection_object_missing",
                       "selected object is unavailable"},
      RejectionMessage{"creative_selection_set_object_missing",
                       "part of the selection is unavailable"},
      RejectionMessage{"creative_semantic_action_mixed_ownership",
                       "selection contains incompatible object owners"},
      RejectionMessage{"creative_semantic_action_pattern_owned",
                       "edit this object through its pattern"},
      RejectionMessage{"creative_semantic_action_world_layout_owned",
                       "edit this object through World Layout"},
      RejectionMessage{
          "creative_editor_transform_pattern_requires_transform_tool",
          "use Transform Selection for this pattern"},
      RejectionMessage{
          "creative_editor_transform_world_layout_use_transform_tool",
          "use Transform Selection for source-owned geometry"},
      RejectionMessage{"no_targets", "no objects selected"},
      RejectionMessage{"no_delete_targets", "no objects selected"},
      RejectionMessage{"missing_object", "object is unavailable"},
      RejectionMessage{"invalid_transform", "transform values are invalid"},
      RejectionMessage{"no_transform_components",
                       "no transform components selected"},
  };
  for (const RejectionMessage& candidate : kMessages) {
    if (candidate.reasonCode == outcome.reasonCode) {
      return std::string(candidate.message);
    }
  }
  return outcome.reasonCode.empty() ? "object action rejected"
                                    : outcome.reasonCode;
}

[[nodiscard]] std::string payloadMismatchMessage(
    const CreativeEditorObjectActionOutcome& outcome) {
  switch (outcome.action) {
    case cr::CreativeSemanticObjectAction::Delete:
      return outcome.target == CreativeEditorObjectActionTarget::Objects
                 ? "delete objects: payload mismatch"
                 : "delete: payload mismatch";
    case cr::CreativeSemanticObjectAction::Rename:
      return "rename: payload mismatch";
    case cr::CreativeSemanticObjectAction::SetVisible:
      return "visibility: payload mismatch";
    case cr::CreativeSemanticObjectAction::SetLocked:
      return "lock: payload mismatch";
    case cr::CreativeSemanticObjectAction::SetTransform:
    case cr::CreativeSemanticObjectAction::TransformSelection:
      return "transform: payload mismatch";
    case cr::CreativeSemanticObjectAction::Inspect:
    case cr::CreativeSemanticObjectAction::Copy:
    case cr::CreativeSemanticObjectAction::Duplicate:
    case cr::CreativeSemanticObjectAction::Cut:
    case cr::CreativeSemanticObjectAction::StructuralMutation:
    case cr::CreativeSemanticObjectAction::Count:
      return "object action: payload mismatch";
  }
  return "object action: payload mismatch";
}

[[nodiscard]] std::string invalidRequestMessage(
    const CreativeEditorObjectActionOutcome& outcome) {
  if (outcome.reasonCode ==
      "creative_editor_transform_container_requires_selection_transform") {
    return "transform complete hierarchy through Transform Selection";
  }
  switch (outcome.action) {
    case cr::CreativeSemanticObjectAction::Rename:
      return "rename: invalid request";
    case cr::CreativeSemanticObjectAction::SetTransform:
    case cr::CreativeSemanticObjectAction::TransformSelection:
      return "transform: invalid request";
    case cr::CreativeSemanticObjectAction::Inspect:
    case cr::CreativeSemanticObjectAction::Copy:
    case cr::CreativeSemanticObjectAction::Duplicate:
    case cr::CreativeSemanticObjectAction::Delete:
    case cr::CreativeSemanticObjectAction::Cut:
    case cr::CreativeSemanticObjectAction::SetVisible:
    case cr::CreativeSemanticObjectAction::SetLocked:
    case cr::CreativeSemanticObjectAction::StructuralMutation:
    case cr::CreativeSemanticObjectAction::Count:
      return rejectedMessage(outcome);
  }
  return rejectedMessage(outcome);
}

[[nodiscard]] std::string unchangedMessage(
    const CreativeEditorObjectActionOutcome& outcome) {
  switch (outcome.action) {
    case cr::CreativeSemanticObjectAction::Duplicate:
      return "nothing to duplicate";
    case cr::CreativeSemanticObjectAction::SetTransform:
    case cr::CreativeSemanticObjectAction::TransformSelection:
      return "transform unchanged";
    case cr::CreativeSemanticObjectAction::Rename:
      return "renamed object";
    case cr::CreativeSemanticObjectAction::Delete:
    case cr::CreativeSemanticObjectAction::SetVisible:
    case cr::CreativeSemanticObjectAction::SetLocked:
      return rejectedMessage(outcome);
    case cr::CreativeSemanticObjectAction::Inspect:
    case cr::CreativeSemanticObjectAction::Copy:
    case cr::CreativeSemanticObjectAction::Cut:
    case cr::CreativeSemanticObjectAction::StructuralMutation:
    case cr::CreativeSemanticObjectAction::Count:
      return rejectedMessage(outcome);
  }
  return rejectedMessage(outcome);
}

[[nodiscard]] std::string appliedMessage(
    const CreativeEditorObjectActionOutcome& outcome) {
  switch (outcome.action) {
    case cr::CreativeSemanticObjectAction::Duplicate:
      return "duplicated selection";
    case cr::CreativeSemanticObjectAction::Delete:
      return outcome.target == CreativeEditorObjectActionTarget::Objects
                 ? "deleted objects"
                 : "deleted selection";
    case cr::CreativeSemanticObjectAction::Rename:
      return "renamed object";
    case cr::CreativeSemanticObjectAction::SetVisible:
      return "visibility updated";
    case cr::CreativeSemanticObjectAction::SetLocked:
      return "lock updated";
    case cr::CreativeSemanticObjectAction::SetTransform:
    case cr::CreativeSemanticObjectAction::TransformSelection:
      return "transform set";
    case cr::CreativeSemanticObjectAction::Inspect:
    case cr::CreativeSemanticObjectAction::Copy:
    case cr::CreativeSemanticObjectAction::Cut:
    case cr::CreativeSemanticObjectAction::StructuralMutation:
    case cr::CreativeSemanticObjectAction::Count:
      return rejectedMessage(outcome);
  }
  return rejectedMessage(outcome);
}

}  // namespace

CreativeEditorObjectActionOutcome makeCreativeEditorObjectActionOutcome(
    cr::CreativeSemanticObjectAction action,
    CreativeEditorObjectActionTarget target,
    bool accepted,
    bool changed,
    std::uint64_t affectedObjectCount,
    std::string_view reasonCode,
    bool requiresAdoption) {
  CreativeEditorObjectActionOutcome outcome;
  outcome.action = action;
  outcome.target = target;
  outcome.affectedObjectCount = affectedObjectCount;
  outcome.reasonCode = reasonCode;
  if (!accepted) {
    outcome.status = CreativeEditorObjectActionOutcomeStatus::Rejected;
  } else if (changed && requiresAdoption) {
    outcome.status =
        CreativeEditorObjectActionOutcomeStatus::AppliedRequiresAdoption;
  } else if (changed) {
    outcome.status = CreativeEditorObjectActionOutcomeStatus::Applied;
  } else {
    outcome.status = CreativeEditorObjectActionOutcomeStatus::Unchanged;
  }
  return outcome;
}

CreativeEditorObjectActionOutcome rejectCreativeEditorObjectAction(
    cr::CreativeSemanticObjectAction action,
    CreativeEditorObjectActionTarget target,
    CreativeEditorObjectActionOutcomeStatus status,
    std::string_view reasonCode) {
  CreativeEditorObjectActionOutcome outcome;
  outcome.action = action;
  outcome.target = target;
  outcome.status = status;
  outcome.reasonCode = reasonCode;
  return outcome;
}

bool creativeEditorObjectActionOutcomeValid(
    const CreativeEditorObjectActionOutcome& outcome) noexcept {
  if (!creativeEditorObjectActionOutcomeRequested(outcome)) {
    return outcome.action == cr::CreativeSemanticObjectAction::Count &&
           outcome.target == CreativeEditorObjectActionTarget::None &&
           outcome.affectedObjectCount == 0U &&
           outcome.reasonCode.empty();
  }
  if (cr::creativeSemanticObjectActionDescriptor(outcome.action) == nullptr ||
      outcome.target == CreativeEditorObjectActionTarget::None) {
    return false;
  }
  if (outcome.status ==
          CreativeEditorObjectActionOutcomeStatus::PayloadMismatch ||
      outcome.status ==
          CreativeEditorObjectActionOutcomeStatus::InvalidRequest ||
      outcome.status == CreativeEditorObjectActionOutcomeStatus::Rejected) {
    return outcome.affectedObjectCount == 0U;
  }
  if (outcome.status ==
      CreativeEditorObjectActionOutcomeStatus::AppliedRequiresAdoption) {
    return outcome.action == cr::CreativeSemanticObjectAction::SetTransform &&
           outcome.affectedObjectCount > 0U;
  }
  return outcome.status ==
             CreativeEditorObjectActionOutcomeStatus::Unchanged ||
         (outcome.status ==
              CreativeEditorObjectActionOutcomeStatus::Applied &&
          outcome.affectedObjectCount > 0U);
}

std::string_view toString(
    CreativeEditorObjectActionOutcomeStatus status) noexcept {
  switch (status) {
    case CreativeEditorObjectActionOutcomeStatus::NotRequested:
      return "not_requested";
    case CreativeEditorObjectActionOutcomeStatus::PayloadMismatch:
      return "payload_mismatch";
    case CreativeEditorObjectActionOutcomeStatus::InvalidRequest:
      return "invalid_request";
    case CreativeEditorObjectActionOutcomeStatus::Rejected:
      return "rejected";
    case CreativeEditorObjectActionOutcomeStatus::Unchanged:
      return "unchanged";
    case CreativeEditorObjectActionOutcomeStatus::Applied:
      return "applied";
    case CreativeEditorObjectActionOutcomeStatus::AppliedRequiresAdoption:
      return "applied_requires_adoption";
  }
  return "unknown";
}

std::string formatCreativeEditorObjectActionOutcome(
    const CreativeEditorObjectActionOutcome& outcome) {
  switch (outcome.status) {
    case CreativeEditorObjectActionOutcomeStatus::NotRequested:
      return {};
    case CreativeEditorObjectActionOutcomeStatus::PayloadMismatch:
      return payloadMismatchMessage(outcome);
    case CreativeEditorObjectActionOutcomeStatus::InvalidRequest:
      return invalidRequestMessage(outcome);
    case CreativeEditorObjectActionOutcomeStatus::Rejected:
      return rejectedMessage(outcome);
    case CreativeEditorObjectActionOutcomeStatus::Unchanged:
      return unchangedMessage(outcome);
    case CreativeEditorObjectActionOutcomeStatus::Applied:
      return appliedMessage(outcome);
    case CreativeEditorObjectActionOutcomeStatus::AppliedRequiresAdoption:
      return "transform set; adopt 3D edit";
  }
  return {};
}

}  // namespace iggy3d_creative_app
