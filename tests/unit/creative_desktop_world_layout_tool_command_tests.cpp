#include "creative_desktop_command_test_runners.hpp"
#include "creative_desktop_command_test_support.hpp"

namespace {

bool worldLayoutCatalogSelectionAndPlacementUseTypedCommands() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd World Layout Catalog");
  static_cast<void>(document.assignId(428U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout, "catalog_commands");
  cr::CreativeCatalogEntry entry;
  entry.category = cr::CreativeCatalogEntryCategory::Asset;
  entry.label = "Dresser";
  entry.searchText = "dresser furnishing interior prop";
  entry.assetAuthoringMetadata.categoryId = "furniture";
  entry.hotbarEntry.objectKind = cr::CreativeObjectKind::Prop;
  static_cast<void>(cr::setCreativeHotbarAsset(
      entry.hotbarEntry, "homestead/interior/dresser_1p3",
      {{-0.65, 0.0, -0.3}, {0.65, 1.1, 0.3}}));
  editor.catalog.model.entries.push_back(entry);

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult selected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectCatalogAsset, context,
      app::CreativeDesktopWorldLayoutCatalogAssetPayload{
          "homestead/interior/dresser_1p3"});
  editor.worldLayout.catalogPlacement.elevationCells = 1.5;
  editor.worldLayout.catalogPlacement.yawDegrees = 90.0;
  editor.worldLayout.catalogPlacement.scale = {1.0, 2.0, 0.5};
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const app::CreativeDesktopCommandResult placed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{3.2, -1.7}});
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectCatalogAsset, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select});
  const cr::CreativeWorldLayoutObject& object =
      editor.worldLayout.source.objects[0];

  return expect(selected.accepted && selected.changed &&
                    !selected.worldLayoutChanged &&
                    editor.worldLayout.tool ==
                        app::CreativeEditorWorldLayoutTool::CatalogAsset,
                "typed catalog selection resolves the existing asset model") &&
         expect(placed.accepted && placed.changed &&
                    placed.worldLayoutChanged && placed.sceneChanged &&
                    editor.worldLayout.source.objects.size() == 1U &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    appState.facade.document().revision() !=
                        documentRevisionBefore &&
                    cr::creativeUndoDepth(appState.history) == undoBefore + 1U &&
                    object.assetId ==
                        "homestead/interior/dresser_1p3" &&
                    object.pointCells.x == 3.0 &&
                    object.pointCells.y == 1.5 &&
                    object.pointCells.z == -2.0 &&
                    object.hasAssetSourceBounds && object.scale.y == 2.0,
                "typed canvas confirm creates one posed source object") &&
         expect(!mismatch.accepted && !mismatch.changed &&
                    mismatch.message == "layout asset: payload mismatch" &&
                    editor.worldLayout.source.objects.size() == 1U,
                "catalog command payload mismatch is transactionally empty");
}

bool worldLayoutOpeningInsertCommandsUseCatalogAndHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Opening Insert");
  static_cast<void>(document.assignId(429U));
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 0.5;
  static_cast<void>(document.setGridSettings(grid));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "opening_insert_commands");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(
      editor.worldLayout, {0.0, 0.0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(
      editor.worldLayout, {8.0, 0.0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutWallSettings(
      editor.worldLayout, 0U, {{0, 0}, {8, 0}, 0.0, 8U, 0.25}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(
      editor.worldLayout, {4.0, 0.1}));

  cr::CreativeCatalogEntry door;
  door.category = cr::CreativeCatalogEntryCategory::Asset;
  door.label = "Asymmetric Door";
  door.assetAuthoringMetadata.categoryId = "door";
  door.hotbarEntry.objectKind = cr::CreativeObjectKind::Door;
  static_cast<void>(cr::setCreativeHotbarAsset(
      door.hotbarEntry, "homestead/modular/door_leaf_1p1x2p2",
      {{-0.2, 0.0, -0.05}, {0.9, 2.2, 0.15}}));
  editor.catalog.model.entries.push_back(door);

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::size_t undoBefore =
      editor.worldLayout.sourceHistory.undoEntries.size();
  const auto fitted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetOpeningInsert, context,
      app::CreativeDesktopWorldLayoutOpeningInsertPayload{
          0U,
          app::CreativeEditorWorldLayoutOpeningInsertOperation::
              FitAssetToOpening,
          "homestead/modular/door_leaf_1p1x2p2",
          {1.0, 1.0, 1.0}});
  const cr::CreativeWorldLayoutOpening fittedOpening =
      editor.worldLayout.source.openings[0];
  const std::uint64_t revisionAfterFit = editor.worldLayout.revision;
  const auto unavailable = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetOpeningInsert, context,
      app::CreativeDesktopWorldLayoutOpeningInsertPayload{
          0U,
          app::CreativeEditorWorldLayoutOpeningInsertOperation::
              FitAssetToOpening,
          "missing/door",
          {1.0, 1.0, 1.0}});
  const auto resized = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetOpeningInsert, context,
      app::CreativeDesktopWorldLayoutOpeningInsertPayload{
          0U,
          app::CreativeEditorWorldLayoutOpeningInsertOperation::
              ResizeOpeningToAsset,
          "homestead/modular/door_leaf_1p1x2p2",
          {1.0, 1.0, 1.0}});
  const cr::CreativeWorldLayoutOpening resizedOpening =
      editor.worldLayout.source.openings[0];
  const auto procedural = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetOpeningInsert, context,
      app::CreativeDesktopWorldLayoutOpeningInsertPayload{
          0U,
          app::CreativeEditorWorldLayoutOpeningInsertOperation::
              UseProceduralInsert,
          {},
          {1.0, 1.0, 1.0}});
  const cr::CreativeWorldLayoutOpening& finalOpening =
      editor.worldLayout.source.openings[0];

  return expect(fitted.accepted && fitted.changed &&
                    fitted.worldLayoutChanged &&
                    fittedOpening.insertAssetId ==
                        "homestead/modular/door_leaf_1p1x2p2" &&
                    fittedOpening.hasInsertAssetSourceBounds &&
                    editor.worldLayout.sourceHistory.undoEntries.size() ==
                        undoBefore + 3U,
                "opening insert command resolves catalog metadata and history") &&
         expect(!unavailable.accepted && !unavailable.changed &&
                    unavailable.message ==
                        "layout opening insert: catalog entry missing" &&
                    revisionAfterFit + 2U == editor.worldLayout.revision,
                "missing catalog insert rejects without a source mutation") &&
         expect(resized.accepted && resized.changed &&
                    near(resizedOpening.widthCells, 2.2) &&
                    near(resizedOpening.cutoutHeightCells, 4.4) &&
                    near(resizedOpening.insertThicknessCells, 0.4),
                "resize command uses live document grid scale") &&
         expect(procedural.accepted && procedural.changed &&
                    finalOpening.includeInsert &&
                    finalOpening.insertAssetId.empty() &&
                    !finalOpening.hasInsertAssetSourceBounds,
                "procedural command clears catalog ownership explicitly");
}

bool worldLayoutAssetRepairCommandsPreservePlacementAndHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Asset Repair");
  static_cast<void>(document.assignId(430U));
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 0.5;
  static_cast<void>(document.setGridSettings(grid));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "asset_repair_commands");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(
      editor.worldLayout, {0.0, 0.0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(
      editor.worldLayout, {8.0, 0.0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(
      editor.worldLayout, {4.0, 0.1}));

  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Prop;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  object.stableKey = "prop.repair";
  object.name = "Repair Prop";
  object.assetId = "props/current";
  object.pointCells = {3.0, 1.25, -2.0};
  object.assetSourceBoundsMeters =
      {{-0.5, 0.0, -0.25}, {0.5, 1.0, 0.25}};
  object.hasAssetSourceBounds = true;
  object.yawRadians = 0.75;
  object.scale = {1.5, 0.8, 2.0};
  object.tags = {"world_layout:catalog_asset"};
  editor.worldLayout.source.objects.push_back(object);
  cr::CreativeWorldLayoutOpening& opening =
      editor.worldLayout.source.openings[0];
  opening.insertAssetId = "doors/current";
  opening.insertAssetSourceBoundsMeters =
      {{-0.5, 0.0, -0.1}, {0.5, 2.0, 0.1}};
  opening.hasInsertAssetSourceBounds = true;
  const cr::CreativeWorldLayout seeded = editor.worldLayout.source;
  app::installCreativeEditorWorldLayout(editor.worldLayout, seeded);

  const auto appendAsset = [&](std::string assetId, std::string label,
                               cr::CreativeObjectKind kind,
                               std::string category,
                               cr::CreativeBounds bounds) {
    cr::CreativeCatalogEntry entry;
    entry.category = cr::CreativeCatalogEntryCategory::Asset;
    entry.label = std::move(label);
    entry.assetAuthoringMetadata.categoryId = std::move(category);
    entry.hotbarEntry.objectKind = kind;
    static_cast<void>(cr::setCreativeHotbarAsset(
        entry.hotbarEntry, assetId, bounds));
    editor.catalog.model.entries.push_back(std::move(entry));
  };
  const cr::CreativeBounds currentPropBounds =
      {{-0.75, -0.1, -0.4}, {0.75, 1.4, 0.4}};
  const cr::CreativeBounds replacementPropBounds =
      {{-1.0, 0.0, -0.5}, {1.0, 2.0, 0.5}};
  const cr::CreativeBounds currentDoorBounds =
      {{-0.6, 0.0, -0.15}, {0.6, 2.2, 0.15}};
  const cr::CreativeBounds replacementDoorBounds =
      {{-0.4, 0.0, -0.08}, {0.8, 2.4, 0.12}};
  appendAsset("props/current", "Current Prop", cr::CreativeObjectKind::Prop,
              "furniture", currentPropBounds);
  appendAsset("props/replacement", "Replacement Prop",
              cr::CreativeObjectKind::Prop, "furniture",
              replacementPropBounds);
  appendAsset("props/incompatible", "Incompatible Crate",
              cr::CreativeObjectKind::Crate, "cover",
              replacementPropBounds);
  appendAsset("doors/current", "Current Door", cr::CreativeObjectKind::Door,
              "door", currentDoorBounds);
  appendAsset("doors/replacement", "Replacement Door",
              cr::CreativeObjectKind::Door, "door",
              replacementDoorBounds);

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const cr::CreativeWorldLayoutObject originalObject =
      editor.worldLayout.source.objects[0];
  const cr::CreativeWorldLayoutOpening originalOpening =
      editor.worldLayout.source.openings[0];
  const std::size_t undoBefore =
      editor.worldLayout.sourceHistory.undoEntries.size();

  const auto repair = [&](app::CreativeDesktopWorldLayoutAssetRepairOperation op,
                          cr::CreativeWorldLayoutTable table,
                          std::size_t index, std::string stableKey,
                          std::string expectedAssetId,
                          std::string replacementAssetId = {}) {
    return dispatchPayload(
        app::CreativeDesktopCommandId::WorldLayoutRepairAsset, context,
        app::CreativeDesktopWorldLayoutAssetRepairPayload{
            op, table, index, std::move(stableKey),
            std::move(expectedAssetId), std::move(replacementAssetId)});
  };

  const auto refreshedObject = repair(
      app::CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds,
      cr::CreativeWorldLayoutTable::Object, 0U, object.stableKey,
      "props/current");
  const cr::CreativeWorldLayoutObject afterObjectRefresh =
      editor.worldLayout.source.objects[0];
  const auto replacedObject = repair(
      app::CreativeDesktopWorldLayoutAssetRepairOperation::ReplaceAsset,
      cr::CreativeWorldLayoutTable::Object, 0U, object.stableKey,
      "props/current", "props/replacement");
  const cr::CreativeWorldLayoutObject afterObjectReplacement =
      editor.worldLayout.source.objects[0];
  const std::uint64_t revisionBeforeReject = editor.worldLayout.revision;
  const auto incompatible = repair(
      app::CreativeDesktopWorldLayoutAssetRepairOperation::ReplaceAsset,
      cr::CreativeWorldLayoutTable::Object, 0U, object.stableKey,
      "props/replacement", "props/incompatible");
  const auto stale = repair(
      app::CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds,
      cr::CreativeWorldLayoutTable::Object, 0U, "wrong.stable.key",
      "props/replacement");

  const auto refreshedOpening = repair(
      app::CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds,
      cr::CreativeWorldLayoutTable::Opening, 0U, originalOpening.stableKey,
      "doors/current");
  const cr::CreativeWorldLayoutOpening afterOpeningRefresh =
      editor.worldLayout.source.openings[0];
  const auto replacedOpening = repair(
      app::CreativeDesktopWorldLayoutAssetRepairOperation::ReplaceAsset,
      cr::CreativeWorldLayoutTable::Opening, 0U, originalOpening.stableKey,
      "doors/current", "doors/replacement");
  const cr::CreativeWorldLayoutOpening afterOpeningReplacement =
      editor.worldLayout.source.openings[0];
  const auto procedural = repair(
      app::CreativeDesktopWorldLayoutAssetRepairOperation::UseProceduralInsert,
      cr::CreativeWorldLayoutTable::Opening, 0U, originalOpening.stableKey,
      "doors/replacement");
  const cr::CreativeWorldLayoutOpening finalOpening =
      editor.worldLayout.source.openings[0];
  const app::CreativeEditorWorldLayoutDiagnosticReport finalDiagnostics =
      app::buildCreativeEditorWorldLayoutDiagnosticReport(
          appState.facade.document(), editor.worldLayout.source,
          &editor.catalog.model);
  const bool hasAssetDiagnostic = std::any_of(
      finalDiagnostics.issues.begin(),
      finalDiagnostics.issues.begin() + finalDiagnostics.issueCount,
      [](const app::CreativeEditorWorldLayoutDiagnostic& diagnostic) {
        return diagnostic.assetIssue !=
               app::CreativeEditorWorldLayoutAssetIssue::None;
      });

  const bool objectPlacementPreserved =
      vecNear(afterObjectRefresh.pointCells, originalObject.pointCells) &&
      near(afterObjectRefresh.yawRadians, originalObject.yawRadians) &&
      vecNear(afterObjectRefresh.scale, originalObject.scale) &&
      vecNear(afterObjectReplacement.pointCells, originalObject.pointCells) &&
      near(afterObjectReplacement.yawRadians, originalObject.yawRadians) &&
      vecNear(afterObjectReplacement.scale, originalObject.scale);
  const bool openingFitPreserved =
      near(afterOpeningRefresh.centerOffsetCells,
           originalOpening.centerOffsetCells) &&
      near(afterOpeningRefresh.widthCells, originalOpening.widthCells) &&
      near(afterOpeningRefresh.cutoutHeightCells,
           originalOpening.cutoutHeightCells) &&
      near(afterOpeningReplacement.centerOffsetCells,
           originalOpening.centerOffsetCells) &&
      near(afterOpeningReplacement.widthCells, originalOpening.widthCells) &&
      near(afterOpeningReplacement.cutoutHeightCells,
           originalOpening.cutoutHeightCells);

  return expect(refreshedObject.accepted && refreshedObject.changed &&
                    refreshedObject.worldLayoutChanged &&
                    cr::creativeBoundsExactlyEqual(
                        afterObjectRefresh.assetSourceBoundsMeters,
                        currentPropBounds) &&
                    objectPlacementPreserved,
                "object bounds refresh preserves authored placement") &&
         expect(replacedObject.accepted && replacedObject.changed &&
                    afterObjectReplacement.assetId == "props/replacement" &&
                    cr::creativeBoundsExactlyEqual(
                        afterObjectReplacement.assetSourceBoundsMeters,
                        replacementPropBounds),
                "compatible object replacement preserves semantic kind") &&
         expect(!incompatible.accepted && !incompatible.changed &&
                    !stale.accepted && !stale.changed &&
                    editor.worldLayout.revision == revisionBeforeReject + 3U,
                "incompatible and stale repair targets mutate nothing") &&
         expect(refreshedOpening.accepted && refreshedOpening.changed &&
                    replacedOpening.accepted && replacedOpening.changed &&
                    openingFitPreserved &&
                    cr::creativeBoundsExactlyEqual(
                        afterOpeningRefresh.insertAssetSourceBoundsMeters,
                        currentDoorBounds) &&
                    afterOpeningReplacement.insertAssetId ==
                        "doors/replacement" &&
                    cr::creativeBoundsExactlyEqual(
                        afterOpeningReplacement.insertAssetSourceBoundsMeters,
                        replacementDoorBounds),
                "opening repair preserves cutout fit while replacing source") &&
         expect(procedural.accepted && procedural.changed &&
                    finalOpening.includeInsert &&
                    finalOpening.insertAssetId.empty() &&
                    !finalOpening.hasInsertAssetSourceBounds,
                "opening repair can explicitly select procedural fallback") &&
         expect(editor.worldLayout.sourceHistory.undoEntries.size() ==
                    undoBefore + 5U,
                "five accepted repairs create exactly five undo entries") &&
         expect(finalDiagnostics.ready && !hasAssetDiagnostic,
                "repaired sources compile without asset diagnostics");
}


// The world-layout terrain-region commands stay guarded outside the map
// workspace and refuse apply without an owned exact preview; cancel is
// always safe.
bool worldLayoutTerrainRegionCommandsRespectWorkspaceGuards() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Region");
  static_cast<void>(document.assignId(470U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  const app::CreativeDesktopCommandContext context{
      appState, editor, {}, nullptr, nullptr, nullptr};

  editor.assetEdit.active = true;
  const app::CreativeDesktopCommandResult guarded = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview,
      context);
  editor.assetEdit.active = false;

  const app::CreativeDesktopCommandResult applyWithoutPreview = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionApply, context);
  const std::uint64_t revisionAfterApply =
      appState.facade.document().revision();

  const app::CreativeDesktopCommandResult idleCancel = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel, context);

  return expect(!guarded.accepted && !guarded.changed &&
                    guarded.message ==
                        "terrain region unavailable in this workspace",
                "terrain region preview refuses outside the map workspace") &&
         expect(!applyWithoutPreview.accepted &&
                    !applyWithoutPreview.changed &&
                    applyWithoutPreview.message ==
                        "No terrain region preview to apply" &&
                    revisionAfterApply ==
                        appState.facade.document().revision(),
                "apply without an owned preview is a rejected no-op") &&
         expect(idleCancel.accepted && !idleCancel.changed,
                "cancel with nothing to cancel stays a safe no-op");
}

bool synchronizedCreationPreviewAndPointCommitAreAtomic() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Synchronized Creation Preview");
  static_cast<void>(document.assignId(445U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId seedObjectId = createCrate(appState.facade, -4.0);

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "synchronized_creation_preview");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::vector<cr::CreativeObjectId> initialDocumentIds =
      documentObjectIds(appState.facade.document());
  const std::uint64_t initialSourceRevision = editor.worldLayout.revision;
  const std::uint64_t initialDocumentRevision =
      appState.facade.document().revision();

  const auto roomTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Room});
  const auto begin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0}});
  const auto update = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Update, {6.2, 4.1}});
  const std::vector<cr::CreativeObjectId> previewIds =
      documentObjectIds(editor.worldLayout.preview.document);
  const std::uint64_t previewContentRevision =
      editor.worldLayout.previewContentRevision;
  const bool previewIsTransient =
      roomTool.accepted && begin.accepted && !begin.changed &&
      update.accepted && update.changed && update.sceneChanged &&
      !update.worldLayoutChanged && editor.worldLayout.anchorActive &&
      editor.worldLayout.liveEditPreviewVisible &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.rooms.empty() &&
      editor.worldLayout.revision == initialSourceRevision &&
      appState.facade.document().revision() == initialDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == 0U &&
      previewContentRevision > 0U &&
      previewIds.size() > initialDocumentIds.size() &&
      std::find(previewIds.begin(), previewIds.end(), seedObjectId) !=
          previewIds.end();

  const auto repeatedUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Update, {6.35, 4.4}});
  const std::uint64_t repeatedPreviewContentRevision =
      editor.worldLayout.previewContentRevision;
  const auto movedUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Update, {7.2, 4.4}});
  const std::uint64_t movedPreviewContentRevision =
      editor.worldLayout.previewContentRevision;
  const std::vector<cr::CreativeObjectId> movedPreviewIds =
      documentObjectIds(editor.worldLayout.preview.document);
  const auto committed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {7.2, 4.4}});
  const bool roomCommittedOnce =
      repeatedUpdate.accepted && !repeatedUpdate.changed &&
      !repeatedUpdate.sceneChanged &&
      repeatedPreviewContentRevision == previewContentRevision &&
      movedUpdate.accepted && movedUpdate.changed && movedUpdate.sceneChanged &&
      movedPreviewContentRevision > repeatedPreviewContentRevision &&
      committed.accepted && committed.changed &&
      editor.worldLayout.previewContentRevision == 0U &&
      committed.worldLayoutChanged && committed.sceneChanged &&
      editor.worldLayout.source.rooms.size() == 1U &&
      editor.worldLayout.revision == initialSourceRevision + 1U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      !editor.worldLayout.anchorActive &&
      !editor.worldLayout.liveEditPreviewVisible &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.tool == app::CreativeEditorWorldLayoutTool::Room &&
      cr::creativeUndoDepth(appState.history) == 1U &&
      documentObjectIds(appState.facade.document()) == movedPreviewIds;

  const std::uint64_t documentRevisionBeforeDoor =
      appState.facade.document().revision();
  const auto doorTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Door});
  const auto door = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{3.0, 0.0}});
  const bool doorCommittedOnce =
      doorTool.accepted && door.accepted && door.changed &&
      door.worldLayoutChanged && door.sceneChanged &&
      editor.worldLayout.source.openings.size() == 1U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      editor.worldLayout.tool == app::CreativeEditorWorldLayoutTool::Door &&
      appState.facade.document().revision() != documentRevisionBeforeDoor &&
      cr::creativeUndoDepth(appState.history) == 2U &&
      appState.facade.findObject(seedObjectId) != nullptr;

  const bool doorUndone =
      app::undoLastEdit(appState, "creation-door-undo", &editor.worldLayout);
  const bool roomUndone =
      app::undoLastEdit(appState, "creation-room-undo", &editor.worldLayout);
  const bool creationUndoRestoredBoth =
      doorUndone && roomUndone && editor.worldLayout.source.rooms.empty() &&
      editor.worldLayout.source.openings.empty() &&
      documentObjectIds(appState.facade.document()) == initialDocumentIds;

  const auto spawnTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::NpcSpawn});
  const auto spawn = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{2.0, 3.0}});
  const bool npcGenerated = std::any_of(
      appState.facade.document().objects().begin(),
      appState.facade.document().objects().end(),
      [](const cr::CreativeObject& object) {
        return object.kind == cr::CreativeObjectKind::NpcSpawn;
      });
  const bool spawnCommittedOnce =
      spawnTool.accepted && spawn.accepted && spawn.changed &&
      spawn.worldLayoutChanged && spawn.sceneChanged &&
      editor.worldLayout.source.objects.size() == 1U && npcGenerated &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      editor.worldLayout.tool == app::CreativeEditorWorldLayoutTool::NpcSpawn &&
      cr::creativeUndoDepth(appState.history) == 1U;
  const bool spawnUndone =
      app::undoLastEdit(appState, "creation-spawn-undo", &editor.worldLayout);

  static_cast<void>(dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Room}));
  const std::uint64_t cancelDocumentRevision =
      appState.facade.document().revision();
  const std::uint64_t cancelUndoDepth = cr::creativeUndoDepth(appState.history);
  const auto cancelBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {1.0, 1.0}});
  const auto cancelUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Update, {5.0, 4.0}});
  const auto cancelled = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Cancel, {}});
  const bool cancelMutatedNothing =
      spawnUndone && cancelBegin.accepted && cancelUpdate.accepted &&
      cancelUpdate.sceneChanged && cancelled.accepted &&
      cancelled.sceneChanged && !editor.worldLayout.anchorActive &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.rooms.empty() &&
      appState.facade.document().revision() == cancelDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == cancelUndoDepth;

  const auto invalidBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {1.0, 1.0}});
  const auto invalidUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Update, {1.0, 1.0}});
  const auto invalidCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {1.0, 1.0}});
  const bool invalidMutatedNothing =
      invalidBegin.accepted && invalidUpdate.accepted &&
      !invalidCommit.accepted && !invalidCommit.changed &&
      !invalidCommit.worldLayoutChanged && !editor.worldLayout.anchorActive &&
      editor.worldLayout.source.rooms.empty() &&
      appState.facade.document().revision() == cancelDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == cancelUndoDepth;

  return expect(previewIsTransient,
                "creation drag previews exact 3D without live mutation") &&
         expect(roomCommittedOnce,
                "creation release commits previewed source and geometry once") &&
         expect(doorCommittedOnce,
                "point opening creation commits source and geometry once") &&
         expect(creationUndoRestoredBoth,
                "creation undo restores point and drag edits independently") &&
         expect(spawnCommittedOnce && spawnUndone,
                "gameplay point creation uses the same atomic path") &&
         expect(cancelMutatedNothing,
                "creation cancel removes only its transient preview") &&
         expect(invalidMutatedNothing,
                "invalid creation leaves source, document, and history unchanged");
}

bool synchronizedTerrainPathDraftPreviewsAndCommitsAtomically() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Synchronized Path Preview");
  static_cast<void>(document.assignId(446U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "synchronized_path_preview");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::uint64_t initialSourceRevision = editor.worldLayout.revision;
  const std::uint64_t initialDocumentRevision =
      appState.facade.document().revision();

  const auto tool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Road});
  const auto first = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{1.0, 1.0}});
  const bool onePointIsTransient =
      tool.accepted && first.accepted && !first.changed &&
      !first.worldLayoutChanged && !first.sceneChanged &&
      editor.worldLayout.terrainPathDraft.active &&
      editor.worldLayout.terrainPathDraft.path.recipe.points.size() == 1U &&
      editor.worldLayout.source.terrainPaths.empty() &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.revision == initialSourceRevision &&
      appState.facade.document().revision() == initialDocumentRevision;

  const auto second = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{5.0, 4.0}});
  const std::uint64_t twoPointPreviewRevision =
      editor.worldLayout.previewContentRevision;
  const bool twoPointsPreviewExact3d =
      second.accepted && !second.changed && !second.worldLayoutChanged &&
      second.sceneChanged && editor.worldLayout.liveEditPreviewVisible &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.terrainPaths.empty() &&
      editor.worldLayout.previewSource.terrainPaths.size() == 1U &&
      editor.worldLayout.previewSource.terrainPaths[0].recipe.points.size() ==
          2U &&
      appState.facade.document().revision() == initialDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == 0U;

  const auto third = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{9.0, 1.0}});
  const bool thirdPointRefreshesPreview =
      third.accepted && !third.changed && third.sceneChanged &&
      editor.worldLayout.previewContentRevision > twoPointPreviewRevision &&
      editor.worldLayout.previewSource.terrainPaths[0].recipe.points.size() ==
          3U;

  const auto committed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {}});
  const bool committedOnce =
      committed.accepted && committed.changed && committed.worldLayoutChanged &&
      committed.sceneChanged && !editor.worldLayout.terrainPathDraft.active &&
      !editor.worldLayout.liveEditPreviewVisible &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.terrainPaths.size() == 1U &&
      editor.worldLayout.source.terrainPaths[0].recipe.points.size() == 3U &&
      editor.worldLayout.revision == initialSourceRevision + 1U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      appState.facade.document().terrainOperationStack().operations.size() ==
          1U &&
      appState.facade.document().terrainOperationStack().operations[0].kind ==
          cr::CreativeTerrainOperationKind::Path &&
      cr::creativeUndoDepth(appState.history) == 1U;
  const std::uint64_t documentRevisionAfterCommit =
      appState.facade.document().revision();

  const bool undone =
      app::undoLastEdit(appState, "terrain-path-draft-undo",
                        &editor.worldLayout);
  const bool wholePathUndone =
      undone && editor.worldLayout.source.terrainPaths.empty() &&
      appState.facade.document().terrainOperationStack().operations.empty() &&
      appState.facade.document().revision() > documentRevisionAfterCommit;

  return expect(onePointIsTransient,
                "one dispatched path point remains a transient draft") &&
         expect(twoPointsPreviewExact3d,
                "two dispatched path points preview exact 3D without history") &&
         expect(thirdPointRefreshesPreview,
                "a dispatched bend refreshes the transient 3D path") &&
         expect(committedOnce,
                "finishing a dispatched path commits source and terrain once") &&
         expect(wholePathUndone,
                "one undo removes the whole dispatched terrain path");
}

bool synchronizedPlanDragPreviewsAndCommitsOneStoreyAtomically() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Plan Live Edit");
  static_cast<void>(document.assignId(471U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "plan_live_edit_layout");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {8, 8}};
  settings.shell.floorTopLayer = 1.0;
  settings.floorToFloorCells = 3U;
  settings.facade.includeExteriorWindows = false;
  settings.storeys.count = 2U;
  const auto created = app::createCreativeEditorWorldLayoutBuildingBlockout(
      editor.worldLayout, settings);
  const auto generated =
      app::confirmCreativeEditorWorldLayout(editor.worldLayout, appState);
  if (!created.accepted || !generated.accepted ||
      editor.worldLayout.source.rooms.size() != 2U) {
    return expect(false, "two-storey live-edit fixture generates");
  }

  const app::CreativeDesktopCommandContext context{
      appState, editor, {}, nullptr, nullptr, nullptr};
  editor.worldLayout.tool = app::CreativeEditorWorldLayoutTool::Select;
  editor.worldLayout.activeLevelIndex = 0U;
  std::size_t openingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  double openingMoveDelta = 0.0;
  constexpr std::array<double, 6U> kCandidateDeltas{
      0.25, -0.25, 0.5, -0.5, 1.0, -1.0};
  for (std::size_t index = 0U;
       index < editor.worldLayout.source.openings.size() &&
       openingIndex == cr::kInvalidCreativeWorldLayoutIndex;
       ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        editor.worldLayout.source.openings[index];
    if (opening.roomIndex != 0U) {
      continue;
    }
    for (const double delta : kCandidateDeltas) {
      app::CreativeEditorWorldLayoutState candidate = editor.worldLayout;
      app::CreativeEditorWorldLayoutOpeningSettings candidateSettings;
      if (!app::readCreativeEditorWorldLayoutOpeningSettings(
              candidate, index, candidateSettings)) {
        continue;
      }
      candidateSettings.centerOffsetCells += delta;
      const app::CreativeEditorWorldLayoutEditReceipt candidateEdit =
          app::setCreativeEditorWorldLayoutOpeningSettings(
              candidate, index, candidateSettings);
      if (candidateEdit.accepted && candidateEdit.changed) {
        openingIndex = index;
        openingMoveDelta = delta;
        break;
      }
    }
  }
  if (openingIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    return expect(false,
                  "two-storey live-edit fixture has a movable ground opening");
  }
  const cr::CreativeWorldLayoutOpening openingBefore =
      editor.worldLayout.source.openings[openingIndex];
  const cr::CreativeObjectKind openingObjectKind =
      openingBefore.kind == cr::CreativeBuildingOpeningKind::Door
          ? cr::CreativeObjectKind::Door
          : cr::CreativeObjectKind::Window;
  const cr::CreativeWorldLayoutRoom& openingRoom =
      editor.worldLayout.source.rooms[openingBefore.roomIndex];
  const auto openingPoint = [&](double offsetCells) {
    switch (openingBefore.roomEdge) {
      case cr::CreativeWorldLayoutRoomEdge::North:
        return app::CreativeEditorWorldLayoutPoint{
            static_cast<double>(openingRoom.footprint.minimum.x) + offsetCells,
            static_cast<double>(openingRoom.footprint.minimum.z)};
      case cr::CreativeWorldLayoutRoomEdge::East:
        return app::CreativeEditorWorldLayoutPoint{
            static_cast<double>(openingRoom.footprint.maximum.x),
            static_cast<double>(openingRoom.footprint.minimum.z) + offsetCells};
      case cr::CreativeWorldLayoutRoomEdge::South:
        return app::CreativeEditorWorldLayoutPoint{
            static_cast<double>(openingRoom.footprint.minimum.x) + offsetCells,
            static_cast<double>(openingRoom.footprint.maximum.z)};
      case cr::CreativeWorldLayoutRoomEdge::West:
        return app::CreativeEditorWorldLayoutPoint{
            static_cast<double>(openingRoom.footprint.minimum.x),
            static_cast<double>(openingRoom.footprint.minimum.z) + offsetCells};
      case cr::CreativeWorldLayoutRoomEdge::Count:
        break;
    }
    return app::CreativeEditorWorldLayoutPoint{};
  };
  const app::CreativeEditorWorldLayoutPoint startPoint =
      openingPoint(openingBefore.centerOffsetCells);
  const app::CreativeEditorWorldLayoutPoint movedPoint =
      openingPoint(openingBefore.centerOffsetCells + openingMoveDelta);
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Opening, openingIndex};

  const cr::CreativeObject* openingObjectBefore = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Opening, openingIndex,
      openingObjectKind);
  const std::vector<cr::CreativeObjectId> upperIdsBefore =
      generatedSourceObjectIds(appState.facade.document(),
                               editor.worldLayout.source,
                               cr::CreativeWorldLayoutTable::Room, 1U);
  if (openingObjectBefore == nullptr || upperIdsBefore.empty()) {
    return expect(false, "two-storey live-edit fixture has generated rooms");
  }
  const cr::CreativeObjectId openingObjectId = openingObjectBefore->id;
  const cr::CreativeTransform openingTransformBefore =
      openingObjectBefore->transform;
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(appState.history);

  const auto begin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
          startPoint, 0.25});
  const auto update = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          movedPoint, 0.25});
  const cr::CreativeObject* previewOpening = findGeneratedObject(
      editor.worldLayout.preview.document, editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Opening, openingIndex,
      openingObjectKind);
  const std::vector<cr::CreativeObjectId> upperPreviewIds =
      generatedSourceObjectIds(editor.worldLayout.preview.document,
                               editor.worldLayout.source,
                               cr::CreativeWorldLayoutTable::Room, 1U);
  const bool previewIsTransient =
      begin.accepted && begin.changed && update.accepted && update.changed &&
      update.sceneChanged &&
      !update.worldLayoutChanged &&
      editor.worldLayout.liveEditPreviewVisible &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.revision == sourceRevisionBefore &&
      editor.worldLayout.source.openings[openingIndex].centerOffsetCells ==
          openingBefore.centerOffsetCells &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoDepthBefore &&
      previewOpening != nullptr && previewOpening->id == openingObjectId &&
      !vecNear(previewOpening->transform.position,
               openingTransformBefore.position) &&
      upperPreviewIds == upperIdsBefore;

  const auto repeatedUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          movedPoint, 0.25});
  const auto committed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
          movedPoint, 0.25});
  const cr::CreativeObject* openingObjectAfter = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Opening, openingIndex,
      openingObjectKind);
  const std::vector<cr::CreativeObjectId> upperIdsAfter =
      generatedSourceObjectIds(appState.facade.document(),
                               editor.worldLayout.source,
                               cr::CreativeWorldLayoutTable::Room, 1U);
  const bool committedOnce =
      repeatedUpdate.accepted && !repeatedUpdate.changed &&
      !repeatedUpdate.sceneChanged && committed.accepted && committed.changed &&
      committed.worldLayoutChanged && committed.sceneChanged &&
      !editor.worldLayout.liveEditPreviewVisible &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      editor.worldLayout.source.openings[openingIndex].centerOffsetCells ==
          openingBefore.centerOffsetCells + openingMoveDelta &&
      appState.facade.document().revision() != documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoDepthBefore + 1U &&
      openingObjectAfter != nullptr &&
      openingObjectAfter->id == openingObjectId &&
      upperIdsAfter == upperIdsBefore;
  if (!previewIsTransient || !committedOnce) {
    return expect(false, "synchronized opening live-edit setup completes");
  }

  const bool undone =
      app::undoLastEdit(appState, "plan-live-edit-undo", &editor.worldLayout);
  const cr::CreativeObject* openingObjectUndone = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Opening, openingIndex,
      openingObjectKind);
  const bool undoRestoredBoth =
      undone &&
      editor.worldLayout.source.openings[openingIndex].centerOffsetCells ==
          openingBefore.centerOffsetCells &&
      openingObjectUndone != nullptr &&
      openingObjectUndone->id == openingObjectId &&
      vecNear(openingObjectUndone->transform.position,
              openingTransformBefore.position);

  editor.worldLayout.tool = app::CreativeEditorWorldLayoutTool::Select;
  editor.worldLayout.activeLevelIndex = 0U;
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Opening, openingIndex};
  const std::uint64_t cancelDocumentRevision =
      appState.facade.document().revision();
  const std::uint64_t cancelUndoDepth = cr::creativeUndoDepth(appState.history);
  const auto cancelBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
          startPoint, 0.25});
  const auto cancelUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          movedPoint, 0.25});
  const auto cancelled = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel,
          {}, 0.25});
  const bool cancelRestoredLiveScene =
      cancelBegin.accepted && cancelUpdate.accepted &&
      cancelUpdate.sceneChanged && cancelled.accepted &&
      cancelled.sceneChanged &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.openings[openingIndex].centerOffsetCells ==
          openingBefore.centerOffsetCells &&
      appState.facade.document().revision() == cancelDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == cancelUndoDepth;

  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Opening, openingIndex};
  const auto invalidBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
          startPoint, 0.25});
  const auto invalidUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          {1.0e30, 1.0}, 0.25});
  const auto invalidCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
          {1.0e30, 1.0}, 0.25});
  const bool invalidMutatedNothing =
      invalidBegin.accepted && invalidUpdate.accepted &&
      !invalidCommit.accepted && !invalidCommit.changed &&
      !invalidCommit.worldLayoutChanged &&
      editor.worldLayout.source.openings[openingIndex].centerOffsetCells ==
          openingBefore.centerOffsetCells &&
      appState.facade.document().revision() == cancelDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == cancelUndoDepth;

  return expect(previewIsTransient,
                "plan drag previews one storey in 3D without live mutation") &&
         expect(committedOnce,
                "release commits source and 3D output in one stable-id edit") &&
         expect(undoRestoredBoth,
                "one undo restores the plan source and generated geometry") &&
         expect(cancelRestoredLiveScene,
                "cancel removes only the transient manipulation preview") &&
         expect(invalidMutatedNothing,
                "invalid release leaves source, document, and history unchanged");
}

bool synchronizedRoofHandlesPreviewAndCommitOneLevelAtomically() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Roof Handle Live Edit");
  static_cast<void>(document.assignId(472U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "roof_handle_live_edit");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {8, 6}};
  settings.shell.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  settings.shell.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  settings.shell.roofPitchDegrees = 35.0;
  settings.facade.includeExteriorWindows = false;
  const auto created = app::createCreativeEditorWorldLayoutBuildingBlockout(
      editor.worldLayout, settings);
  const auto generated =
      app::confirmCreativeEditorWorldLayout(editor.worldLayout, appState);
  if (!created.accepted || !generated.accepted ||
      editor.worldLayout.source.levels.empty()) {
    return expect(false, "roof live-edit fixture generates");
  }

  const std::size_t levelIndex = editor.worldLayout.source.levels.size() - 1U;
  editor.worldLayout.tool = app::CreativeEditorWorldLayoutTool::Select;
  editor.worldLayout.activeLevelIndex = levelIndex;
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Level, levelIndex};
  const app::CreativeDesktopCommandContext context{
      appState, editor, {}, nullptr, nullptr, nullptr};
  const app::CreativeEditorWorldLayoutRoofTarget target{
      levelIndex, app::CreativeEditorWorldLayoutRoofHandleKind::EastEave};
  const double originalOverhang =
      editor.worldLayout.source.levels[levelIndex].roofOverhangCells;
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(appState.history);

  const auto mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutPointPayload{{8.0, 3.0}});
  const auto begin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Begin, target,
          8.0});
  const auto update = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Update, {},
          8.37});
  const bool previewIsTransient =
      !mismatch.accepted && begin.accepted && !begin.changed &&
      update.accepted && update.changed && update.sceneChanged &&
      !update.worldLayoutChanged && editor.worldLayout.roofManipulation.active &&
      editor.worldLayout.liveEditPreviewVisible &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.previewSource.levels[levelIndex].roofOverhangCells ==
          originalOverhang + 0.25 &&
      editor.worldLayout.source.levels[levelIndex].roofOverhangCells ==
          originalOverhang &&
      editor.worldLayout.revision == sourceRevisionBefore &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoDepthBefore;

  const auto repeated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Update, {},
          8.37});
  const auto committed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Commit, {},
          8.37});
  const bool committedOnce =
      repeated.accepted && !repeated.changed && !repeated.sceneChanged &&
      committed.accepted && committed.changed &&
      committed.worldLayoutChanged && committed.sceneChanged &&
      !editor.worldLayout.roofManipulation.active &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.levels[levelIndex].roofOverhangCells ==
          originalOverhang + 0.25 &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      appState.facade.document().revision() != documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoDepthBefore + 1U;
  if (!previewIsTransient || !committedOnce) {
    return expect(false, "synchronized roof live-edit setup completes");
  }
  const bool undone =
      app::undoLastEdit(appState, "roof-handle-undo", &editor.worldLayout);
  const bool undoRestoredSourceAndScene =
      undone &&
      editor.worldLayout.source.levels[levelIndex].roofOverhangCells ==
          originalOverhang &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision;

  editor.worldLayout.tool = app::CreativeEditorWorldLayoutTool::Select;
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Level, levelIndex};
  const std::uint64_t cancelDocumentRevision =
      appState.facade.document().revision();
  const std::uint64_t cancelUndoDepth = cr::creativeUndoDepth(appState.history);
  const auto cancelBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Begin, target,
          8.0});
  const auto cancelUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Update, {},
          8.5});
  const auto cancelled = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Cancel, {},
          0.0});
  const bool cancelIsEmpty =
      cancelBegin.accepted && cancelUpdate.accepted &&
      cancelUpdate.sceneChanged && cancelled.accepted &&
      cancelled.sceneChanged &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.levels[levelIndex].roofOverhangCells ==
          originalOverhang &&
      appState.facade.document().revision() == cancelDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == cancelUndoDepth;

  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Level, levelIndex};
  const auto staleBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Begin, target,
          8.0});
  ++editor.worldLayout.revision;
  const auto staleUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, context,
      app::CreativeDesktopWorldLayoutRoofManipulationPayload{
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Update, {},
          8.5});
  const bool staleIsClosed =
      staleBegin.accepted && !staleUpdate.accepted &&
      !staleUpdate.changed && !editor.worldLayout.roofManipulation.active &&
      editor.worldLayout.source.levels[levelIndex].roofOverhangCells ==
          originalOverhang &&
      appState.facade.document().revision() == cancelDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == cancelUndoDepth;

  return expect(previewIsTransient,
                "roof command previews exact 3D without source mutation") &&
         expect(committedOnce,
                "roof command commits source and scene in one history step") &&
         expect(undoRestoredSourceAndScene,
                "roof handle undo restores semantic and generated truth") &&
         expect(cancelIsEmpty,
                "roof command cancel removes only its transient preview") &&
         expect(staleIsClosed,
                "stale roof command fails closed without partial history");
}

bool sourcePropertyTablesAreDerivedFromSettingsTypes() {
  const bool allMatch =
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutLevelSettings>() ==
          cr::CreativeWorldLayoutTable::Level &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutRoomSettings>() ==
          cr::CreativeWorldLayoutTable::Room &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutVerticalConnectorSettings>() ==
          cr::CreativeWorldLayoutTable::VerticalConnector &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutBoxSettings>() ==
          cr::CreativeWorldLayoutTable::Box &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutWallSettings>() ==
          cr::CreativeWorldLayoutTable::Wall &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutOpeningSettings>() ==
          cr::CreativeWorldLayoutTable::Opening &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutRoofApertureSettings>() ==
          cr::CreativeWorldLayoutTable::RoofAperture &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutTerrainProfileSettings>() ==
          cr::CreativeWorldLayoutTable::TerrainProfile &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutTerrainPathSettings>() ==
          cr::CreativeWorldLayoutTable::TerrainPath &&
      app::creativeDesktopWorldLayoutPropertyTable<
          app::CreativeEditorWorldLayoutObjectSettings>() ==
          cr::CreativeWorldLayoutTable::Object;
  return expect(allMatch,
                "each property settings type owns one source table");
}

bool roofApertureCommandsSynchronizeSourceSceneAndHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Roof Aperture Commands");
  static_cast<void>(document.assignId(911U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "roof_aperture_commands");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  editor.worldLayout.source.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level";
  level.name = "Upper floor";
  level.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  editor.worldLayout.source.levels.push_back(level);
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room";
  room.name = "Hall";
  room.footprint = {{0, 0}, {8, 8}};
  editor.worldLayout.source.rooms.push_back(room);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted) {
    return expect(false, "roof aperture command fixture generated");
  }

  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult created = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCreateRoofAperture, context,
      app::CreativeDesktopWorldLayoutRoofApertureCreatePayload{
          0U, cr::CreativeStructuralRoofApertureKind::Skylight});
  if (!created.accepted || editor.worldLayout.source.roofApertures.size() !=
                               1U) {
    return expect(false, "roof aperture command creates one source");
  }

  app::CreativeEditorWorldLayoutRoofApertureSettings createdSettings;
  if (!app::readCreativeEditorWorldLayoutRoofApertureSettings(
          editor.worldLayout, 0U, createdSettings)) {
    return expect(false, "created roof aperture settings are readable");
  }
  const std::string stableKey =
      editor.worldLayout.source.roofApertures[0].stableKey;
  const std::vector<cr::CreativeObjectId> createdIds =
      generatedSourceObjectIds(appState.facade.document(),
                               editor.worldLayout.source,
                               cr::CreativeWorldLayoutTable::RoofAperture,
                               0U);
  const cr::CreativeObject* createdObject =
      createdIds.size() == 1U
          ? appState.facade.document().findObject(createdIds.front())
          : nullptr;
  const cr::CreativeWorldLayoutObjectProvenance provenance =
      createdObject == nullptr
          ? cr::CreativeWorldLayoutObjectProvenance{}
          : cr::resolveCreativeWorldLayoutObjectProvenance(
                editor.worldLayout.source, *createdObject);
  const app::CreativeDesktopGeneratedSourceScopeModel scopes =
      app::buildCreativeDesktopGeneratedSourceScopeModel(
          editor.worldLayout.source, provenance);
  const bool createdAtomically =
      created.changed && created.worldLayoutChanged && created.sceneChanged &&
      createdObject != nullptr &&
      createdObject->kind == cr::CreativeObjectKind::Window &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U;
  const bool scopesAreExact =
      scopes.count == 3U && scopes.directEntryIndex == 2U &&
      scopes.entries[0].table == cr::CreativeWorldLayoutTable::Building &&
      scopes.entries[0].index == 0U &&
      scopes.entries[1].table == cr::CreativeWorldLayoutTable::Level &&
      scopes.entries[1].index == 0U &&
      scopes.entries[2].table ==
          cr::CreativeWorldLayoutTable::RoofAperture &&
      scopes.entries[2].index == 0U;

  constexpr double kDragTolerance = 0.20;
  const app::CreativeEditorWorldLayoutPoint dragStart{
      (createdSettings.minimumXCells + createdSettings.maximumXCells) * 0.5,
      (createdSettings.minimumZCells + createdSettings.maximumZCells) * 0.5};
  const app::CreativeEditorWorldLayoutPoint dragEnd{dragStart.x + 1.12,
                                                     dragStart.z + 0.62};
  const std::uint64_t dragUndoBefore =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult dragBegun = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
      context, app::CreativeDesktopWorldLayoutRoofApertureManipulationPayload{
                   app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::
                       Begin,
                   dragStart, kDragTolerance});
  const app::CreativeDesktopCommandResult dragPreviewed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
      context, app::CreativeDesktopWorldLayoutRoofApertureManipulationPayload{
                   app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::
                       Update,
                   dragEnd, kDragTolerance});
  app::CreativeEditorWorldLayoutRoofApertureSettings sourceDuringDrag;
  static_cast<void>(app::readCreativeEditorWorldLayoutRoofApertureSettings(
      editor.worldLayout, 0U, sourceDuringDrag));
  const bool dragPreviewIsTransient =
      dragBegun.accepted && dragPreviewed.accepted && dragPreviewed.changed &&
      dragPreviewed.sceneChanged && !dragPreviewed.worldLayoutChanged &&
      sourceDuringDrag == createdSettings &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      cr::creativeUndoDepth(appState.history) == dragUndoBefore;
  const app::CreativeDesktopCommandResult dragCommitted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
      context, app::CreativeDesktopWorldLayoutRoofApertureManipulationPayload{
                   app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::
                       Commit,
                   dragEnd, kDragTolerance});
  app::CreativeEditorWorldLayoutRoofApertureSettings draggedSettings;
  static_cast<void>(app::readCreativeEditorWorldLayoutRoofApertureSettings(
      editor.worldLayout, 0U, draggedSettings));
  const bool dragCommittedOnce =
      dragCommitted.accepted && dragCommitted.changed &&
      dragCommitted.worldLayoutChanged && dragCommitted.sceneChanged &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      draggedSettings.minimumXCells == createdSettings.minimumXCells + 1.0 &&
      draggedSettings.maximumXCells == createdSettings.maximumXCells + 1.0 &&
      draggedSettings.minimumZCells == createdSettings.minimumZCells + 0.5 &&
      draggedSettings.maximumZCells == createdSettings.maximumZCells + 0.5 &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 2U;

  app::CreativeEditorWorldLayoutRoofApertureSettings editedSettings =
      draggedSettings;
  editedSettings.minimumXCells += 0.25;
  editedSettings.maximumXCells += 0.25;
  const app::CreativeDesktopCommandResult edited = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      app::CreativeDesktopWorldLayoutPropertyEditPayload{
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit,
          cr::CreativeWorldLayoutTable::RoofAperture, 0U, stableKey,
          editedSettings});
  const std::vector<cr::CreativeObjectId> editedIds =
      generatedSourceObjectIds(appState.facade.document(),
                               editor.worldLayout.source,
                               cr::CreativeWorldLayoutTable::RoofAperture,
                               0U);
  const bool editedAtomically =
      edited.accepted && edited.changed && edited.worldLayoutChanged &&
      edited.sceneChanged &&
      editor.worldLayout.source.roofApertures[0].minimumXCells ==
          editedSettings.minimumXCells &&
      editor.worldLayout.source.roofApertures[0].maximumXCells ==
          editedSettings.maximumXCells &&
      editedIds.size() == 1U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 3U;

  const app::CreativeDesktopCommandResult deleted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutDeleteSource, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::RoofAperture, 0U, stableKey});
  const bool deletedAtomically =
      deleted.accepted && deleted.changed && deleted.worldLayoutChanged &&
      deleted.sceneChanged && editor.worldLayout.source.roofApertures.empty() &&
      (editedIds.empty() ||
       appState.facade.document().findObject(editedIds.front()) == nullptr) &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 4U;

  const app::CreativeDesktopCommandResult undoDelete =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  app::CreativeEditorWorldLayoutRoofApertureSettings restoredEditedSettings;
  const bool deleteUndoRestored =
      undoDelete.accepted &&
      app::readCreativeEditorWorldLayoutRoofApertureSettings(
          editor.worldLayout, 0U, restoredEditedSettings) &&
      restoredEditedSettings == editedSettings &&
      generatedSourceObjectIds(appState.facade.document(),
                               editor.worldLayout.source,
                               cr::CreativeWorldLayoutTable::RoofAperture,
                               0U)
              .size() ==
          1U;
  const app::CreativeDesktopCommandResult undoEdit =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  app::CreativeEditorWorldLayoutRoofApertureSettings restoredDraggedSettings;
  const bool editUndoRestored =
      undoEdit.accepted &&
      app::readCreativeEditorWorldLayoutRoofApertureSettings(
          editor.worldLayout, 0U, restoredDraggedSettings) &&
      restoredDraggedSettings == draggedSettings;
  const app::CreativeDesktopCommandResult undoDrag =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  app::CreativeEditorWorldLayoutRoofApertureSettings restoredCreatedSettings;
  const bool dragUndoRestored =
      undoDrag.accepted &&
      app::readCreativeEditorWorldLayoutRoofApertureSettings(
          editor.worldLayout, 0U, restoredCreatedSettings) &&
      restoredCreatedSettings == createdSettings;
  const app::CreativeDesktopCommandResult undoCreate =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool createUndoRestored =
      undoCreate.accepted && editor.worldLayout.source.roofApertures.empty() &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision;

  return expect(createdAtomically,
                "roof aperture create synchronizes source scene and history") &&
         expect(scopesAreExact,
                "generated skylight exposes building level and source scopes") &&
         expect(dragPreviewIsTransient,
                "roof aperture drag previews in 3D without mutating source") &&
         expect(dragCommittedOnce,
                "roof aperture drag commits source scene and one undo step") &&
         expect(editedAtomically,
                "roof aperture edit synchronizes source scene and history") &&
         expect(deletedAtomically,
                "roof aperture delete synchronizes source scene and history") &&
         expect(deleteUndoRestored && editUndoRestored && dragUndoRestored &&
                    createUndoRestored,
                "roof aperture create drag edit and delete undo independently");
}

bool sourcePropertyPreviewAndCommitAreAtomic() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Source Property Edit");
  static_cast<void>(document.assignId(910U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "source_property_edit");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  editor.worldLayout.source.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  editor.worldLayout.source.levels.push_back(level);
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room";
  room.name = "Room";
  room.footprint = {{0, 0}, {4, 4}};
  editor.worldLayout.source.rooms.push_back(room);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted) {
    return expect(false, "source property fixture generated");
  }

  app::CreativeEditorWorldLayoutRoomSettings settings;
  if (!app::readCreativeEditorWorldLayoutRoomSettings(editor.worldLayout, 0U,
                                                       settings)) {
    return expect(false, "source property room settings readable");
  }
  settings.footprint.maximum.x = 7;
  const auto payload = [&](app::CreativeDesktopWorldLayoutPropertyEditPhase
                               phase) {
    return app::CreativeDesktopWorldLayoutPropertyEditPayload{
        phase, cr::CreativeWorldLayoutTable::Room, 0U, "room", settings};
  };

  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult preview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      payload(app::CreativeDesktopWorldLayoutPropertyEditPhase::Preview));
  cr::CreativeBounds previewFloorBounds;
  const bool previewBoundsReady = generatedBounds(
      editor.worldLayout.preview.document, editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor,
      previewFloorBounds);
  const std::uint64_t previewContentRevision =
      editor.worldLayout.previewContentRevision;
  const app::CreativeDesktopCommandResult repeatedPreview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      payload(app::CreativeDesktopWorldLayoutPropertyEditPhase::Preview));
  const bool previewStayedTransient =
      preview.accepted && preview.sceneChanged && previewBoundsReady &&
      near(previewFloorBounds.max.x - previewFloorBounds.min.x, 7.0) &&
      repeatedPreview.accepted && !repeatedPreview.sceneChanged &&
      editor.worldLayout.previewContentRevision == previewContentRevision &&
      editor.worldLayout.liveEditPreviewVisible &&
      editor.worldLayout.source.rooms[0].footprint.maximum.x == 4 &&
      editor.worldLayout.revision == sourceRevisionBefore &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore;

  const app::CreativeDesktopCommandResult committed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      payload(app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit));
  cr::CreativeBounds committedFloorBounds;
  const bool committedBoundsReady = generatedBounds(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor,
      committedFloorBounds);
  const bool committedOnce =
      committed.accepted && committed.changed && committed.sceneChanged &&
      committed.worldLayoutChanged && committedBoundsReady &&
      near(committedFloorBounds.max.x - committedFloorBounds.min.x, 7.0) &&
      !editor.worldLayout.liveEditPreviewVisible &&
      editor.worldLayout.source.rooms[0].footprint.maximum.x == 7 &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      appState.facade.document().revision() > documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRestoredBoth =
      undone.accepted &&
      editor.worldLayout.source.rooms[0].footprint.maximum.x == 4;

  const std::uint64_t rejectSourceRevision = editor.worldLayout.revision;
  const std::uint64_t rejectDocumentRevision =
      appState.facade.document().revision();
  const std::uint64_t rejectUndoDepth =
      cr::creativeUndoDepth(appState.history);
  app::CreativeEditorWorldLayoutWallSettings wrongSettings;
  const app::CreativeDesktopCommandResult wrongType = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      app::CreativeDesktopWorldLayoutPropertyEditPayload{
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit,
          cr::CreativeWorldLayoutTable::Room, 0U, "room", wrongSettings});
  settings.footprint.maximum.x = 0;
  const app::CreativeDesktopCommandResult invalid = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      payload(app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit));
  const bool rejectionWasAtomic =
      !wrongType.accepted && !wrongType.changed && !invalid.accepted &&
      !invalid.changed && editor.worldLayout.revision == rejectSourceRevision &&
      appState.facade.document().revision() == rejectDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == rejectUndoDepth;

  settings.footprint.maximum.x = 6;
  const app::CreativeDesktopCommandResult previewToCancel = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      payload(app::CreativeDesktopWorldLayoutPropertyEditPhase::Preview));
  const app::CreativeDesktopCommandResult foreignCancel = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      app::CreativeDesktopWorldLayoutPropertyEditPayload{
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Cancel,
          cr::CreativeWorldLayoutTable::Room, 1U, "other-room", settings});
  const bool foreignCancelPreservedPreview =
      foreignCancel.accepted && !foreignCancel.changed &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
  const app::CreativeDesktopCommandResult canceled = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      payload(app::CreativeDesktopWorldLayoutPropertyEditPhase::Cancel));
  const bool cancelWasTransient =
      previewToCancel.accepted && canceled.accepted && canceled.sceneChanged &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.revision == rejectSourceRevision &&
      appState.facade.document().revision() == rejectDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == rejectUndoDepth;

  return expect(previewStayedTransient,
                "source property updates deduplicate exact 3D previews") &&
         expect(committedOnce,
                "source property release commits one source and scene edit") &&
         expect(undoRestoredBoth,
                "source property undo restores source and generated scene") &&
         expect(rejectionWasAtomic,
                "invalid and mismatched source properties mutate nothing") &&
         expect(foreignCancelPreservedPreview,
                "a property cancel cannot clear another target's preview") &&
         expect(cancelWasTransient,
                "source property cancel clears only the transient preview");
}

bool buildingUsabilityRepairIsTypedAtomicAndUndoable() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Building Usability Repair Command");
  static_cast<void>(document.assignId(911U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "building_usability_repair_command");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};

  app::CreativeEditorWorldLayoutBuildingBlockoutSettings blockout;
  blockout.shell.footprint = {{0, 0}, {8, 8}};
  blockout.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  blockout.connectRooms = false;
  blockout.facade.includeEntrance = false;
  blockout.facade.includeExteriorWindows = false;
  const app::CreativeDesktopCommandResult staged = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      context,
      app::CreativeDesktopWorldLayoutBuildingBlockoutPayload{blockout});
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!expect(staged.accepted && generated.accepted &&
                  editor.worldLayout.source.buildings.size() == 1U &&
                  editor.worldLayout.source.openings.empty(),
              "repair command fixture is synchronized without an entrance")) {
    return false;
  }

  cr::CreativeWorldLayoutBuildingUsabilityConfig config;
  config.gridCellSizeMeters =
      appState.facade.document().gridSettings().cellSizeMeters;
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt usability =
      cr::validateCreativeWorldLayoutBuildingUsability(
          {&editor.worldLayout.source, nullptr, config});
  const auto issueFound = std::find_if(
      usability.issues.begin(), usability.issues.begin() + usability.issueCount,
      [](const cr::CreativeWorldLayoutBuildingUsabilityIssue& issue) {
        return issue.kind ==
                   cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                       MissingExteriorEntrance &&
               issue.table == cr::CreativeWorldLayoutTable::Building &&
               issue.index == 0U;
      });
  if (!expect(issueFound != usability.issues.begin() + usability.issueCount,
              "repair command consumes the exact validator issue")) {
    return false;
  }

  const cr::CreativeWorldLayoutBuildingUsabilityIssue issue = *issueFound;
  const std::string stableKey =
      editor.worldLayout.source.buildings[0].stableKey;
  const std::uint64_t sourceRevision = editor.worldLayout.revision;
  const std::uint64_t documentRevision =
      appState.facade.document().revision();
  const std::uint64_t undoDepth = cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult mismatched = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutRepairBuildingUsability,
      context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Building, 0U, stableKey});
  const app::CreativeDesktopCommandResult stale = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutRepairBuildingUsability,
      context,
      app::CreativeDesktopWorldLayoutBuildingRepairPayload{issue,
                                                            "stale-building"});
  const bool rejectedAtomically =
      !mismatched.accepted && !stale.accepted &&
      editor.worldLayout.revision == sourceRevision &&
      appState.facade.document().revision() == documentRevision &&
      cr::creativeUndoDepth(appState.history) == undoDepth;

  const app::CreativeDesktopCommandResult repaired = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutRepairBuildingUsability,
      context,
      app::CreativeDesktopWorldLayoutBuildingRepairPayload{issue, stableKey});
  const bool repairedAtomically =
      repaired.accepted && repaired.changed && repaired.worldLayoutChanged &&
      repaired.sceneChanged && editor.worldLayout.source.openings.size() == 1U &&
      editor.worldLayout.source.openings[0].kind ==
          cr::CreativeBuildingOpeningKind::Door &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      appState.facade.document().revision() > documentRevision &&
      std::count_if(appState.facade.document().objects().begin(),
                    appState.facade.document().objects().end(),
                    [](const cr::CreativeObject& object) {
                      return object.kind == cr::CreativeObjectKind::Door;
                    }) == 1 &&
      cr::creativeUndoDepth(appState.history) == undoDepth + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRestoredBoth =
      undone.accepted && editor.worldLayout.source.openings.empty() &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      std::none_of(appState.facade.document().objects().begin(),
                   appState.facade.document().objects().end(),
                   [](const cr::CreativeObject& object) {
                     return object.kind == cr::CreativeObjectKind::Door;
                   });

  return expect(rejectedAtomically,
                "mismatched and stale repair commands mutate nothing") &&
         expect(repairedAtomically,
                "safe building repair updates source scene and one history entry") &&
         expect(undoRestoredBoth,
                "building repair undo restores source and generated scene");
}

bool measurementAnnotationsUseTypedCommandsAndHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Measurement Command Test");
  static_cast<void>(document.assignId(4510U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "measurement command document installed")) {
    return false;
  }
  app::CreativeEditorState editor;
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, nullptr};

  const std::uint64_t revisionBefore = appState.facade.document().revision();
  completeDistanceMeasurement(appState.facade);
  const bool transientIsNotHistory =
      appState.facade.document().revision() == revisionBefore &&
      cr::creativeUndoDepth(appState.history) == 0U &&
      appState.facade.document()
          .measurementAnnotationStore()
          .annotations.empty();

  const app::CreativeDesktopCommandResult saved = dispatchPayload(
      app::CreativeDesktopCommandId::SaveMeasurementAnnotation, context,
      app::CreativeDesktopMeasurementAnnotationPayload{
          cr::kInvalidCreativeMeasurementAnnotationId, "Measured aisle"});
  const auto& savedStore =
      appState.facade.document().measurementAnnotationStore();
  const cr::CreativeMeasurementAnnotationId annotationId =
      savedStore.annotations.empty()
          ? cr::kInvalidCreativeMeasurementAnnotationId
          : savedStore.annotations.front().id;
  const bool saveRecordedOnce =
      saved.accepted && saved.changed && saved.sceneChanged &&
      saved.affectedObjectCount == 1U && savedStore.annotations.size() == 1U &&
      savedStore.annotations.front().name == "Measured aisle" &&
      !appState.facade.measurementState().hasMeasurement &&
      cr::creativeUndoDepth(appState.history) == 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRemoved =
      undone.accepted && undone.changed &&
      appState.facade.document()
          .measurementAnnotationStore()
          .annotations.empty();
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redoRestored =
      redone.accepted && redone.changed &&
      cr::findCreativeMeasurementAnnotation(
          appState.facade.document().measurementAnnotationStore(),
          annotationId) != nullptr;

  const app::CreativeDesktopCommandResult removed = dispatchPayload(
      app::CreativeDesktopCommandId::RemoveMeasurementAnnotation, context,
      app::CreativeDesktopMeasurementAnnotationPayload{annotationId, {}});
  const bool removeRecordedOnce =
      removed.accepted && removed.changed && removed.sceneChanged &&
      appState.facade.document()
          .measurementAnnotationStore()
          .annotations.empty() &&
      cr::creativeUndoDepth(appState.history) == 2U;
  const app::CreativeDesktopCommandResult removeUndone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool removeUndoRestored =
      removeUndone.accepted &&
      cr::findCreativeMeasurementAnnotation(
          appState.facade.document().measurementAnnotationStore(),
          annotationId) != nullptr;

  const std::uint64_t revisionBeforeMissingRemove =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeMissingRemove =
      cr::creativeUndoDepth(appState.history);
  const std::uint64_t redoBeforeMissingRemove =
      cr::creativeRedoDepth(appState.history);
  const app::CreativeDesktopCommandResult missingRemove = dispatchPayload(
      app::CreativeDesktopCommandId::RemoveMeasurementAnnotation, context,
      app::CreativeDesktopMeasurementAnnotationPayload{annotationId + 100U, {}});
  const bool missingRemoveAtomic =
      !missingRemove.accepted && !missingRemove.changed &&
      !missingRemove.sceneChanged &&
      appState.facade.document().revision() == revisionBeforeMissingRemove &&
      cr::creativeUndoDepth(appState.history) == undoBeforeMissingRemove &&
      cr::creativeRedoDepth(appState.history) == redoBeforeMissingRemove;

  const std::uint64_t revisionBeforeMismatch =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeMismatch =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::SaveMeasurementAnnotation, context,
      app::CreativeDesktopSaveAsPayload{"wrong payload"});
  const bool mismatchAtomic =
      !mismatch.accepted && !mismatch.changed && !mismatch.sceneChanged &&
      appState.facade.document().revision() == revisionBeforeMismatch &&
      cr::creativeUndoDepth(appState.history) == undoBeforeMismatch;

  return expect(transientIsNotHistory,
                "transient measurement changes no revision or history") &&
         expect(saveRecordedOnce,
                "measurement save records one semantic history entry") &&
         expect(undoRemoved && redoRestored,
                "measurement annotation save supports undo and redo") &&
         expect(removeRecordedOnce && removeUndoRestored,
                "measurement annotation removal is one undoable command") &&
         expect(missingRemoveAtomic,
                "missing measurement removal preserves document and history") &&
         expect(mismatchAtomic,
                "measurement payload mismatch mutates no document or history");
}

bool terrainOperationTransformCommandOpensTheSharedSession() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Terrain Transform Command");
  static_cast<void>(document.assignId(4511U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "terrain transform command document installed")) {
    return false;
  }
  cr::CreativeTerrainOperationMutationRequest add;
  add.kind = cr::CreativeTerrainOperationMutationKind::Add;
  add.operationKind = cr::CreativeTerrainOperationKind::Landform;
  add.landform.bounds = {{4, 5}, 4U, 4U};
  add.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  add.landform.edgeWidthCells = 0U;
  const cr::CreativeTerrainOperationMutationReceipt operation =
      appState.facade.applyTerrainOperationMutation(add);
  appState.history = {};
  app::CreativeEditorState editor;
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, nullptr};
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();

  const app::CreativeDesktopCommandResult opened = dispatchPayload(
      app::CreativeDesktopCommandId::TerrainOperationTransform, context,
      app::CreativeDesktopTerrainOperationPayload{
          operation.operationId, true, 0U});
  const bool sharedSession =
      opened.accepted && opened.changed && !opened.sceneChanged &&
      editor.transform.active && editor.transform.plan.accepted &&
      editor.transform.preflight.ownershipRoute ==
          app::CreativeEditorTransformOwnershipRoute::TerrainOperation &&
      editor.transform.preflight.terrainOperationId == operation.operationId &&
      appState.facade.document().revision() == revisionBefore &&
      cr::creativeUndoDepth(appState.history) == 0U;

  const app::CreativeDesktopCommandResult blocked = dispatchPayload(
      app::CreativeDesktopCommandId::TerrainOperationTransform, context,
      app::CreativeDesktopTerrainOperationPayload{
          operation.operationId, true, 0U});
  const bool secondSessionBlocked =
      !blocked.accepted && !blocked.changed && editor.transform.active &&
      blocked.message == "finish the current transform first";
  return expect(operation.accepted && sharedSession,
                "terrain command opens the shared transient transform") &&
         expect(secondSessionBlocked,
                "terrain command cannot replace an active transform session");
}


}  // namespace

bool runCreativeDesktopWorldLayoutToolCommandTests() {
  bool ok = true;
  ok = worldLayoutCatalogSelectionAndPlacementUseTypedCommands() && ok;
  ok = worldLayoutOpeningInsertCommandsUseCatalogAndHistory() && ok;
  ok = worldLayoutAssetRepairCommandsPreservePlacementAndHistory() && ok;
  ok = worldLayoutTerrainRegionCommandsRespectWorkspaceGuards() && ok;
  ok = synchronizedCreationPreviewAndPointCommitAreAtomic() && ok;
  ok = synchronizedTerrainPathDraftPreviewsAndCommitsAtomically() && ok;
  ok = synchronizedPlanDragPreviewsAndCommitsOneStoreyAtomically() && ok;
  ok = synchronizedRoofHandlesPreviewAndCommitOneLevelAtomically() && ok;
  ok = sourcePropertyTablesAreDerivedFromSettingsTypes() && ok;
  ok = roofApertureCommandsSynchronizeSourceSceneAndHistory() && ok;
  ok = sourcePropertyPreviewAndCommitAreAtomic() && ok;
  ok = buildingUsabilityRepairIsTypedAtomicAndUndoable() && ok;
  ok = measurementAnnotationsUseTypedCommandsAndHistory() && ok;
  ok = terrainOperationTransformCommandOpensTheSharedSession() && ok;
  return ok;
}
