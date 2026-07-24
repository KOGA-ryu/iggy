#pragma once

#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {

enum class CreativeEditorObjectActionTarget : std::uint8_t {
  None,
  Selection,
  Object,
  Objects,
};

enum class CreativeEditorObjectActionOutcomeStatus : std::uint8_t {
  NotRequested,
  PayloadMismatch,
  InvalidRequest,
  Rejected,
  Unchanged,
  Applied,
  AppliedRequiresAdoption,
};

// Typed bridge between object-action receipts and presentation. It carries no
// redundant accepted/changed booleans: those facts are derived from status so
// command and UI surfaces cannot report contradictory outcomes.
struct CreativeEditorObjectActionOutcome {
  iggy3d::creative::CreativeSemanticObjectAction action =
      iggy3d::creative::CreativeSemanticObjectAction::Count;
  CreativeEditorObjectActionTarget target =
      CreativeEditorObjectActionTarget::None;
  CreativeEditorObjectActionOutcomeStatus status =
      CreativeEditorObjectActionOutcomeStatus::NotRequested;
  std::uint64_t affectedObjectCount = 0U;
  std::string reasonCode;
};

[[nodiscard]] constexpr bool creativeEditorObjectActionOutcomeRequested(
    const CreativeEditorObjectActionOutcome& outcome) noexcept {
  return outcome.status !=
         CreativeEditorObjectActionOutcomeStatus::NotRequested;
}

[[nodiscard]] constexpr bool creativeEditorObjectActionOutcomeAccepted(
    const CreativeEditorObjectActionOutcome& outcome) noexcept {
  return outcome.status == CreativeEditorObjectActionOutcomeStatus::Unchanged ||
         outcome.status == CreativeEditorObjectActionOutcomeStatus::Applied ||
         outcome.status ==
             CreativeEditorObjectActionOutcomeStatus::
                 AppliedRequiresAdoption;
}

[[nodiscard]] constexpr bool creativeEditorObjectActionOutcomeChanged(
    const CreativeEditorObjectActionOutcome& outcome) noexcept {
  return outcome.status == CreativeEditorObjectActionOutcomeStatus::Applied ||
         outcome.status ==
             CreativeEditorObjectActionOutcomeStatus::
                 AppliedRequiresAdoption;
}

[[nodiscard]] CreativeEditorObjectActionOutcome
makeCreativeEditorObjectActionOutcome(
    iggy3d::creative::CreativeSemanticObjectAction action,
    CreativeEditorObjectActionTarget target,
    bool accepted,
    bool changed,
    std::uint64_t affectedObjectCount,
    std::string_view reasonCode,
    bool requiresAdoption = false);

[[nodiscard]] CreativeEditorObjectActionOutcome
rejectCreativeEditorObjectAction(
    iggy3d::creative::CreativeSemanticObjectAction action,
    CreativeEditorObjectActionTarget target,
    CreativeEditorObjectActionOutcomeStatus status,
    std::string_view reasonCode);

[[nodiscard]] bool creativeEditorObjectActionOutcomeValid(
    const CreativeEditorObjectActionOutcome& outcome) noexcept;

[[nodiscard]] std::string_view toString(
    CreativeEditorObjectActionOutcomeStatus status) noexcept;

// Presentation is intentionally the only owner of human-facing text. Kernels
// and dispatch paths carry status plus stable reason codes.
[[nodiscard]] std::string formatCreativeEditorObjectActionOutcome(
    const CreativeEditorObjectActionOutcome& outcome);

}  // namespace iggy3d_creative_app
