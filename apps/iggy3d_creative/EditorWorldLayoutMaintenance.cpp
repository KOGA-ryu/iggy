#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>

namespace iggy3d_creative_app {

using detail::noteWorldLayoutSourceChange;

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutAssetBoundsUpdates(
    CreativeEditorWorldLayoutState& state,
    std::span<const CreativeEditorWorldLayoutAssetBoundsUpdate> updates) {
  if (updates.empty()) {
    return {true, false,
            "creative_editor_world_layout_asset_bounds_no_change"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  bool changed = false;
  for (std::size_t updateIndex = 0U; updateIndex < updates.size();
       ++updateIndex) {
    const CreativeEditorWorldLayoutAssetBoundsUpdate& update =
        updates[updateIndex];
    const cr::CreativeBoundsMetrics bounds =
        cr::measureCreativeBounds(update.sourceBoundsMeters);
    if (update.target >= CreativeEditorWorldLayoutAssetBoundsTarget::Count ||
        update.assetId.empty() || !bounds.valid ||
        !cr::isPositiveCreativeVec3(bounds.size)) {
      state.statusMessage = "asset bounds refresh request is invalid";
      return {false, false,
              "creative_editor_world_layout_asset_bounds_update_invalid"};
    }
    for (std::size_t prior = 0U; prior < updateIndex; ++prior) {
      if (updates[prior].target == update.target &&
          updates[prior].index == update.index) {
        state.statusMessage = "asset bounds refresh target is duplicated";
        return {
            false, false,
            "creative_editor_world_layout_asset_bounds_update_duplicate"};
      }
    }

    cr::CreativeBounds* currentBounds = nullptr;
    if (update.target ==
        CreativeEditorWorldLayoutAssetBoundsTarget::Object) {
      if (update.index >= candidate.objects.size()) {
        state.statusMessage = "asset bounds refresh target is stale";
        return {false, false,
                "creative_editor_world_layout_asset_bounds_index_invalid"};
      }
      cr::CreativeWorldLayoutObject& object = candidate.objects[update.index];
      if (object.assetId != update.assetId ||
          !object.hasAssetSourceBounds) {
        state.statusMessage = "asset bounds refresh identity changed";
        return {
            false, false,
            "creative_editor_world_layout_asset_bounds_identity_mismatch"};
      }
      currentBounds = &object.assetSourceBoundsMeters;
    } else {
      if (update.index >= candidate.openings.size()) {
        state.statusMessage = "asset bounds refresh target is stale";
        return {false, false,
                "creative_editor_world_layout_asset_bounds_index_invalid"};
      }
      cr::CreativeWorldLayoutOpening& opening =
          candidate.openings[update.index];
      if (opening.insertAssetId != update.assetId ||
          !opening.hasInsertAssetSourceBounds) {
        state.statusMessage = "asset bounds refresh identity changed";
        return {
            false, false,
            "creative_editor_world_layout_asset_bounds_identity_mismatch"};
      }
      currentBounds = &opening.insertAssetSourceBoundsMeters;
    }
    if (!cr::creativeBoundsExactlyEqual(*currentBounds,
                                        update.sourceBoundsMeters)) {
      *currentBounds = update.sourceBoundsMeters;
      changed = true;
    }
  }

  if (!changed) {
    return {true, false,
            "creative_editor_world_layout_asset_bounds_no_change"};
  }
  state.source = std::move(candidate);
  detail::noteWorldLayoutSourceChange(state, "asset source bounds refreshed");
  return {true, true,
          "creative_editor_world_layout_asset_bounds_refreshed"};
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state) {
  const CreativeEditorWorldLayoutSelection selected = state.selection;
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Level) {
    return applyCreativeEditorWorldLayoutLevelOperation(
        state, CreativeEditorWorldLayoutLevelOperation::Delete,
        cr::kInvalidCreativeWorldLayoutIndex, selected.index);
  }
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Building) {
    return deleteCreativeEditorWorldLayoutBuilding(state, selected.index);
  }
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      selected.index < state.source.rooms.size()) {
    const std::size_t removedRoom = selected.index;
    const std::size_t buildingIndex =
        state.source.rooms[removedRoom].buildingIndex;
    state.source.rooms.erase(state.source.rooms.begin() +
                             static_cast<std::ptrdiff_t>(removedRoom));
    std::erase_if(state.source.openings,
                  [&](cr::CreativeWorldLayoutOpening& opening) {
                    if (opening.hostKind !=
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
                      return false;
                    }
                    if (opening.roomIndex == removedRoom) {
                      return true;
                    }
                    if (opening.roomIndex > removedRoom) {
                      --opening.roomIndex;
                    }
                    return false;
                  });
    std::erase_if(state.source.verticalConnectors,
                  [&](cr::CreativeWorldLayoutVerticalConnector& connector) {
                    if (connector.lowerRoomIndex == removedRoom ||
                        connector.upperRoomIndex == removedRoom) {
                      return true;
                    }
                    if (connector.lowerRoomIndex > removedRoom) {
                      --connector.lowerRoomIndex;
                    }
                    if (connector.upperRoomIndex > removedRoom) {
                      --connector.upperRoomIndex;
                    }
                    return false;
                  });
    static_cast<void>(cr::refreshCreativeWorldLayoutBuildingRoomFootprint(
        state.source, buildingIndex));
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             selected.index < state.source.verticalConnectors.size()) {
    state.source.verticalConnectors.erase(
        state.source.verticalConnectors.begin() +
        static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Box &&
             selected.index < state.source.boxes.size()) {
    state.source.boxes.erase(state.source.boxes.begin() +
                             static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Opening &&
             selected.index < state.source.openings.size()) {
    state.source.openings.erase(state.source.openings.begin() +
                                static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::RoofAperture &&
             selected.index < state.source.roofApertures.size()) {
    state.source.roofApertures.erase(
        state.source.roofApertures.begin() +
        static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Wall &&
             selected.index < state.source.walls.size()) {
    const std::size_t removedWall = selected.index;
    state.source.walls.erase(state.source.walls.begin() +
                             static_cast<std::ptrdiff_t>(removedWall));
    std::erase_if(state.source.openings,
                  [&](cr::CreativeWorldLayoutOpening& opening) {
                    if (opening.hostKind !=
                        cr::CreativeWorldLayoutOpeningHostKind::Wall) {
                      return false;
                    }
                    if (opening.wallIndex == removedWall) {
                      return true;
                    }
                    if (opening.wallIndex > removedWall) {
                      --opening.wallIndex;
                    }
                    return false;
                  });
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Object &&
             selected.index < state.source.objects.size()) {
    state.source.objects.erase(state.source.objects.begin() +
                               static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::TerrainProfile &&
             selected.index < state.source.terrainProfiles.size()) {
    state.source.terrainProfiles.erase(
        state.source.terrainProfiles.begin() +
        static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::TerrainPath &&
             selected.index < state.source.terrainPaths.size()) {
    state.source.terrainPaths.erase(
        state.source.terrainPaths.begin() +
        static_cast<std::ptrdiff_t>(selected.index));
  } else {
    return {false, false, "creative_editor_world_layout_selection_missing"};
  }
  state.selection = {};
  noteWorldLayoutSourceChange(state, "layout symbol deleted");
  return {true, true, "creative_editor_world_layout_selection_deleted"};
}

}  // namespace iggy3d_creative_app
