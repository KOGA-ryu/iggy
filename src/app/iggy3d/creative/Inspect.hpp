#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Tools.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeInspectionChangeKind : std::uint8_t {
  None,
  SetInspectedTarget,
  ClearInspection,
  UpdateCandidate,
};

struct CreativeInspectionState {
  TargetRef inspectedTarget;
  TargetRef candidateTarget;
};

struct CreativeInspectionReceipt {
  CreativeInspectionChangeKind requestedChange =
      CreativeInspectionChangeKind::None;
  CreativeInspectionChangeKind appliedChange =
      CreativeInspectionChangeKind::None;
  TargetRef inspectedTargetBefore;
  TargetRef inspectedTargetAfter;
  TargetRef candidateTargetBefore;
  TargetRef candidateTargetAfter;
  bool changed = false;
  bool accepted = false;
  std::string_view message = "no_inspection_change";
};

[[nodiscard]] CreativeInspectionState makeDefaultCreativeInspectionState() noexcept;
[[nodiscard]] CreativeInspectionReceipt clearInspection(
    CreativeInspectionState& state) noexcept;
[[nodiscard]] CreativeInspectionReceipt setInspectedTarget(
    CreativeInspectionState& state,
    TargetRef target) noexcept;
[[nodiscard]] CreativeInspectionReceipt updateInspectionCandidate(
    CreativeInspectionState& state,
    TargetRef target) noexcept;
[[nodiscard]] CreativeInspectionReceipt applyInspectionToolIntent(
    CreativeInspectionState& state,
    const CreativeToolIntent& intent) noexcept;

}  // namespace iggy3d::creative
