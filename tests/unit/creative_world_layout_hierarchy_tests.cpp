#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutHierarchy.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayout hierarchyFixture() {
  cr::CreativeWorldLayout layout;

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_house";
  building.name = "House";
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level_ground";
  level.name = "Ground Floor";
  layout.levels.push_back(level);

  cr::CreativeWorldLayoutRoofAperture aperture;
  aperture.levelIndex = 0U;
  aperture.kind = cr::CreativeStructuralRoofApertureKind::Skylight;
  aperture.stableKey = "roof_skylight";
  aperture.name = "Hall Skylight";
  aperture.minimumXCells = 1.0;
  aperture.maximumXCells = 2.0;
  aperture.minimumZCells = 1.0;
  aperture.maximumZCells = 2.0;
  layout.roofApertures.push_back(aperture);

  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room_hall";
  room.name = "Great Hall";
  room.footprint = {{0, 0}, {6, 5}};
  layout.rooms.push_back(room);

  cr::CreativeWorldLayoutRoom orphanRoom = room;
  orphanRoom.buildingIndex = 99U;
  orphanRoom.stableKey = "room_missing_owner";
  orphanRoom.name = "Missing Room";
  layout.rooms.push_back(orphanRoom);

  cr::CreativeWorldLayoutTopologyVertex westVertex;
  westVertex.levelIndex = 0U;
  westVertex.stableKey = "vertex_west";
  westVertex.position = {0, 0};
  layout.topologyVertices.push_back(westVertex);
  cr::CreativeWorldLayoutTopologyVertex eastVertex;
  eastVertex.levelIndex = 0U;
  eastVertex.stableKey = "vertex_east";
  eastVertex.position = {6, 0};
  layout.topologyVertices.push_back(eastVertex);
  cr::CreativeWorldLayoutTopologyEdge topologyEdge;
  topologyEdge.levelIndex = 0U;
  topologyEdge.stableKey = "topology_wall_north";
  topologyEdge.startVertexIndex = 0U;
  topologyEdge.endVertexIndex = 1U;
  layout.topologyEdges.push_back(topologyEdge);

  cr::CreativeWorldLayoutBox box;
  box.buildingIndex = 0U;
  box.stableKey = "floor_main";
  box.name = "Main Floor";
  box.footprint = room.footprint;
  layout.boxes.push_back(box);

  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "wall_east";
  wall.name = "East Partition";
  wall.start = {6, 0};
  wall.end = {6, 5};
  layout.walls.push_back(wall);

  cr::CreativeWorldLayoutOpening roomDoor;
  roomDoor.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  roomDoor.roomIndex = 0U;
  roomDoor.kind = cr::CreativeBuildingOpeningKind::Door;
  roomDoor.stableKey = "door_north";
  roomDoor.name = "North Door";
  layout.openings.push_back(roomDoor);

  cr::CreativeWorldLayoutOpening wallWindow;
  wallWindow.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  wallWindow.wallIndex = 0U;
  wallWindow.kind = cr::CreativeBuildingOpeningKind::Window;
  wallWindow.stableKey = "window_east";
  wallWindow.name = "East Window";
  layout.openings.push_back(wallWindow);

  cr::CreativeWorldLayoutOpening orphanOpening = wallWindow;
  orphanOpening.wallIndex = 99U;
  orphanOpening.stableKey = "opening_missing_host";
  orphanOpening.name = "Missing Opening";
  layout.openings.push_back(orphanOpening);

  cr::CreativeWorldLayoutVerticalConnector connector;
  connector.buildingIndex = 0U;
  connector.lowerRoomIndex = 0U;
  connector.upperRoomIndex = 0U;
  connector.stableKey = "stair_main";
  connector.name = "Main Stair";
  connector.footprint = {{2, 2}, {4, 4}};
  layout.verticalConnectors.push_back(connector);

  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "terrain_hill";
  layout.terrainProfiles.push_back(profile);

  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "terrain_road";
  layout.terrainPaths.push_back(path);

  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Crate;
  object.stableKey = "object_crate";
  object.name = "Supply Crate";
  layout.objects.push_back(object);
  return layout;
}

const app::CreativeEditorWorldLayoutHierarchyRow* findRow(
    const app::CreativeEditorWorldLayoutHierarchyModel& model,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  const auto found = std::find_if(
      model.rows.begin(), model.rows.end(),
      [table, index](const app::CreativeEditorWorldLayoutHierarchyRow& row) {
        return row.kind ==
                   app::CreativeEditorWorldLayoutHierarchyRowKind::Symbol &&
               row.table == table && row.sourceIndex == index;
      });
  return found == model.rows.end() ? nullptr : &*found;
}

const app::CreativeEditorWorldLayoutHierarchyRow* findGroup(
    const app::CreativeEditorWorldLayoutHierarchyModel& model,
    std::string_view label) {
  const auto found = std::find_if(
      model.rows.begin(), model.rows.end(),
      [label](const app::CreativeEditorWorldLayoutHierarchyRow& row) {
        return row.kind ==
                   app::CreativeEditorWorldLayoutHierarchyRowKind::Group &&
               row.label == label;
      });
  return found == model.rows.end() ? nullptr : &*found;
}

std::size_t rowIndex(
    const app::CreativeEditorWorldLayoutHierarchyModel& model,
    const app::CreativeEditorWorldLayoutHierarchyRow* row) {
  return row == nullptr
             ? app::kInvalidCreativeEditorWorldLayoutHierarchyRow
             : static_cast<std::size_t>(row - model.rows.data());
}

bool hierarchyProjectsOwnershipExactlyOnce() {
  const app::CreativeEditorWorldLayoutHierarchyModel model =
      app::buildCreativeEditorWorldLayoutHierarchy(3U, 41U,
                                                   hierarchyFixture());
  std::set<std::pair<unsigned, std::size_t>> symbols;
  for (const auto& row : model.rows) {
    if (row.kind ==
        app::CreativeEditorWorldLayoutHierarchyRowKind::Symbol) {
      symbols.emplace(static_cast<unsigned>(row.table), row.sourceIndex);
    }
  }

  const auto* building = findRow(model, cr::CreativeWorldLayoutTable::Building,
                                 0U);
  const auto* level = findRow(model, cr::CreativeWorldLayoutTable::Level, 0U);
  const auto* room = findRow(model, cr::CreativeWorldLayoutTable::Room, 0U);
  const auto* topologyWall =
      findRow(model, cr::CreativeWorldLayoutTable::TopologyEdge, 0U);
  const auto* topologyWalls = findGroup(model, "Walls");
  const auto* roofApertures = findGroup(model, "Roof apertures");
  const auto* skylight = findRow(
      model, cr::CreativeWorldLayoutTable::RoofAperture, 0U);
  const auto* roomDoor =
      findRow(model, cr::CreativeWorldLayoutTable::Opening, 0U);
  const auto* wall = findRow(model, cr::CreativeWorldLayoutTable::Wall, 0U);
  const auto* wallWindow =
      findRow(model, cr::CreativeWorldLayoutTable::Opening, 1U);
  const auto* orphanRoom =
      findRow(model, cr::CreativeWorldLayoutTable::Room, 1U);
  const auto* orphanOpening =
      findRow(model, cr::CreativeWorldLayoutTable::Opening, 2U);
  const auto* terrain = findGroup(model, "Terrain");
  const auto* terrainPath =
      findRow(model, cr::CreativeWorldLayoutTable::TerrainPath, 0U);
  const auto* unassigned = findGroup(model, "Unassigned");

  return expect(model.sourceEpoch == 3U && model.layoutRevision == 41U,
                "hierarchy records its source identity and revision") &&
         expect(model.sourceSymbolCount == 15U && symbols.size() == 15U,
                "every user-facing source symbol appears exactly once") &&
         expect(model.recoveredSymbolCount == 2U,
                "malformed ownership is recovered rather than dropped") &&
         expect(building != nullptr && level != nullptr && room != nullptr &&
                    level->parentRow !=
                        app::kInvalidCreativeEditorWorldLayoutHierarchyRow &&
                    room->parentRow == rowIndex(model, level),
                "building levels and rooms retain explicit ownership") &&
         expect(topologyWall != nullptr && topologyWalls != nullptr &&
                    topologyWalls->parentRow == rowIndex(model, level) &&
                    topologyWall->parentRow ==
                        rowIndex(model, topologyWalls),
                "canonical walls appear once under their owning level") &&
         expect(roofApertures != nullptr && skylight != nullptr &&
                    roofApertures->parentRow == rowIndex(model, level) &&
                    skylight->parentRow == rowIndex(model, roofApertures) &&
                    skylight->typeLabel == "Skylight",
                "roof apertures appear once under their owning level") &&
         expect(roomDoor != nullptr &&
                    roomDoor->parentRow == rowIndex(model, room),
                "room-edge openings are children of their room") &&
         expect(wall != nullptr && wallWindow != nullptr &&
                    wallWindow->parentRow == rowIndex(model, wall),
                "wall-hosted openings are children of their wall") &&
         expect(terrain != nullptr && terrainPath != nullptr &&
                    terrainPath->parentRow == rowIndex(model, terrain),
                "terrain remains a global source category") &&
         expect(unassigned != nullptr && orphanRoom != nullptr &&
                    orphanOpening != nullptr &&
                    orphanRoom->parentRow == rowIndex(model, unassigned) &&
                    orphanOpening->parentRow == rowIndex(model, unassigned),
                "invalid ownership is visible under Unassigned");
}

bool hierarchyFilteringRetainsContext() {
  const app::CreativeEditorWorldLayoutHierarchyModel model =
      app::buildCreativeEditorWorldLayoutHierarchy(1U, 1U,
                                                   hierarchyFixture());
  const std::vector<std::size_t> door =
      app::filterCreativeEditorWorldLayoutHierarchy(model, "north door");
  const auto* building = findRow(model, cr::CreativeWorldLayoutTable::Building,
                                 0U);
  const auto* level = findRow(model, cr::CreativeWorldLayoutTable::Level, 0U);
  const auto* room = findRow(model, cr::CreativeWorldLayoutTable::Room, 0U);
  const auto* opening =
      findRow(model, cr::CreativeWorldLayoutTable::Opening, 0U);
  const auto contains = [&](const auto* row) {
    return std::find(door.begin(), door.end(), rowIndex(model, row)) !=
           door.end();
  };
  const std::vector<std::size_t> missing =
      app::filterCreativeEditorWorldLayoutHierarchy(model, "missing room");
  const auto* unassigned = findGroup(model, "Unassigned");
  const auto* orphan = findRow(model, cr::CreativeWorldLayoutTable::Room, 1U);
  const std::vector<std::size_t> skylight =
      app::filterCreativeEditorWorldLayoutHierarchy(model, "hall skylight");
  const auto* roofApertures = findGroup(model, "Roof apertures");
  const auto* aperture = findRow(
      model, cr::CreativeWorldLayoutTable::RoofAperture, 0U);

  return expect(contains(building) && contains(level) && contains(room) &&
                    contains(opening),
                "a matching descendant retains its ancestor path") &&
         expect(std::find(missing.begin(), missing.end(),
                          rowIndex(model, unassigned)) != missing.end() &&
                    std::find(missing.begin(), missing.end(),
                              rowIndex(model, orphan)) != missing.end(),
                "multi-term filtering finds recovered sources with context") &&
         expect(app::filterCreativeEditorWorldLayoutHierarchy(model, "xyzzy")
                    .empty(),
                "an unmatched query yields no rows") &&
         expect(app::filterCreativeEditorWorldLayoutHierarchy(model, "")
                        .size() == model.rows.size(),
                "an empty query returns the full deterministic model") &&
         expect(std::find(skylight.begin(), skylight.end(),
                          rowIndex(model, building)) != skylight.end() &&
                    std::find(skylight.begin(), skylight.end(),
                              rowIndex(model, level)) != skylight.end() &&
                    std::find(skylight.begin(), skylight.end(),
                              rowIndex(model, roofApertures)) !=
                        skylight.end() &&
                    std::find(skylight.begin(), skylight.end(),
                              rowIndex(model, aperture)) != skylight.end(),
                "roof aperture filtering retains the complete owner path");
}

bool hierarchyCacheUsesRevisionLaw() {
  cr::CreativeWorldLayout layout = hierarchyFixture();
  app::CreativeEditorWorldLayoutHierarchyCache cache;
  const auto& first = app::refreshCreativeEditorWorldLayoutHierarchy(
      cache, 5U, 7U, layout);
  const std::size_t firstRows = first.rows.size();
  for (std::size_t frame = 0U; frame < 300U; ++frame) {
    static_cast<void>(app::refreshCreativeEditorWorldLayoutHierarchy(
        cache, 5U, 7U, layout));
  }
  layout.buildings[0].name = "Renamed House";
  static_cast<void>(app::refreshCreativeEditorWorldLayoutHierarchy(
      cache, 5U, 7U, layout));
  const bool idleReused = cache.buildCount == 1U &&
                          cache.model.rows.size() == firstRows &&
                          cache.model.rows.front().label == "House";
  const auto& replaced = app::refreshCreativeEditorWorldLayoutHierarchy(
      cache, 6U, 7U, layout);
  const bool replacementRebuilt =
      cache.buildCount == 2U && replaced.rows.front().label == "Renamed House";
  layout.buildings[0].name = "Revised House";
  const auto& revised = app::refreshCreativeEditorWorldLayoutHierarchy(
      cache, 6U, 8U, layout);
  return expect(idleReused,
                "idle frames reuse the revision-owned hierarchy") &&
         expect(replacementRebuilt,
                "a replacement epoch invalidates an equal revision") &&
         expect(cache.buildCount == 3U &&
                    revised.rows.front().label == "Revised House",
                "a source revision rebuilds the hierarchy exactly once");
}

bool sourceEpochTracksSourceReplacement() {
  app::CreativeEditorWorldLayoutState state;
  const std::uint64_t initial = state.sourceEpoch;
  app::resetCreativeEditorWorldLayout(state, "epoch_reset");
  const std::uint64_t reset = state.sourceEpoch;
  app::installCreativeEditorWorldLayout(state, hierarchyFixture());
  const std::uint64_t installed = state.sourceEpoch;
  app::CreativeEditorWorldLayoutSnapshot snapshot =
      app::captureCreativeEditorWorldLayoutSnapshot(state);
  app::installCreativeEditorWorldLayoutSnapshot(state, std::move(snapshot));
  const std::uint64_t restored = state.sourceEpoch;
  return expect(reset != initial && installed != reset &&
                    restored != installed,
                "reset, load, and history restore each replace source identity");
}

app::CreativeEditorWorldLayoutState shellState() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "hierarchy_source_ops");
  const auto created = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {8, 6}}, 0.0, 4U, 0.25, 1U});
  if (!created.accepted) {
    std::cerr << "FAIL: hierarchy source fixture\n";
  }
  return state;
}

bool sourceOperationsRespectCapabilities() {
  app::CreativeEditorWorldLayoutState renamed = shellState();
  const std::string roomKey = renamed.source.rooms[0].stableKey;
  const std::uint64_t beforeRename = renamed.revision;
  const auto rename = app::renameCreativeEditorWorldLayoutSource(
      renamed, cr::CreativeWorldLayoutTable::Room, 0U, "Library");
  const std::uint64_t afterRename = renamed.revision;
  const auto blankRename = app::renameCreativeEditorWorldLayoutSource(
      renamed, cr::CreativeWorldLayoutTable::Room, 0U, "  \t");
  const auto unsupportedRename = app::renameCreativeEditorWorldLayoutSource(
      renamed, cr::CreativeWorldLayoutTable::TerrainPath, 0U, "Road");

  app::CreativeEditorWorldLayoutState levelState = shellState();
  const auto duplicateLevel = app::duplicateCreativeEditorWorldLayoutSource(
      levelState, cr::CreativeWorldLayoutTable::Level, 0U);
  app::CreativeEditorWorldLayoutState roomState = shellState();
  const std::size_t roomVertexCount =
      roomState.source.topologyVertices.size();
  const std::size_t roomEdgeCount = roomState.source.topologyEdges.size();
  const std::size_t roomBoundaryCount =
      roomState.source.roomBoundaries.size();
  const std::size_t roomUndoCount =
      roomState.sourceHistory.undoEntries.size();
  const auto duplicateRoom = app::duplicateCreativeEditorWorldLayoutSource(
      roomState, cr::CreativeWorldLayoutTable::Room, 0U);
  const auto unsupportedDuplicate =
      app::duplicateCreativeEditorWorldLayoutSource(
          roomState, cr::CreativeWorldLayoutTable::Wall, 0U);

  app::CreativeEditorWorldLayoutState deleteState = shellState();
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.stableKey = "delete_with_room";
  opening.name = "Delete With Room";
  deleteState.source.openings.push_back(opening);
  const auto removed = app::deleteCreativeEditorWorldLayoutSource(
      deleteState, cr::CreativeWorldLayoutTable::Room, 0U);

  return expect(rename.accepted && rename.changed &&
                    renamed.revision == beforeRename + 1U &&
                    renamed.source.rooms[0].name == "Library" &&
                    renamed.source.rooms[0].stableKey == roomKey,
                "rename changes only the source label and revision") &&
         expect(!blankRename.accepted && !blankRename.changed &&
                    renamed.revision == afterRename &&
                    renamed.source.rooms[0].name == "Library",
                "blank source names are rejected without mutation") &&
         expect(!unsupportedRename.accepted,
                "unnamed terrain storage cannot be renamed") &&
         expect(duplicateLevel.accepted && duplicateLevel.changed &&
                    levelState.source.levels.size() == 2U &&
                    levelState.source.rooms.size() == 2U,
                "generic duplication delegates to the ownership-safe level kernel") &&
         expect(duplicateRoom.accepted && duplicateRoom.changed &&
                    roomState.source.rooms.size() == 2U &&
                    roomState.source.topologyVertices.size() ==
                        roomVertexCount * 2U &&
                    roomState.source.topologyEdges.size() ==
                        roomEdgeCount * 2U &&
                    roomState.source.roomBoundaries.size() ==
                        roomBoundaryCount * 2U &&
                    roomState.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Room &&
                    roomState.selection.index == 1U &&
                    roomState.sourceHistory.undoEntries.size() ==
                        roomUndoCount + 1U,
                "room duplication copies owned topology and records one source edit") &&
         expect(!unsupportedDuplicate.accepted &&
                    !app::creativeEditorWorldLayoutSourceCanDuplicate(
                        cr::CreativeWorldLayoutTable::Wall),
                "derived wall duplication is rejected") &&
         expect(removed.accepted && removed.changed &&
                    deleteState.source.rooms.empty() &&
                    deleteState.source.openings.empty(),
                "source delete preserves hosted-symbol cleanup") &&
         expect(app::creativeEditorWorldLayoutSourceStableKeyMatches(
                    renamed, cr::CreativeWorldLayoutTable::Room, 0U, roomKey) &&
                    !app::creativeEditorWorldLayoutSourceStableKeyMatches(
                        renamed, cr::CreativeWorldLayoutTable::Room, 0U,
                        "stale_key"),
                "stable-key guards distinguish live and stale targets");
}

bool canonicalWallSourceFocusIsExactAndNonDestructive() {
  app::CreativeEditorWorldLayoutState state;
  app::installCreativeEditorWorldLayout(state, hierarchyFixture());
  const std::uint64_t expectedOrdinal =
      1U + state.source.buildings.size() + state.source.levels.size() +
      state.source.rooms.size() + state.source.verticalConnectors.size() +
      state.source.roofApertures.size() +
      state.source.topologyVertices.size() +
      state.source.topologyEdges.size() + state.source.boxes.size() +
      state.source.walls.size() + state.source.openings.size() +
      state.source.objects.size() + state.source.terrainProfiles.size() +
      state.source.terrainPaths.size();
  const std::uint64_t revisionBefore = state.revision;
  const app::CreativeEditorWorldLayoutEditReceipt focused =
      app::focusCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::TopologyEdge, 0U);
  const app::CreativeEditorWorldLayoutEditReceipt removed =
      app::deleteCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::TopologyEdge, 0U);

  return expect(focused.accepted && focused.changed &&
                    state.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::
                            TopologyEdge &&
                    state.selection.index == 0U &&
                    state.activeLevelIndex == 0U &&
                    state.canvasPanX == -84.0F && state.canvasPanZ == 0.0F,
                "canonical wall focus selects its exact level and midpoint") &&
         expect(state.nextStableOrdinal == expectedOrdinal,
                "loaded layout ordinal accounts for topology identities") &&
         expect(!removed.accepted && !removed.changed &&
                    state.revision == revisionBefore &&
                    state.source.topologyEdges.size() == 1U,
                "canonical walls cannot be deleted outside topology operations");
}

cr::CreativeAppState makeApp() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Hierarchy");
  static_cast<void>(document.assignId(9401U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool sourceCommandsInvalidatePreviewAndRejectStaleTargets() {
  cr::CreativeAppState live = makeApp();
  app::CreativeEditorState editor;
  editor.worldLayout = shellState();
  const auto preview = app::previewCreativeEditorWorldLayout(
      editor.worldLayout, live.facade.document());
  const std::string buildingKey = editor.worldLayout.source.buildings[0].stableKey;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};

  app::CreativeDesktopCommandFrame renameFrame;
  renameFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutRenameSource,
      app::CreativeDesktopWorldLayoutSourceRenamePayload{
          cr::CreativeWorldLayoutTable::Building, 0U, buildingKey,
          "Workshop"});
  const app::CreativeDesktopCommandResult renamed =
      app::dispatchCreativeDesktopCommands(renameFrame, context);

  app::CreativeDesktopCommandFrame staleFrame;
  staleFrame.push(app::CreativeDesktopCommandId::WorldLayoutDeleteSource,
                  app::CreativeDesktopWorldLayoutSourcePayload{
                      cr::CreativeWorldLayoutTable::Building, 0U,
                      "stale_building_key"});
  const app::CreativeDesktopCommandResult stale =
      app::dispatchCreativeDesktopCommands(staleFrame, context);

  return expect(preview.accepted,
                "source command fixture has an active exact preview") &&
         expect(renamed.accepted && renamed.changed &&
                    renamed.worldLayoutChanged && renamed.sceneChanged &&
                    editor.worldLayout.source.buildings[0].name == "Workshop" &&
                    !app::creativeEditorWorldLayoutPreviewActive(
                        editor.worldLayout),
                "typed rename invalidates an active preview and reports both changes") &&
         expect(!stale.accepted && !stale.changed &&
                    editor.worldLayout.source.buildings.size() == 1U,
                "a delayed command cannot act on a stale stable key");
}

}  // namespace

int main() {
  const bool ok = hierarchyProjectsOwnershipExactlyOnce() &&
                  hierarchyFilteringRetainsContext() &&
                  hierarchyCacheUsesRevisionLaw() &&
                  sourceEpochTracksSourceReplacement() &&
                  sourceOperationsRespectCapabilities() &&
                  canonicalWallSourceFocusIsExactAndNonDestructive() &&
                  sourceCommandsInvalidatePreviewAndRejectStaleTargets();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
