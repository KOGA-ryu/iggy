#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeSelectionChangeKind : std::uint8_t {
  None,
  SetSelectedTarget,
  ReplaceSelectedTargets,
  ToggleSelectedTarget,
  RemoveSelectedTarget,
  ClearSelection,
  UpdateCandidate,
};

struct CreativeSelectionState {
  // selectedTarget is the primary selection retained for existing inspector,
  // move-drag, and receipt readers. selectedTargets owns the full ordered set.
  TargetRef selectedTarget;
  std::vector<TargetRef> selectedTargets;
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
  std::uint64_t selectedCountBefore = 0;
  std::uint64_t selectedCountAfter = 0;
  bool changed = false;
  bool accepted = false;
  std::string_view message = "no_selection_change";
};

[[nodiscard]] CreativeSelectionState makeDefaultCreativeSelectionState() noexcept;
[[nodiscard]] std::uint64_t selectedTargetCount(
    const CreativeSelectionState& state) noexcept;
[[nodiscard]] bool selectionContainsTarget(
    const CreativeSelectionState& state,
    TargetRef target) noexcept;
[[nodiscard]] std::span<const TargetRef> selectedTargetList(
    const CreativeSelectionState& state) noexcept;
[[nodiscard]] CreativeSelectionReceipt clearSelection(
    CreativeSelectionState& state) noexcept;
[[nodiscard]] CreativeSelectionReceipt setSelectedTarget(
    CreativeSelectionState& state,
    TargetRef target);
[[nodiscard]] CreativeSelectionReceipt setSelectedTargets(
    CreativeSelectionState& state,
    std::span<const TargetRef> targets,
    TargetRef primaryTarget = {});
[[nodiscard]] CreativeSelectionReceipt toggleSelectedTarget(
    CreativeSelectionState& state,
    TargetRef target);
[[nodiscard]] CreativeSelectionReceipt removeSelectedTarget(
    CreativeSelectionState& state,
    TargetRef target) noexcept;
[[nodiscard]] CreativeSelectionReceipt updateSelectionCandidate(
    CreativeSelectionState& state,
    TargetRef target) noexcept;
[[nodiscard]] CreativeSelectionReceipt applySelectionToolIntent(
    CreativeSelectionState& state,
    const CreativeToolIntent& intent);

}  // namespace iggy3d::creative
