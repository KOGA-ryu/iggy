#include "EditorAuthoredAssets.hpp"
#include "EditorDesktopCommands.hpp"
#include "EditorGroup.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainRecipe.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsCollisionQueries.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
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

bool sameTransform(const cr::CreativeTransform& lhs,
                   const cr::CreativeTransform& rhs) noexcept {
  return lhs.position.x == rhs.position.x &&
         lhs.position.y == rhs.position.y &&
         lhs.position.z == rhs.position.z &&
         lhs.rotationEulerRadians.x == rhs.rotationEulerRadians.x &&
         lhs.rotationEulerRadians.y == rhs.rotationEulerRadians.y &&
         lhs.rotationEulerRadians.z == rhs.rotationEulerRadians.z &&
         lhs.scale.x == rhs.scale.x && lhs.scale.y == rhs.scale.y &&
         lhs.scale.z == rhs.scale.z;
}

bool sameBounds(const cr::CreativeBounds& lhs,
                const cr::CreativeBounds& rhs) noexcept {
  return lhs.min.x == rhs.min.x && lhs.min.y == rhs.min.y &&
         lhs.min.z == rhs.min.z && lhs.max.x == rhs.max.x &&
         lhs.max.y == rhs.max.y && lhs.max.z == rhs.max.z;
}

const cr::CreativeObject* findRecipeMember(
    const cr::CreativeDocument& document,
    std::string_view instanceKey,
    std::string_view stableKey) {
  const auto found = std::find_if(
      document.objects().begin(), document.objects().end(),
      [instanceKey, stableKey](const cr::CreativeObject& object) {
        return cr::creativeRecipeObjectHasInstanceProvenance(
            object, cr::CreativeRecipeKind::Bridge, instanceKey,
            cr::CreativeRecipeObjectRole::Generated, stableKey);
      });
  return found == document.objects().end() ? nullptr : &*found;
}

class TemporarySaveRoot {
 public:
  explicit TemporarySaveRoot(std::string_view taskName) {
    const auto nonce = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    path_ = std::filesystem::temp_directory_path() /
            ("iggy3d_creator_task_" + std::string{taskName} + "_" +
             std::to_string(nonce));
    std::error_code error;
    std::filesystem::create_directories(path_, error);
    ready_ = !error;
  }

  ~TemporarySaveRoot() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  [[nodiscard]] bool ready() const noexcept { return ready_; }
  [[nodiscard]] const std::filesystem::path& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
  bool ready_ = false;
};

struct CreatorTaskHarness {
  explicit CreatorTaskHarness(std::string taskName,
                              cr::CreativeDocumentId documentId)
      : saveRoot(taskName),
        saveId(std::move(taskName)),
        context{appState, editor, saveRoot.path(), &saveId} {
    cr::CreativeDocument document =
        cr::CreativeDocument::create("Canonical Creator Task");
    static_cast<void>(document.assignId(documentId));
    installed = appState.facade.installDocument(std::move(document)).accepted;
    app::resetCreativeEditorWorldLayout(editor.worldLayout, saveId);
  }

  TemporarySaveRoot saveRoot;
  cr::CreativeAppState appState;
  app::CreativeEditorState editor;
  std::string saveId;
  app::CreativeDesktopCommandContext context;
  bool installed = false;
};

app::CreativeDesktopCommandResult dispatchOne(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id);
  return app::dispatchCreativeDesktopCommands(frame, context);
}

template <typename Payload>
app::CreativeDesktopCommandResult dispatchPayload(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context, Payload payload) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(payload));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

bool saveReopen(CreatorTaskHarness& task) {
  const app::CreativeDesktopCommandResult saved =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocument, task.context);
  const app::CreativeDesktopCommandResult cleared =
      dispatchOne(app::CreativeDesktopCommandId::NewDocument, task.context);
  const bool blank = task.appState.facade.document().objectCount() == 0U &&
                     task.appState.facade.document()
                             .terrainField()
                             .controlCount() == 0U;
  const app::CreativeDesktopCommandResult reopened =
      dispatchOne(app::CreativeDesktopCommandId::OpenDocument, task.context);
  return expect(saved.accepted && cleared.accepted && blank &&
                    reopened.accepted && reopened.documentReplaced,
                "task saves, clears, and reopens through desktop commands") &&
         expect(cr::creativeUndoDepth(task.appState.history) == 0U,
                "save and reopen establish a clean history boundary");
}

template <typename Item>
bool equalItems(std::span<const Item> live,
                const std::vector<Item>& expected) {
  return live.size() == expected.size() &&
         std::equal(live.begin(), live.end(), expected.begin());
}

bool canonicalBuildingTaskSurvivesLifecycle() {
  CreatorTaskHarness task{"canonical_building", 20'001U};
  if (!expect(task.saveRoot.ready() && task.installed,
              "building task harness initializes")) {
    return false;
  }

  app::CreativeEditorWorldLayoutBuildingBlockoutSettings blockout;
  blockout.shell.footprint = {{0, 0}, {12, 10}};
  blockout.shell.wallHeightCells = 3U;
  blockout.shell.wallThicknessCells = 0.25;
  blockout.pattern = cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitX;
  blockout.connectRooms = true;
  blockout.facade.includeEntrance = true;
  blockout.facade.includeExteriorWindows = true;
  const app::CreativeDesktopCommandResult staged = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      task.context,
      app::CreativeDesktopWorldLayoutBuildingBlockoutPayload{blockout});
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, task.context);
  if (!expect(staged.accepted && staged.worldLayoutChanged &&
                  generated.accepted && generated.sceneChanged &&
                  task.editor.worldLayout.source.buildings.size() == 1U &&
                  task.editor.worldLayout.source.rooms.size() == 2U &&
                  task.editor.worldLayout.source.openings.size() >= 2U &&
                  task.appState.facade.document().objectCount() > 0U,
              "building task authors and generates a two-room shell")) {
    return false;
  }

  const std::size_t generatedObjectCount =
      task.appState.facade.document().objectCount();
  if (!saveReopen(task) ||
      !expect(task.editor.worldLayout.source.buildings.size() == 1U &&
                  task.editor.worldLayout.source.rooms.size() == 2U &&
                  task.appState.facade.document().objectCount() ==
                      generatedObjectCount,
              "building recipe and generated output reopen together")) {
    return false;
  }

  app::CreativeEditorWorldLayoutLevelSettings settings;
  if (!expect(app::readCreativeEditorWorldLayoutLevelSettings(
                  task.editor.worldLayout, 0U, settings),
              "reopened building level remains editable")) {
    return false;
  }
  const std::uint16_t originalWallHeight = settings.wallHeightCells;
  const std::string originalName = settings.name;
  settings.wallHeightCells = static_cast<std::uint16_t>(originalWallHeight + 1U);
  settings.name = "Modified Ground Storey";
  const app::CreativeDesktopWorldLayoutPropertyEditPayload edit{
      app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit,
      cr::CreativeWorldLayoutTable::Level, 0U,
      task.editor.worldLayout.source.levels[0].stableKey, settings};
  const app::CreativeDesktopCommandResult modified = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
      task.context, edit);
  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, task.context);
  const cr::CreativeWorldLayoutCompileResult synchronized =
      cr::buildCreativeWorldLayoutPlan(task.appState.facade.document(),
                                       task.editor.worldLayout.source);
  return expect(modified.accepted && modified.changed &&
                    modified.worldLayoutChanged && modified.sceneChanged &&
                    cr::creativeUndoDepth(task.appState.history) == 0U &&
                    undone.accepted,
                "building post-reopen modification is one undoable edit") &&
         expect(task.editor.worldLayout.source.levels[0].wallHeightCells ==
                        originalWallHeight &&
                    task.editor.worldLayout.source.levels[0].name ==
                        originalName &&
                    task.editor.worldLayout.generatedRevision ==
                        task.editor.worldLayout.revision &&
                    task.appState.facade.document().objectCount() ==
                        generatedObjectCount &&
                    synchronized.receipt.accepted &&
                    synchronized.receipt.status ==
                        cr::CreativeWorldLayoutStatus::NoChange,
                "building undo restores source and generated output exactly");
}

cr::CreativeTerrainPathRecipeRequest roadRequest(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeTerrainPathPoint> points) {
  cr::CreativeTerrainPathRecipeRequest request;
  request.document = &document;
  request.kind = cr::CreativeTerrainRecipeKind::Road;
  request.points = points;
  request.elevation = cr::CreativeTerrainPathElevation::Level;
  request.halfWidthCells = 1U;
  request.amplitudeCells = 1U;
  return request;
}

cr::CreativeTerrainRecipeApplyReceipt applyTerrainForTaskTransaction(
    cr::CreativeAppState& appState,
    const cr::CreativeTerrainRecipePlan& plan,
    std::string_view source) {
  const std::optional<cr::CreativeAuthoringOperationRecord> operation =
      cr::makeCreativeAuthoringOperationRecord(
          cr::CreativeAuthoringFamily::Terrain,
          cr::CreativeAuthoringOperationKind::Apply, cr::toString(plan.kind),
          cr::fingerprintCreativeTerrainRecipePlan(plan),
          plan.controlEdits.size() + plan.materialEdits.size());
  if (!operation.has_value()) {
    cr::CreativeTerrainRecipeApplyReceipt receipt;
    receipt.requested = true;
    receipt.status = cr::CreativeTerrainRecipeStatus::InvalidKind;
    receipt.reasonCode = "creative_terrain_recipe_operation_record_invalid";
    return receipt;
  }

  cr::CreativeDocumentHistoryTransaction transaction =
      cr::beginCreativeHistoryTransaction(appState.facade, source, *operation);
  cr::CreativeTerrainRecipeApplyReceipt receipt =
      cr::applyCreativeTerrainRecipe(appState.facade, plan);
  if (!receipt.accepted || !receipt.changed) {
    cr::cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }
  const cr::CreativeHistoryRecordReceipt historyReceipt =
      cr::commitCreativeHistoryTransaction(
          appState.history, std::move(transaction), appState.facade);
  if (!historyReceipt.accepted || !historyReceipt.recorded) {
    receipt.reasonCode = std::string(historyReceipt.reasonCode);
  }
  return receipt;
}

bool canonicalTerrainCorridorTaskSurvivesLifecycle() {
  CreatorTaskHarness task{"canonical_terrain_corridor", 20'002U};
  if (!expect(task.saveRoot.ready() && task.installed,
              "terrain corridor task harness initializes")) {
    return false;
  }

  constexpr std::array initialPoints{
      cr::CreativeTerrainPathPoint{{0, 0}, 8U},
      cr::CreativeTerrainPathPoint{{4, 0}, 8U},
      cr::CreativeTerrainPathPoint{{8, 0}, 8U},
  };
  const cr::CreativeTerrainRecipeResult recipe =
      cr::buildCreativeTerrainPathRecipe(
          roadRequest(task.appState.facade.document(), initialPoints));
  const cr::CreativeTerrainRecipeApplyReceipt applied =
      applyTerrainForTaskTransaction(
          task.appState, recipe.plan, "canonical_terrain_corridor_create");
  if (!expect(recipe.receipt.accepted && applied.accepted && applied.changed &&
                  applied.terrainReceipt.changed &&
                  applied.materialReceipt.changed,
              "terrain corridor authors height and road material atomically")) {
    return false;
  }

  const std::vector<cr::CreativeTerrainControlPoint> savedControls(
      task.appState.facade.document().terrainField().controls().begin(),
      task.appState.facade.document().terrainField().controls().end());
  const std::vector<cr::CreativeTerrainMaterialOverride> savedMaterials(
      task.appState.facade.document().terrainMaterialField().overrides().begin(),
      task.appState.facade.document().terrainMaterialField().overrides().end());
  if (!saveReopen(task) ||
      !expect(equalItems(
                  task.appState.facade.document().terrainField().controls(),
                  savedControls) &&
                  equalItems(task.appState.facade.document()
                                 .terrainMaterialField()
                                 .overrides(),
                             savedMaterials),
              "terrain corridor height and material reopen exactly")) {
    return false;
  }

  constexpr std::array extendedPoints{
      cr::CreativeTerrainPathPoint{{0, 0}, 8U},
      cr::CreativeTerrainPathPoint{{4, 0}, 8U},
      cr::CreativeTerrainPathPoint{{8, 0}, 8U},
      cr::CreativeTerrainPathPoint{{12, 3}, 8U},
  };
  const cr::CreativeTerrainRecipeResult extension =
      cr::buildCreativeTerrainPathRecipe(
          roadRequest(task.appState.facade.document(), extendedPoints));
  const cr::CreativeTerrainRecipeApplyReceipt modified =
      applyTerrainForTaskTransaction(
          task.appState, extension.plan, "canonical_terrain_corridor_extend");
  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, task.context);
  return expect(extension.receipt.accepted && modified.accepted &&
                    modified.changed && undone.accepted &&
                    cr::creativeUndoDepth(task.appState.history) == 0U,
                "terrain corridor extension is one undoable edit") &&
         expect(equalItems(
                    task.appState.facade.document().terrainField().controls(),
                    savedControls) &&
                    equalItems(task.appState.facade.document()
                                   .terrainMaterialField()
                                   .overrides(),
                               savedMaterials),
                "terrain corridor undo restores exact height and material");
}

bool canonicalSiteCorridorTaskSurvivesLifecycle() {
  CreatorTaskHarness task{"canonical_site_corridor", 20'004U};
  if (!expect(task.saveRoot.ready() && task.installed,
              "site corridor task harness initializes")) {
    return false;
  }

  const app::CreativeDesktopCommandResult generated = dispatchPayload(
      app::CreativeDesktopCommandId::RegenerateMapTemplate, task.context,
      app::CreativeDesktopMapTemplatePayload{
          std::string{cr::kBuilderEstateMapTemplateId}});
  const auto ditch = std::find_if(
      task.editor.worldLayout.source.terrainPaths.begin(),
      task.editor.worldLayout.source.terrainPaths.end(),
      [](const cr::CreativeWorldLayoutTerrainPath& path) {
        return path.stableKey == "path.ditch";
      });
  const auto road = std::find_if(
      task.editor.worldLayout.source.terrainPaths.begin(),
      task.editor.worldLayout.source.terrainPaths.end(),
      [](const cr::CreativeWorldLayoutTerrainPath& path) {
        return path.stableKey == "path.estate_road";
      });
  const cr::CreativeObject* generatedDeck = findRecipeMember(
      task.appState.facade.document(), "bridge.ditch", "deck");
  if (!expect(
          generated.accepted && generated.documentReplaced &&
              generated.worldLayoutChanged && generated.sceneChanged &&
              ditch != task.editor.worldLayout.source.terrainPaths.end() &&
              road != task.editor.worldLayout.source.terrainPaths.end() &&
              ditch->recipe.kind == cr::CreativeTerrainPathKind::Trench &&
              road->recipe.kind == cr::CreativeTerrainPathKind::Road &&
              ditch->recipe.watercourse.crossings.size() == 1U &&
              !task.editor.worldLayout.source.objects.empty() &&
              task.editor.worldLayout.source.objects[0].usesBridgeRecipe &&
              task.editor.worldLayout.source.objects[0]
                      .bridge.watercoursePathKey == "path.ditch" &&
              task.editor.worldLayout.source.terrainOwnership ==
                  cr::CreativeWorldLayoutTerrainOwnership::PreserveExisting &&
              generatedDeck != nullptr &&
              task.appState.facade.document()
                      .terrainOperationStack()
                      .operations.size() == 4U,
          "site corridor generates terrain road trench bridge and approaches")) {
    return false;
  }

  const std::size_t roadIndex = static_cast<std::size_t>(
      std::distance(task.editor.worldLayout.source.terrainPaths.begin(), road));
  app::CreativeEditorWorldLayoutTerrainPathSettings roadSettings;
  if (!expect(app::readCreativeEditorWorldLayoutTerrainPathSettings(
                  task.editor.worldLayout, roadIndex, roadSettings) &&
                  roadSettings.recipe.points.size() == 2U,
              "site road exposes its durable grade settings")) {
    return false;
  }
  roadSettings.recipe.elevation = cr::CreativeTerrainPathElevation::Grade;
  roadSettings.recipe.points[0].heightCells = 3U;
  roadSettings.recipe.points[1].heightCells = 5U;
  roadSettings.recipe.road.maximumGradePermille = 100U;
  const cr::CreativeTerrainPathSourceRecipe gradedRoadRecipe =
      roadSettings.recipe;
  const app::CreativeDesktopCommandResult roadGraded = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
      task.context,
      app::CreativeDesktopWorldLayoutPropertyEditPayload{
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit,
          cr::CreativeWorldLayoutTable::TerrainPath, roadIndex, road->stableKey,
          roadSettings});

  const app::CreativeEditorWorldLayoutTopographyPlan gradedTopography =
      app::buildCreativeEditorWorldLayoutTopography(
          task.appState.facade.document(), 1U, 2U);
  const cr::CreativeTerrainAnalysisCell* steepestRoadCell = nullptr;
  for (const cr::CreativeTerrainAnalysisCell& cell :
       gradedTopography.analysis.cells) {
    if (!cell.terrainPresent || cell.coord.x < 12 || cell.coord.x > 64 ||
        cell.coord.z < 77 || cell.coord.z > 79) {
      continue;
    }
    if (steepestRoadCell == nullptr ||
        cell.slopeDegrees > steepestRoadCell->slopeDegrees) {
      steepestRoadCell = &cell;
    }
  }
  const app::CreativeEditorWorldLayoutTopographySample inspectedSlope =
      steepestRoadCell == nullptr
          ? app::CreativeEditorWorldLayoutTopographySample{}
          : app::sampleCreativeEditorWorldLayoutTopography(
                gradedTopography,
                static_cast<double>(steepestRoadCell->coord.x) + 0.5,
                static_cast<double>(steepestRoadCell->coord.z) + 0.5);

  const cr::CreativeTerrainContourSegment* refinementContour = nullptr;
  for (const cr::CreativeTerrainContourSegment& segment :
       gradedTopography.analysis.contours.segments) {
    const double midpointZ = (segment.start.z + segment.end.z) * 0.5;
    if (midpointZ >= 28.0) {
      refinementContour = &segment;
      break;
    }
  }
  const app::CreativeEditorWorldLayoutTerrainAnalysisEditPlan refinementEdit =
      refinementContour == nullptr
          ? app::CreativeEditorWorldLayoutTerrainAnalysisEditPlan{}
          : app::planCreativeEditorWorldLayoutTerrainAnalysisEdit(
                gradedTopography,
                (refinementContour->start.x + refinementContour->end.x) * 0.5,
                (refinementContour->start.z + refinementContour->end.z) * 0.5,
                0.05, cr::CreativeTerrainAnalysisHitMode::ContourOnly);
  app::CreativeEditorWorldLayoutTerrainRegionState& region =
      task.editor.worldLayoutTopography.region;
  region.editingEnabled = true;
  const bool refinementSelected =
      app::selectCreativeEditorWorldLayoutTerrainAnalysisEdit(region,
                                                               refinementEdit);
  if (refinementSelected) {
    region.recipe.targetHeightCells =
        region.recipe.targetHeightCells < cr::kCreativeTerrainMaximumHeightCells
            ? static_cast<std::uint16_t>(
                  region.recipe.targetHeightCells + 1U)
            : static_cast<std::uint16_t>(
                  region.recipe.targetHeightCells - 1U);
  }
  const std::uint64_t revisionBeforePreview =
      task.appState.facade.document().revision();
  const app::CreativeDesktopCommandResult refinementPreview =
      dispatchOne(app::CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview,
                  task.context);
  const std::uint64_t revisionAfterPreview =
      task.appState.facade.document().revision();
  const app::CreativeDesktopCommandResult refinementApplied =
      dispatchOne(app::CreativeDesktopCommandId::WorldLayoutTerrainRegionApply,
                  task.context);
  const auto manualRegion = std::find_if(
      task.appState.facade.document().terrainOperationStack().operations.begin(),
      task.appState.facade.document().terrainOperationStack().operations.end(),
      [](const cr::CreativeTerrainOperation& operation) {
        return operation.owner == cr::CreativeTerrainOperationOwner::Manual &&
               operation.kind == cr::CreativeTerrainOperationKind::Region;
      });
  if (!expect(
          roadGraded.accepted && roadGraded.changed &&
              roadGraded.worldLayoutChanged && roadGraded.sceneChanged &&
              gradedTopography.accepted &&
              gradedTopography.status ==
                  app::CreativeEditorWorldLayoutTopographyStatus::Ready &&
              gradedTopography.analysis.contours.accepted &&
              refinementContour != nullptr && refinementEdit.accepted &&
              refinementSelected && steepestRoadCell != nullptr &&
              inspectedSlope.present && inspectedSlope.slopeDegrees > 0.0 &&
              inspectedSlope.slopeBand !=
                  cr::CreativeTerrainSlopeBand::Unavailable &&
              inspectedSlope.slopeBand != cr::CreativeTerrainSlopeBand::Flat,
          "site road grades and exposes contour and slope inspection") ||
      !expect(
          refinementPreview.accepted && refinementPreview.changed &&
              revisionAfterPreview == revisionBeforePreview &&
              refinementApplied.accepted && refinementApplied.changed &&
              refinementApplied.sceneChanged &&
              task.appState.facade.document().revision() >
                  revisionAfterPreview &&
              manualRegion != task.appState.facade.document()
                                  .terrainOperationStack()
                                  .operations.end(),
          "contour target previews without mutation then applies one manual region")) {
    return false;
  }

  const cr::CreativeTerrainOperationId regionOperationId = manualRegion->id;
  const cr::CreativeTerrainRegionRecipe regionRecipe = manualRegion->region;
  generatedDeck = findRecipeMember(task.appState.facade.document(),
                                   "bridge.ditch", "deck");
  if (!expect(generatedDeck != nullptr,
              "site bridge remains generated after terrain refinement")) {
    return false;
  }
  const cr::CreativeObjectId deckId = generatedDeck->id;
  const cr::CreativeTransform originalDeckTransform = generatedDeck->transform;
  const cr::CreativeBounds originalDeckBounds = generatedDeck->bounds;
  const std::size_t objectCount =
      task.appState.facade.document().objectCount();
  if (!saveReopen(task)) {
    return false;
  }

  const auto reopenedDitch = std::find_if(
      task.editor.worldLayout.source.terrainPaths.begin(),
      task.editor.worldLayout.source.terrainPaths.end(),
      [](const cr::CreativeWorldLayoutTerrainPath& path) {
        return path.stableKey == "path.ditch";
      });
  const cr::CreativeObject* reopenedDeck = findRecipeMember(
      task.appState.facade.document(), "bridge.ditch", "deck");
  const auto reopenedRoad = std::find_if(
      task.editor.worldLayout.source.terrainPaths.begin(),
      task.editor.worldLayout.source.terrainPaths.end(),
      [](const cr::CreativeWorldLayoutTerrainPath& path) {
        return path.stableKey == "path.estate_road";
      });
  const cr::CreativeTerrainOperation* reopenedRegion =
      cr::findCreativeTerrainOperation(
          task.appState.facade.document().terrainOperationStack(),
          regionOperationId);
  if (!expect(
          reopenedDitch !=
                  task.editor.worldLayout.source.terrainPaths.end() &&
              reopenedRoad !=
                  task.editor.worldLayout.source.terrainPaths.end() &&
              reopenedRoad->recipe == gradedRoadRecipe &&
              reopenedRegion != nullptr &&
              reopenedRegion->owner ==
                  cr::CreativeTerrainOperationOwner::Manual &&
              reopenedRegion->kind ==
                  cr::CreativeTerrainOperationKind::Region &&
              reopenedRegion->region == regionRecipe &&
              reopenedDeck != nullptr && reopenedDeck->id == deckId &&
              sameTransform(reopenedDeck->transform, originalDeckTransform) &&
              sameBounds(reopenedDeck->bounds, originalDeckBounds) &&
              task.appState.facade.document().objectCount() == objectCount,
          "site corridor source and generated bridge reopen with stable identity")) {
    return false;
  }

  const std::size_t ditchIndex = static_cast<std::size_t>(
      std::distance(task.editor.worldLayout.source.terrainPaths.begin(),
                    reopenedDitch));
  app::CreativeEditorWorldLayoutTerrainPathSettings settings;
  if (!expect(app::readCreativeEditorWorldLayoutTerrainPathSettings(
                  task.editor.worldLayout, ditchIndex, settings) &&
                  settings.recipe.points.size() == 3U,
              "reopened site corridor remains editable")) {
    return false;
  }
  const cr::CreativeTerrainPathSourceRecipe originalRecipe = settings.recipe;
  settings.recipe.points[1].coord.z += 1;
  const app::CreativeDesktopCommandResult modified = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
      task.context,
      app::CreativeDesktopWorldLayoutPropertyEditPayload{
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit,
          cr::CreativeWorldLayoutTable::TerrainPath, ditchIndex,
          reopenedDitch->stableKey, settings});
  const cr::CreativeObject* movedDeck = findRecipeMember(
      task.appState.facade.document(), "bridge.ditch", "deck");
  const cr::CreativeTerrainOperation* retainedRegion =
      cr::findCreativeTerrainOperation(
          task.appState.facade.document().terrainOperationStack(),
          regionOperationId);
  const cr::CreativeWorldLayoutCompileResult synchronized =
      cr::buildCreativeWorldLayoutPlan(task.appState.facade.document(),
                                       task.editor.worldLayout.source);

  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &task.appState.facade.document();
  bakeRequest.roomId = "canonical_site_corridor";
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const float deckX =
      movedDeck != nullptr
          ? static_cast<float>(movedDeck->transform.position.x)
          : 0.0F;
  const float deckZ =
      movedDeck != nullptr
          ? static_cast<float>(movedDeck->transform.position.z)
          : 0.0F;
  const iggy3d::CollisionQueryResult deckTop =
      iggy3d::sampleSurfaceHeight(surfaces, {deckX, 0.0F, deckZ});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const bool centerlineBlocked =
      deckTop.status == iggy3d::CollisionQueryStatus::Hit &&
      iggy3d::segmentHitsAnyPhysicsAabb(
          physics.colliders, {deckX, deckTop.heightMeters + 0.5F, deckZ - 4.0F},
          {deckX, deckTop.heightMeters + 0.5F, deckZ + 4.0F}, 0.05F,
          nullptr);
  const std::array<iggy3d::Vec3, 2U> bridgeWaypoints{
      iggy3d::Vec3{deckX, deckTop.heightMeters, deckZ - 4.0F},
      iggy3d::Vec3{deckX, deckTop.heightMeters, deckZ + 4.0F},
  };
  const iggy3d::ReasoningGraph bridgeReasoning =
      iggy3d::buildReasoningGraph(baked.room, bridgeWaypoints);
  std::array<std::uint32_t, 2U> bridgeNodeIds{};
  std::size_t bridgeNodeCount = 0U;
  for (const iggy3d::ReasoningNode& node : bridgeReasoning.nodes) {
    if (node.kind == iggy3d::ReasoningNodeKind::patrolPost &&
        node.sourceLabel == "waypoint" &&
        bridgeNodeCount < bridgeNodeIds.size()) {
      bridgeNodeIds[bridgeNodeCount++] = node.id;
    }
  }
  const bool bridgeEdge =
      bridgeNodeCount == bridgeNodeIds.size() &&
      std::any_of(bridgeReasoning.edges.begin(), bridgeReasoning.edges.end(),
                  [&](const iggy3d::ReasoningEdge& edge) {
                    return (edge.from == bridgeNodeIds[0] &&
                            edge.to == bridgeNodeIds[1]) ||
                           (edge.from == bridgeNodeIds[1] &&
                            edge.to == bridgeNodeIds[0]);
                  });
  if (!modified.accepted || !modified.changed ||
      !modified.worldLayoutChanged || !modified.sceneChanged ||
      cr::creativeUndoDepth(task.appState.history) != 1U ||
      movedDeck == nullptr || movedDeck->id != deckId ||
      retainedRegion == nullptr || retainedRegion->region != regionRecipe ||
      movedDeck->transform.position.z !=
          originalDeckTransform.position.z + 1.0 ||
      task.appState.facade.document().objectCount() != objectCount ||
      !synchronized.receipt.accepted ||
      synchronized.receipt.status != cr::CreativeWorldLayoutStatus::NoChange) {
    std::cerr << "Site corridor edit: accepted=" << modified.accepted
              << " changed=" << modified.changed
              << " layout=" << modified.worldLayoutChanged
              << " scene=" << modified.sceneChanged
              << " undo=" << cr::creativeUndoDepth(task.appState.history)
              << " deck=" << (movedDeck != nullptr)
              << " deck-id="
              << (movedDeck != nullptr ? movedDeck->id : cr::kInvalidObjectId)
              << " expected-id=" << deckId
              << " region=" << (retainedRegion != nullptr)
              << " deck-z="
              << (movedDeck != nullptr ? movedDeck->transform.position.z : 0.0)
              << " expected-z=" << originalDeckTransform.position.z + 1.0
              << " objects=" << task.appState.facade.document().objectCount()
              << " expected-objects=" << objectCount
              << " sync-status=" << cr::toString(synchronized.receipt.status)
              << " sync-reason=" << synchronized.receipt.reasonCode
              << " sync-kernel=" << synchronized.receipt.kernelReasonCode
              << " sync-recipes="
              << synchronized.receipt.objectRecipeCount
              << " sync-patches="
              << synchronized.receipt.objectRecipePatchCount
              << " sync-removes=" << synchronized.receipt.objectRemoveCount
              << " sync-terrain="
              << synchronized.receipt.terrainControlEditCount
              << " sync-materials="
              << synchronized.receipt.terrainMaterialEditCount
              << " sync-operations="
              << synchronized.receipt.terrainOperationMutationCount
              << '\n';
  }
  if (!baked.receipt.accepted || !physics.ok ||
      deckTop.status != iggy3d::CollisionQueryStatus::Hit ||
      centerlineBlocked || !bridgeEdge) {
    std::cerr << "Site corridor traversal: bake=" << baked.receipt.accepted
              << " bake-reason=" << baked.receipt.reasonCode
              << " physics=" << physics.ok
              << " surface-status=" << static_cast<int>(deckTop.status)
              << " surface-y=" << deckTop.heightMeters
              << " blocked=" << centerlineBlocked
              << " nodes=" << bridgeReasoning.nodes.size()
              << " edges=" << bridgeReasoning.edges.size()
              << " bridge-nodes=" << bridgeNodeCount
              << " bridge-edge=" << bridgeEdge
              << " deck-x=" << deckX << " deck-z=" << deckZ << '\n';
  }
  if (!expect(
          modified.accepted && modified.changed &&
              modified.worldLayoutChanged && modified.sceneChanged &&
              cr::creativeUndoDepth(task.appState.history) == 1U &&
              movedDeck != nullptr && movedDeck->id == deckId &&
              retainedRegion != nullptr &&
              retainedRegion->owner ==
                  cr::CreativeTerrainOperationOwner::Manual &&
              retainedRegion->region == regionRecipe &&
              movedDeck->transform.position.z ==
                  originalDeckTransform.position.z + 1.0 &&
              task.appState.facade.document().objectCount() == objectCount &&
              synchronized.receipt.accepted &&
              synchronized.receipt.status ==
                  cr::CreativeWorldLayoutStatus::NoChange,
          "site corridor edit regenerates its attached bridge in one history record") ||
      !expect(baked.receipt.accepted && physics.ok &&
                  deckTop.status == iggy3d::CollisionQueryStatus::Hit &&
                  !centerlineBlocked && bridgeEdge,
              "regenerated bridge remains baked collidable and traversable")) {
    return false;
  }

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, task.context);
  const cr::CreativeObject* restoredDeck = findRecipeMember(
      task.appState.facade.document(), "bridge.ditch", "deck");
  const cr::CreativeTerrainOperation* restoredRegion =
      cr::findCreativeTerrainOperation(
          task.appState.facade.document().terrainOperationStack(),
          regionOperationId);
  const cr::CreativeWorldLayoutCompileResult restored =
      cr::buildCreativeWorldLayoutPlan(task.appState.facade.document(),
                                       task.editor.worldLayout.source);
  const bool restoredExact =
      undone.accepted && cr::creativeUndoDepth(task.appState.history) == 0U &&
      task.editor.worldLayout.source.terrainPaths[ditchIndex].recipe ==
          originalRecipe &&
      task.editor.worldLayout.generatedRevision ==
          task.editor.worldLayout.revision &&
      restoredDeck != nullptr && restoredDeck->id == deckId &&
      restoredRegion != nullptr &&
      restoredRegion->owner == cr::CreativeTerrainOperationOwner::Manual &&
      restoredRegion->region == regionRecipe &&
      sameTransform(restoredDeck->transform, originalDeckTransform) &&
      sameBounds(restoredDeck->bounds, originalDeckBounds) &&
      task.appState.facade.document().objectCount() == objectCount &&
      restored.receipt.accepted &&
      restored.receipt.status == cr::CreativeWorldLayoutStatus::NoChange;
  if (!restoredExact) {
    std::cerr
        << "Site corridor undo: accepted=" << undone.accepted
        << " undo=" << cr::creativeUndoDepth(task.appState.history)
        << " source="
        << (task.editor.worldLayout.source.terrainPaths[ditchIndex].recipe ==
            originalRecipe)
        << " revisions="
        << (task.editor.worldLayout.generatedRevision ==
            task.editor.worldLayout.revision)
        << " deck=" << (restoredDeck != nullptr)
        << " deck-id="
        << (restoredDeck != nullptr ? restoredDeck->id
                                    : cr::kInvalidObjectId)
        << " expected-id=" << deckId
        << " region=" << (restoredRegion != nullptr)
        << " region-owner="
        << (restoredRegion != nullptr
                ? static_cast<int>(restoredRegion->owner)
                : -1)
        << " region-recipe="
        << (restoredRegion != nullptr &&
            restoredRegion->region == regionRecipe)
        << " transform="
        << (restoredDeck != nullptr &&
            sameTransform(restoredDeck->transform, originalDeckTransform))
        << " bounds="
        << (restoredDeck != nullptr &&
            sameBounds(restoredDeck->bounds, originalDeckBounds))
        << " objects=" << task.appState.facade.document().objectCount()
        << " expected-objects=" << objectCount
        << " sync=" << restored.receipt.accepted
        << " sync-status=" << cr::toString(restored.receipt.status)
        << " sync-reason=" << restored.receipt.reasonCode << '\n';
  }
  return expect(
             restoredExact,
             "site corridor undo restores source structure and generated output");
}

cr::CreativeDocumentCreateReceipt createAssetObject(
    cr::CreativeDocument& document, cr::CreativeObjectKind kind,
    std::string name, std::string assetId, cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.assetId = std::move(assetId);
  request.assetContentHash = 0xC0FFEEU;
  request.assetMaterialVariant = "weathered_oak";
  request.transform.position = position;
  request.hasTransformOverride = true;
  return document.createObject(request);
}

bool canonicalAssetCompositionTaskSurvivesLifecycle() {
  CreatorTaskHarness task{"canonical_asset_composition", 20'003U};
  if (!expect(task.saveRoot.ready() && task.installed,
              "asset composition task harness initializes")) {
    return false;
  }

  cr::CreativeDocument composed =
      cr::CreativeDocument::create("Canonical Asset Composition");
  static_cast<void>(composed.assignId(20'003U));
  const cr::CreativeDocumentCreateReceipt table = createAssetObject(
      composed, cr::CreativeObjectKind::Furniture, "Workbench",
      "settlement/workbench", {2.0, 0.0, 2.0});
  const cr::CreativeDocumentCreateReceipt crate = createAssetObject(
      composed, cr::CreativeObjectKind::Crate, "Supply Crate",
      "settlement/supply_crate", {3.5, 0.0, 2.0});
  if (!expect(table.accepted && crate.accepted &&
                  task.appState.facade.installDocument(std::move(composed))
                      .accepted,
              "asset composition installs two asset-backed parts")) {
    return false;
  }

  const std::array partIds{table.objectId, crate.objectId};
  const cr::CreativeSelectionReceipt selected =
      task.appState.facade.selectTargets(partIds, crate.objectId);
  const cr::CreativeGroupCommandReceipt grouped =
      app::applyCreativeEditorGroupCommandWithHistory(
          task.appState, "canonical_asset_composition_group");
  if (!expect(selected.accepted && grouped.accepted && grouped.changed &&
                  grouped.groupObjectId != cr::kInvalidObjectId,
              "asset composition groups both parts in one edit")) {
    return false;
  }

  const std::array groupSelection{grouped.groupObjectId};
  static_cast<void>(task.appState.facade.selectTargets(
      groupSelection, grouped.groupObjectId));
  app::CreativeEditorAuthoredAssetLibrary library;
  const app::CreativeEditorAuthoredAssetLoadReceipt initialized =
      app::loadCreativeEditorAuthoredAssetLibrary(library,
                                                  task.saveRoot.path());
  const app::CreativeEditorAuthoredAssetSaveReceipt assetSaved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(
          task.appState, library, "Workshop Supply Station");
  if (!expect(initialized.accepted && assetSaved.accepted &&
                  assetSaved.durableWriteOk &&
                  assetSaved.capture.definition.content.objects.size() == 3U,
              "asset composition publishes a durable reusable definition")) {
    return false;
  }

  if (!saveReopen(task)) {
    return false;
  }
  app::CreativeEditorAuthoredAssetLibrary reloadedLibrary;
  const app::CreativeEditorAuthoredAssetLoadReceipt libraryReloaded =
      app::loadCreativeEditorAuthoredAssetLibrary(reloadedLibrary,
                                                  task.saveRoot.path());
  const cr::CreativeAuthoredAssetDefinition* reloadedDefinition =
      app::findCreativeEditorAuthoredAsset(reloadedLibrary, assetSaved.assetId);
  const cr::CreativeObject* reopenedGroup =
      task.appState.facade.findObject(grouped.groupObjectId);
  const cr::CreativeObject* reopenedTable =
      task.appState.facade.findObject(table.objectId);
  const cr::CreativeObject* reopenedCrate =
      task.appState.facade.findObject(crate.objectId);
  if (!expect(libraryReloaded.accepted && reloadedDefinition != nullptr &&
                  reloadedDefinition->content.objects.size() == 3U &&
                  reopenedGroup != nullptr && reopenedTable != nullptr &&
                  reopenedCrate != nullptr &&
                  reopenedTable->parentId == grouped.groupObjectId &&
                  reopenedCrate->parentId == grouped.groupObjectId &&
                  reopenedTable->assetId == "settlement/workbench" &&
                  reopenedCrate->assetId == "settlement/supply_crate",
              "composition hierarchy, asset identity, and library reopen")) {
    return false;
  }

  const cr::CreativeTransform originalGroupTransform = reopenedGroup->transform;
  const cr::CreativeTransform originalCrateTransform = reopenedCrate->transform;
  cr::CreativeVec3 movedPivot = originalGroupTransform.position;
  movedPivot.x += 1.25;
  const app::CreativeDesktopCommandResult modified = dispatchPayload(
      app::CreativeDesktopCommandId::SetGroupPivot, task.context,
      app::CreativeDesktopGroupPivotPayload{grouped.groupObjectId, movedPivot});
  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, task.context);
  const cr::CreativeObject* restoredGroup =
      task.appState.facade.findObject(grouped.groupObjectId);
  const cr::CreativeObject* restoredCrate =
      task.appState.facade.findObject(crate.objectId);
  return expect(modified.accepted && modified.changed && undone.accepted &&
                    cr::creativeUndoDepth(task.appState.history) == 0U,
                "reopened composition edit is one undoable action") &&
         expect(restoredGroup != nullptr && restoredCrate != nullptr &&
                    sameTransform(restoredGroup->transform,
                                  originalGroupTransform) &&
                    sameTransform(restoredCrate->transform,
                                  originalCrateTransform) &&
                    restoredCrate->parentId == grouped.groupObjectId &&
                    restoredCrate->assetId == "settlement/supply_crate",
                "composition undo restores pivot, hierarchy, and asset id");
}

}  // namespace

int main() {
  bool ok = true;
  ok = canonicalBuildingTaskSurvivesLifecycle() && ok;
  ok = canonicalTerrainCorridorTaskSurvivesLifecycle() && ok;
  ok = canonicalSiteCorridorTaskSurvivesLifecycle() && ok;
  ok = canonicalAssetCompositionTaskSurvivesLifecycle() && ok;
  if (ok) {
    std::cout << "creative creator task workflow tests passed\n";
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
