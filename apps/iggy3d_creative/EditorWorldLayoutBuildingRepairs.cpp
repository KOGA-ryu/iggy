#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

#include <string>
#include <utility>

namespace iggy3d_creative_app {

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingRepair(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeWorldLayoutBuildingUsabilityIssue& issue) {
  cr::CreativeWorldLayoutBuildingUsabilityConfig config;
  config.gridCellSizeMeters = grid.cellSizeMeters;
  cr::CreativeWorldLayoutBuildingRepairResult repaired =
      cr::planCreativeWorldLayoutBuildingRepair(
          {&state.source, issue, config, state.nextStableOrdinal});
  if (!repaired.accepted || !repaired.changed) {
    state.statusMessage =
        repaired.status ==
                cr::CreativeWorldLayoutBuildingRepairStatus::UnsupportedIssue
            ? "This issue needs an explicit creator edit"
        : repaired.status == cr::CreativeWorldLayoutBuildingRepairStatus::
                                NoValidCandidate
            ? "No safe automatic repair fits this building"
            : "Building repair is stale or invalid";
    return {false, false, std::string(repaired.reasonCode)};
  }

  state.source = std::move(repaired.edited);
  state.nextStableOrdinal = repaired.nextStableOrdinal;
  if (repaired.resultTable == cr::CreativeWorldLayoutTable::Opening &&
      repaired.resultIndex < state.source.openings.size()) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                       repaired.resultIndex};
    const cr::CreativeWorldLayoutOpeningHostFrame host =
        cr::resolveCreativeWorldLayoutOpeningHost(
            state.source, state.source.openings[repaired.resultIndex]);
    if (host.accepted && host.levelIndex < state.source.levels.size()) {
      state.activeLevelIndex = host.levelIndex;
    }
  }

  std::string message;
  switch (repaired.operation) {
    case cr::CreativeWorldLayoutBuildingRepairOperation::AddExteriorEntrance:
      message = "Exterior entrance added";
      break;
    case cr::CreativeWorldLayoutBuildingRepairOperation::ConnectRoom:
      message = "Room connection added";
      break;
    case cr::CreativeWorldLayoutBuildingRepairOperation::
        ExpandOpeningClearance:
      message = "Door clearance repaired";
      break;
    case cr::CreativeWorldLayoutBuildingRepairOperation::None:
    case cr::CreativeWorldLayoutBuildingRepairOperation::Count:
      message = "Building repaired";
      break;
  }
  detail::noteWorldLayoutSourceChange(state, message);
  return {true, true, std::string(repaired.reasonCode)};
}

}  // namespace iggy3d_creative_app
