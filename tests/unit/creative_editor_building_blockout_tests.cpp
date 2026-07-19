#include "EditorDesktopCommands.hpp"
#include "EditorEdits.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameRect(cr::CreativeWorldLayoutRect lhs,
              cr::CreativeWorldLayoutRect rhs) noexcept {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

bool stableKeysUnique(const cr::CreativeWorldLayout& layout) {
  std::vector<std::string> keys;
  const auto append = [&keys](const auto& values) {
    for (const auto& value : values) {
      keys.push_back(value.stableKey);
    }
  };
  append(layout.buildings);
  append(layout.levels);
  append(layout.rooms);
  append(layout.verticalConnectors);
  append(layout.openings);
  std::sort(keys.begin(), keys.end());
  return std::adjacent_find(keys.begin(), keys.end()) == keys.end();
}

cr::CreativeWorldLayoutBuildingBlockoutRequest blockoutRequest(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutBuildingBlockoutPattern pattern,
    double wallThicknessCells = 0.25) noexcept {
  cr::CreativeWorldLayoutBuildingBlockoutRequest request;
  request.footprint = footprint;
  request.pattern = pattern;
  request.wallThicknessCells = wallThicknessCells;
  return request;
}

std::size_t openingIntentCount(
    const cr::CreativeWorldLayoutBuildingBlockoutPlan& plan,
    cr::CreativeWorldLayoutBuildingBlockoutOpeningRole role) noexcept {
  return static_cast<std::size_t>(std::count_if(
      plan.openings.begin(), plan.openings.begin() + plan.openingCount,
      [role](const cr::CreativeWorldLayoutBuildingBlockoutOpening& opening) {
        return opening.role == role;
      }));
}

cr::CreativeAppState makeAppState() {
  cr::CreativeAppState state;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Building Blockout Tests");
  static_cast<void>(document.assignId(9601U));
  static_cast<void>(state.facade.installDocument(std::move(document)));
  return state;
}

app::CreativeEditorWorldLayoutBuildingBlockoutSettings gridBlockout() {
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{-5, -3}, {4, 4}};
  settings.shell.floorTopLayer = 2.0;
  settings.shell.wallHeightCells = 4U;
  settings.shell.wallThicknessCells = 0.25;
  settings.shell.floorThicknessLayers = 2U;
  settings.shell.roofThicknessLayers = 1U;
  settings.pattern = cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2;
  return settings;
}

bool plannerOwnsEveryPresetAndOddSplit() {
  const cr::CreativeWorldLayoutRect footprint{{-5, -3}, {4, 4}};
  const auto plan = [&](cr::CreativeWorldLayoutBuildingBlockoutPattern pattern) {
    return cr::planCreativeWorldLayoutBuildingBlockout(
        blockoutRequest(footprint, pattern));
  };
  const cr::CreativeWorldLayoutBuildingBlockoutPlan single =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan splitX =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitX);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan splitZ =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitZ);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan grid =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2);
  cr::CreativeWorldLayoutBuildingBlockoutRequest emptyRequest =
      blockoutRequest(
          footprint,
          cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2);
  emptyRequest.connectRooms = false;
  emptyRequest.facade.includeEntrance = false;
  emptyRequest.facade.includeExteriorWindows = false;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan emptyOpenings =
      cr::planCreativeWorldLayoutBuildingBlockout(emptyRequest);

  cr::CreativeWorldLayoutBuildingBlockoutRequest offsetRequest =
      blockoutRequest(
          {{0, 0}, {8, 8}},
          cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  offsetRequest.connectRooms = false;
  offsetRequest.facade.entranceEdge = cr::CreativeWorldLayoutRoomEdge::East;
  offsetRequest.facade.entranceOffsetCells = 1.25;
  offsetRequest.facade.includeExteriorWindows = false;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan offsetEntrance =
      cr::planCreativeWorldLayoutBuildingBlockout(offsetRequest);
  offsetRequest.facade.entranceEdge = cr::CreativeWorldLayoutRoomEdge::North;
  offsetRequest.facade.entranceOffsetCells = -1.5;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan northEntrance =
      cr::planCreativeWorldLayoutBuildingBlockout(offsetRequest);
  offsetRequest.facade.entranceEdge = cr::CreativeWorldLayoutRoomEdge::West;
  offsetRequest.facade.entranceOffsetCells = 0.75;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan westEntrance =
      cr::planCreativeWorldLayoutBuildingBlockout(offsetRequest);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan extreme =
      cr::planCreativeWorldLayoutBuildingBlockout(
          blockoutRequest(
              {{std::numeric_limits<std::int32_t>::min(), 0},
               {std::numeric_limits<std::int32_t>::max(), 4}},
              cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitX));

  return expect(single.accepted && single.roomCount == 1U &&
                    sameRect(single.rooms[0], footprint),
                "single-room blockout preserves the outer footprint") &&
         expect(splitX.accepted && splitX.roomCount == 2U &&
                    sameRect(splitX.rooms[0], {{-5, -3}, {-1, 4}}) &&
                    sameRect(splitX.rooms[1], {{-1, -3}, {4, 4}}),
                "odd X span gives its extra cell to positive X") &&
         expect(splitZ.accepted && splitZ.roomCount == 2U &&
                    sameRect(splitZ.rooms[0], {{-5, -3}, {4, 0}}) &&
                    sameRect(splitZ.rooms[1], {{-5, 0}, {4, 4}}),
                "odd Z span gives its extra cell to positive Z") &&
         expect(grid.accepted && grid.roomCount == 4U &&
                    sameRect(grid.rooms[0], {{-5, -3}, {-1, 0}}) &&
                    sameRect(grid.rooms[1], {{-1, -3}, {4, 0}}) &&
                    sameRect(grid.rooms[2], {{-5, 0}, {-1, 4}}) &&
                    sameRect(grid.rooms[3], {{-1, 0}, {4, 4}}),
                "2x2 blockout is deterministic row-major geometry") &&
         expect(single.openingCount == 4U &&
                    openingIntentCount(
                        single,
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            Entrance) == 1U &&
                    openingIntentCount(
                        single,
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            ExteriorWindow) == 3U &&
                    openingIntentCount(
                        single,
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            InteriorConnection) == 0U,
                "single-room facade owns one entrance and three windows") &&
         expect(splitX.openingCount == 7U &&
                    splitX.openings[0].role ==
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            InteriorConnection &&
                    splitX.openings[0].roomIndex == 0U &&
                    splitX.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    splitX.openings[0].adjacentRoomIndex == 1U &&
                    splitX.openings[0].adjacentRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::West &&
                    splitX.openings[0].xCells == -1.0 &&
                    splitX.openings[0].zCells == 0.5 &&
                    splitZ.openingCount == 7U &&
                    splitZ.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::South &&
                    splitZ.openings[0].adjacentRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::North &&
                    splitZ.openings[0].xCells == -0.5 &&
                    splitZ.openings[0].zCells == 0.0,
                "split presets expose one centered circulation edge") &&
         expect(grid.openingCount == 11U &&
                    openingIntentCount(
                        grid,
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            InteriorConnection) == 3U &&
                    openingIntentCount(
                        grid,
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            Entrance) == 1U &&
                    openingIntentCount(
                        grid,
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            ExteriorWindow) == 7U &&
                    grid.openings[0].roomIndex == 0U &&
                    grid.openings[0].adjacentRoomIndex == 1U &&
                    grid.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    grid.openings[0].adjacentRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::West &&
                    grid.openings[0].xCells == -1.0 &&
                    grid.openings[0].zCells == -1.5 &&
                    grid.openings[1].roomIndex == 0U &&
                    grid.openings[1].adjacentRoomIndex == 2U &&
                    grid.openings[1].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::South &&
                    grid.openings[1].adjacentRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::North &&
                    grid.openings[1].xCells == -3.0 &&
                    grid.openings[1].zCells == 0.0 &&
                    grid.openings[2].roomIndex == 2U &&
                    grid.openings[2].adjacentRoomIndex == 3U &&
                    grid.openings[2].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    grid.openings[2].adjacentRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::West &&
                    grid.openings[2].xCells == -1.0 &&
                    grid.openings[2].zCells == 2.0,
                "2x2 circulation is a deterministic row-major spanning tree") &&
         expect(grid.openings[3].role ==
                        cr::CreativeWorldLayoutBuildingBlockoutOpeningRole::
                            Entrance &&
                    grid.openings[3].kind ==
                        cr::CreativeBuildingOpeningKind::Door &&
                    grid.openings[3].roomIndex == 3U &&
                    grid.openings[3].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::South &&
                    grid.openings[3].xCells == 1.5 &&
                    grid.openings[3].zCells == 4.0,
                "the entrance uses the central eligible south facade segment") &&
         expect(emptyOpenings.accepted && emptyOpenings.openingCount == 0U,
                "callers may explicitly request an opening-free blockout") &&
         expect(offsetEntrance.accepted &&
                    offsetEntrance.openingCount == 1U &&
                    offsetEntrance.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    offsetEntrance.openings[0].xCells == 8.0 &&
                    offsetEntrance.openings[0].zCells == 5.25,
                "entrance offsets are measured along the selected facade") &&
         expect(northEntrance.accepted &&
                    northEntrance.openings[0].xCells == 2.5 &&
                    northEntrance.openings[0].zCells == 0.0 &&
                    westEntrance.accepted &&
                    westEntrance.openings[0].xCells == 0.0 &&
                    westEntrance.openings[0].zCells == 4.75,
                "all cardinal facade orientations map to world coordinates") &&
         expect(extreme.accepted && extreme.roomCount == 2U &&
                    extreme.rooms[0].maximum.x == -1 &&
                    extreme.rooms[1].minimum.x == -1,
                "midpoint planning is safe across the full coordinate range") &&
         expect(cr::toString(grid.pattern) == "Grid2x2" &&
                    cr::toString(grid.status) == "Ready",
                "blockout plan exposes stable labels");
}

bool plannerOwnsBoundedMultiStoreyShaft() {
  cr::CreativeWorldLayoutBuildingBlockoutRequest request = blockoutRequest(
      {{0, 0}, {8, 8}},
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  request.storeys.count = 3U;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan preferred =
      cr::planCreativeWorldLayoutBuildingBlockout(request);

  request.footprint = {{0, 0}, {4, 8}};
  request.storeys.preferredDirection =
      cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan fallback =
      cr::planCreativeWorldLayoutBuildingBlockout(request);

  request.footprint = {{0, 0}, {8, 8}};
  request.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan unfit =
      cr::planCreativeWorldLayoutBuildingBlockout(request);

  request.storeys.connectStoreys = false;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan disconnected =
      cr::planCreativeWorldLayoutBuildingBlockout(request);

  request.storeys.count = 0U;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan zeroStoreys =
      cr::planCreativeWorldLayoutBuildingBlockout(request);
  request.storeys.count =
      cr::kCreativeWorldLayoutBuildingBlockoutStoreyCapacity + 1U;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan tooManyStoreys =
      cr::planCreativeWorldLayoutBuildingBlockout(request);

  request.storeys.count = 2U;
  request.storeys.connectStoreys = true;
  request.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  request.storeys.preferredDirection =
      cr::CreativeWorldLayoutVerticalDirection::Count;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan invalidDirection =
      cr::planCreativeWorldLayoutBuildingBlockout(request);

  return expect(preferred.accepted && preferred.storeyCount == 3U &&
                    preferred.hasVerticalConnector &&
                    preferred.verticalConnector.roomIndex == 0U &&
                    preferred.verticalConnector.kind ==
                        cr::CreativeWorldLayoutVerticalConnectorKind::Stair &&
                    preferred.verticalConnector.direction ==
                        cr::CreativeWorldLayoutVerticalDirection::PositiveZ &&
                    sameRect(preferred.verticalConnector.footprint,
                             {{3, 2}, {4, 5}}),
                "multi-storey plans own one centered preferred stair shaft") &&
         expect(fallback.accepted && fallback.hasVerticalConnector &&
                    fallback.verticalConnector.direction ==
                        cr::CreativeWorldLayoutVerticalDirection::PositiveZ &&
                    sameRect(fallback.verticalConnector.footprint,
                             {{1, 2}, {2, 5}}),
                "shaft planning falls back through cardinal directions") &&
         expect(!unfit.accepted &&
                    unfit.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            VerticalConnectorDoesNotFit,
                "multi-room presets reject when no room can own the shaft") &&
         expect(disconnected.accepted && disconnected.storeyCount == 3U &&
                    !disconnected.hasVerticalConnector,
                "callers may explicitly request disconnected storeys") &&
         expect(!zeroStoreys.accepted && !tooManyStoreys.accepted &&
                    zeroStoreys.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidStoreyCount &&
                    tooManyStoreys.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidStoreyCount,
                "storey count is bounded at the pure recipe boundary") &&
         expect(!invalidDirection.accepted &&
                    invalidDirection.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidVerticalConnector,
                "active connector settings reject invalid enum values");
}

bool editorRejectsUnconnectableRoomsWithoutPartialMutation() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "blockout_connection_failure");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {4, 1}};
  settings.shell.wallThicknessCells = 0.25;
  settings.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitX;

  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  const bool rejectionWasAtomic =
      !rejected.accepted && !rejected.changed &&
      state.revision == revisionBefore &&
      state.nextStableOrdinal == ordinalBefore &&
      state.source.buildings.empty() && state.source.levels.empty() &&
      state.source.rooms.empty() && state.source.openings.empty() &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 0U;

  settings.connectRooms = false;
  settings.facade.includeEntrance = false;
  settings.facade.includeExteriorWindows = false;
  const app::CreativeEditorWorldLayoutEditReceipt openingFree =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  return expect(rejectionWasAtomic &&
                    rejected.reasonCode ==
                        "creative_editor_world_layout_wall_too_short",
                "an unconnectable preset fails without publishing partial "
                "source") &&
         expect(openingFree.accepted && openingFree.changed &&
                    state.source.rooms.size() == 2U &&
                    state.source.openings.empty(),
                "the same geometry remains available when connections are "
                "disabled");
}

bool editorRejectsUnfitFacadeWithoutPartialMutation() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "blockout_facade_failure");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {8, 1}};
  settings.connectRooms = false;
  settings.facade.includeEntrance = false;

  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  const bool rejectionWasAtomic =
      !rejected.accepted && !rejected.changed &&
      rejected.reasonCode == "creative_editor_world_layout_wall_too_short" &&
      state.revision == revisionBefore &&
      state.nextStableOrdinal == ordinalBefore &&
      state.source.buildings.empty() && state.source.levels.empty() &&
      state.source.rooms.empty() && state.source.openings.empty() &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 0U;

  settings.facade.includeEntrance = true;
  settings.facade.includeExteriorWindows = false;
  const app::CreativeEditorWorldLayoutEditReceipt entranceOnly =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  return expect(rejectionWasAtomic,
                "an unfit facade window rejects the whole staged blockout") &&
         expect(entranceOnly.accepted && entranceOnly.changed &&
                    state.source.rooms.size() == 1U &&
                    state.source.openings.size() == 1U &&
                    state.source.openings[0].kind ==
                        cr::CreativeBuildingOpeningKind::Door,
                "the same shallow building remains available entrance-only");
}

bool editorRejectsUnfitStoreysWithoutPartialMutation() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "blockout_storey_failure");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {4, 4}};
  settings.facade.includeEntrance = false;
  settings.facade.includeExteriorWindows = false;
  settings.storeys.count = 2U;

  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  const bool rejectionWasAtomic =
      !rejected.accepted && !rejected.changed &&
      rejected.reasonCode ==
          "creative_world_layout_building_blockout_vertical_connector_does_not_fit" &&
      state.revision == revisionBefore &&
      state.nextStableOrdinal == ordinalBefore &&
      state.source.buildings.empty() && state.source.levels.empty() &&
      state.source.rooms.empty() && state.source.verticalConnectors.empty() &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 0U;

  settings.storeys.connectStoreys = false;
  const app::CreativeEditorWorldLayoutEditReceipt disconnected =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  return expect(rejectionWasAtomic,
                "an unfit stair shaft rejects the whole staged blockout") &&
         expect(disconnected.accepted && disconnected.changed &&
                    state.source.levels.size() == 2U &&
                    state.source.rooms.size() == 2U &&
                    state.source.verticalConnectors.empty(),
                "the same storeys remain available when stairs are disabled");
}

bool editorChecksTheWholeStoreySpanForOverlap() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "blockout_storey_overlap");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings upper;
  upper.shell.footprint = {{0, 0}, {8, 8}};
  upper.shell.floorTopLayer = 3.0;
  upper.facade.includeEntrance = false;
  upper.facade.includeExteriorWindows = false;
  const app::CreativeEditorWorldLayoutEditReceipt upperCreated =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, upper);

  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const std::uint64_t undoDepthBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings lower = upper;
  lower.shell.floorTopLayer = 0.0;
  lower.storeys.count = 2U;
  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, lower);

  return expect(upperCreated.accepted && upperCreated.changed,
                "vertical-overlap fixture begins with an upper building") &&
         expect(!rejected.accepted && !rejected.changed &&
                    rejected.reasonCode ==
                        "creative_editor_world_layout_building_blockout_overlap" &&
                    state.revision == revisionBefore &&
                    state.nextStableOrdinal == ordinalBefore &&
                    app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
                        undoDepthBefore &&
                    state.source.buildings.size() == 1U &&
                    state.source.levels.size() == 1U &&
                    state.source.rooms.size() == 1U,
                "overlap checks include every requested storey before commit");
}

bool editorRejectsOverlapWithoutPartialMutation() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "blockout_overlap");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings first = gridBlockout();
  first.shell.footprint = {{0, 0}, {8, 8}};
  const app::CreativeEditorWorldLayoutEditReceipt created =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, first);

  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const std::uint64_t undoDepthBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);
  const std::size_t buildingCountBefore = state.source.buildings.size();
  const std::size_t levelCountBefore = state.source.levels.size();
  const std::size_t roomCountBefore = state.source.rooms.size();
  const std::size_t openingCountBefore = state.source.openings.size();

  app::CreativeEditorWorldLayoutBuildingBlockoutSettings overlapping = first;
  overlapping.shell.footprint = {{4, 4}, {12, 12}};
  overlapping.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, overlapping);
  const bool rejectionPreservedMetadata =
      state.revision == revisionBefore &&
      state.nextStableOrdinal == ordinalBefore &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == undoDepthBefore;
  const bool rejectionPreservedTables =
      state.source.buildings.size() == buildingCountBefore &&
      state.source.levels.size() == levelCountBefore &&
      state.source.rooms.size() == roomCountBefore &&
      state.source.openings.size() == openingCountBefore;

  app::CreativeEditorWorldLayoutBuildingBlockoutSettings stacked = first;
  stacked.shell.floorTopLayer =
      first.shell.floorTopLayer + first.shell.wallHeightCells;
  const app::CreativeEditorWorldLayoutEditReceipt stackedResult =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, stacked);

  return expect(created.accepted && created.changed,
                "overlap fixture begins with a valid blockout") &&
         expect(!rejected.accepted && !rejected.changed,
                "overlapping blockout is rejected") &&
         expect(rejectionPreservedMetadata,
                "rejection preserves revision, ids, and history") &&
         expect(rejectionPreservedTables,
                "rejection publishes no partial ownership tables") &&
         expect(stackedResult.accepted && stackedResult.changed &&
                    state.source.buildings.size() == buildingCountBefore + 1U &&
                    state.source.levels.size() == levelCountBefore + 1U &&
                    state.source.rooms.size() == roomCountBefore + 4U &&
                    state.source.openings.size() == openingCountBefore + 11U,
                "vertically separate blockouts may reuse one facade plan");
}

bool plannerRejectsInvalidOrUnbuildableRooms() {
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badPattern =
      cr::planCreativeWorldLayoutBuildingBlockout(
          blockoutRequest(
              {{0, 0}, {8, 8}},
              cr::CreativeWorldLayoutBuildingBlockoutPattern::Count));
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badFootprint =
      cr::planCreativeWorldLayoutBuildingBlockout(
          blockoutRequest(
              {{8, 0}, {0, 8}},
              cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom));
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badThickness =
      cr::planCreativeWorldLayoutBuildingBlockout(
          blockoutRequest(
              {{0, 0}, {8, 8}},
              cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom,
              std::numeric_limits<double>::quiet_NaN()));
  const cr::CreativeWorldLayoutBuildingBlockoutPlan smallRooms =
      cr::planCreativeWorldLayoutBuildingBlockout(
          blockoutRequest(
              {{0, 0}, {3, 8}},
              cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2, 0.5));
  cr::CreativeWorldLayoutBuildingBlockoutRequest badHeightRequest =
      blockoutRequest(
          {{0, 0}, {8, 8}},
          cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  badHeightRequest.wallHeightCells = 0U;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badHeight =
      cr::planCreativeWorldLayoutBuildingBlockout(badHeightRequest);
  cr::CreativeWorldLayoutBuildingBlockoutRequest badEdgeRequest =
      blockoutRequest(
          {{0, 0}, {8, 8}},
          cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  badEdgeRequest.facade.entranceEdge = cr::CreativeWorldLayoutRoomEdge::Count;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badEdge =
      cr::planCreativeWorldLayoutBuildingBlockout(badEdgeRequest);
  cr::CreativeWorldLayoutBuildingBlockoutRequest badOffsetRequest =
      blockoutRequest(
          {{0, 0}, {8, 8}},
          cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  badOffsetRequest.facade.entranceOffsetCells =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badOffset =
      cr::planCreativeWorldLayoutBuildingBlockout(badOffsetRequest);
  badOffsetRequest.facade.entranceOffsetCells = 4.0;
  const cr::CreativeWorldLayoutBuildingBlockoutPlan outsideSegment =
      cr::planCreativeWorldLayoutBuildingBlockout(badOffsetRequest);

  return expect(!badPattern.accepted &&
                    badPattern.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidPattern,
                "invalid pattern fails closed") &&
         expect(!badFootprint.accepted &&
                    badFootprint.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidFootprint,
                "inverted footprint fails closed") &&
         expect(!badThickness.accepted &&
                    badThickness.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidWallThickness,
                "non-finite wall thickness fails closed") &&
         expect(!smallRooms.accepted && smallRooms.roomCount == 0U &&
                    smallRooms.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            RoomTooSmall,
                "preset rejects rooms whose walls consume the footprint") &&
         expect(!badHeight.accepted &&
                    badHeight.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidWallHeight,
                "zero wall height fails at the pure recipe boundary") &&
         expect(!badEdge.accepted &&
                    badEdge.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidEntranceEdge,
                "invalid entrance edges fail closed") &&
         expect(!badOffset.accepted &&
                    badOffset.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidEntranceOffset &&
                    !outsideSegment.accepted &&
                    outsideSegment.status ==
                        cr::CreativeWorldLayoutBuildingBlockoutStatus::
                            InvalidEntranceOffset,
                "non-finite and out-of-segment entrance offsets fail closed");
}

bool editorCreatesAndGeneratesOneAtomicBlockout() {
  cr::CreativeAppState live = makeAppState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "blockout_layout");
  const app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings =
      gridBlockout();
  const std::uint64_t revisionBefore = state.revision;

  const app::CreativeEditorWorldLayoutEditReceipt created =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  const bool oneSourceEdit =
      created.accepted && created.changed &&
      state.revision == revisionBefore + 1U &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 1U;
  const bool sourceShapeReady =
      state.source.buildings.size() == 1U &&
      state.source.levels.size() == 1U && state.source.rooms.size() == 4U &&
      state.source.openings.size() == 11U;
  std::size_t interiorDoorCount = 0U;
  std::size_t entranceCount = 0U;
  std::size_t exteriorWindowCount = 0U;
  bool sourceOpeningsMatchTopology = true;
  for (const cr::CreativeWorldLayoutOpening& opening : state.source.openings) {
    const bool shared = cr::creativeWorldLayoutRoomEdgeIntervalIsShared(
        state.source, opening.roomIndex, opening.roomEdge,
        opening.centerOffsetCells, opening.widthCells);
    sourceOpeningsMatchTopology =
        sourceOpeningsMatchTopology &&
        opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
    if (opening.kind == cr::CreativeBuildingOpeningKind::Door && shared) {
      ++interiorDoorCount;
    } else if (opening.kind == cr::CreativeBuildingOpeningKind::Door &&
               !shared) {
      ++entranceCount;
      sourceOpeningsMatchTopology =
          sourceOpeningsMatchTopology && opening.name == "Entrance";
    } else if (opening.kind == cr::CreativeBuildingOpeningKind::Window &&
               !shared) {
      ++exteriorWindowCount;
    } else {
      sourceOpeningsMatchTopology = false;
    }
  }
  sourceOpeningsMatchTopology =
      sourceOpeningsMatchTopology && interiorDoorCount == 3U &&
      entranceCount == 1U && exteriorWindowCount == 7U;
  const bool selectedStableBuilding =
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Building &&
      state.selection.index == 0U && stableKeysUnique(state.source);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(state.source);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(state.source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(encoded.encodedText);
  const app::CreativeEditorWorldLayoutPreviewReceipt preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());

  std::uint64_t floorCount = 0U;
  std::uint64_t roofCount = 0U;
  std::uint64_t doorCount = 0U;
  std::uint64_t windowCount = 0U;
  for (const cr::CreativeObject& object : state.preview.document.objects()) {
    floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
    roofCount += object.kind == cr::CreativeObjectKind::Roof ? 1U : 0U;
    doorCount += object.kind == cr::CreativeObjectKind::Door ? 1U : 0U;
    windowCount += object.kind == cr::CreativeObjectKind::Window ? 1U : 0U;
  }
  const std::uint64_t previewObjectCount = state.preview.document.objectCount();
  const app::CreativeEditorWorldLayoutApplyReceipt confirmed =
      app::confirmCreativeEditorWorldLayout(state, live);
  const bool oneDocumentEdit =
      confirmed.accepted && confirmed.changed &&
      cr::creativeUndoDepth(live.history) == 1U &&
      live.facade.document().objectCount() == previewObjectCount;
  const bool undone = app::undoLastEdit(live, "blockout-undo", &state);
  const bool undoRestored =
      undone && state.source.buildings.empty() && state.source.rooms.empty() &&
      state.source.openings.empty() &&
      live.facade.document().objectCount() == 0U;
  const bool redone = app::redoLastEdit(live, "blockout-redo", &state);

  return expect(oneSourceEdit,
                "whole blockout is one source revision") &&
         expect(sourceShapeReady,
                "blockout owns rooms, circulation, an entrance, and windows") &&
         expect(sourceOpeningsMatchTopology,
                "opening roles match shared and exterior room edges") &&
         expect(selectedStableBuilding,
                "blockout selects one stable-keyed building") &&
         expect(expanded.accepted && expanded.expanded.walls.size() == 6U &&
                    expanded.expanded.openings.size() == 11U &&
                    std::all_of(expanded.expanded.openings.begin(),
                                expanded.expanded.openings.end(),
                                [](const cr::CreativeWorldLayoutOpening&
                                       opening) {
                                  return opening.hostKind ==
                                         cr::CreativeWorldLayoutOpeningHostKind::Wall;
                                }),
                "2x2 rooms and doors compile to canonical wall ownership") &&
         expect(encoded.accepted && decoded.accepted &&
                    decoded.layout.rooms.size() == 4U &&
                    decoded.layout.openings.size() == 11U,
                "blockout rooms and facade survive source codec round trip") &&
         expect(preview.accepted && floorCount == 4U && roofCount == 4U &&
                    doorCount == 4U && windowCount == 7U,
                "exact preview contains floors, roofs, doors, and windows") &&
         expect(oneDocumentEdit,
                "blockout confirms as one document transaction") &&
         expect(undoRestored && redone && state.source.rooms.size() == 4U &&
                    state.source.openings.size() == 11U &&
                    live.facade.document().objectCount() == previewObjectCount,
                "undo and redo restore source and generated geometry together");
}

bool editorCreatesOneAtomicMultiStoreyBlockout() {
  cr::CreativeAppState live = makeAppState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "multi_storey_blockout");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {8, 8}};
  settings.shell.floorTopLayer = 1.0;
  settings.shell.wallHeightCells = 3U;
  settings.shell.floorThicknessLayers = 2U;
  settings.shell.roofThicknessLayers = 2U;
  settings.storeys.count = 3U;

  const std::uint64_t revisionBefore = state.revision;
  const app::CreativeEditorWorldLayoutEditReceipt created =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  const bool oneSourceEdit =
      created.accepted && created.changed &&
      state.revision == revisionBefore + 1U &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 1U;
  const bool sourceShapeReady =
      state.source.buildings.size() == 1U &&
      state.source.levels.size() == 3U && state.source.rooms.size() == 3U &&
      state.source.verticalConnectors.size() == 2U &&
      state.source.openings.size() == 12U &&
      state.source.levels[0].floorTopLayer == 1.0 &&
      state.source.levels[1].floorTopLayer == 4.0 &&
      state.source.levels[2].floorTopLayer == 7.0 &&
      stableKeysUnique(state.source);
  const auto windowsForRoom = [&](std::size_t roomIndex) {
    return std::count_if(
        state.source.openings.begin(), state.source.openings.end(),
        [roomIndex](const cr::CreativeWorldLayoutOpening& opening) {
          return opening.roomIndex == roomIndex &&
                 opening.kind == cr::CreativeBuildingOpeningKind::Window;
        });
  };
  const bool facadeStackReady =
      windowsForRoom(0U) == 3 && windowsForRoom(1U) == 4 &&
      windowsForRoom(2U) == 4 &&
      std::count_if(
          state.source.openings.begin(), state.source.openings.end(),
          [](const cr::CreativeWorldLayoutOpening& opening) {
            return opening.kind == cr::CreativeBuildingOpeningKind::Door &&
                   opening.name == "Entrance";
          }) == 1;
  const bool connectorOwnershipReady =
      state.source.verticalConnectors[0].lowerRoomIndex == 0U &&
      state.source.verticalConnectors[0].upperRoomIndex == 1U &&
      state.source.verticalConnectors[1].lowerRoomIndex == 1U &&
      state.source.verticalConnectors[1].upperRoomIndex == 2U &&
      sameRect(state.source.verticalConnectors[0].footprint,
               {{3, 2}, {4, 5}}) &&
      sameRect(state.source.verticalConnectors[0].footprint,
               state.source.verticalConnectors[1].footprint) &&
      cr::planCreativeWorldLayoutVerticalConnector({}, state.source, 0U)
          .accepted &&
      cr::planCreativeWorldLayoutVerticalConnector({}, state.source, 1U)
          .accepted;
  const cr::CreativeWorldLayoutResolvedRoomGeometry groundGeometry =
      cr::resolveCreativeWorldLayoutRoomGeometry(state.source, 0U);
  const cr::CreativeWorldLayoutResolvedRoomGeometry middleGeometry =
      cr::resolveCreativeWorldLayoutRoomGeometry(state.source, 1U);
  const cr::CreativeWorldLayoutResolvedRoomGeometry topGeometry =
      cr::resolveCreativeWorldLayoutRoomGeometry(state.source, 2U);
  const bool upperSurfacesReady =
      groundGeometry.valid && middleGeometry.valid && topGeometry.valid &&
      groundGeometry.upperSurfaceKind == cr::CreativeObjectKind::Ceiling &&
      middleGeometry.upperSurfaceKind == cr::CreativeObjectKind::Ceiling &&
      topGeometry.upperSurfaceKind == cr::CreativeObjectKind::Roof;

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(state.source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(encoded.encodedText);
  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          state.source, {0U, "multi_storey", "Multi Storey"});
  cr::CreativeWorldLayout templateDestination;
  templateDestination.stableKey = "multi_storey_destination";
  const cr::CreativeWorldLayoutBuildingEditResult stamped =
      captured.accepted
          ? cr::stampCreativeWorldLayoutBuildingTemplate(
                templateDestination, captured.value,
                {{20, 20}, 100U, false, true})
          : cr::CreativeWorldLayoutBuildingEditResult{};

  const app::CreativeEditorWorldLayoutPreviewReceipt preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  std::uint64_t floorCount = 0U;
  std::uint64_t ceilingCount = 0U;
  std::uint64_t roofCount = 0U;
  std::uint64_t stairCount = 0U;
  std::uint64_t doorCount = 0U;
  std::uint64_t windowCount = 0U;
  for (const cr::CreativeObject& object : state.preview.document.objects()) {
    floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
    ceilingCount += object.kind == cr::CreativeObjectKind::Ceiling ? 1U : 0U;
    roofCount += object.kind == cr::CreativeObjectKind::Roof ? 1U : 0U;
    stairCount += object.kind == cr::CreativeObjectKind::Stair ? 1U : 0U;
    doorCount += object.kind == cr::CreativeObjectKind::Door ? 1U : 0U;
    windowCount += object.kind == cr::CreativeObjectKind::Window ? 1U : 0U;
  }
  const std::uint64_t previewObjectCount = state.preview.document.objectCount();
  const app::CreativeEditorWorldLayoutApplyReceipt confirmed =
      app::confirmCreativeEditorWorldLayout(state, live);
  const bool oneDocumentEdit =
      confirmed.accepted && confirmed.changed &&
      cr::creativeUndoDepth(live.history) == 1U &&
      live.facade.document().objectCount() == previewObjectCount;
  const bool undone = app::undoLastEdit(live, "multi-storey-undo", &state);
  const bool undoRestored =
      undone && state.source.buildings.empty() && state.source.levels.empty() &&
      state.source.rooms.empty() && state.source.verticalConnectors.empty() &&
      live.facade.document().objectCount() == 0U;
  const bool redone =
      app::redoLastEdit(live, "multi-storey-redo", &state);

  return expect(oneSourceEdit,
                "multi-storey blockout is one source revision") &&
         expect(sourceShapeReady,
                "multi-storey source owns repeated levels and facade rows") &&
         expect(facadeStackReady,
                "upper storeys replace the entrance bay with a window") &&
         expect(connectorOwnershipReady,
                "one shaft connects every adjacent level through the kernel") &&
         expect(upperSurfacesReady,
                "intermediate levels use ceilings and only the top uses roof") &&
         expect(encoded.accepted && decoded.accepted &&
                    decoded.layout.levels.size() == 3U &&
                    decoded.layout.verticalConnectors.size() == 2U &&
                    decoded.layout.openings.size() == 12U,
                "multi-storey ownership survives source codec round trip") &&
         expect(captured.accepted &&
                    captured.value.normalizedLayout.levels.size() == 3U &&
                    captured.value.normalizedLayout.verticalConnectors.size() ==
                        2U &&
                    stamped.accepted && stamped.edited.levels.size() == 3U &&
                    stamped.edited.verticalConnectors.size() == 2U &&
                    stamped.edited.verticalConnectors[0].lowerRoomIndex == 0U &&
                    stamped.edited.verticalConnectors[0].upperRoomIndex == 1U,
                "building templates preserve generated storeys and stairs") &&
         expect(preview.accepted && floorCount >= 3U && ceilingCount >= 2U &&
                    roofCount == 1U && stairCount == 2U && doorCount == 1U &&
                    windowCount == 11U,
                "preview generates cut slabs, one roof, stairs, and facade") &&
         expect(oneDocumentEdit,
                "multi-storey confirmation is one document transaction") &&
         expect(undoRestored && redone && state.source.levels.size() == 3U &&
                    state.source.verticalConnectors.size() == 2U &&
                    live.facade.document().objectCount() == previewObjectCount,
                "undo and redo restore the complete multi-storey building");
}

bool desktopCommandRoutesTypedBlockoutRequest() {
  cr::CreativeAppState live = makeAppState();
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout, "command_blockout");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};

  app::CreativeDesktopCommandFrame mismatchFrame;
  mismatchFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      app::CreativeDesktopWorldLayoutToolPayload{});
  const app::CreativeDesktopCommandResult mismatch =
      app::dispatchCreativeDesktopCommands(mismatchFrame, context);
  const bool mismatchWasInert =
      !mismatch.accepted && !mismatch.changed &&
      editor.worldLayout.source.buildings.empty();

  app::CreativeDesktopCommandFrame frame;
  frame.push(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      app::CreativeDesktopWorldLayoutBuildingBlockoutPayload{gridBlockout()});
  const app::CreativeDesktopCommandResult created =
      app::dispatchCreativeDesktopCommands(frame, context);

  app::CreativeEditorState multiEditor;
  app::resetCreativeEditorWorldLayout(multiEditor.worldLayout,
                                      "command_multi_storey_blockout");
  const app::CreativeDesktopCommandContext multiContext{
      live, multiEditor, std::filesystem::path{}, &saveId};
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings multiSettings;
  multiSettings.shell.footprint = {{0, 0}, {8, 8}};
  multiSettings.storeys.count = 2U;
  app::CreativeDesktopCommandFrame multiFrame;
  multiFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      app::CreativeDesktopWorldLayoutBuildingBlockoutPayload{multiSettings});
  const app::CreativeDesktopCommandResult multiCreated =
      app::dispatchCreativeDesktopCommands(multiFrame, multiContext);

  return expect(mismatchWasInert,
                "mismatched blockout payload cannot mutate source") &&
         expect(created.accepted && created.changed &&
                    created.worldLayoutChanged && !created.sceneChanged &&
                    editor.worldLayout.source.rooms.size() == 4U &&
                    editor.worldLayout.source.openings.size() == 11U,
                "typed desktop command reaches the blockout kernel") &&
         expect(multiCreated.accepted && multiCreated.changed &&
                    multiEditor.worldLayout.source.levels.size() == 2U &&
                    multiEditor.worldLayout.source.verticalConnectors.size() ==
                        1U,
                "typed desktop payload preserves multi-storey settings");
}

}  // namespace

int main() {
  const bool ok = plannerOwnsEveryPresetAndOddSplit() &&
                  plannerOwnsBoundedMultiStoreyShaft() &&
                  plannerRejectsInvalidOrUnbuildableRooms() &&
                  editorRejectsOverlapWithoutPartialMutation() &&
                  editorChecksTheWholeStoreySpanForOverlap() &&
                  editorRejectsUnconnectableRoomsWithoutPartialMutation() &&
                  editorRejectsUnfitFacadeWithoutPartialMutation() &&
                  editorRejectsUnfitStoreysWithoutPartialMutation() &&
                  editorCreatesAndGeneratesOneAtomicBlockout() &&
                  editorCreatesOneAtomicMultiStoreyBlockout() &&
                  desktopCommandRoutesTypedBlockoutRequest();
  if (!ok) {
    return 1;
  }
  std::cout << "creative editor building blockout tests passed\n";
  return 0;
}
