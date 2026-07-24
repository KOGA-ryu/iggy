#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

app::CreativeEditorWorldLayoutState rectangularRoom() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "room_operation_editor_test");
  const app::CreativeEditorWorldLayoutEditReceipt created =
      app::createCreativeEditorWorldLayoutBuildingShell(
          state, {{{0, 0}, {8, 4}}, 0.0, 3U, 0.25, 1U});
  if (!created.accepted) {
    std::cerr << "FAIL: room operation fixture could not be created\n";
  }
  return state;
}

cr::CreativeAppState appState() {
  cr::CreativeAppState state;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Room Operations");
  static_cast<void>(document.assignId(9481U));
  static_cast<void>(state.facade.installDocument(std::move(document)));
  return state;
}

bool splitAndMergeAreSingleSourceEdits() {
  app::CreativeEditorWorldLayoutState state = rectangularRoom();
  const std::string originalRoomKey = state.source.rooms[0].stableKey;
  const std::uint64_t splitRevision = state.revision;
  const std::uint64_t splitOrdinal = state.nextStableOrdinal;
  const std::uint64_t splitUndoDepth =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);
  state.roomTopologyDraft.roomIndex = 0U;

  const app::CreativeEditorWorldLayoutEditReceipt split =
      app::splitCreativeEditorWorldLayoutRoom(
          state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4);
  const cr::CreativeWorldLayoutRoomGraph splitGraph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const std::size_t splitSharedEdgeCount =
      cr::inspectCreativeWorldLayoutSharedRoomEdges(state.source).size();
  const bool splitIdentity =
      state.source.rooms.size() == 2U &&
      state.source.rooms[0].stableKey == originalRoomKey &&
      !state.source.rooms[1].stableKey.empty() &&
      state.source.rooms[1].stableKey != originalRoomKey;
  const bool splitState =
      state.revision == splitRevision + 1U &&
      state.nextStableOrdinal > splitOrdinal &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
          splitUndoDepth + 1U &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Room &&
      state.selection.index == 0U && state.activeLevelIndex == 0U &&
      state.roomTopologyDraft.roomIndex ==
          cr::kInvalidCreativeWorldLayoutIndex;

  const std::uint64_t mergeRevision = state.revision;
  const std::uint64_t mergeOrdinal = state.nextStableOrdinal;
  const app::CreativeEditorWorldLayoutEditReceipt merged =
      app::mergeCreativeEditorWorldLayoutRooms(state, 0U, 1U);
  const cr::CreativeWorldLayoutRoomGraph mergedGraph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);

  return expect(split.accepted && split.changed && splitGraph.accepted &&
                    splitGraph.sourceWasExplicit && splitIdentity &&
                    splitState && splitSharedEdgeCount == 1U,
                "split publishes explicit topology and one native source edit") &&
         expect(merged.accepted && merged.changed && mergedGraph.accepted &&
                    mergedGraph.sourceWasExplicit &&
                    state.source.rooms.size() == 1U &&
                    state.source.rooms[0].stableKey == originalRoomKey &&
                    state.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    state.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 4} &&
                    state.revision == mergeRevision + 1U &&
                    state.nextStableOrdinal == mergeOrdinal,
                "merge removes the partition and preserves primary identity");
}

bool rejectedSplitLeavesStateUntouched() {
  app::CreativeEditorWorldLayoutState state = rectangularRoom();
  const cr::CreativeWorldLayout sourceBefore = state.source;
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const std::uint64_t undoDepthBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);

  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::splitCreativeEditorWorldLayoutRoom(
          state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 0);

  return expect(!rejected.accepted && !rejected.changed &&
                    state.revision == revisionBefore &&
                    state.nextStableOrdinal == ordinalBefore &&
                    app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
                        undoDepthBefore &&
                    state.source.rooms.size() == sourceBefore.rooms.size() &&
                    state.source.roomBoundaries.empty() &&
                    state.source.rooms[0].stableKey ==
                        sourceBefore.rooms[0].stableKey,
                "rejected split is atomic across source history and identity");
}

bool explicitTopologyRejectsLegacyRectangleEdits() {
  app::CreativeEditorWorldLayoutState state = rectangularRoom();
  const app::CreativeEditorWorldLayoutEditReceipt split =
      app::splitCreativeEditorWorldLayoutRoom(
          state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4);
  app::CreativeEditorWorldLayoutRoomSettings settings;
  const bool read =
      app::readCreativeEditorWorldLayoutRoomSettings(state, 0U, settings);
  const std::uint64_t revisionBefore = state.revision;

  settings.footprint.maximum.x = 5;
  const app::CreativeEditorWorldLayoutEditReceipt resized =
      app::setCreativeEditorWorldLayoutRoomSettings(state, 0U, settings);
  settings.footprint.maximum.x = 4;
  settings.wallThicknessCells = 0.5;
  const app::CreativeEditorWorldLayoutEditReceipt thickened =
      app::setCreativeEditorWorldLayoutRoomSettings(state, 0U, settings);
  app::CreativeEditorWorldLayoutLevelSettings levelSettings;
  const bool levelRead = app::readCreativeEditorWorldLayoutLevelSettings(
      state, state.source.rooms[0U].levelIndex, levelSettings);
  levelSettings.wallHeightCells = 4U;
  const app::CreativeEditorWorldLayoutEditReceipt raised =
      app::setCreativeEditorWorldLayoutLevelSettings(
          state, state.source.rooms[0U].levelIndex, levelSettings);
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const app::CreativeEditorWorldLayoutRoomTarget target =
      app::findCreativeEditorWorldLayoutRoomTarget(state, {4.0, 2.0}, 0.4);

  return expect(split.accepted && read && !resized.accepted &&
                    !thickened.accepted && state.revision == revisionBefore + 1U,
                "explicit rooms reject bounding-box and scalar wall edits") &&
         expect(levelRead && raised.accepted && raised.changed &&
                    graph.accepted &&
                    state.source.levels[0].wallHeightCells == 4U &&
                    target.handle ==
                        app::CreativeEditorWorldLayoutRoomHandle::None,
                "level settings remain editable without exposing legacy room handles");
}

bool sharedBoundaryDragPreviewsCommitsAndCancels() {
  app::CreativeEditorWorldLayoutState state = rectangularRoom();
  const app::CreativeEditorWorldLayoutEditReceipt split =
      app::splitCreativeEditorWorldLayoutRoom(
          state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4);
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t undoDepthBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);
  const app::CreativeEditorWorldLayoutRoomBoundaryTarget target =
      app::findCreativeEditorWorldLayoutRoomBoundaryTarget(
          state, {4.0, 2.0}, 0.3);
  const app::CreativeEditorWorldLayoutEditReceipt began =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
          {4.0, 2.0}, 0.3);
  const app::CreativeEditorWorldLayoutEditReceipt updated =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update,
          {5.1, 2.0}, 0.3);
  const cr::CreativeWorldLayout& preview =
      app::creativeEditorWorldLayoutDisplaySource(state);
  const bool exactPreview =
      preview.rooms[0].footprint.maximum.x == 5 &&
      preview.rooms[1].footprint.minimum.x == 5 &&
      state.source.rooms[0].footprint.maximum.x == 4 &&
      state.revision == revisionBefore;
  const app::CreativeEditorWorldLayoutEditReceipt committed =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Commit,
          {5.1, 2.0}, 0.3);

  const std::uint64_t committedRevision = state.revision;
  const app::CreativeEditorWorldLayoutEditReceipt secondBegin =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
          {5.0, 2.0}, 0.3);
  const app::CreativeEditorWorldLayoutEditReceipt secondUpdate =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update,
          {6.0, 2.0}, 0.3);
  const app::CreativeEditorWorldLayoutEditReceipt cancelled =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Cancel);

  return expect(split.accepted &&
                    target.topologyEdgeIndex !=
                        cr::kInvalidCreativeWorldLayoutIndex &&
                    !target.horizontal && began.accepted && began.changed &&
                    updated.accepted && updated.changed && exactPreview,
                "shared wall drag exposes an exact non-mutating candidate") &&
         expect(committed.accepted && committed.changed &&
                    !state.roomBoundaryManipulation.active &&
                    state.source.rooms[0].footprint.maximum.x == 5 &&
                    state.source.rooms[1].footprint.minimum.x == 5 &&
                    state.revision == revisionBefore + 1U &&
                    app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
                        undoDepthBefore + 1U,
                "shared wall drag commits one source-history entry") &&
         expect(secondBegin.accepted && secondUpdate.accepted &&
                    cancelled.accepted && cancelled.changed &&
                    state.revision == committedRevision &&
                    state.source.rooms[0].footprint.maximum.x == 5 &&
                    state.source.rooms[1].footprint.minimum.x == 5,
                "cancel discards the complete shared-wall candidate");
}

bool invalidBoundaryDragShowsFailureWithoutSourceMutation() {
  app::CreativeEditorWorldLayoutState state = rectangularRoom();
  static_cast<void>(app::splitCreativeEditorWorldLayoutRoom(
      state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4));
  const std::uint64_t revisionBefore = state.revision;
  const auto began =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
          {4.0, 2.0}, 0.3);
  const auto invalid =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update,
          {0.0, 2.0}, 0.3);
  const auto committed =
      app::applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Commit,
          {0.0, 2.0}, 0.3);

  return expect(began.accepted && invalid.accepted && invalid.changed &&
                    !state.roomBoundaryManipulation.active &&
                    !committed.accepted && !committed.changed &&
                    state.revision == revisionBefore &&
                    state.source.rooms[0].footprint.maximum.x == 4 &&
                    state.source.rooms[1].footprint.minimum.x == 4,
                "invalid wall drag fails closed at commit");
}

bool cornerDragPreviewsCommitsAndCancels() {
  app::CreativeEditorWorldLayoutState state = rectangularRoom();
  static_cast<void>(app::splitCreativeEditorWorldLayoutRoom(
      state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4));
  static_cast<void>(app::mergeCreativeEditorWorldLayoutRooms(state, 0U, 1U));
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t undoDepthBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);
  const app::CreativeEditorWorldLayoutRoomCornerTarget target =
      app::findCreativeEditorWorldLayoutRoomCornerTarget(
          state, 0U, {0.0, 0.0}, 0.3);
  const auto began = app::applyCreativeEditorWorldLayoutRoomCornerManipulation(
      state,
      app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin,
      {0.0, 0.0}, 0.3);
  const auto updated =
      app::applyCreativeEditorWorldLayoutRoomCornerManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Update,
          {1.1, 1.2}, 0.3);
  const cr::CreativeWorldLayout& preview =
      app::creativeEditorWorldLayoutDisplaySource(state);
  const bool exactPreview =
      preview.rooms[0].footprint.minimum == cr::CreativeTerrainCoord2{1, 1} &&
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      state.revision == revisionBefore;
  const auto committed =
      app::applyCreativeEditorWorldLayoutRoomCornerManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Commit,
          {1.1, 1.2}, 0.3);

  const std::uint64_t committedRevision = state.revision;
  const auto secondBegin =
      app::applyCreativeEditorWorldLayoutRoomCornerManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin,
          {1.0, 1.0}, 0.3);
  const auto secondUpdate =
      app::applyCreativeEditorWorldLayoutRoomCornerManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Update,
          {2.0, 2.0}, 0.3);
  const auto cancelled =
      app::applyCreativeEditorWorldLayoutRoomCornerManipulation(
          state,
          app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Cancel);

  return expect(target.topologyVertexIndex !=
                        cr::kInvalidCreativeWorldLayoutIndex &&
                    target.northWestSouthEast,
                "corner handle resolves the selected topology vertex") &&
         expect(began.accepted && began.changed,
                "corner handle starts a live edit") &&
         expect(updated.accepted && updated.changed,
                "corner handle accepts the snapped update") &&
         expect(exactPreview,
                "corner handle exposes an exact non-mutating candidate") &&
         expect(committed.accepted && committed.changed &&
                    !state.roomCornerManipulation.active &&
                    state.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{1, 1} &&
                    state.revision == revisionBefore + 1U &&
                    app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
                        undoDepthBefore + 1U,
                "corner gesture commits one source-history entry") &&
         expect(secondBegin.accepted && secondUpdate.accepted &&
                    cancelled.accepted && cancelled.changed &&
                    state.revision == committedRevision &&
                    state.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{1, 1},
                "corner cancellation discards the complete candidate");
}

bool desktopDispatcherAppliesSplitAndBoundaryDragToTheDocument() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "room_dispatch_test");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {8, 4}}, 0.0, 3U, 0.25, 1U}));
  const app::CreativeEditorWorldLayoutApplyReceipt generated =
      app::confirmCreativeEditorWorldLayout(editor.worldLayout, live);
  const app::CreativeDesktopCommandContext context{live, editor, {}};

  app::CreativeDesktopCommandFrame splitFrame;
  splitFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutSplitRoom,
      app::CreativeDesktopWorldLayoutRoomSplitPayload{
          0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4});
  const app::CreativeDesktopCommandResult split =
      app::dispatchCreativeDesktopCommands(splitFrame, context);
  const std::uint64_t historyBeforeDrag = cr::creativeUndoDepth(live.history);

  const auto dispatchDrag = [&](auto phase, double x) {
    app::CreativeDesktopCommandFrame frame;
    frame.push(
        app::CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
        app::CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload{
            phase, {x, 2.0}, 0.3});
    return app::dispatchCreativeDesktopCommands(frame, context);
  };
  const app::CreativeDesktopCommandResult began = dispatchDrag(
      app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin, 4.0);
  const app::CreativeDesktopCommandResult updated = dispatchDrag(
      app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update, 5.0);
  const app::CreativeDesktopCommandResult committed = dispatchDrag(
      app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Commit, 5.0);

  return expect(generated.accepted && split.accepted && split.changed &&
                    split.worldLayoutChanged && split.sceneChanged &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    editor.worldLayout.source.rooms.size() == 2U,
                "desktop split command refreshes synchronized generated output") &&
         expect(began.accepted && began.changed && updated.accepted &&
                    updated.changed && updated.sceneChanged &&
                    committed.accepted && committed.changed &&
                    committed.worldLayoutChanged && committed.sceneChanged,
                "desktop boundary drag owns begin preview and commit phases") &&
         expect(editor.worldLayout.source.rooms[0].footprint.maximum.x == 5 &&
                    editor.worldLayout.source.rooms[1].footprint.minimum.x == 5 &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    cr::creativeUndoDepth(live.history) ==
                        historyBeforeDrag + 1U,
                "boundary commit updates source document and one history entry");
}

bool desktopDispatcherAppliesCornerDragToTheDocument() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "corner_dispatch_test");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {8, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::confirmCreativeEditorWorldLayout(editor.worldLayout,
                                                          live));
  const app::CreativeDesktopCommandContext context{live, editor, {}};

  app::CreativeDesktopCommandFrame splitFrame;
  splitFrame.push(
      app::CreativeDesktopCommandId::WorldLayoutSplitRoom,
      app::CreativeDesktopWorldLayoutRoomSplitPayload{
          0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4});
  static_cast<void>(app::dispatchCreativeDesktopCommands(splitFrame, context));
  app::CreativeDesktopCommandFrame mergeFrame;
  mergeFrame.push(app::CreativeDesktopCommandId::WorldLayoutMergeRooms,
                  app::CreativeDesktopWorldLayoutRoomMergePayload{0U, 1U});
  const app::CreativeDesktopCommandResult merged =
      app::dispatchCreativeDesktopCommands(mergeFrame, context);
  const std::uint64_t historyBeforeDrag = cr::creativeUndoDepth(live.history);

  const auto dispatchDrag = [&](auto phase, double x, double z) {
    app::CreativeDesktopCommandFrame frame;
    frame.push(
        app::CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
        app::CreativeDesktopWorldLayoutRoomCornerManipulationPayload{
            phase, {x, z}, 0.3});
    return app::dispatchCreativeDesktopCommands(frame, context);
  };
  const app::CreativeDesktopCommandResult began = dispatchDrag(
      app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin, 0.0,
      0.0);
  const app::CreativeDesktopCommandResult updated = dispatchDrag(
      app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Update, 1.0,
      1.0);
  const app::CreativeDesktopCommandResult committed = dispatchDrag(
      app::CreativeEditorWorldLayoutRoomCornerManipulationPhase::Commit, 1.0,
      1.0);

  return expect(merged.accepted && merged.changed && began.accepted &&
                    began.changed && updated.accepted && updated.changed &&
                    updated.sceneChanged && committed.accepted &&
                    committed.changed && committed.worldLayoutChanged &&
                    committed.sceneChanged,
                "desktop corner drag owns begin preview and commit phases") &&
         expect(editor.worldLayout.source.rooms.size() == 1U &&
                    editor.worldLayout.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{1, 1} &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    cr::creativeUndoDepth(live.history) ==
                        historyBeforeDrag + 1U,
                "corner commit updates source document and one history entry");
}

std::size_t northEdgeIndex(
    const cr::CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex) {
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       cr::creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const cr::CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const cr::CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    if (start.z == 0 && end.z == 0) {
      return boundary.topologyEdgeIndex;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

bool roomMetadataAndEdgeSettingsAreSingleSourceEdits() {
  app::CreativeEditorWorldLayoutState state = rectangularRoom();
  static_cast<void>(app::splitCreativeEditorWorldLayoutRoom(
      state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4));
  static_cast<void>(app::mergeCreativeEditorWorldLayoutRooms(state, 0U, 1U));
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t undoBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);

  app::CreativeEditorWorldLayoutRoomMetadata roomMetadata;
  const bool read =
      app::readCreativeEditorWorldLayoutRoomMetadata(state, 0U, roomMetadata);
  roomMetadata.name = "Kitchen";
  roomMetadata.type = cr::CreativeWorldLayoutRoomType::Kitchen;
  const app::CreativeEditorWorldLayoutEditReceipt metadata =
      app::setCreativeEditorWorldLayoutRoomMetadata(state, 0U, roomMetadata);

  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const std::size_t northEdge = northEdgeIndex(graph, 0U);
  const std::string northKey =
      northEdge < graph.edges.size() ? graph.edges[northEdge].stableKey : "";
  const app::CreativeEditorWorldLayoutEditReceipt wall =
      app::setCreativeEditorWorldLayoutRoomEdgeSettings(
          state,
          {northEdge, cr::CreativeWorldLayoutRoomEdgeAnchor::Start, 10U,
           0.5});
  const cr::CreativeWorldLayoutRoomGraph editedGraph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const auto editedEdge = std::find_if(
      editedGraph.edges.begin(), editedGraph.edges.end(),
      [&](const cr::CreativeWorldLayoutTopologyEdge& edge) {
        return edge.stableKey == northKey;
      });

  return expect(read && metadata.accepted && metadata.changed &&
                    state.source.rooms[0].name == "Kitchen" &&
                    state.source.rooms[0].type ==
                        cr::CreativeWorldLayoutRoomType::Kitchen,
                "room name and type are durable source metadata") &&
         expect(wall.accepted && wall.changed && editedGraph.accepted &&
                    state.source.rooms[0].footprint.maximum.x == 10 &&
                    editedEdge != editedGraph.edges.end() &&
                    editedEdge->wallThicknessCells == 0.5 &&
                    state.selectedRoomTopologyEdgeStableKey == northKey,
                "numeric wall edit preserves selected stable wall identity") &&
         expect(state.revision == revisionBefore + 2U &&
                    app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
                        undoBefore + 2U,
                "metadata and wall apply each own one source history entry");
}

bool desktopDispatcherSplitsAndMergesCanonicalWalls() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "wall_structure_dispatch_test");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {8, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::splitCreativeEditorWorldLayoutRoom(
      editor.worldLayout, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4));
  static_cast<void>(app::mergeCreativeEditorWorldLayoutRooms(
      editor.worldLayout, 0U, 1U));
  static_cast<void>(app::confirmCreativeEditorWorldLayout(editor.worldLayout,
                                                          live));
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(editor.worldLayout.source);
  const std::size_t edgeIndex = northEdgeIndex(graph, 0U);
  if (edgeIndex >= graph.edges.size()) {
    return expect(false, "wall split dispatcher fixture has north wall");
  }
  const std::string primaryKey = graph.edges[edgeIndex].stableKey;
  const std::size_t edgeCountBefore = graph.edges.size();
  const std::uint64_t historyBefore = cr::creativeUndoDepth(live.history);
  const std::uint64_t revisionBefore = editor.worldLayout.revision;
  const app::CreativeDesktopCommandContext context{live, editor, {}};

  app::CreativeDesktopCommandFrame splitFrame;
  splitFrame.push(app::CreativeDesktopCommandId::WorldLayoutSplitWall,
                  app::CreativeDesktopWorldLayoutWallSplitPayload{
                      edgeIndex, 3U});
  const app::CreativeDesktopCommandResult split =
      app::dispatchCreativeDesktopCommands(splitFrame, context);
  const std::uint64_t splitRevision = editor.worldLayout.revision;
  const std::uint64_t splitHistoryDepth = cr::creativeUndoDepth(live.history);
  const std::size_t appendedEdgeIndex =
      editor.worldLayout.source.topologyEdges.size() - 1U;
  const std::string appendedKey =
      editor.worldLayout.source.topologyEdges[appendedEdgeIndex].stableKey;

  app::CreativeDesktopCommandFrame mergeFrame;
  mergeFrame.push(app::CreativeDesktopCommandId::WorldLayoutMergeWalls,
                  app::CreativeDesktopWorldLayoutWallMergePayload{
                      edgeIndex, appendedEdgeIndex});
  const app::CreativeDesktopCommandResult merged =
      app::dispatchCreativeDesktopCommands(mergeFrame, context);
  const cr::CreativeWorldLayoutRoomGraph mergedGraph =
      cr::buildCreativeWorldLayoutRoomGraph(editor.worldLayout.source);

  return expect(split.accepted && split.changed &&
                    split.worldLayoutChanged && split.sceneChanged &&
                    splitRevision == revisionBefore + 1U &&
                    splitHistoryDepth == historyBefore + 1U,
                "wall split dispatches through synchronized source apply") &&
         expect(!appendedKey.empty() && appendedKey != primaryKey &&
                    merged.accepted && merged.changed &&
                    merged.worldLayoutChanged && merged.sceneChanged,
                "wall merge dispatches the newly minted adjacent segment") &&
         expect(mergedGraph.accepted && mergedGraph.sourceWasExplicit &&
                    editor.worldLayout.source.topologyEdges.size() ==
                        edgeCountBefore &&
                    editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::
                            TopologyEdge &&
                    editor.worldLayout.source.topologyEdges[
                        editor.worldLayout.selection.index]
                            .stableKey == primaryKey &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    cr::creativeUndoDepth(live.history) == historyBefore + 2U,
                "split and merge each own one history entry and preserve primary identity");
}

bool desktopDispatcherPreviewsAndCommitsRoomMetadata() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "room_metadata_dispatch_test");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {8, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::confirmCreativeEditorWorldLayout(editor.worldLayout,
                                                          live));
  const std::string roomKey = editor.worldLayout.source.rooms[0].stableKey;
  const std::uint64_t sourceRevision = editor.worldLayout.revision;
  const std::uint64_t documentRevision = live.facade.document().revision();
  const std::uint64_t historyDepth = cr::creativeUndoDepth(live.history);
  const app::CreativeEditorWorldLayoutRoomMetadata metadata{
      "Kitchen", cr::CreativeWorldLayoutRoomType::Kitchen};
  const auto dispatch = [&](app::CreativeDesktopWorldLayoutPropertyEditPhase
                                phase) {
    app::CreativeDesktopCommandFrame frame;
    frame.push(
        app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
        app::CreativeDesktopWorldLayoutPropertyEditPayload{
            phase, cr::CreativeWorldLayoutTable::Room, 0U, roomKey,
            metadata});
    const app::CreativeDesktopCommandContext context{live, editor, {}};
    return app::dispatchCreativeDesktopCommands(frame, context);
  };

  const app::CreativeDesktopCommandResult preview = dispatch(
      app::CreativeDesktopWorldLayoutPropertyEditPhase::Preview);
  const bool previewIsTransient =
      preview.accepted && preview.sceneChanged && !preview.worldLayoutChanged &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.rooms[0].name == "Room 1" &&
      editor.worldLayout.source.rooms[0].type ==
          cr::CreativeWorldLayoutRoomType::Generic &&
      editor.worldLayout.revision == sourceRevision &&
      live.facade.document().revision() == documentRevision &&
      cr::creativeUndoDepth(live.history) == historyDepth;

  const app::CreativeDesktopCommandResult committed = dispatch(
      app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit);
  return expect(previewIsTransient,
                "room identity preview never mutates source or history") &&
         expect(committed.accepted && committed.changed &&
                    committed.worldLayoutChanged && committed.sceneChanged &&
                    !app::creativeEditorWorldLayoutPreviewActive(
                        editor.worldLayout) &&
                    editor.worldLayout.source.rooms[0].name == "Kitchen" &&
                    editor.worldLayout.source.rooms[0].type ==
                        cr::CreativeWorldLayoutRoomType::Kitchen &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    editor.worldLayout.revision == sourceRevision + 1U &&
                    live.facade.document().revision() > documentRevision &&
                    cr::creativeUndoDepth(live.history) == historyDepth + 1U,
                "room identity commit synchronizes source scene and history");
}

bool desktopDispatcherPreviewsAndCommitsCanonicalWallSettings() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "wall_property_dispatch_test");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {8, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::splitCreativeEditorWorldLayoutRoom(
      editor.worldLayout, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4));
  static_cast<void>(app::mergeCreativeEditorWorldLayoutRooms(
      editor.worldLayout, 0U, 1U));
  static_cast<void>(app::confirmCreativeEditorWorldLayout(editor.worldLayout,
                                                          live));
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(editor.worldLayout.source);
  const std::size_t northEdge = northEdgeIndex(graph, 0U);
  if (northEdge >= graph.edges.size()) {
    return expect(false, "canonical wall property fixture has north edge");
  }
  const std::string stableKey = graph.edges[northEdge].stableKey;
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::TopologyEdge, northEdge};
  const std::uint64_t sourceRevision = editor.worldLayout.revision;
  const std::uint64_t documentRevision = live.facade.document().revision();
  const std::uint64_t historyDepth = cr::creativeUndoDepth(live.history);
  const app::CreativeEditorWorldLayoutTopologyEdgeSettings settings{
      cr::CreativeWorldLayoutRoomEdgeAnchor::Start,
      10U,
      0.5,
      5U,
      cr::CreativeWorldLayoutWallProfile::Exterior,
      cr::CreativeStructuralMaterial::Stone,
      cr::CreativeWorldLayoutWallJoinStyle::Square};
  const auto dispatch = [&](app::CreativeDesktopWorldLayoutPropertyEditPhase
                                phase) {
    app::CreativeDesktopCommandFrame frame;
    frame.push(
        app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
        app::CreativeDesktopWorldLayoutPropertyEditPayload{
            phase, cr::CreativeWorldLayoutTable::TopologyEdge, northEdge,
            stableKey, settings});
    const app::CreativeDesktopCommandContext context{live, editor, {}};
    return app::dispatchCreativeDesktopCommands(frame, context);
  };

  const app::CreativeDesktopCommandResult preview = dispatch(
      app::CreativeDesktopWorldLayoutPropertyEditPhase::Preview);
  const cr::CreativeWorldLayout& display =
      app::creativeEditorWorldLayoutDisplaySource(editor.worldLayout);
  const bool synchronizedPreview =
      preview.accepted && preview.sceneChanged && !preview.worldLayoutChanged &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      display.rooms[0].footprint.maximum.x == 10 &&
      display.topologyEdges[northEdge].wallThicknessCells == 0.5 &&
      display.topologyEdges[northEdge].wallHeightCells == 5U &&
      display.topologyEdges[northEdge].material ==
          cr::CreativeStructuralMaterial::Stone &&
      editor.worldLayout.source.rooms[0].footprint.maximum.x == 8 &&
      editor.worldLayout.revision == sourceRevision &&
      live.facade.document().revision() == documentRevision &&
      cr::creativeUndoDepth(live.history) == historyDepth;

  const app::CreativeDesktopCommandResult committed = dispatch(
      app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit);
  const cr::CreativeWorldLayoutRoomGraph committedGraph =
      cr::buildCreativeWorldLayoutRoomGraph(editor.worldLayout.source);
  return expect(synchronizedPreview,
                "wall draft previews the same candidate in 2D and 3D") &&
         expect(committed.accepted && committed.changed &&
                    committed.worldLayoutChanged && committed.sceneChanged &&
                    !app::creativeEditorWorldLayoutPreviewActive(
                        editor.worldLayout) &&
                    committedGraph.accepted &&
                    editor.worldLayout.source.rooms[0].footprint.maximum.x ==
                        10 &&
                    editor.worldLayout.source.topologyEdges[northEdge]
                            .material == cr::CreativeStructuralMaterial::Stone &&
                    editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::TopologyEdge &&
                    editor.worldLayout.source.topologyEdges[
                        editor.worldLayout.selection.index]
                            .stableKey == stableKey &&
                    editor.worldLayout.revision == sourceRevision + 1U &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    cr::creativeUndoDepth(live.history) == historyDepth + 1U,
                "wall property commit updates source scene and one history entry");
}

}  // namespace

int main() {
  const bool ok = splitAndMergeAreSingleSourceEdits() &&
                  rejectedSplitLeavesStateUntouched() &&
                  explicitTopologyRejectsLegacyRectangleEdits() &&
                  sharedBoundaryDragPreviewsCommitsAndCancels() &&
                  invalidBoundaryDragShowsFailureWithoutSourceMutation() &&
                  cornerDragPreviewsCommitsAndCancels() &&
                  desktopDispatcherAppliesSplitAndBoundaryDragToTheDocument() &&
                  desktopDispatcherAppliesCornerDragToTheDocument() &&
                  roomMetadataAndEdgeSettingsAreSingleSourceEdits() &&
                  desktopDispatcherSplitsAndMergesCanonicalWalls() &&
                  desktopDispatcherPreviewsAndCommitsRoomMetadata() &&
                  desktopDispatcherPreviewsAndCommitsCanonicalWallSettings();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
