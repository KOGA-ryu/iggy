#include "EditorDesktopCommands.hpp"
#include "EditorEdits.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

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
  std::sort(keys.begin(), keys.end());
  return std::adjacent_find(keys.begin(), keys.end()) == keys.end();
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
        {footprint, pattern, 0.25});
  };
  const cr::CreativeWorldLayoutBuildingBlockoutPlan single =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan splitX =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitX);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan splitZ =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitZ);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan grid =
      plan(cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan extreme =
      cr::planCreativeWorldLayoutBuildingBlockout(
          {{{std::numeric_limits<std::int32_t>::min(), 0},
            {std::numeric_limits<std::int32_t>::max(), 4}},
           cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitX, 0.25});

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
         expect(extreme.accepted && extreme.roomCount == 2U &&
                    extreme.rooms[0].maximum.x == -1 &&
                    extreme.rooms[1].minimum.x == -1,
                "midpoint planning is safe across the full coordinate range") &&
         expect(cr::toString(grid.pattern) == "Grid2x2" &&
                    cr::toString(grid.status) == "Ready",
                "blockout plan exposes stable labels");
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
      state.source.rooms.size() == roomCountBefore;

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
                    state.source.rooms.size() == roomCountBefore + 4U,
                "a building beginning at the prior wall top may stack legally");
}

bool plannerRejectsInvalidOrUnbuildableRooms() {
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badPattern =
      cr::planCreativeWorldLayoutBuildingBlockout(
          {{{0, 0}, {8, 8}},
           cr::CreativeWorldLayoutBuildingBlockoutPattern::Count, 0.25});
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badFootprint =
      cr::planCreativeWorldLayoutBuildingBlockout(
          {{{8, 0}, {0, 8}},
           cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom, 0.25});
  const cr::CreativeWorldLayoutBuildingBlockoutPlan badThickness =
      cr::planCreativeWorldLayoutBuildingBlockout(
          {{{0, 0}, {8, 8}},
           cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom,
           std::numeric_limits<double>::quiet_NaN()});
  const cr::CreativeWorldLayoutBuildingBlockoutPlan smallRooms =
      cr::planCreativeWorldLayoutBuildingBlockout(
          {{{0, 0}, {3, 8}},
           cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2, 0.5});

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
                "preset rejects rooms whose walls consume the footprint");
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
      state.source.levels.size() == 1U && state.source.rooms.size() == 4U;
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
  for (const cr::CreativeObject& object : state.preview.document.objects()) {
    floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
    roofCount += object.kind == cr::CreativeObjectKind::Roof ? 1U : 0U;
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
      live.facade.document().objectCount() == 0U;
  const bool redone = app::redoLastEdit(live, "blockout-redo", &state);

  return expect(oneSourceEdit,
                "whole blockout is one source revision") &&
         expect(sourceShapeReady,
                "blockout owns one building, one level, and four rooms") &&
         expect(selectedStableBuilding,
                "blockout selects one stable-keyed building") &&
         expect(expanded.accepted && expanded.expanded.walls.size() == 6U,
                "2x2 rooms compile to six canonical wall lanes") &&
         expect(encoded.accepted && decoded.accepted &&
                    decoded.layout.rooms.size() == 4U,
                "blockout survives source codec round trip") &&
         expect(preview.accepted && floorCount == 4U && roofCount == 4U,
                "exact preview contains one floor and roof per room") &&
         expect(oneDocumentEdit,
                "blockout confirms as one document transaction") &&
         expect(undoRestored && redone && state.source.rooms.size() == 4U &&
                    live.facade.document().objectCount() == previewObjectCount,
                "undo and redo restore source and generated geometry together");
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

  return expect(mismatchWasInert,
                "mismatched blockout payload cannot mutate source") &&
         expect(created.accepted && created.changed &&
                    created.worldLayoutChanged && !created.sceneChanged &&
                    editor.worldLayout.source.rooms.size() == 4U,
                "typed desktop command reaches the blockout kernel");
}

}  // namespace

int main() {
  const bool ok = plannerOwnsEveryPresetAndOddSplit() &&
                  plannerRejectsInvalidOrUnbuildableRooms() &&
                  editorRejectsOverlapWithoutPartialMutation() &&
                  editorCreatesAndGeneratesOneAtomicBlockout() &&
                  desktopCommandRoutesTypedBlockoutRequest();
  if (!ok) {
    return 1;
  }
  std::cout << "creative editor building blockout tests passed\n";
  return 0;
}
