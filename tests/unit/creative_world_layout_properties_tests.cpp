#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeAppState makeApp();

app::CreativeEditorWorldLayoutState shellState() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "properties_test");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {8, 6}}, 0.0, 3U, 0.25, 1U});
  if (!shell.accepted) {
    std::cerr << "FAIL: properties fixture shell\n";
  }
  return state;
}

bool levelSettingsAreOwnedAndValidated() {
  app::CreativeEditorWorldLayoutState state = shellState();
  app::CreativeEditorWorldLayoutLevelSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutLevelSettings(
      state, 0U, settings);
  const std::uint64_t before = state.revision;
  settings.name = "Upper Hall";
  settings.floorTopLayer = 1.0;
  settings.wallHeightCells = 4U;
  settings.floorThicknessLayers = 2U;
  settings.ceilingThicknessLayers = 2U;
  settings.roofThicknessLayers = 2U;
  settings.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  settings.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  settings.roofPitchDegrees = 35.0;
  settings.roofOverhangCells = 0.5;
  const auto updated = app::setCreativeEditorWorldLayoutLevelSettings(
      state, 0U, settings);

  const auto added = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Add, 0U);
  app::CreativeEditorWorldLayoutLevelSettings conflicting;
  static_cast<void>(app::readCreativeEditorWorldLayoutLevelSettings(
      state, 1U, conflicting));
  conflicting.floorTopLayer = settings.floorTopLayer;
  const std::uint64_t beforeConflict = state.revision;
  const auto rejected = app::setCreativeEditorWorldLayoutLevelSettings(
      state, 1U, conflicting);
  const auto focused = app::focusCreativeEditorWorldLayoutSource(
      state, cr::CreativeWorldLayoutTable::Level, 0U);

  return expect(read && updated.accepted && updated.changed,
                "level settings accept a valid complete edit") &&
         expect(state.source.levels[0].name == "Upper Hall" &&
                    state.source.levels[0].ceilingThicknessLayers == 2U &&
                    state.source.levels[0].roofStyle ==
                        cr::CreativeStructuralRoofStyle::Gable &&
                    before + 2U == state.revision,
                "level settings and level add each advance source once") &&
         expect(added.accepted && added.changed && !rejected.accepted &&
                    !rejected.changed && state.revision == beforeConflict,
                "duplicate level elevations reject without revision drift") &&
         expect(focused.accepted &&
                    state.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Level &&
                    state.selection.index == 0U &&
                    state.activeLevelIndex == 0U &&
                    app::creativeEditorWorldLayoutSelectedBuilding(state) ==
                        0U,
                "a level is a real selection with an owning building");
}

bool buildingGroundingSettingsAreExplicitAndBounded() {
  app::CreativeEditorWorldLayoutState state = shellState();
  app::CreativeEditorWorldLayoutBuildingGroundingSettings settings;
  const bool read =
      app::readCreativeEditorWorldLayoutBuildingGroundingSettings(
          state, 0U, settings);
  const std::uint64_t before = state.revision;
  settings.mode = cr::CreativeWorldLayoutGroundingMode::Foundation;
  settings.maximumReliefCells = 2U;
  const auto updated =
      app::setCreativeEditorWorldLayoutBuildingGroundingSettings(
          state, 0U, settings);
  const auto unchanged =
      app::setCreativeEditorWorldLayoutBuildingGroundingSettings(
          state, 0U, settings);
  settings.mode = cr::CreativeWorldLayoutGroundingMode::Count;
  const auto rejected =
      app::setCreativeEditorWorldLayoutBuildingGroundingSettings(
          state, 0U, settings);

  return expect(read &&
                    settings.maximumReliefCells == 2U &&
                    updated.accepted && updated.changed &&
                    state.source.buildings[0].groundingMode ==
                        cr::CreativeWorldLayoutGroundingMode::Foundation &&
                    state.source.buildings[0].maximumGroundReliefCells == 2U &&
                    state.revision == before + 1U,
                "building grounding is one semantic source property") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    !rejected.accepted && !rejected.changed &&
                    state.revision == before + 1U,
                "unchanged and invalid grounding settings do not drift source");
}

bool terrainProfileSettingsAreBounded() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "profile_1";
  profile.kind = cr::CreativeTerrainRecipeKind::Hill;
  profile.center = {4, 5};
  profile.baseHeightCells = 4U;
  profile.radiusCells = 4U;
  profile.amplitudeCells = 4U;
  profile.spacingCells = 1U;
  state.source.terrainProfiles.push_back(profile);

  app::CreativeEditorWorldLayoutTerrainProfileSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);
  settings.kind = cr::CreativeTerrainRecipeKind::Ridge;
  settings.center = {12, -3};
  settings.baseHeightCells = 7U;
  settings.radiusCells = 8U;
  settings.amplitudeCells = 2U;
  settings.spacingCells = 2U;
  settings.direction =
      cr::CreativeTerrainProfileDirection::NegativeXPositiveZ;
  settings.frequency = 2U;
  const auto updated = app::setCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);
  const std::uint64_t afterUpdate = state.revision;
  settings.radiusCells = 3U;
  const auto rejected = app::setCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);

  return expect(read && updated.accepted && updated.changed &&
                    state.source.terrainProfiles[0].kind ==
                        cr::CreativeTerrainRecipeKind::Ridge &&
                    state.source.terrainProfiles[0].blend ==
                        cr::CreativeTerrainProfileBlend::Set &&
                    state.source.terrainProfiles[0].rodPolicy ==
                        cr::CreativeTerrainProfileRodPolicy::Fill,
                "terrain profiles retain absolute World Layout semantics") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == afterUpdate,
                "unsupported terrain sampling values reject atomically");
}

bool terrainLandformSettingsRoundTripAtomically() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "terrace_1";
  profile.kind = cr::CreativeTerrainRecipeKind::Terrace;
  profile.usesLandformRecipe = true;
  profile.landform.kind = cr::CreativeTerrainLandformKind::Terrace;
  profile.landform.bounds = {{-2, 3}, 6U, 5U};
  profile.landform.baseHeightCells = 1U;
  profile.landform.targetHeightCells = 7U;
  profile.landform.terraceCount = 3U;
  profile.landform.direction =
      cr::CreativeTerrainLandformDirection::NegativeZ;
  profile.landform.edge = cr::CreativeTerrainLandformEdge::Slope;
  profile.landform.edgeWidthCells = 2U;
  profile.landform.featherCells = 1U;
  profile.landform.material = cr::CreativeTerrainMaterial::Dirt;
  state.source.terrainProfiles.push_back(profile);

  app::CreativeEditorWorldLayoutTerrainProfileSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);
  const bool readExactly =
      settings.kind == profile.kind && settings.usesLandformRecipe &&
      settings.landform == profile.landform;
  settings.kind = cr::CreativeTerrainRecipeKind::Cliff;
  settings.landform.kind = cr::CreativeTerrainLandformKind::Cliff;
  settings.landform.bounds = {{4, -5}, 9U, 7U};
  settings.landform.targetHeightCells = 12U;
  settings.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  settings.landform.edgeWidthCells = 0U;
  settings.landform.material = cr::CreativeTerrainMaterial::Stone;
  settings.usesRetainingEdgeRecipe = true;
  settings.retainingEdge.terrainProfileKey = profile.stableKey;
  settings.retainingEdge.settings.selection =
      cr::CreativeRetainingEdgeSelection::Internal;
  settings.retainingEdge.settings.kit =
      cr::CreativeRetainingEdgeKit::InfrastructureStone;
  settings.retainingEdge.settings.thicknessMeters = 0.45;
  settings.retainingEdge.settings.maximumHeightMeters = 12.0;
  settings.retainingEdge.settings.material =
      cr::CreativeStructuralMaterial::Brick;
  settings.retainingEdge.settings.transitionCount = 1U;
  settings.retainingEdge.settings.transitions[0] = {
      cr::canonicalCreativeTerrainHardEdge({4, -5}, {5, -5}),
      cr::CreativeRetainingEdgeTransitionKind::Stair,
      3U,
  };
  const std::uint64_t before = state.revision;
  const auto updated = app::setCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);
  const auto committed = state.source.terrainProfiles[0];
  const std::uint64_t afterUpdate = state.revision;

  auto mismatchedAttachment = settings;
  mismatchedAttachment.retainingEdge.terrainProfileKey = "terrace.other";
  const auto attachmentRejected =
      app::setCreativeEditorWorldLayoutTerrainProfileSettings(
          state, 0U, mismatchedAttachment);
  settings.landform.kind = cr::CreativeTerrainLandformKind::Terrace;
  const auto rejected = app::setCreativeEditorWorldLayoutTerrainProfileSettings(
      state, 0U, settings);

  return expect(read && readExactly && updated.accepted && updated.changed &&
                    committed.kind == cr::CreativeTerrainRecipeKind::Cliff &&
                    committed.usesLandformRecipe &&
                    committed.landform.kind ==
                        cr::CreativeTerrainLandformKind::Cliff &&
                    committed.landform.bounds ==
                        cr::CreativeTerrainHeightFieldBounds{{4, -5}, 9U, 7U} &&
                    committed.landform.targetHeightCells == 12U &&
                    committed.landform.edge ==
                        cr::CreativeTerrainLandformEdge::Retaining &&
                    committed.landform.edgeWidthCells == 0U &&
                    committed.landform.material ==
                        cr::CreativeTerrainMaterial::Stone &&
                    committed.usesRetainingEdgeRecipe &&
                    committed.retainingEdge.terrainProfileKey ==
                        profile.stableKey &&
                    committed.retainingEdge.settings.selection ==
                        cr::CreativeRetainingEdgeSelection::Internal &&
                    committed.retainingEdge.settings.kit ==
                        cr::CreativeRetainingEdgeKit::InfrastructureStone &&
                    committed.retainingEdge.settings.transitions[0].kind ==
                        cr::CreativeRetainingEdgeTransitionKind::Stair &&
                    afterUpdate == before + 1U,
                "bounded landform and retaining properties round-trip exactly") &&
         expect(!attachmentRejected.accepted &&
                    !attachmentRejected.changed && !rejected.accepted &&
                    !rejected.changed &&
                    state.revision == afterUpdate &&
                    state.source.terrainProfiles[0].landform ==
                        committed.landform &&
                    state.source.terrainProfiles[0].retainingEdge ==
                        committed.retainingEdge,
                "mismatched landform kinds and retaining owners reject without source drift");
}

bool terrainPathSettingsUseTheSharedRecipe() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "path_1";
  path.recipe.kind = cr::CreativeTerrainPathKind::Road;
  path.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  path.recipe.nextPointId = 3U;
  path.recipe.points = {
      {1U, {0, 0}, 4U, 1U, 0U, 0},
      {2U, {5, 0}, 4U, 1U, 0U, 0},
  };
  state.source.terrainPaths.push_back(path);

  app::CreativeEditorWorldLayoutTerrainPathSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutTerrainPathSettings(
      state, 0U, settings);
  settings.recipe.kind = cr::CreativeTerrainPathKind::River;
  settings.recipe.elevation = cr::CreativeTerrainPathElevation::Grade;
  settings.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  settings.recipe.paintSurface = true;
  settings.recipe.material = cr::CreativeTerrainMaterial::Sand;
  settings.recipe.points[1].coord = {7, 3};
  settings.recipe.points[1].heightCells = 8U;
  settings.recipe.points[1].halfWidthCells = 2U;
  settings.recipe.points[1].amplitudeCells = 4U;
  const auto updated = app::setCreativeEditorWorldLayoutTerrainPathSettings(
      state, 0U, settings);
  const std::uint64_t afterUpdate = state.revision;
  settings.recipe.points[1].coord = settings.recipe.points[0].coord;
  const auto rejected = app::setCreativeEditorWorldLayoutTerrainPathSettings(
      state, 0U, settings);

  return expect(read && updated.accepted && updated.changed &&
                    state.source.terrainPaths[0].recipe.kind ==
                        cr::CreativeTerrainPathKind::River &&
                    state.source.terrainPaths[0].recipe.points[1].coord ==
                        cr::CreativeTerrainCoord2{7, 3} &&
                    state.source.terrainPaths[0]
                            .recipe.points[1].heightCells == 8U &&
                    state.source.terrainPaths[0].recipe.points[1].id == 2U,
                "path properties and control points update together") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == afterUpdate &&
                    state.source.terrainPaths[0].recipe.points[1].coord ==
                        cr::CreativeTerrainCoord2{7, 3},
                "the terrain path kernel rejects degenerate segments");
}

bool objectSettingsPreserveSemanticIdentity() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Rock;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  object.stableKey = "rock_1";
  object.name = "Boulder";
  object.assetId = "boulder_01";
  object.boundsCells = {{1.0, 0.0, 1.0}, {3.0, 2.0, 3.0}};
  object.tags = {"world_layout:object"};
  state.source.objects.push_back(object);

  app::CreativeEditorWorldLayoutObjectSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  settings.name = "Gate Boulder";
  settings.assetId = "boulder_gate";
  settings.boundsCells = {{2.0, 0.0, 2.0}, {5.0, 3.0, 5.0}};
  settings.visible = false;
  const auto updated = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  const std::uint64_t afterUpdate = state.revision;
  settings.boundsCells.max = settings.boundsCells.min;
  const auto rejected = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);

  return expect(read && updated.accepted && updated.changed &&
                    state.source.objects[0].name == "Gate Boulder" &&
                    state.source.objects[0].kind ==
                        cr::CreativeObjectKind::Rock &&
                    state.source.objects[0].mode ==
                        cr::CreativeObjectLibraryPlacementMode::Bounds &&
                    state.source.objects[0].stableKey == "rock_1" &&
                    state.source.objects[0].tags ==
                        std::vector<std::string>{"world_layout:object"} &&
                    !state.source.objects[0].visible,
                "object edits preserve kind, mode, tags, and stable key") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == afterUpdate,
                "zero-volume object bounds reject atomically");
}

bool npcObjectSettingsRoundTripAtomically() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutObject actor;
  actor.kind = cr::CreativeObjectKind::NpcSpawn;
  actor.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  actor.stableKey = "guard_courtyard";
  actor.name = "Courtyard Guard";
  actor.pointCells = {4.0, 0.25, 2.0};
  actor.tags = {"world_layout:object"};
  state.source.objects.push_back(actor);

  app::CreativeEditorWorldLayoutObjectSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  settings.npcSpawn.behaviorProfileId = "default";
  settings.npcSpawn.team = cr::CreativeNpcTeam::Hostile;
  settings.npcSpawn.hitPoints = 85U;
  settings.npcSpawn.initialAlertLevel = 0.4;
  settings.npcSpawn.spawnPolicy = cr::CreativeNpcSpawnPolicy::Disabled;
  const std::uint64_t before = state.revision;
  const auto updated = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  const std::uint64_t afterUpdate = state.revision;

  settings.npcSpawn.initialAlertLevel = 2.0;
  const auto rejected = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);

  return expect(read && updated.accepted && updated.changed &&
                    afterUpdate == before + 1U &&
                    state.source.objects[0].npcSpawn.behaviorProfileId ==
                        "default" &&
                    state.source.objects[0].npcSpawn.team ==
                        cr::CreativeNpcTeam::Hostile &&
                    state.source.objects[0].npcSpawn.hitPoints == 85U &&
                    state.source.objects[0].npcSpawn.initialAlertLevel == 0.4 &&
                    state.source.objects[0].npcSpawn.spawnPolicy ==
                        cr::CreativeNpcSpawnPolicy::Disabled,
                "world layout object settings preserve every NPC policy") &&
         expect(!rejected.accepted && !rejected.changed &&
                    state.revision == afterUpdate,
                "invalid NPC settings reject without partial source edits");
}

bool bridgeRecipeSettingsRetainAttachmentAndGeneratedPlacementOwnership() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutTerrainPath river;
  river.stableKey = "river.bridge";
  river.recipe.kind = cr::CreativeTerrainPathKind::River;
  river.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  river.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  river.recipe.watercourse.crossings = {
      {5U, 2U, 1U, 2U, 3U},
      {6U, 1U, 1U, 2U, 3U},
  };
  river.recipe.watercourse.nextCrossingId = 7U;
  river.recipe.nextPointId = 4U;
  river.recipe.points = {
      {1U, {-4, 0}, 5U, 2U, 2U, 0},
      {2U, {0, 0}, 4U, 2U, 2U, 0},
      {3U, {4, 0}, 3U, 2U, 2U, 0},
  };
  state.source.terrainPaths.push_back(river);
  cr::CreativeWorldLayoutObject bridge;
  bridge.kind = cr::CreativeObjectKind::Bridge;
  bridge.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  bridge.stableKey = "bridge_1";
  bridge.name = "River Bridge";
  bridge.boundsCells = {{-2.0, 0.0, -3.0}, {2.0, 0.35, 3.0}};
  bridge.tags = {"world_layout:object"};
  bridge.usesBridgeRecipe = true;
  bridge.bridge.watercoursePathKey = river.stableKey;
  bridge.bridge.crossingId = 5U;
  state.source.objects.push_back(bridge);
  cr::CreativeWorldLayoutObject occupiedBridge = bridge;
  occupiedBridge.stableKey = "bridge_2";
  occupiedBridge.name = "Second River Bridge";
  occupiedBridge.bridge.crossingId = 6U;
  state.source.objects.push_back(occupiedBridge);

  app::CreativeEditorWorldLayoutObjectSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  const cr::CreativeBounds authoredFootprint = settings.boundsCells;
  settings.bridge.settings.deckWidthMeters = 3.0;
  settings.bridge.settings.deckElevationOffsetMeters = 0.5;
  settings.bridge.settings.supportStyle =
      cr::CreativeBridgeSupportStyle::None;
  settings.bridge.settings.rails = false;
  settings.bridge.settings.maximumApproachGradePermille = 350U;
  settings.bridge.settings.materials.deck =
      cr::CreativeStructuralMaterial::Stone;
  const std::uint64_t before = state.revision;
  const auto updated = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  const cr::CreativeBridgeSourceRecipe committed =
      state.source.objects[0].bridge;
  const std::uint64_t afterUpdate = state.revision;

  settings.bridge.crossingId = 6U;
  const auto occupiedAttachment =
      app::setCreativeEditorWorldLayoutObjectSettings(state, 0U, settings);
  settings.bridge.crossingId = 99U;
  const auto missingAttachment =
      app::setCreativeEditorWorldLayoutObjectSettings(state, 0U, settings);
  settings = {};
  static_cast<void>(app::readCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings));
  settings.boundsCells.min.x -= 1.0;
  const auto movedFootprint = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, settings);
  const auto manipulation =
      app::beginCreativeEditorWorldLayoutObjectManipulation(state, 0U,
                                                            {0.0, 0.0});

  return expect(read && updated.accepted && updated.changed &&
                    afterUpdate == before + 1U &&
                    committed.watercoursePathKey == "river.bridge" &&
                    committed.crossingId == 5U &&
                    committed.settings.deckWidthMeters == 3.0 &&
                    committed.settings.deckElevationOffsetMeters == 0.5 &&
                    committed.settings.supportStyle ==
                        cr::CreativeBridgeSupportStyle::None &&
                    !committed.settings.rails &&
                    committed.settings.materials.deck ==
                        cr::CreativeStructuralMaterial::Stone,
                "bridge settings edit one durable attached recipe") &&
         expect(!missingAttachment.accepted &&
                    !missingAttachment.changed &&
                    !occupiedAttachment.accepted &&
                    !occupiedAttachment.changed &&
                    !movedFootprint.accepted && !movedFootprint.changed &&
                    state.revision == afterUpdate &&
                    state.source.objects[0].boundsCells.min.x ==
                        authoredFootprint.min.x,
                "missing attachments and detached footprints reject atomically") &&
         expect(!manipulation.accepted && !manipulation.changed &&
                    manipulation.reasonCode ==
                        "creative_editor_world_layout_bridge_manipulation_attached",
                "attached bridge placement is owned by its crossing");
}

bool objectManipulationPreviewsThenCommitsOnce() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutObject boundsObject;
  boundsObject.kind = cr::CreativeObjectKind::Rock;
  boundsObject.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  boundsObject.stableKey = "rock_drag";
  boundsObject.name = "Drag Boulder";
  boundsObject.assetId = "boulder_01";
  boundsObject.boundsCells = {{1.0, 0.25, 1.0}, {3.0, 2.25, 3.0}};
  boundsObject.tags = {"world_layout:object"};
  cr::CreativeWorldLayoutObject pointObject;
  pointObject.kind = cr::CreativeObjectKind::SpawnPoint;
  pointObject.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  pointObject.stableKey = "spawn_drag";
  pointObject.name = "Drag Spawn";
  pointObject.pointCells = {8.0, 1.5, -4.0};
  pointObject.tags = {"world_layout:object"};
  state.source.objects = {boundsObject, pointObject};
  app::installCreativeEditorWorldLayout(state, state.source);

  const std::uint64_t revisionBefore = state.revision;
  const std::size_t undoBefore = state.sourceHistory.undoEntries.size();
  const std::size_t hit = app::findCreativeEditorWorldLayoutObjectAt(
      state, {2.0, 2.0});
  const auto begun = app::beginCreativeEditorWorldLayoutObjectManipulation(
      state, hit, {2.0, 2.0});
  const auto reselected = app::selectCreativeEditorWorldLayoutSource(
      state, cr::CreativeWorldLayoutTable::Object, hit);
  const bool reselectionPreservedManipulation =
      reselected.accepted && !reselected.changed &&
      state.objectManipulation.active &&
      state.objectManipulation.objectIndex == hit;
  const auto previewed = app::updateCreativeEditorWorldLayoutObjectManipulation(
      state, {5.49, 0.51});

  cr::CreativeAppState live = makeApp();
  app::CreativeEditorState editor;
  editor.worldLayout = std::move(state);
  const std::size_t objectIndex =
      editor.worldLayout.objectManipulation.objectIndex;
  const std::string stableKey = editor.worldLayout.objectManipulation.stableKey;
  app::CreativeEditorWorldLayoutObjectSettings settings =
      editor.worldLayout.objectManipulation.previewSettings;
  editor.worldLayout.objectManipulation = {};
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};
  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::WorldLayoutSetObjectSettings,
             app::CreativeDesktopWorldLayoutObjectSettingsPayload{
                 objectIndex, stableKey, settings});
  const app::CreativeDesktopCommandResult committed =
      app::dispatchCreativeDesktopCommands(frame, context);

  const cr::CreativeBounds& moved =
      editor.worldLayout.source.objects[0].boundsCells;
  return expect(hit == 0U && begun.accepted && begun.changed &&
                    reselectionPreservedManipulation &&
                    previewed.accepted && previewed.changed &&
                    editor.worldLayout.sourceHistory.undoEntries.size() ==
                        undoBefore + 1U,
                "object dragging previews transiently and records one edit") &&
         expect(committed.accepted && committed.changed &&
                    committed.worldLayoutChanged &&
                    editor.worldLayout.revision == revisionBefore + 1U &&
                    moved.min.x == 4.0 && moved.max.x == 6.0 &&
                    moved.min.z == 0.0 && moved.max.z == 2.0 &&
                    moved.min.y == 0.25 && moved.max.y == 2.25,
                "bounds dragging snaps XZ and preserves vertical geometry");
}

bool pointObjectManipulationCancelsAndRejectsStaleInput() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::SpawnPoint;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  object.stableKey = "spawn_drag";
  object.name = "Drag Spawn";
  object.pointCells = {8.0, 1.5, -4.0};
  object.tags = {"world_layout:object"};
  state.source.objects.push_back(object);
  app::installCreativeEditorWorldLayout(state, state.source);

  const auto begun = app::beginCreativeEditorWorldLayoutObjectManipulation(
      state, 0U, {8.0, -4.0});
  const auto moved = app::updateCreativeEditorWorldLayoutObjectManipulation(
      state, {6.2, -0.7});
  const cr::CreativeVec3 preview =
      state.objectManipulation.previewSettings.pointCells;
  const auto invalid = app::updateCreativeEditorWorldLayoutObjectManipulation(
      state, {std::numeric_limits<double>::infinity(), 0.0});
  const bool invalidPreview = !state.objectManipulation.previewValid;
  const auto cancelled =
      app::cancelCreativeEditorWorldLayoutObjectManipulation(state);
  const cr::CreativeVec3 unchanged = state.source.objects[0].pointCells;

  static_cast<void>(app::beginCreativeEditorWorldLayoutObjectManipulation(
      state, 0U, {8.0, -4.0}));
  ++state.revision;
  const auto stale = app::updateCreativeEditorWorldLayoutObjectManipulation(
      state, {9.0, -4.0});

  return expect(begun.accepted && moved.accepted && preview.x == 6.0 &&
                    preview.y == 1.5 && preview.z == -1.0,
                "point dragging snaps XZ and preserves authored height") &&
         expect(invalid.accepted && invalidPreview && cancelled.accepted &&
                    cancelled.changed && unchanged.x == 8.0 &&
                    unchanged.y == 1.5 && unchanged.z == -4.0,
                "invalid or cancelled previews never mutate layout truth") &&
         expect(!stale.accepted && !stale.changed &&
                    !state.objectManipulation.active &&
                    stale.reasonCode ==
                        "creative_editor_world_layout_object_manipulation_stale",
                "source revision changes invalidate an active object drag");
}

bool catalogObjectManipulationMovesOnlyItsPivot() {
  app::CreativeEditorWorldLayoutState state = shellState();
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Prop;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  object.stableKey = "catalog_drag";
  object.name = "Drag Dresser";
  object.assetId = "homestead/interior/dresser_1p3";
  object.pointCells = {8.0, 1.5, -4.0};
  object.assetSourceBoundsMeters =
      {{-0.65, 0.0, -0.3}, {0.65, 1.1, 0.3}};
  object.hasAssetSourceBounds = true;
  object.yawRadians = 0.5;
  object.scale = {1.25, 0.75, 1.5};
  object.tags = {"world_layout:object", "world_layout:catalog_asset"};
  state.source.objects.push_back(object);
  app::installCreativeEditorWorldLayout(state, state.source);

  const std::uint64_t revisionBefore = state.revision;
  const std::size_t undoBefore = state.sourceHistory.undoEntries.size();
  const auto begun = app::beginCreativeEditorWorldLayoutObjectManipulation(
      state, 0U, {8.0, -4.0});
  const auto moved = app::updateCreativeEditorWorldLayoutObjectManipulation(
      state, {6.2, -0.7});
  const app::CreativeEditorWorldLayoutObjectSettings preview =
      state.objectManipulation.previewSettings;
  state.objectManipulation = {};
  const auto committed = app::setCreativeEditorWorldLayoutObjectSettings(
      state, 0U, preview);
  const cr::CreativeWorldLayoutObject& result = state.source.objects[0];

  return expect(begun.accepted && moved.accepted && moved.changed &&
                    preview.pointCells.x == 6.0 &&
                    preview.pointCells.y == 1.5 &&
                    preview.pointCells.z == -1.0 &&
                    preview.hasAssetSourceBounds &&
                    preview.assetSourceBoundsMeters.min.x == -0.65 &&
                    preview.yawRadians == 0.5 && preview.scale.x == 1.25,
                "catalog drag preview moves only the placement pivot") &&
         expect(committed.accepted && committed.changed &&
                    state.revision == revisionBefore + 1U &&
                    state.sourceHistory.undoEntries.size() == undoBefore + 1U &&
                    result.pointCells.x == 6.0 && result.pointCells.y == 1.5 &&
                    result.pointCells.z == -1.0 &&
                    result.assetId == object.assetId &&
                    cr::creativeBoundsExactlyEqual(
                        result.assetSourceBoundsMeters,
                        object.assetSourceBoundsMeters) &&
                    result.yawRadians == object.yawRadians &&
                    cr::creativeVec3ExactlyEqual(result.scale, object.scale),
                "catalog drag commits once without geometry drift");
}

bool roofApertureManipulationSnapsRejectsAndCommitsOnce() {
  app::CreativeEditorWorldLayoutState state = shellState();
  const auto created = app::createCreativeEditorWorldLayoutRoofAperture(
      state, 0U, cr::CreativeStructuralRoofApertureKind::Skylight);
  app::CreativeEditorWorldLayoutRoofApertureSettings original;
  if (!created.accepted ||
      !app::readCreativeEditorWorldLayoutRoofApertureSettings(state, 0U,
                                                              original)) {
    return expect(false, "roof aperture manipulation fixture is valid");
  }

  constexpr double kTolerance = 0.20;
  const app::CreativeEditorWorldLayoutPoint center{
      (original.minimumXCells + original.maximumXCells) * 0.5,
      (original.minimumZCells + original.maximumZCells) * 0.5};
  const auto centerTarget =
      app::findCreativeEditorWorldLayoutRoofApertureTarget(
          state, center, kTolerance);
  const std::uint64_t revisionBefore = state.revision;
  const std::size_t undoBefore = state.sourceHistory.undoEntries.size();
  const auto begun =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
          center, kTolerance);
  const auto previewed =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update,
          {center.x + 1.12, center.z + 0.62}, kTolerance);
  const app::CreativeEditorWorldLayoutRoofApertureSettings movedPreview =
      state.roofApertureManipulation.previewSettings;
  app::CreativeEditorWorldLayoutRoofApertureSettings sourceDuringPreview;
  static_cast<void>(app::readCreativeEditorWorldLayoutRoofApertureSettings(
      state, 0U, sourceDuringPreview));
  const auto canceled =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Cancel,
          {}, kTolerance);
  const bool movePreviewAndCancel =
      centerTarget.handle ==
          app::CreativeEditorWorldLayoutRoofApertureHandle::Move &&
      begun.accepted && previewed.accepted && previewed.changed &&
      movedPreview.minimumXCells == original.minimumXCells + 1.0 &&
      movedPreview.maximumXCells == original.maximumXCells + 1.0 &&
      movedPreview.minimumZCells == original.minimumZCells + 0.5 &&
      movedPreview.maximumZCells == original.maximumZCells + 0.5 &&
      sourceDuringPreview == original && canceled.accepted && canceled.changed &&
      state.revision == revisionBefore;

  const double width = original.maximumXCells - original.minimumXCells;
  const app::CreativeEditorWorldLayoutPoint eastEdge{
      original.maximumXCells,
      (original.minimumZCells + original.maximumZCells) * 0.5};
  const auto invalidBegun =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
          eastEdge, kTolerance);
  const app::CreativeEditorWorldLayoutPoint invertedEast{
      eastEdge.x - width - 0.25, eastEdge.z};
  const auto invalidPreview =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update,
          invertedEast, kTolerance);
  const bool invalidPreviewVisible =
      state.roofApertureManipulation.active &&
      !state.roofApertureManipulation.previewValid;
  const auto invalidCommit =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Commit,
          invertedEast, kTolerance);
  const bool invalidRejectedAtomically =
      invalidBegun.accepted && invalidPreview.accepted &&
      invalidPreview.changed && invalidPreviewVisible &&
      !invalidCommit.accepted && !invalidCommit.changed &&
      !state.roofApertureManipulation.active &&
      state.revision == revisionBefore &&
      state.sourceHistory.undoEntries.size() == undoBefore;

  const app::CreativeEditorWorldLayoutPoint northEast{
      original.maximumXCells, original.minimumZCells};
  const auto resizeBegun =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
          northEast, kTolerance);
  const app::CreativeEditorWorldLayoutPoint expandedNorthEast{
      northEast.x + 0.48, northEast.z - 0.52};
  const auto resizePreview =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update,
          expandedNorthEast, kTolerance);
  const auto resized =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Commit,
          expandedNorthEast, kTolerance);
  app::CreativeEditorWorldLayoutRoofApertureSettings committed;
  static_cast<void>(app::readCreativeEditorWorldLayoutRoofApertureSettings(
      state, 0U, committed));

  app::CreativeEditorWorldLayoutState staleState = state;
  const app::CreativeEditorWorldLayoutPoint committedCenter{
      (committed.minimumXCells + committed.maximumXCells) * 0.5,
      (committed.minimumZCells + committed.maximumZCells) * 0.5};
  const auto staleBegun =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          staleState,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
          committedCenter, kTolerance);
  ++staleState.revision;
  const auto stale =
      app::applyCreativeEditorWorldLayoutRoofApertureManipulation(
          staleState,
          app::CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update,
          {committedCenter.x + 0.25, committedCenter.z}, kTolerance);

  return expect(movePreviewAndCancel,
                "roof aperture move snaps to quarter cells and cancels cleanly") &&
         expect(invalidRejectedAtomically,
                "an inverted roof aperture resize rejects atomically") &&
         expect(resizeBegun.accepted && resizePreview.accepted &&
                    resizePreview.changed && resized.accepted && resized.changed &&
                    committed.maximumXCells ==
                        original.maximumXCells + 0.5 &&
                    committed.minimumZCells == original.minimumZCells - 0.5 &&
                    committed.minimumXCells == original.minimumXCells &&
                    committed.maximumZCells == original.maximumZCells &&
                    state.revision == revisionBefore + 1U &&
                    state.sourceHistory.undoEntries.size() == undoBefore + 1U,
                "roof aperture resize commits one exact source edit") &&
         expect(staleBegun.accepted && !stale.accepted && !stale.changed &&
                    !staleState.roofApertureManipulation.active &&
                    stale.reasonCode ==
                        "creative_editor_world_layout_roof_aperture_manipulation_stale",
                "source revision changes invalidate a roof aperture drag");
}

cr::CreativeAppState makeApp() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Properties");
  static_cast<void>(document.assignId(9501U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool typedSettingsCommandsGuardIdentityAndPreview() {
  cr::CreativeAppState live = makeApp();
  app::CreativeEditorState editor;
  editor.worldLayout = shellState();
  const auto preview = app::previewCreativeEditorWorldLayout(
      editor.worldLayout, live.facade.document());
  app::CreativeEditorWorldLayoutLevelSettings settings;
  static_cast<void>(app::readCreativeEditorWorldLayoutLevelSettings(
      editor.worldLayout, 0U, settings));
  settings.name = "Command Level";
  const std::string levelKey = editor.worldLayout.source.levels[0].stableKey;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};

  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::WorldLayoutSetLevelSettings,
             app::CreativeDesktopWorldLayoutLevelSettingsPayload{
                 0U, levelKey, settings});
  const app::CreativeDesktopCommandResult applied =
      app::dispatchCreativeDesktopCommands(frame, context);

  app::CreativeDesktopCommandFrame staleFrame;
  staleFrame.push(app::CreativeDesktopCommandId::WorldLayoutSetLevelSettings,
                  app::CreativeDesktopWorldLayoutLevelSettingsPayload{
                      0U, "stale_level", settings});
  const app::CreativeDesktopCommandResult stale =
      app::dispatchCreativeDesktopCommands(staleFrame, context);

  return expect(preview.accepted && applied.accepted && applied.changed &&
                    applied.worldLayoutChanged && applied.sceneChanged &&
                    editor.worldLayout.source.levels[0].name ==
                        "Command Level" &&
                    !app::creativeEditorWorldLayoutPreviewActive(
                        editor.worldLayout),
                "typed property commands invalidate an exact preview") &&
         expect(!stale.accepted && !stale.changed &&
                    stale.message == "layout level settings: stale target",
                "stable keys prevent delayed property retargeting");
}

}  // namespace

int main() {
  const bool ok = levelSettingsAreOwnedAndValidated() &&
                  buildingGroundingSettingsAreExplicitAndBounded() &&
                  terrainProfileSettingsAreBounded() &&
                  terrainLandformSettingsRoundTripAtomically() &&
                  terrainPathSettingsUseTheSharedRecipe() &&
                  objectSettingsPreserveSemanticIdentity() &&
                  npcObjectSettingsRoundTripAtomically() &&
                  bridgeRecipeSettingsRetainAttachmentAndGeneratedPlacementOwnership() &&
                  objectManipulationPreviewsThenCommitsOnce() &&
                  pointObjectManipulationCancelsAndRejectsStaleInput() &&
                  catalogObjectManipulationMovesOnlyItsPivot() &&
                  roofApertureManipulationSnapsRejectsAndCommitsOnce() &&
                  typedSettingsCommandsGuardIdentityAndPreview();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
