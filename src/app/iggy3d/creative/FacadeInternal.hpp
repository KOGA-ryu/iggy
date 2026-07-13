#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

#include <limits>

namespace iggy3d::creative {

struct CreativeGhostState;
struct CreativeMeasurementState;
struct CreativeSelectionState;
struct CreativeToolState;
struct State;
struct Stats;

}  // namespace iggy3d::creative

namespace iggy3d::creative::facade_internal {

void resetStats(Stats& stats) noexcept;
void recordCommandAttempt(Stats& stats) noexcept;
void recordCommandSuccess(Stats& stats) noexcept;
void recordCommandFailure(Stats& stats) noexcept;
void recordObjectCreated(Stats& stats) noexcept;
void recordRoomCreated(Stats& stats) noexcept;

void invalidateRemovedObjectEditorState(
    CreativeObjectId objectId,
    State& state,
    CreativeToolState& toolState,
    CreativeSelectionState& selectionState,
    CreativeMeasurementState& measurementState,
    CreativeGhostState& ghostState) noexcept;

// branch-gate-relocation: BG-1228 from=src/app/iggy3d/creative/Facade.cpp
[[nodiscard]] inline bool targetRefToObjectId(
    TargetRef target, CreativeObjectId& objectId) noexcept {
  if (target.value == kInvalidId) {
    return false;
  }

  if constexpr (std::numeric_limits<Id>::max() >
                std::numeric_limits<CreativeObjectId>::max()) {
    if (target.value > std::numeric_limits<CreativeObjectId>::max()) {
      return false;
    }
  }

  objectId = static_cast<CreativeObjectId>(target.value);
  return objectId != kInvalidObjectId;
}

[[nodiscard]] inline TargetRef objectIdToTargetRef(
    CreativeObjectId objectId) noexcept {
  if (objectId == kInvalidObjectId ||
      objectId > std::numeric_limits<Id>::max()) {
    return {};
  }

  return TargetRef{static_cast<Id>(objectId)};
}

}  // namespace iggy3d::creative::facade_internal
