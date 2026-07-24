#include "creative_desktop_command_test_runners.hpp"
#include "creative_desktop_command_test_support.hpp"

namespace {

bool buildingGroundingCommandsShareSourceAndGeneratedTransactions() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Building Grounding");
  static_cast<void>(document.assignId(478U));
  constexpr std::array<std::uint16_t, 4U> heights{2U, 3U, 2U, 3U};
  static_cast<void>(
      document.replaceTerrainHeightField({{0, 0}, 2U, 2U}, heights));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "building_grounding_commands");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "grounded_house";
  building.name = "Grounded House";
  building.rootFootprint = {{0, 0}, {2, 2}};
  editor.worldLayout.source.buildings.push_back(building);
  editor.worldLayout.source.levels.push_back(
      {0U, "ground", "Ground", 0.05, 3U, 1U, 1U, 1U});
  editor.worldLayout.source.rooms.push_back(
      {0U, 0U, "room", "Room", {{0, 0}, {2, 2}}, 0.25});
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopWorldLayoutBuildingGroundingPayload sourceGrounded{
      cr::kInvalidObjectId,
      0U,
      "grounded_house",
      {cr::CreativeWorldLayoutGroundingMode::Foundation, 1U}};
  const app::CreativeDesktopCommandResult sourceChanged = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetBuildingGrounding, context,
      sourceGrounded);
  app::CreativeDesktopWorldLayoutBuildingGroundingPayload sourceAbsolute =
      sourceGrounded;
  sourceAbsolute.settings.mode =
      cr::CreativeWorldLayoutGroundingMode::Absolute;
  const app::CreativeDesktopCommandResult sourceRestored = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetBuildingGrounding, context,
      sourceAbsolute);

  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeObject* initialFloor = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor);
  if (!generated.accepted || initialFloor == nullptr) {
    return expect(false, "building grounding command fixture generated");
  }
  const cr::CreativeObjectId floorId = initialFloor->id;
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  app::CreativeDesktopWorldLayoutBuildingGroundingPayload generatedGrounded =
      sourceGrounded;
  generatedGrounded.objectId = floorId;
  const app::CreativeDesktopCommandResult grounded = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingGrounding,
      context, generatedGrounded);
  const cr::CreativeObject* groundedFloor = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor);
  const auto foundationFound = std::find_if(
      appState.facade.document().objects().begin(),
      appState.facade.document().objects().end(),
      [](const cr::CreativeObject& object) {
        return object.name == "Grounded House Foundation";
      });
  const bool groundedState =
      grounded.accepted && grounded.changed && grounded.worldLayoutChanged &&
      grounded.sceneChanged && groundedFloor != nullptr &&
      near(groundedFloor->bounds.min.y, 3.0) &&
      foundationFound != appState.facade.document().objects().end() &&
      editor.worldLayout.source.buildings[0].groundingMode ==
          cr::CreativeWorldLayoutGroundingMode::Foundation &&
      editor.worldLayout.source.buildings[0].maximumGroundReliefCells == 1U &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* restoredFloor = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor);
  const bool undoRestored =
      undone.accepted && restoredFloor != nullptr &&
      near(restoredFloor->bounds.min.y, 0.0) &&
      editor.worldLayout.source.buildings[0].groundingMode ==
          cr::CreativeWorldLayoutGroundingMode::Absolute &&
      std::none_of(appState.facade.document().objects().begin(),
                   appState.facade.document().objects().end(),
                   [](const cr::CreativeObject& object) {
                     return object.name == "Grounded House Foundation";
                   });
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeObject* redoneFloor = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor);

  app::CreativeDesktopWorldLayoutBuildingGroundingPayload stale =
      generatedGrounded;
  stale.stableKey = "stale_house";
  const std::uint64_t rejectRevision = editor.worldLayout.revision;
  const std::uint64_t rejectDocumentRevision =
      appState.facade.document().revision();
  const app::CreativeDesktopCommandResult rejected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingGrounding,
      context, stale);

  return expect(sourceChanged.accepted && sourceChanged.changed &&
                    sourceChanged.worldLayoutChanged &&
                    sourceRestored.accepted && sourceRestored.changed,
                "2D grounding command edits semantic source only") &&
         expect(groundedState,
                "generated grounding updates source scene and one undo") &&
         expect(undoRestored && redone.accepted && redoneFloor != nullptr &&
                    near(redoneFloor->bounds.min.y, 3.0),
                "grounding undo and redo restore source and geometry") &&
         expect(!rejected.accepted && !rejected.changed &&
                    editor.worldLayout.revision == rejectRevision &&
                    appState.facade.document().revision() ==
                        rejectDocumentRevision,
                "stale grounding target rejects atomically");
}

bool generatedLevelSettingsRebuildEveryRoomAtomically() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Generated Level Editing");
  static_cast<void>(document.assignId(439U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "generated_level_editing");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "level_house";
  building.name = "Level House";
  editor.worldLayout.source.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel ground;
  ground.buildingIndex = 0U;
  ground.stableKey = "ground";
  ground.name = "Ground";
  ground.floorTopLayer = 0.0;
  ground.wallHeightCells = 3U;
  editor.worldLayout.source.levels.push_back(ground);
  cr::CreativeWorldLayoutLevel upper;
  upper.buildingIndex = 0U;
  upper.stableKey = "upper";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  upper.wallHeightCells = 3U;
  upper.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  upper.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  upper.roofPitchDegrees = 25.0;
  editor.worldLayout.source.levels.push_back(upper);

  editor.worldLayout.source.rooms.push_back(
      {0U, 0U, "ground_room", "Ground Room", {{0, 0}, {12, 6}}, 0.25});
  editor.worldLayout.source.rooms.push_back(
      {0U, 1U, "upper_west", "Upper West", {{0, 0}, {6, 6}}, 0.25});
  editor.worldLayout.source.rooms.push_back(
      {0U, 1U, "upper_east", "Upper East", {{6, 0}, {12, 6}}, 0.25});

  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 2U;
  opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  opening.kind = cr::CreativeBuildingOpeningKind::Window;
  opening.stableKey = "upper_window";
  opening.name = "Upper Window";
  opening.centerOffsetCells = 3.0;
  opening.widthCells = 1.5;
  opening.cutoutBottomCells = 1.0;
  opening.cutoutHeightCells = 1.0;
  editor.worldLayout.source.openings.push_back(opening);
  editor.worldLayout.source.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "level_stair",
       "Level Stair",
       {{1, 2}, {5, 4}}});
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted) {
    return expect(false, "generated level editing fixture generated");
  }

  const cr::CreativeObject* roof = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Level, 1U,
      cr::CreativeObjectKind::RoofSlope);
  const cr::CreativeObject* roomFloor = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  if (roof == nullptr || roomFloor == nullptr) {
    return expect(false, "generated level roof and room floor exist");
  }
  const cr::CreativeObjectId roomFloorScopeObjectId = roomFloor->id;

  app::CreativeEditorWorldLayoutLevelSettings settings;
  static_cast<void>(app::readCreativeEditorWorldLayoutLevelSettings(
      editor.worldLayout, 1U, settings));
  settings.name = "Upper Edited";
  settings.wallHeightCells = 4U;
  settings.floorThicknessLayers = 2U;
  settings.ceilingThicknessLayers = 2U;
  settings.roofThicknessLayers = 2U;
  settings.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  settings.roofPitchDegrees = 35.0;
  settings.roofOverhangCells = 0.5;

  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult previewed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPreviewGeneratedLevelSettings,
      context,
      app::CreativeDesktopGeneratedLevelSettingsPayload{
          roomFloorScopeObjectId, 1U, "upper", settings});
  cr::CreativeBounds previewWestWalls;
  cr::CreativeBounds previewEastWalls;
  const bool previewHasBothRooms =
      generatedRoomContributorBounds(editor.worldLayout.preview.document,
                                     editor.worldLayout.source, 1U,
                                     cr::CreativeObjectKind::Wall,
                                     previewWestWalls) &&
      generatedRoomContributorBounds(editor.worldLayout.preview.document,
                                     editor.worldLayout.source, 2U,
                                     cr::CreativeObjectKind::Wall,
                                     previewEastWalls);
  const bool previewStayedTransient =
      previewed.accepted && previewed.sceneChanged &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      previewHasBothRooms &&
      near(previewWestWalls.max.y - previewWestWalls.min.y, 7.0) &&
      near(previewEastWalls.max.y - previewEastWalls.min.y, 7.0) &&
      editor.worldLayout.source.levels[1].name == "Upper" &&
      editor.worldLayout.source.levels[1].wallHeightCells == 3U &&
      editor.worldLayout.revision == sourceRevisionBefore &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore;

  const app::CreativeDesktopCommandResult applied = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings,
      context,
      app::CreativeDesktopGeneratedLevelSettingsPayload{
          roomFloorScopeObjectId, 1U, "upper", settings});
  cr::CreativeBounds westWalls;
  cr::CreativeBounds eastWalls;
  cr::CreativeBounds groundWalls;
  const bool rebuiltRoomBounds =
      generatedRoomContributorBounds(appState.facade.document(),
                                     editor.worldLayout.source, 1U,
                                     cr::CreativeObjectKind::Wall, westWalls) &&
      generatedRoomContributorBounds(appState.facade.document(),
                                     editor.worldLayout.source, 2U,
                                     cr::CreativeObjectKind::Wall, eastWalls) &&
      generatedRoomContributorBounds(appState.facade.document(),
                                     editor.worldLayout.source, 0U,
                                     cr::CreativeObjectKind::Wall, groundWalls);
  const bool rebuiltLevelAndPreservedDependents =
      applied.accepted && applied.changed && applied.worldLayoutChanged &&
      applied.sceneChanged && rebuiltRoomBounds &&
      near(westWalls.max.y - westWalls.min.y, 7.0) &&
      near(eastWalls.max.y - eastWalls.min.y, 7.0) &&
      near(groundWalls.max.y - groundWalls.min.y, 7.0) &&
      editor.worldLayout.source.levels[1].name == "Upper Edited" &&
      editor.worldLayout.source.levels[1].floorThicknessLayers == 2U &&
      editor.worldLayout.source.levels[1].ceilingThicknessLayers == 2U &&
      editor.worldLayout.source.levels[1].roofThicknessLayers == 2U &&
      findGeneratedObject(appState.facade.document(),
                          editor.worldLayout.source,
                          cr::CreativeWorldLayoutTable::Opening, 0U,
                          cr::CreativeObjectKind::Window) != nullptr &&
      findGeneratedObject(appState.facade.document(),
                          editor.worldLayout.source,
                          cr::CreativeWorldLayoutTable::VerticalConnector, 0U,
                          cr::CreativeObjectKind::Stair) != nullptr &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRestoredLevel =
      undone.accepted && editor.worldLayout.source.levels[1].name == "Upper" &&
      editor.worldLayout.source.levels[1].wallHeightCells == 3U &&
      editor.worldLayout.source.levels[1].floorThicknessLayers == 1U;
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redoRestoredLevel =
      redone.accepted &&
      editor.worldLayout.source.levels[1].name == "Upper Edited" &&
      editor.worldLayout.source.levels[1].wallHeightCells == 4U &&
      editor.worldLayout.source.levels[1].roofPitchDegrees == 35.0;

  const cr::CreativeObject* liveRoof = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Level, 1U,
      cr::CreativeObjectKind::RoofSlope);
  const cr::CreativeObject* liveRoomFloor = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  if (liveRoof == nullptr || liveRoomFloor == nullptr) {
    return expect(false, "level edit targets survive redo");
  }
  const std::uint64_t sourceRevisionBeforeReject =
      editor.worldLayout.revision;
  const std::uint64_t documentRevisionBeforeReject =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeReject = cr::creativeUndoDepth(appState.history);
  app::CreativeEditorWorldLayoutLevelSettings connectorInvalid = settings;
  connectorInvalid.floorTopLayer = 5.0;
  const app::CreativeDesktopCommandResult rejectedPreview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPreviewGeneratedLevelSettings,
      context, app::CreativeDesktopGeneratedLevelSettingsPayload{
                   liveRoof->id, 1U, "upper", connectorInvalid});
  const app::CreativeDesktopCommandResult rejectedConnector = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings,
      context, app::CreativeDesktopGeneratedLevelSettingsPayload{
                   liveRoof->id, 1U, "upper", connectorInvalid});
  app::CreativeEditorWorldLayoutLevelSettings duplicateElevation = settings;
  duplicateElevation.floorTopLayer = 0.0;
  const app::CreativeDesktopCommandResult rejectedDuplicate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings,
      context, app::CreativeDesktopGeneratedLevelSettingsPayload{
                   liveRoof->id, 1U, "upper", duplicateElevation});
  const app::CreativeDesktopCommandResult wrongSource = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings,
      context, app::CreativeDesktopGeneratedLevelSettingsPayload{
                   liveRoomFloor->id, 0U, "ground", settings});
  const bool rejectedAtomically =
      !rejectedPreview.accepted && !rejectedPreview.changed &&
      !rejectedConnector.accepted && !rejectedConnector.changed &&
      !rejectedDuplicate.accepted && !rejectedDuplicate.changed &&
      !wrongSource.accepted && !wrongSource.changed &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.levels[1].floorTopLayer == 3.0 &&
      editor.worldLayout.source.levels[1].wallHeightCells == 4U &&
      editor.worldLayout.source.openings[0].stableKey == "upper_window" &&
      editor.worldLayout.source.verticalConnectors[0].stableKey ==
          "level_stair" &&
      editor.worldLayout.revision == sourceRevisionBeforeReject &&
      appState.facade.document().revision() == documentRevisionBeforeReject &&
      cr::creativeUndoDepth(appState.history) == undoBeforeReject;

  return expect(previewStayedTransient,
                "generated level preview rebuilds every room transiently") &&
         expect(rebuiltLevelAndPreservedDependents,
                "level edit rebuilds all rooms and preserves dependents") &&
         expect(undoRestoredLevel && redoRestoredLevel,
                "level edit records exactly one semantic undo step") &&
         expect(rejectedAtomically,
                "invalid elevation topology and wrong source reject atomically");
}

bool generatedRoomSettingsRebuildTopologyAtomically() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Generated Room Editing");
  static_cast<void>(document.assignId(438U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "generated_room_editing");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  editor.worldLayout.source.buildings.push_back(building);
  editor.worldLayout.source.levels.push_back(
      {0U, "ground", "Ground", 0.0, 3U, 1U, 1U, 1U});
  editor.worldLayout.source.levels.push_back(
      {0U, "upper", "Upper", 3.0, 3U, 1U, 1U, 1U});
  editor.worldLayout.source.levels[1].wallHeightCells = 4U;
  editor.worldLayout.source.levels[1].floorThicknessLayers = 2U;
  editor.worldLayout.source.levels[1].roofThicknessLayers = 2U;
  editor.worldLayout.source.levels[1].roofStyle =
      cr::CreativeStructuralRoofStyle::Gable;
  editor.worldLayout.source.levels[1].roofRidgeAxis =
      cr::CreativeStructuralRoofRidgeAxis::Z;
  editor.worldLayout.source.levels[1].roofPitchDegrees = 35.0;
  editor.worldLayout.source.levels[1].roofOverhangCells = 0.5;
  editor.worldLayout.source.rooms.push_back(
      {0U, 0U, "ground_room", "Ground Room", {{0, 0}, {8, 8}}, 0.25});
  editor.worldLayout.source.rooms.push_back(
      {0U, 1U, "upper_room", "Upper Room", {{0, 0}, {8, 8}}, 0.25});
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 1U;
  opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  opening.stableKey = "upper_door";
  opening.name = "Upper Door";
  opening.centerOffsetCells = 4.0;
  opening.widthCells = 1.0;
  opening.cutoutHeightCells = 2.1;
  editor.worldLayout.source.openings.push_back(opening);
  editor.worldLayout.source.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "main_stair",
       "Main Stair",
       {{1, 2}, {5, 4}}});
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted) {
    return expect(false, "generated room editing fixture generated");
  }

  const auto generatedObject =
      [&](cr::CreativeWorldLayoutTable table, std::size_t index,
          cr::CreativeObjectKind kind) -> const cr::CreativeObject* {
    for (const cr::CreativeObject& object :
         appState.facade.document().objects()) {
      const cr::CreativeWorldLayoutObjectProvenance provenance =
          cr::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, object);
      if (provenance.owned && provenance.table == table &&
          provenance.index == index && object.kind == kind) {
        return &object;
      }
    }
    return nullptr;
  };
  const auto generatedBounds =
      [&](const cr::CreativeDocument& sourceDocument,
          cr::CreativeWorldLayoutTable table, std::size_t index,
          cr::CreativeObjectKind kind, cr::CreativeBounds& output) {
    bool found = false;
    for (const cr::CreativeObject& object : sourceDocument.objects()) {
      const cr::CreativeWorldLayoutObjectProvenance provenance =
          cr::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, object);
      if (!provenance.owned || provenance.table != table ||
          provenance.index != index || object.kind != kind) {
        continue;
      }
      const cr::CreativeTransformedBounds bounds =
          cr::resolveCreativeObjectBounds(object);
      if (!bounds.valid) {
        continue;
      }
      if (!found) {
        output = bounds.worldBounds;
        found = true;
      } else {
        output.min.x = std::min(output.min.x, bounds.worldBounds.min.x);
        output.min.y = std::min(output.min.y, bounds.worldBounds.min.y);
        output.min.z = std::min(output.min.z, bounds.worldBounds.min.z);
        output.max.x = std::max(output.max.x, bounds.worldBounds.max.x);
        output.max.y = std::max(output.max.y, bounds.worldBounds.max.y);
        output.max.z = std::max(output.max.z, bounds.worldBounds.max.z);
      }
    }
    return found;
  };

  const cr::CreativeObject* upperFloor = generatedObject(
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  const cr::CreativeObject* connector = generatedObject(
      cr::CreativeWorldLayoutTable::VerticalConnector, 0U,
      cr::CreativeObjectKind::Stair);
  const cr::CreativeObject* generatedDoor = generatedObject(
      cr::CreativeWorldLayoutTable::Opening, 0U,
      cr::CreativeObjectKind::Door);
  if (upperFloor == nullptr || connector == nullptr ||
      generatedDoor == nullptr) {
    return expect(false,
                  "room opening and connector provenance objects exist");
  }
  const cr::CreativeObjectId roomScopeObjectId = generatedDoor->id;
  cr::CreativeBounds upperFloorBoundsBefore;
  const bool initialFloorBounds = generatedBounds(
      appState.facade.document(), cr::CreativeWorldLayoutTable::Room, 1U,
      cr::CreativeObjectKind::Floor, upperFloorBoundsBefore);

  app::CreativeEditorWorldLayoutRoomSettings settings;
  static_cast<void>(app::readCreativeEditorWorldLayoutRoomSettings(
      editor.worldLayout, 1U, settings));
  settings.footprint.maximum.x = 10;

  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult previewed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPreviewGeneratedRoomSettings,
      context,
      app::CreativeDesktopGeneratedRoomSettingsPayload{
          roomScopeObjectId, 1U, "upper_room", settings});
  cr::CreativeBounds previewFloorBounds;
  const bool hasPreviewFloorBounds = generatedBounds(
      editor.worldLayout.preview.document, cr::CreativeWorldLayoutTable::Room,
      1U, cr::CreativeObjectKind::Floor, previewFloorBounds);
  const bool previewStayedTransient =
      previewed.accepted && previewed.sceneChanged &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      hasPreviewFloorBounds &&
      near(previewFloorBounds.max.x - previewFloorBounds.min.x, 10.0) &&
      editor.worldLayout.source.rooms[1].footprint.maximum.x == 8 &&
      editor.worldLayout.source.levels[1].wallHeightCells == 4U &&
      editor.worldLayout.revision == sourceRevisionBefore &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore;

  const app::CreativeDesktopCommandResult applied = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings,
      context,
      app::CreativeDesktopGeneratedRoomSettingsPayload{
          roomScopeObjectId, 1U, "upper_room", settings});
  if (!applied.accepted || editor.worldLayout.source.rooms.size() < 2U ||
      editor.worldLayout.source.levels.size() < 2U) {
    return expect(false,
                  "generated room settings apply preserves source topology");
  }
  const cr::CreativeObject* resizedFloor = generatedObject(
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  const cr::CreativeObject* resizedWall = generatedObject(
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Wall);
  const cr::CreativeObject* resizedRoof = generatedObject(
      cr::CreativeWorldLayoutTable::Level, 1U,
      cr::CreativeObjectKind::RoofSlope);
  const cr::CreativeObject* preservedOpening = generatedObject(
      cr::CreativeWorldLayoutTable::Opening, 0U,
      cr::CreativeObjectKind::Door);
  const cr::CreativeObject* preservedConnector = generatedObject(
      cr::CreativeWorldLayoutTable::VerticalConnector, 0U,
      cr::CreativeObjectKind::Stair);
  cr::CreativeBounds resizedFloorBounds;
  cr::CreativeBounds resizedWallBounds;
  const bool hasResizedFloorBounds = generatedBounds(
      appState.facade.document(), cr::CreativeWorldLayoutTable::Room, 1U,
      cr::CreativeObjectKind::Floor, resizedFloorBounds);
  const bool hasResizedWallBounds = generatedBounds(
      appState.facade.document(), cr::CreativeWorldLayoutTable::Room, 1U,
      cr::CreativeObjectKind::Wall, resizedWallBounds);
  const bool rebuiltCompleteShell =
      applied.accepted && applied.changed && applied.worldLayoutChanged &&
      applied.sceneChanged &&
      editor.worldLayout.source.rooms[1].footprint.maximum.x == 10 &&
      editor.worldLayout.source.levels[1].wallHeightCells == 4U &&
      editor.worldLayout.source.levels[1].floorThicknessLayers == 2U &&
      editor.worldLayout.source.levels[1].roofStyle ==
          cr::CreativeStructuralRoofStyle::Gable &&
      editor.worldLayout.source.openings[0].stableKey == "upper_door" &&
      editor.worldLayout.source.verticalConnectors[0].stableKey ==
          "main_stair" &&
      resizedFloor != nullptr && resizedWall != nullptr &&
      resizedRoof != nullptr && preservedOpening != nullptr &&
      preservedConnector != nullptr && initialFloorBounds &&
      hasResizedFloorBounds && hasResizedWallBounds &&
      near(resizedFloorBounds.max.x - resizedFloorBounds.min.x, 10.0) &&
      near(resizedWallBounds.max.y - resizedWallBounds.min.y, 4.0) &&
      near(upperFloorBoundsBefore.max.x - upperFloorBoundsBefore.min.x, 8.0) &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      appState.facade.document().revision() > documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  if (!undone.accepted || editor.worldLayout.source.rooms.size() < 2U ||
      editor.worldLayout.source.levels.size() < 2U) {
    return expect(false, "generated room settings undo preserves topology");
  }
  const cr::CreativeObject* undoneFloor = generatedObject(
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  cr::CreativeBounds undoneFloorBounds;
  const bool hasUndoneFloorBounds = generatedBounds(
      appState.facade.document(), cr::CreativeWorldLayoutTable::Room, 1U,
      cr::CreativeObjectKind::Floor, undoneFloorBounds);
  const bool undoRestoredTopology =
      undone.accepted &&
      editor.worldLayout.source.rooms[1].footprint.maximum.x == 8 &&
      editor.worldLayout.source.levels[1].wallHeightCells == 4U &&
      editor.worldLayout.source.levels[1].roofStyle ==
          cr::CreativeStructuralRoofStyle::Gable &&
      undoneFloor != nullptr && hasUndoneFloorBounds &&
      near(undoneFloorBounds.max.x - undoneFloorBounds.min.x, 8.0);
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  if (!redone.accepted || editor.worldLayout.source.rooms.size() < 2U ||
      editor.worldLayout.source.levels.size() < 2U) {
    return expect(false, "generated room settings redo preserves topology");
  }
  const cr::CreativeObject* redoneFloor = generatedObject(
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  cr::CreativeBounds redoneFloorBounds;
  const bool hasRedoneFloorBounds = generatedBounds(
      appState.facade.document(), cr::CreativeWorldLayoutTable::Room, 1U,
      cr::CreativeObjectKind::Floor, redoneFloorBounds);
  const bool redoRestoredTopology =
      redone.accepted &&
      editor.worldLayout.source.rooms[1].footprint.maximum.x == 10 &&
      editor.worldLayout.source.levels[1].roofStyle ==
          cr::CreativeStructuralRoofStyle::Gable &&
      redoneFloor != nullptr && hasRedoneFloorBounds &&
      near(redoneFloorBounds.max.x - redoneFloorBounds.min.x, 10.0);

  const cr::CreativeObject* liveRoomObject = generatedObject(
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  const cr::CreativeObject* liveConnector = generatedObject(
      cr::CreativeWorldLayoutTable::VerticalConnector, 0U,
      cr::CreativeObjectKind::Stair);
  if (liveRoomObject == nullptr || liveConnector == nullptr) {
    return expect(false, "room topology survives semantic redo");
  }
  const std::uint64_t sourceRevisionBeforeReject =
      editor.worldLayout.revision;
  const std::uint64_t documentRevisionBeforeReject =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeReject =
      cr::creativeUndoDepth(appState.history);
  app::CreativeEditorWorldLayoutRoomSettings openingInvalid = settings;
  openingInvalid.footprint.maximum.x = 4;
  const app::CreativeDesktopCommandResult rejectedOpening = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings,
      context, app::CreativeDesktopGeneratedRoomSettingsPayload{
                   liveRoomObject->id, 1U, "upper_room", openingInvalid});
  app::CreativeEditorWorldLayoutRoomSettings connectorInvalid = settings;
  connectorInvalid.footprint = {{2, 0}, {10, 8}};
  const app::CreativeDesktopCommandResult rejectedConnector = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings,
      context, app::CreativeDesktopGeneratedRoomSettingsPayload{
                   liveRoomObject->id, 1U, "upper_room", connectorInvalid});
  const app::CreativeDesktopCommandResult wrongSource = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings,
      context, app::CreativeDesktopGeneratedRoomSettingsPayload{
                   liveConnector->id, 1U, "upper_room", settings});
  const bool rejectedAtomically =
      !rejectedOpening.accepted && !rejectedOpening.changed &&
      !rejectedConnector.accepted && !rejectedConnector.changed &&
      !wrongSource.accepted && !wrongSource.changed &&
      editor.worldLayout.source.rooms[1].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      editor.worldLayout.source.rooms[1].footprint.maximum ==
          cr::CreativeTerrainCoord2{10, 8} &&
      editor.worldLayout.revision == sourceRevisionBeforeReject &&
      appState.facade.document().revision() == documentRevisionBeforeReject &&
      cr::creativeUndoDepth(appState.history) == undoBeforeReject;

  return expect(previewStayedTransient,
                "generated room preview is exact and transient") &&
         expect(rebuiltCompleteShell,
                "room edit rebuilds shell dependents in one history step") &&
         expect(undoRestoredTopology && redoRestoredTopology,
                "room edit undo and redo restore source and generated scene") &&
         expect(rejectedAtomically,
                "opening connector and source conflicts reject atomically");
}

bool generatedWallAndOpeningSettingsCommitSourceAndSceneTogether() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Generated Structure Editing");
  static_cast<void>(document.assignId(435U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "generated_structure_editing");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  editor.worldLayout.source.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  editor.worldLayout.source.levels.push_back(level);
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "partition";
  wall.name = "Partition";
  wall.start = {0, 0};
  wall.end = {8, 0};
  editor.worldLayout.source.walls.push_back(wall);
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  opening.wallIndex = 0U;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  opening.stableKey = "door";
  opening.name = "Door";
  opening.centerOffsetCells = 4.0;
  opening.widthCells = 1.0;
  opening.cutoutHeightCells = 2.1;
  editor.worldLayout.source.openings.push_back(opening);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted) {
    return expect(false, "generated structure editing fixture generated");
  }

  const auto generatedObjectId = [&](cr::CreativeWorldLayoutTable table) {
    for (const cr::CreativeObject& object :
         appState.facade.document().objects()) {
      const cr::CreativeWorldLayoutObjectProvenance provenance =
          cr::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, object);
      if (provenance.owned && provenance.table == table) {
        return object.id;
      }
    }
    return cr::kInvalidObjectId;
  };
  const cr::CreativeObjectId wallObjectId =
      generatedObjectId(cr::CreativeWorldLayoutTable::Wall);
  const cr::CreativeObjectId openingObjectId =
      generatedObjectId(cr::CreativeWorldLayoutTable::Opening);
  if (wallObjectId == cr::kInvalidObjectId ||
      openingObjectId == cr::kInvalidObjectId) {
    return expect(false, "wall and opening provenance objects exist");
  }

  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  app::CreativeEditorWorldLayoutWallSettings wallSettings;
  static_cast<void>(app::readCreativeEditorWorldLayoutWallSettings(
      editor.worldLayout, 0U, wallSettings));
  wallSettings.heightCells = 5U;
  wallSettings.thicknessCells = 0.35;
  const app::CreativeDesktopCommandResult wallPreview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPreviewGeneratedWallSettings,
      context,
      app::CreativeDesktopGeneratedWallSettingsPayload{wallObjectId,
                                                        wallSettings});
  const bool wallPreviewOnly =
      wallPreview.accepted && wallPreview.sceneChanged &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.source.walls[0].heightCells ==
          cr::kDefaultCreativeWorldLayoutWallHeightCells &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore;
  const app::CreativeDesktopCommandResult wallUpdated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings,
      context,
      app::CreativeDesktopGeneratedWallSettingsPayload{wallObjectId,
                                                        wallSettings});
  const bool wallSynchronized =
      editor.worldLayout.source.walls[0].heightCells == 5U &&
      near(editor.worldLayout.source.walls[0].thicknessCells, 0.35) &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision;

  const cr::CreativeObjectId liveOpeningObjectId =
      generatedObjectId(cr::CreativeWorldLayoutTable::Opening);
  app::CreativeEditorWorldLayoutOpeningSettings openingSettings;
  static_cast<void>(app::readCreativeEditorWorldLayoutOpeningSettings(
      editor.worldLayout, 0U, openingSettings));
  openingSettings.widthCells = 1.5;
  openingSettings.heightCells = 2.5;
  openingSettings.door.hingeSide = cr::CreativeDoorHingeSide::MinimumEdge;
  openingSettings.door.swingSide = cr::CreativeDoorSwingSide::PositiveNormal;
  openingSettings.door.initialState = cr::CreativeDoorInitialState::Open;
  const std::uint64_t documentRevisionBeforeOpeningPreview =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeOpeningPreview =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult openingPreview = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{
          liveOpeningObjectId, openingSettings});
  const bool openingPreviewOnly =
      openingPreview.accepted && openingPreview.sceneChanged &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      near(editor.worldLayout.source.openings[0].widthCells, 1.0) &&
      appState.facade.document().revision() ==
          documentRevisionBeforeOpeningPreview &&
      cr::creativeUndoDepth(appState.history) == undoBeforeOpeningPreview;
  const app::CreativeDesktopCommandResult openingUpdated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{
          liveOpeningObjectId, openingSettings});
  const bool openingSynchronized =
      near(editor.worldLayout.source.openings[0].widthCells, 1.5) &&
      near(editor.worldLayout.source.openings[0].cutoutHeightCells, 2.5) &&
      editor.worldLayout.source.openings[0].door == openingSettings.door &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision;

  const std::uint64_t revisionBeforeReject = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBeforeReject =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeReject =
      cr::creativeUndoDepth(appState.history);
  openingSettings.widthCells = -1.0;
  const app::CreativeDesktopCommandResult invalidOpening = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{
          generatedObjectId(cr::CreativeWorldLayoutTable::Opening),
          openingSettings});
  const app::CreativeDesktopCommandResult wrongSource = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings,
      context,
      app::CreativeDesktopGeneratedWallSettingsPayload{
          generatedObjectId(cr::CreativeWorldLayoutTable::Opening),
          wallSettings});
  const bool rejectedAtomically =
      !invalidOpening.accepted && !invalidOpening.changed &&
      !wrongSource.accepted && !wrongSource.changed &&
      editor.worldLayout.revision == revisionBeforeReject &&
      documentRevisionBeforeReject ==
          appState.facade.document().revision() &&
      cr::creativeUndoDepth(appState.history) == undoBeforeReject;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRestoredOpening =
      near(editor.worldLayout.source.openings[0].widthCells, 1.0) &&
      editor.worldLayout.source.openings[0].door.initialState ==
          cr::CreativeDoorInitialState::Closed &&
      editor.worldLayout.source.walls[0].heightCells == 5U;
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redoRestoredOpening =
      redone.accepted &&
      near(editor.worldLayout.source.openings[0].widthCells, 1.5);

  const cr::CreativeObjectId refinedWallObjectId =
      generatedObjectId(cr::CreativeWorldLayoutTable::Wall);
  const cr::CreativeObject* refinedWallObject =
      appState.facade.document().findObject(refinedWallObjectId);
  if (refinedWallObject == nullptr) {
    return expect(false, "generated wall survives semantic redo");
  }
  const cr::CreativeDocumentMutationReceipt refined = appState.facade.mutateObject(
      refinedWallObjectId, cr::CreativeMutationKind::Move,
      cr::makeMovePayload({refinedWallObject->transform.position.x + 0.5,
                           refinedWallObject->transform.position.y,
                           refinedWallObject->transform.position.z}));
  const std::uint64_t sourceRevisionBeforeConflict =
      editor.worldLayout.revision;
  const std::uint64_t documentRevisionBeforeConflict =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeConflict =
      cr::creativeUndoDepth(appState.history);
  wallSettings.heightCells = 6U;
  const app::CreativeDesktopCommandResult conflicted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings,
      context,
      app::CreativeDesktopGeneratedWallSettingsPayload{refinedWallObjectId,
                                                        wallSettings});
  const bool conflictPreservedTruth =
      refined.changed && !conflicted.accepted && !conflicted.changed &&
      editor.worldLayout.source.walls[0].heightCells == 5U &&
      editor.worldLayout.revision == sourceRevisionBeforeConflict &&
      appState.facade.document().revision() ==
          documentRevisionBeforeConflict &&
      cr::creativeUndoDepth(appState.history) == undoBeforeConflict;

  ++editor.worldLayout.revision;
  const std::uint64_t documentRevisionBeforeUnsynchronized =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeUnsynchronized =
      cr::creativeUndoDepth(appState.history);
  static_cast<void>(app::readCreativeEditorWorldLayoutOpeningSettings(
      editor.worldLayout, 0U, openingSettings));
  openingSettings.widthCells = 1.75;
  const app::CreativeDesktopCommandResult unsynchronized = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{
          generatedObjectId(cr::CreativeWorldLayoutTable::Opening),
          openingSettings});
  const bool unsynchronizedRejected =
      !unsynchronized.accepted && !unsynchronized.changed &&
      unsynchronized.message ==
          "creative_editor_world_layout_generated_edit_unsynchronized_source" &&
      near(editor.worldLayout.source.openings[0].widthCells, 1.5) &&
      appState.facade.document().revision() ==
          documentRevisionBeforeUnsynchronized &&
      cr::creativeUndoDepth(appState.history) == undoBeforeUnsynchronized;

  return expect(wallPreviewOnly,
                "generated partition settings preview without mutation") &&
         expect(wallUpdated.accepted && wallUpdated.changed &&
                    wallUpdated.worldLayoutChanged &&
                    wallUpdated.sceneChanged && wallSynchronized &&
                    appState.facade.document().revision() >
                        documentRevisionBefore,
                "generated partition edit synchronizes source and scene") &&
         expect(openingPreviewOnly,
                "generated opening settings preview without mutation") &&
         expect(openingUpdated.accepted && openingUpdated.changed &&
                    openingUpdated.worldLayoutChanged &&
                    openingUpdated.sceneChanged && openingSynchronized &&
                    cr::creativeUndoDepth(appState.history) == undoBefore + 2U,
                "generated opening edit records one semantic history step") &&
         expect(rejectedAtomically,
                "invalid and mismatched generated edits are atomic no-ops") &&
         expect(undone.accepted && undoRestoredOpening &&
                    redoRestoredOpening,
                "generated opening edit undo and redo keep source parity") &&
         expect(conflictPreservedTruth,
                "refinement conflict leaves source document and history untouched") &&
         expect(unsynchronizedRejected,
                "generated edit rejects while 2D source changes are pending");
}

bool generatedVerticalConnectorSettingsCommitSourceAndSceneTogether() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Generated Connector Editing");
  static_cast<void>(document.assignId(437U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "generated_connector_editing");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  editor.worldLayout.source.buildings.push_back(building);
  editor.worldLayout.source.levels.push_back(
      {0U, "ground", "Ground", 0.0, 3U, 1U, 1U, 1U});
  editor.worldLayout.source.levels.push_back(
      {0U, "upper", "Upper", 3.0, 3U, 1U, 1U, 1U});
  editor.worldLayout.source.rooms.push_back(
      {0U, 0U, "ground_room", "Ground Room", {{0, 0}, {8, 8}}, 0.25});
  editor.worldLayout.source.rooms.push_back(
      {0U, 1U, "upper_room", "Upper Room", {{0, 0}, {8, 8}}, 0.25});
  editor.worldLayout.source.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "main_stair",
       "Main Stair",
       {{1, 2}, {5, 4}}});
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted) {
    return expect(false, "generated connector editing fixture generated");
  }

  const auto generatedObjectId = [&](cr::CreativeWorldLayoutTable table) {
    for (const cr::CreativeObject& object :
         appState.facade.document().objects()) {
      const cr::CreativeWorldLayoutObjectProvenance provenance =
          cr::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, object);
      if (provenance.owned && provenance.table == table) {
        return object.id;
      }
    }
    return cr::kInvalidObjectId;
  };
  const cr::CreativeObjectId connectorObjectId =
      generatedObjectId(cr::CreativeWorldLayoutTable::VerticalConnector);
  const cr::CreativeObjectId roomObjectId =
      generatedObjectId(cr::CreativeWorldLayoutTable::Room);
  if (connectorObjectId == cr::kInvalidObjectId ||
      roomObjectId == cr::kInvalidObjectId) {
    return expect(false, "connector and room provenance objects exist");
  }

  app::CreativeEditorWorldLayoutVerticalConnectorSettings settings;
  static_cast<void>(
      app::readCreativeEditorWorldLayoutVerticalConnectorSettings(
          editor.worldLayout, 0U, settings));
  settings.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  settings.direction = cr::CreativeWorldLayoutVerticalDirection::NegativeX;
  settings.footprint = {{2, 2}, {6, 4}};
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult previewed = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedVerticalConnectorSettings,
      context,
      app::CreativeDesktopGeneratedVerticalConnectorSettingsPayload{
          connectorObjectId, settings});
  const bool previewStayedTransient =
      previewed.accepted && previewed.sceneChanged &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.revision == sourceRevisionBefore &&
      editor.worldLayout.source.verticalConnectors[0].kind ==
          cr::CreativeWorldLayoutVerticalConnectorKind::Stair &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore;

  const app::CreativeDesktopCommandResult applied = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutApplyGeneratedVerticalConnectorSettings,
      context,
      app::CreativeDesktopGeneratedVerticalConnectorSettingsPayload{
          connectorObjectId, settings});
  const cr::CreativeObjectId liveRampId =
      generatedObjectId(cr::CreativeWorldLayoutTable::VerticalConnector);
  const cr::CreativeObject* liveRamp =
      appState.facade.document().findObject(liveRampId);
  const cr::CreativeWorldLayoutVerticalConnector& appliedSource =
      editor.worldLayout.source.verticalConnectors[0];
  const bool appliedOnce =
      applied.accepted && applied.changed && applied.worldLayoutChanged &&
      applied.sceneChanged &&
      appliedSource.kind == cr::CreativeWorldLayoutVerticalConnectorKind::Ramp &&
      appliedSource.direction ==
          cr::CreativeWorldLayoutVerticalDirection::NegativeX &&
      appliedSource.footprint.minimum == cr::CreativeTerrainCoord2{2, 2} &&
      appliedSource.footprint.maximum == cr::CreativeTerrainCoord2{6, 4} &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      liveRamp != nullptr && liveRamp->kind == cr::CreativeObjectKind::Ramp &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObjectId restoredStairId =
      generatedObjectId(cr::CreativeWorldLayoutTable::VerticalConnector);
  const cr::CreativeObject* restoredStair =
      appState.facade.document().findObject(restoredStairId);
  const cr::CreativeWorldLayoutVerticalConnector& undoneSource =
      editor.worldLayout.source.verticalConnectors[0];
  const bool undoRestoredBoth =
      undone.accepted &&
      undoneSource.kind == cr::CreativeWorldLayoutVerticalConnectorKind::Stair &&
      undoneSource.direction ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
      undoneSource.footprint.minimum == cr::CreativeTerrainCoord2{1, 2} &&
      undoneSource.footprint.maximum == cr::CreativeTerrainCoord2{5, 4} &&
      restoredStair != nullptr &&
      restoredStair->kind == cr::CreativeObjectKind::Stair;
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeObjectId restoredRampId =
      generatedObjectId(cr::CreativeWorldLayoutTable::VerticalConnector);
  const cr::CreativeObject* restoredRamp =
      appState.facade.document().findObject(restoredRampId);
  const bool redoRestoredBoth =
      redone.accepted &&
      editor.worldLayout.source.verticalConnectors[0].kind ==
          cr::CreativeWorldLayoutVerticalConnectorKind::Ramp &&
      restoredRamp != nullptr &&
      restoredRamp->kind == cr::CreativeObjectKind::Ramp;

  const std::uint64_t sourceRevisionBeforeReject =
      editor.worldLayout.revision;
  const std::uint64_t documentRevisionBeforeReject =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeReject =
      cr::creativeUndoDepth(appState.history);
  settings.footprint = {{0, 2}, {2, 4}};
  const app::CreativeDesktopCommandResult invalid = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutApplyGeneratedVerticalConnectorSettings,
      context,
      app::CreativeDesktopGeneratedVerticalConnectorSettingsPayload{
          restoredRampId, settings});
  const app::CreativeDesktopCommandResult wrongSource = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutApplyGeneratedVerticalConnectorSettings,
      context,
      app::CreativeDesktopGeneratedVerticalConnectorSettingsPayload{
          roomObjectId, settings});
  const bool rejectedAtomically =
      !invalid.accepted && !invalid.changed && !wrongSource.accepted &&
      !wrongSource.changed &&
      editor.worldLayout.revision == sourceRevisionBeforeReject &&
      appState.facade.document().revision() == documentRevisionBeforeReject &&
      cr::creativeUndoDepth(appState.history) == undoBeforeReject &&
      editor.worldLayout.source.verticalConnectors[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{2, 2};

  return expect(previewStayedTransient,
                "generated connector preview stays transient") &&
         expect(appliedOnce,
                "generated connector edit synchronizes source scene and history") &&
         expect(undoRestoredBoth && redoRestoredBoth,
                "generated connector undo and redo restore source and scene") &&
         expect(rejectedAtomically,
                "invalid and mismatched connector edits are atomic no-ops");
}

bool generatedSourceOnlyOpeningEditUsesSourceHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Source Only Opening Editing");
  static_cast<void>(document.assignId(436U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "source_only_opening_editing");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  editor.worldLayout.source.buildings.push_back(building);
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "partition";
  wall.name = "Partition";
  wall.start = {0, 0};
  wall.end = {8, 0};
  editor.worldLayout.source.walls.push_back(wall);
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  opening.wallIndex = 0U;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  opening.stableKey = "door";
  opening.name = "Door";
  opening.centerOffsetCells = 4.0;
  opening.widthCells = 1.0;
  opening.cutoutHeightCells = 2.1;
  opening.includeInsert = false;
  editor.worldLayout.source.openings.push_back(opening);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  cr::CreativeObjectId openingObjectId = cr::kInvalidObjectId;
  for (const cr::CreativeObject& object : appState.facade.document().objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(
            editor.worldLayout.source, object);
    if (provenance.owned &&
        provenance.table == cr::CreativeWorldLayoutTable::Opening) {
      openingObjectId = object.id;
      break;
    }
  }
  if (!generated.accepted || openingObjectId == cr::kInvalidObjectId) {
    return expect(false, "source-only opening fixture generated");
  }

  app::CreativeEditorWorldLayoutOpeningSettings settings;
  static_cast<void>(app::readCreativeEditorWorldLayoutOpeningSettings(
      editor.worldLayout, 0U, settings));
  settings.door.hingeSide = cr::CreativeDoorHingeSide::MaximumEdge;
  settings.door.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  settings.door.initialState = cr::CreativeDoorInitialState::Open;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t documentUndoBefore =
      cr::creativeUndoDepth(appState.history);
  const std::size_t sourceUndoBefore =
      editor.worldLayout.sourceHistory.undoEntries.size();
  editor.desktopUi.showWorldLayout = false;
  const app::CreativeDesktopCommandResult previewed = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{openingObjectId,
                                                           settings});
  const app::CreativeDesktopCommandResult previewCancelled = dispatchOne(
      app::CreativeDesktopCommandId::
          WorldLayoutCancelGeneratedSettingsPreview,
      context);
  const bool previewCancelStayedInInspector =
      previewed.accepted && previewCancelled.accepted &&
      previewCancelled.changed &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      !editor.desktopUi.showWorldLayout &&
      appState.facade.document().revision() == documentRevisionBefore &&
      editor.worldLayout.revision == editor.worldLayout.generatedRevision;
  static_cast<void>(dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{openingObjectId,
                                                           settings}));
  const app::CreativeDesktopCommandResult focused = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutFocusObjectSource, context,
      app::CreativeDesktopWorldLayoutObjectSourcePayload{openingObjectId});
  const bool focusCancelledPreview =
      focused.accepted && focused.sceneChanged &&
      editor.desktopUi.showWorldLayout &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
  editor.desktopUi.showWorldLayout = false;
  static_cast<void>(dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{openingObjectId,
                                                           settings}));
  const app::CreativeDesktopCommandResult updated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings,
      context,
      app::CreativeDesktopGeneratedOpeningSettingsPayload{openingObjectId,
                                                           settings});
  const bool sourceOnlyRecorded =
      updated.accepted && updated.changed && updated.worldLayoutChanged &&
      updated.sceneChanged &&
      editor.worldLayout.source.openings[0].door == settings.door &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == documentUndoBefore &&
      editor.worldLayout.sourceHistory.undoEntries.size() ==
          sourceUndoBefore + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRestored =
      undone.accepted &&
      editor.worldLayout.source.openings[0].door.initialState ==
          cr::CreativeDoorInitialState::Closed &&
      appState.facade.document().revision() == documentRevisionBefore;
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);

  return expect(previewCancelStayedInInspector,
                "generated preview cancel keeps the 3D Inspector active") &&
         expect(focusCancelledPreview,
                "source focus cancels preview and opens the 2D owner") &&
         expect(sourceOnlyRecorded,
                "source-only semantic edit records no fake document revision") &&
         expect(undoRestored && redone.accepted &&
                    editor.worldLayout.source.openings[0].door == settings.door,
                "source-only semantic edit remains undoable and redoable");
}


}  // namespace

bool runCreativeDesktopWorldLayoutGeneratedCommandTests() {
  bool ok = true;
  ok = buildingGroundingCommandsShareSourceAndGeneratedTransactions() && ok;
  ok = generatedLevelSettingsRebuildEveryRoomAtomically() && ok;
  ok = generatedRoomSettingsRebuildTopologyAtomically() && ok;
  ok = generatedWallAndOpeningSettingsCommitSourceAndSceneTogether() && ok;
  ok = generatedVerticalConnectorSettingsCommitSourceAndSceneTogether() && ok;
  ok = generatedSourceOnlyOpeningEditUsesSourceHistory() && ok;
  return ok;
}
