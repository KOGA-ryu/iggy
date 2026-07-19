#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutHistory.hpp"

#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutAdoption.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

namespace {

[[nodiscard]] bool applyWorldLayoutSourceSelection(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutObjectProvenance provenance) {
  CreativeEditorWorldLayoutSelectionKind kind =
      CreativeEditorWorldLayoutSelectionKind::None;
  switch (provenance.table) {
    case cr::CreativeWorldLayoutTable::Building:
      kind = CreativeEditorWorldLayoutSelectionKind::Building;
      break;
    case cr::CreativeWorldLayoutTable::Level:
      return false;
    case cr::CreativeWorldLayoutTable::Room:
      kind = CreativeEditorWorldLayoutSelectionKind::Room;
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      kind = CreativeEditorWorldLayoutSelectionKind::VerticalConnector;
      break;
    case cr::CreativeWorldLayoutTable::Box:
      kind = CreativeEditorWorldLayoutSelectionKind::Box;
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      kind = CreativeEditorWorldLayoutSelectionKind::Wall;
      break;
    case cr::CreativeWorldLayoutTable::Opening:
      kind = CreativeEditorWorldLayoutSelectionKind::Opening;
      break;
    case cr::CreativeWorldLayoutTable::Object:
      kind = CreativeEditorWorldLayoutSelectionKind::Object;
      break;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  if (!provenance.owned ||
      provenance.index == cr::kInvalidCreativeWorldLayoutIndex) {
    return false;
  }
  state.selection = {kind, provenance.index};
  if (kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      provenance.index < state.source.rooms.size()) {
    state.activeLevelIndex =
        state.source.rooms[provenance.index].levelIndex;
  } else if (kind == CreativeEditorWorldLayoutSelectionKind::Building) {
    repairCreativeEditorWorldLayoutActiveLevel(state, provenance.index);
  } else if (kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             provenance.index < state.source.verticalConnectors.size()) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        state.source.verticalConnectors[provenance.index];
    if (connector.lowerRoomIndex < state.source.rooms.size()) {
      state.activeLevelIndex =
          state.source.rooms[connector.lowerRoomIndex].levelIndex;
    }
  } else if (kind == CreativeEditorWorldLayoutSelectionKind::Opening &&
             provenance.index < state.source.openings.size()) {
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[provenance.index];
    if (opening.hostKind ==
            cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomIndex < state.source.rooms.size()) {
      state.activeLevelIndex =
          state.source.rooms[opening.roomIndex].levelIndex;
    }
  }
  state.statusMessage = provenance.contributorCount > 1U
                            ? "condensed generated wall selected"
                            : "generated layout source selected";
  return true;
}

}  // namespace

bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object) {
  if (state.generatedRevision != state.revision) {
    return false;
  }
  const cr::CreativeWorldLayoutObjectProvenance provenance =
      cr::resolveCreativeWorldLayoutObjectProvenance(state.source, object);
  return applyWorldLayoutSourceSelection(state, provenance);
}

CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object) {
  if (state.generatedRevision != state.revision) {
    state.statusMessage = "generate pending 2D edits before focusing output";
    return {false, false,
            "creative_editor_world_layout_object_source_stale"};
  }
  const cr::CreativeWorldLayoutObjectProvenance provenance =
      cr::resolveCreativeWorldLayoutObjectProvenance(state.source, object);
  if (!provenance.owned ||
      provenance.index == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = "selected object has no World Layout source";
    return {false, false,
            "creative_editor_world_layout_object_source_missing"};
  }
  return focusCreativeEditorWorldLayoutSource(
      state, provenance.table, provenance.index);
}

CreativeEditorWorldLayoutAdoptionReceipt
adoptCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    cr::CreativeObjectId objectId) {
  CreativeEditorWorldLayoutAdoptionReceipt result;
  if (state.generatedRevision != state.revision) {
    result.reasonCode =
        "creative_editor_world_layout_adoption_source_stale";
    state.statusMessage = "generate pending 2D edits before adopting 3D";
    return result;
  }
  const cr::CreativeObject* object = appState.facade.findObject(objectId);
  if (object == nullptr) {
    result.reasonCode =
        "creative_editor_world_layout_adoption_object_missing";
    state.statusMessage = "selected 3D object no longer exists";
    return result;
  }

  cr::CreativeWorldLayoutAdoptionResult adoption =
      cr::planCreativeWorldLayoutObjectAdoption(
          state.source, *object, appState.facade.document().gridSettings());
  result.table = adoption.table;
  result.index = adoption.index;
  if (!adoption.accepted) {
    result.reasonCode = std::string(adoption.reasonCode);
    state.statusMessage =
        adoption.status == cr::CreativeWorldLayoutAdoptionStatus::UnsupportedSource
            ? "edit this generated fragment from its 2D source"
            : std::string(adoption.reasonCode);
    return result;
  }
  if (adoption.table == cr::CreativeWorldLayoutTable::Box) {
    const cr::CreativeWorldLayoutBox& box =
        adoption.candidate.boxes[adoption.index];
    const cr::CreativeWorldLayoutBuilding& building =
        adoption.candidate.buildings[box.buildingIndex];
    if (building.rootMode == cr::CreativeBuildingRootMode::CreateRoom) {
      const cr::CreativeObject* parent =
          object->parentId.has_value()
              ? appState.facade.findObject(*object->parentId)
              : nullptr;
      const std::string instanceKey = adoption.candidate.stableKey + "." +
                                      building.stableKey;
      if (parent == nullptr ||
          !cr::creativeRecipeObjectHasInstanceProvenance(
              *parent, cr::CreativeRecipeKind::Building, instanceKey,
              cr::CreativeRecipeObjectRole::Source, "root")) {
        result.reasonCode =
            "creative_editor_world_layout_adoption_parent_unsupported";
        state.statusMessage =
            "3D edit changed ownership the 2D source cannot represent";
        return result;
      }
    }
  }
  if (!adoption.changed) {
    const CreativeEditorWorldLayoutEditReceipt focused =
        focusCreativeEditorWorldLayoutSource(state, adoption.table,
                                             adoption.index);
    result.accepted = focused.accepted;
    result.reasonCode = focused.accepted
                            ? "creative_editor_world_layout_adoption_no_change"
                            : focused.reasonCode;
    state.statusMessage = focused.accepted ? "3D output already matches source"
                                           : state.statusMessage;
    return result;
  }
  if (state.revision == std::numeric_limits<std::uint64_t>::max()) {
    result.reasonCode =
        "creative_editor_world_layout_adoption_revision_overflow";
    state.statusMessage = "World Layout revision is exhausted";
    return result;
  }

  const cr::CreativeWorldLayoutTerrainReconciliationResult terrain =
      reconcileCreativeEditorWorldLayoutTerrain(
          state, appState.facade.document(), adoption.candidate);
  if (!terrain.accepted) {
    result.reasonCode = terrain.reasonCode;
    state.statusMessage = terrain.blocked
                              ? "resolve refined terrain before adopting 3D output"
                              : terrain.reasonCode;
    return result;
  }

  const cr::CreativeWorldLayoutCompileResult diagnostic =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       adoption.candidate);
  cr::CreativeWorldLayoutCompileResult compiled;
  if (diagnostic.receipt.accepted) {
    compiled = diagnostic;
  } else if (diagnostic.receipt.status ==
             cr::CreativeWorldLayoutStatus::RefinementConflict) {
    const cr::CreativeWorldLayoutRecipeMemberConflict* target = nullptr;
    std::size_t conflictCount = 0U;
    std::string instanceKey;
    for (const cr::CreativeWorldLayoutRecipeChange& change :
         diagnostic.recipeChanges) {
      for (const cr::CreativeWorldLayoutRecipeMemberConflict& conflict :
           change.memberConflicts) {
        ++conflictCount;
        if (conflict.objectId == objectId) {
          target = &conflict;
          instanceKey = change.instanceKey;
        }
      }
    }
    const bool exact =
        adoption.mode == cr::CreativeWorldLayoutAdoptionMode::Exact;
    if (conflictCount != 1U || target == nullptr ||
        target->kind !=
            cr::CreativeWorldLayoutMemberConflictKind::ConcurrentEdit ||
        (exact &&
         target->currentFingerprint != target->desiredFingerprint)) {
      result.reasonCode =
          "creative_editor_world_layout_adoption_conflict_unsupported";
      state.statusMessage = "3D edit contains fields the 2D source cannot own";
      return result;
    }
    const cr::CreativeWorldLayoutConflictDecision decision =
        cr::makeCreativeWorldLayoutMemberConflictDecision(
            std::move(instanceKey), *target,
            exact
                ? cr::CreativeWorldLayoutConflictResolution::KeepRefinement
                : cr::CreativeWorldLayoutConflictResolution::UseSource);
    compiled = cr::buildCreativeWorldLayoutPlan(
        appState.facade.document(), adoption.candidate, {{&decision, 1U}});
  } else {
    result.reasonCode = diagnostic.receipt.reasonCode;
    state.statusMessage = diagnostic.receipt.reasonCode;
    return result;
  }
  if (!compiled.receipt.accepted ||
      compiled.receipt.status != cr::CreativeWorldLayoutStatus::Ready) {
    result.reasonCode = compiled.receipt.reasonCode;
    state.statusMessage = compiled.receipt.reasonCode;
    return result;
  }

  CreativeEditorWorldLayoutSnapshot committed =
      captureCreativeEditorWorldLayoutSnapshot(state);
  committed.source = std::move(adoption.candidate);
  ++committed.revision;
  result.apply = applyCreativeEditorWorldLayoutPlanWithHistory(
      state, appState, compiled.plan, std::move(committed),
      "desktop_world_layout_adopt_3d");
  result.accepted = result.apply.accepted;
  result.changed = result.apply.changed;
  result.reasonCode = result.apply.reasonCode;
  if (!result.accepted || !result.changed) {
    state.statusMessage = result.reasonCode;
    return result;
  }
  static_cast<void>(focusCreativeEditorWorldLayoutSource(
      state, result.table, result.index));
  state.statusMessage = "3D edit adopted into World Layout";
  result.reasonCode = "creative_editor_world_layout_adoption_applied";
  return result;
}

bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object,
    cr::CreativeGridSettings grid,
    cr::CreativeVec3 worldPoint) {
  if (state.generatedRevision != state.revision ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !std::isfinite(grid.origin.x) || !std::isfinite(grid.origin.y) ||
      !std::isfinite(grid.origin.z) ||
      !std::isfinite(worldPoint.x) || !std::isfinite(worldPoint.y) ||
      !std::isfinite(worldPoint.z)) {
    return false;
  }
  const cr::CreativeVec3 sourcePointCells{
      (worldPoint.x - grid.origin.x) / grid.cellSizeMeters,
      (worldPoint.y - grid.origin.y) / grid.cellSizeMeters,
      (worldPoint.z - grid.origin.z) / grid.cellSizeMeters,
  };
  const cr::CreativeWorldLayoutObjectProvenance provenance =
      cr::resolveCreativeWorldLayoutObjectProvenance(
          state.source, object, sourcePointCells);
  return applyWorldLayoutSourceSelection(state, provenance);
}

}  // namespace iggy3d_creative_app
