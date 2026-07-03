#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Tools.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeSelectionChangeKind : std::uint8_t {
  None,
  SetSelectedTarget,
  ClearSelection,
  UpdateCandidate,
};

struct CreativeSelectionState {
  TargetRef selectedTarget;
  TargetRef candidateTarget;
};

struct CreativeSelectionReceipt {
  CreativeSelectionChangeKind requestedChange =
      CreativeSelectionChangeKind::None;
  CreativeSelectionChangeKind appliedChange = CreativeSelectionChangeKind::None;
  TargetRef selectedTargetBefore;
  TargetRef selectedTargetAfter;
  TargetRef candidateTargetBefore;
  TargetRef candidateTargetAfter;
  bool changed = false;
  bool accepted = false;
  std::string_view message = "no_selection_change";
};

[[nodiscard]] CreativeSelectionState makeDefaultCreativeSelectionState() noexcept;
[[nodiscard]] CreativeSelectionReceipt clearSelection(
    CreativeSelectionState& state) noexcept;
[[nodiscard]] CreativeSelectionReceipt setSelectedTarget(
    CreativeSelectionState& state,
    TargetRef target) noexcept;
[[nodiscard]] CreativeSelectionReceipt updateSelectionCandidate(
    CreativeSelectionState& state,
    TargetRef target) noexcept;
[[nodiscard]] CreativeSelectionReceipt applySelectionToolIntent(
    CreativeSelectionState& state,
    const CreativeToolIntent& intent) noexcept;

}  // namespace iggy3d::creative
