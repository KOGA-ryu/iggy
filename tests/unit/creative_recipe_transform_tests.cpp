#include "app/iggy3d/creative/tools/RecipeTransform.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocument makeDocument(std::string name, cr::CreativeDocumentId id) {
  cr::CreativeDocument document = cr::CreativeDocument::create(std::move(name));
  static_cast<void>(document.assignId(id));
  return document;
}

cr::CreativeDocumentCreateReceipt createCrate(cr::CreativeDocument& document,
                                              std::string name,
                                              cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  return document.createObject(request);
}

cr::CreativePatternRecipeMutationReceipt addPattern(
    cr::CreativeDocument& document,
    cr::CreativePatternRecipe recipe) {
  cr::CreativePatternRecipeMutationRequest request;
  request.kind = cr::CreativePatternRecipeMutationKind::Add;
  request.recipe = std::move(recipe);
  return document.applyPatternRecipeMutation(request);
}

bool patternTranslationMovesTheWholeRelationship() {
  cr::CreativeDocument document = makeDocument("Pattern translation", 9101U);
  const cr::CreativeDocumentCreateReceipt source =
      createCrate(document, "Source", {1.0, 2.0, 3.0});
  const cr::CreativeDocumentCreateReceipt generatedA =
      createCrate(document, "Generated A", {3.0, 2.0, 3.0});
  const cr::CreativeDocumentCreateReceipt generatedB =
      createCrate(document, "Generated B", {5.0, 2.0, 3.0});
  cr::CreativePatternRecipe recipe;
  recipe.kind = cr::CreativePatternRecipeKind::RadialArray;
  recipe.sourceObjectIds = {source.objectId};
  recipe.generatedObjectIds = {generatedA.objectId, generatedB.objectId};
  recipe.radial.pivot = {2.0, 2.0, 3.0};
  const cr::CreativePatternRecipeMutationReceipt added =
      addPattern(document, recipe);
  if (!expect(source.accepted && generatedA.accepted && generatedB.accepted &&
                  added.accepted,
              "pattern fixture created")) {
    return false;
  }

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVec3 delta{4.0, -1.0, 2.0};
  const cr::CreativePatternRecipeTranslationPlan plan =
      cr::planCreativePatternRecipeTranslation(document, added.recipeId,
                                                delta);
  const cr::CreativePatternRecipeTranslationReceipt applied =
      cr::applyCreativePatternRecipeTranslation(document, plan);
  const cr::CreativePatternRecipe* translated = cr::findCreativePatternRecipe(
      document.patternRecipeStore(), added.recipeId);
  const cr::CreativeObject* movedSource = document.findObject(source.objectId);
  const cr::CreativeObject* movedA = document.findObject(generatedA.objectId);
  const cr::CreativeObject* movedB = document.findObject(generatedB.objectId);
  return expect(plan.accepted && plan.changed &&
                    plan.memberObjectIds.size() == 3U &&
                    plan.placementPlan.objects.size() == 3U,
                "pattern plan owns source and every generated member") &&
         expect(cr::creativeVec3ExactlyEqual(plan.translatedRecipe.radial.pivot,
                                             {6.0, 1.0, 5.0}),
                "radial pivot translates with relationship") &&
         expect(applied.accepted && applied.changed &&
                    document.revision() == revisionBefore + 1U,
                "pattern translation publishes one document revision") &&
         expect(translated != nullptr && movedSource != nullptr &&
                    movedA != nullptr && movedB != nullptr &&
                    translated->sourceObjectIds == recipe.sourceObjectIds &&
                    translated->generatedObjectIds == recipe.generatedObjectIds,
                "pattern identity and membership survive translation") &&
         expect(cr::creativeVec3ExactlyEqual(movedSource->transform.position,
                                             {5.0, 1.0, 5.0}) &&
                    cr::creativeVec3ExactlyEqual(movedA->transform.position,
                                                 {7.0, 1.0, 5.0}) &&
                    cr::creativeVec3ExactlyEqual(movedB->transform.position,
                                                 {9.0, 1.0, 5.0}),
                "all pattern members move by the exact displacement");
}

bool scatterSourcesAndExclusionsTranslate() {
  cr::CreativeDocument document = makeDocument("Scatter translation", 9102U);
  const cr::CreativeDocumentCreateReceipt generated =
      createCrate(document, "Scatter output", {2.0, 0.0, 2.0});
  cr::CreativePatternRecipe recipe;
  recipe.kind = cr::CreativePatternRecipeKind::AssetScatter;
  recipe.generatedObjectIds = {generated.objectId};
  recipe.scatter.objectKind = cr::CreativeObjectKind::Crate;
  recipe.scatter.assetId = "test/scatter";
  recipe.scatter.assetSourceBounds = {{-0.5, 0.0, -0.5},
                                      {0.5, 1.0, 0.5}};
  recipe.scatter.paintCenters = {{2.0, 0.0, 2.0}, {8.0, 0.0, 2.0}};
  recipe.scatter.exclusions = {{{4.0, 0.0, 2.0}, 1.0}};
  const cr::CreativePatternRecipeMutationReceipt added =
      addPattern(document, recipe);
  const cr::CreativePatternRecipeTranslationPlan plan =
      cr::planCreativePatternRecipeTranslation(document, added.recipeId,
                                                {-2.0, 3.0, 5.0});
  return expect(generated.accepted && added.accepted && plan.accepted,
                "scatter translation plans") &&
         expect(cr::creativeVec3ExactlyEqual(
                    plan.translatedRecipe.scatter.paintCenters[0],
                    {0.0, 3.0, 7.0}) &&
                    cr::creativeVec3ExactlyEqual(
                        plan.translatedRecipe.scatter.paintCenters[1],
                        {6.0, 3.0, 7.0}) &&
                    cr::creativeVec3ExactlyEqual(
                        plan.translatedRecipe.scatter.exclusions[0].center,
                        {2.0, 3.0, 7.0}),
                "scatter source regions and exclusions move with output");
}

bool patternStalenessAndDependenciesFailAtomically() {
  cr::CreativeDocument document = makeDocument("Pattern conflicts", 9103U);
  const cr::CreativeDocumentCreateReceipt source =
      createCrate(document, "Source", {});
  const cr::CreativeDocumentCreateReceipt generated =
      createCrate(document, "Generated", {2.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt dependentGenerated =
      createCrate(document, "Dependent", {4.0, 0.0, 0.0});
  cr::CreativePatternRecipe first;
  first.kind = cr::CreativePatternRecipeKind::LinearArray;
  first.sourceObjectIds = {source.objectId};
  first.generatedObjectIds = {generated.objectId};
  const cr::CreativePatternRecipeMutationReceipt firstAdded =
      addPattern(document, first);
  cr::CreativePatternRecipe dependent;
  dependent.kind = cr::CreativePatternRecipeKind::LinearArray;
  dependent.sourceObjectIds = {generated.objectId};
  dependent.generatedObjectIds = {dependentGenerated.objectId};
  const cr::CreativePatternRecipeMutationReceipt dependentAdded =
      addPattern(document, dependent);
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativePatternRecipeTranslationPlan blocked =
      cr::planCreativePatternRecipeTranslation(document, firstAdded.recipeId,
                                                {1.0, 0.0, 0.0});
  bool ok = expect(firstAdded.accepted && dependentAdded.accepted &&
                       !blocked.accepted &&
                       blocked.status ==
                           cr::CreativeRecipeTranslationStatus::DependencyConflict &&
                       document.revision() == revisionBefore,
                   "dependent recipe rejects without mutation");

  cr::CreativeDocument clean = makeDocument("Pattern stale", 9104U);
  const auto cleanSource = createCrate(clean, "Source", {});
  const auto cleanGenerated = createCrate(clean, "Generated", {2.0, 0.0, 0.0});
  cr::CreativePatternRecipe cleanRecipe;
  cleanRecipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  cleanRecipe.sourceObjectIds = {cleanSource.objectId};
  cleanRecipe.generatedObjectIds = {cleanGenerated.objectId};
  const auto cleanAdded = addPattern(clean, cleanRecipe);
  const cr::CreativePatternRecipeTranslationPlan stalePlan =
      cr::planCreativePatternRecipeTranslation(clean, cleanAdded.recipeId,
                                                {1.0, 0.0, 0.0});
  static_cast<void>(createCrate(clean, "Intervening", {9.0, 0.0, 0.0}));
  const std::uint64_t staleRevision = clean.revision();
  const cr::CreativePatternRecipeTranslationReceipt stale =
      cr::applyCreativePatternRecipeTranslation(clean, stalePlan);
  return expect(!stale.accepted && !stale.changed &&
                    stale.status ==
                        cr::CreativeRecipeTranslationStatus::StaleSource &&
                    clean.revision() == staleRevision,
                "stale pattern plan rejects atomically") &&
         ok;
}

cr::CreativeTerrainOperationMutationRequest generatedBase() {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = cr::CreativeTerrainOperationKind::GeneratedTerrain;
  request.generation.bounds = {{-32, -32}, 64U, 64U};
  request.generation.baseHeightCells = 4U;
  request.generation.reliefCells = 0U;
  request.generation.horizontalScaleCells = 8.0;
  request.generation.octaveCount = 1U;
  request.generation.slopeDamping = 0.0;
  return request;
}

cr::CreativeTerrainOperationMutationReceipt addTerrainOperation(
    cr::CreativeDocument& document,
    cr::CreativeTerrainOperationMutationRequest request) {
  const auto base = document.terrainOperationStack().operations.empty()
                        ? document.applyTerrainOperationMutation(generatedBase())
                        : cr::CreativeTerrainOperationMutationReceipt{};
  if (document.terrainOperationStack().operations.empty() ||
      (!base.accepted && base.requested)) {
    return {};
  }
  return document.applyTerrainOperationMutation(request);
}

cr::CreativeTerrainOperationMutationRequest manualOperation(
    cr::CreativeTerrainOperationKind kind) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.operationKind = kind;
  request.owner = cr::CreativeTerrainOperationOwner::Manual;
  switch (kind) {
    case cr::CreativeTerrainOperationKind::Region:
      request.region.bounds = {{1, 2}, 3U, 4U};
      request.region.mode = cr::CreativeTerrainRegionMode::Flatten;
      request.region.targetHeightCells = 5U;
      break;
    case cr::CreativeTerrainOperationKind::Grade:
      request.grade.start = {-4, -2};
      request.grade.end = {4, 2};
      request.grade.startHeightCells = 4U;
      request.grade.endHeightCells = 5U;
      break;
    case cr::CreativeTerrainOperationKind::Profile:
      request.profile.center = {2, -3};
      request.profile.radiusCells = 3U;
      request.profile.amplitudeCells = 1U;
      break;
    case cr::CreativeTerrainOperationKind::Path:
      request.path.points = {{1U, {-6, 0}, 4U, 1U, 0U, 0},
                             {2U, {6, 0}, 4U, 1U, 0U, 0}};
      request.path.nextPointId = 3U;
      break;
    case cr::CreativeTerrainOperationKind::Stamp:
      request.stamp.stamp.assetId = "test/terrain_stamp";
      request.stamp.stamp.label = "Test terrain stamp";
      request.stamp.stamp.sourceDocumentId = 1U;
      request.stamp.stamp.sourceRevision = 1U;
      request.stamp.stamp.widthCells = 2U;
      request.stamp.stamp.depthCells = 1U;
      request.stamp.stamp.minimumHeightCells = 4U;
      request.stamp.stamp.heights = {4U, 5U};
      request.stamp.stamp.materials = {
          cr::creativeTerrainMaterialSolidWeights(
              cr::CreativeTerrainMaterial::Dirt),
          cr::creativeTerrainMaterialSolidWeights(
              cr::CreativeTerrainMaterial::Stone)};
      request.stamp.stamp.contentSignature =
          cr::creativeTerrainStampContentSignature(request.stamp.stamp);
      request.stamp.targetMinimum = {-2, 4};
      request.stamp.quarterTurns = 1U;
      request.stamp.elevationMode =
          cr::CreativeTerrainStampElevationMode::Absolute;
      break;
    case cr::CreativeTerrainOperationKind::Landform:
      request.landform.bounds = {{-3, -3}, 6U, 6U};
      request.landform.baseHeightCells = 4U;
      request.landform.targetHeightCells = 6U;
      request.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
      break;
    case cr::CreativeTerrainOperationKind::GeneratedTerrain:
    case cr::CreativeTerrainOperationKind::Count:
      break;
  }
  return request;
}

bool translatedOperationCoordinatesEqual(
    const cr::CreativeTerrainOperation& operation,
    cr::CreativeTerrainCoord2 delta) {
  switch (operation.kind) {
    case cr::CreativeTerrainOperationKind::Region:
      return operation.region.bounds.minimum ==
             cr::CreativeTerrainCoord2{1 + delta.x, 2 + delta.z};
    case cr::CreativeTerrainOperationKind::Grade:
      return operation.grade.start ==
                 cr::CreativeTerrainCoord2{-4 + delta.x, -2 + delta.z} &&
             operation.grade.end ==
                 cr::CreativeTerrainCoord2{4 + delta.x, 2 + delta.z};
    case cr::CreativeTerrainOperationKind::Profile:
      return operation.profile.center ==
             cr::CreativeTerrainCoord2{2 + delta.x, -3 + delta.z};
    case cr::CreativeTerrainOperationKind::Path:
      return operation.path.points.size() == 2U &&
             operation.path.points[0].coord ==
                 cr::CreativeTerrainCoord2{-6 + delta.x, delta.z} &&
             operation.path.points[1].coord ==
                 cr::CreativeTerrainCoord2{6 + delta.x, delta.z};
    case cr::CreativeTerrainOperationKind::Stamp:
      return operation.stamp.targetMinimum ==
             cr::CreativeTerrainCoord2{-2 + delta.x, 4 + delta.z};
    case cr::CreativeTerrainOperationKind::Landform:
      return operation.landform.bounds.minimum ==
             cr::CreativeTerrainCoord2{-3 + delta.x, -3 + delta.z};
    case cr::CreativeTerrainOperationKind::GeneratedTerrain:
    case cr::CreativeTerrainOperationKind::Count:
      return false;
  }
  return false;
}

bool durableTerrainKindsPlanReplayAndApply() {
  constexpr std::array kinds{
      cr::CreativeTerrainOperationKind::Region,
      cr::CreativeTerrainOperationKind::Grade,
      cr::CreativeTerrainOperationKind::Profile,
      cr::CreativeTerrainOperationKind::Path,
      cr::CreativeTerrainOperationKind::Stamp,
      cr::CreativeTerrainOperationKind::Landform,
  };
  const cr::CreativeTerrainCoord2 delta{3, -2};
  bool ok = true;
  for (std::size_t index = 0U; index < kinds.size(); ++index) {
    cr::CreativeDocument document =
        makeDocument("Terrain translation " + std::to_string(index),
                     9200U + index);
    const cr::CreativeTerrainOperationMutationReceipt added =
        addTerrainOperation(document, manualOperation(kinds[index]));
    const std::uint64_t revisionBefore = document.revision();
    const cr::CreativeTerrainOperationTranslationPlan plan =
        cr::planCreativeTerrainOperationTranslation(document,
                                                     added.operationId, delta);
    cr::CreativeTerrainHeightFieldBounds bounds{};
    const bool bounded = plan.accepted &&
                         cr::creativeTerrainOperationSpatialBounds(
                             plan.translatedOperation, bounds);
    const cr::CreativeTerrainOperationTranslationReceipt applied =
        cr::applyCreativeTerrainOperationTranslation(document, plan);
    const cr::CreativeTerrainOperation* operation =
        cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                         added.operationId);
    ok = expect(added.accepted && plan.accepted && plan.changed &&
                    plan.mutationPlan.receipt.replay.accepted && bounded,
                "terrain operation translation replays exact candidate") &&
         expect(applied.accepted && applied.changed &&
                    document.revision() == revisionBefore + 1U &&
                    operation != nullptr && operation->id == added.operationId &&
                    translatedOperationCoordinatesEqual(*operation, delta),
                "terrain operation retains identity and translated source") &&
         ok;
  }
  return ok;
}

bool terrainUnsupportedOverflowAndStalePlansFailClosed() {
  cr::CreativeDocument document = makeDocument("Terrain rejection", 9301U);
  const cr::CreativeTerrainOperationMutationReceipt generated =
      document.applyTerrainOperationMutation(generatedBase());
  const cr::CreativeTerrainOperationTranslationPlan global =
      cr::planCreativeTerrainOperationTranslation(document,
                                                   generated.operationId,
                                                   {1, 0});

  cr::CreativeTerrainOperationMutationRequest world =
      manualOperation(cr::CreativeTerrainOperationKind::Region);
  world.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
  world.sourceKey = "world-layout/region/1";
  const cr::CreativeTerrainOperationMutationReceipt worldAdded =
      document.applyTerrainOperationMutation(world);
  const cr::CreativeTerrainOperationTranslationPlan worldPlan =
      cr::planCreativeTerrainOperationTranslation(document,
                                                   worldAdded.operationId,
                                                   {1, 0});

  const cr::CreativeTerrainOperationMutationReceipt overflowAdded =
      document.applyTerrainOperationMutation(
          manualOperation(cr::CreativeTerrainOperationKind::Profile));
  const cr::CreativeTerrainOperationTranslationPlan overflow =
      cr::planCreativeTerrainOperationTranslation(document,
                                                   overflowAdded.operationId,
                                                   {std::numeric_limits<
                                                        std::int32_t>::max(),
                                                    0});

  cr::CreativeDocument staleDocument = makeDocument("Terrain stale", 9302U);
  const auto staleAdded = addTerrainOperation(
      staleDocument, manualOperation(cr::CreativeTerrainOperationKind::Region));
  const cr::CreativeTerrainOperationTranslationPlan stalePlan =
      cr::planCreativeTerrainOperationTranslation(staleDocument,
                                                   staleAdded.operationId,
                                                   {1, 0});
  cr::CreativeTerrainOperationMutationRequest enabled;
  enabled.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  enabled.operationId = staleAdded.operationId;
  enabled.enabled = false;
  static_cast<void>(staleDocument.applyTerrainOperationMutation(enabled));
  const std::uint64_t staleRevision = staleDocument.revision();
  const cr::CreativeTerrainOperationTranslationReceipt stale =
      cr::applyCreativeTerrainOperationTranslation(staleDocument, stalePlan);

  return expect(generated.accepted && !global.accepted &&
                    global.status ==
                        cr::CreativeRecipeTranslationStatus::UnsupportedOwner,
                "global generation is not a local transform source") &&
         expect(worldAdded.accepted && !worldPlan.accepted &&
                    worldPlan.status ==
                        cr::CreativeRecipeTranslationStatus::UnsupportedOwner,
                "World Layout terrain remains source-owned") &&
         expect(overflowAdded.accepted && !overflow.accepted &&
                    overflow.status ==
                        cr::CreativeRecipeTranslationStatus::CoordinateOverflow,
                "terrain coordinate overflow fails before replay") &&
         expect(!stale.accepted && !stale.changed &&
                    stale.status ==
                        cr::CreativeRecipeTranslationStatus::StaleSource &&
                    staleDocument.revision() == staleRevision,
                "stale terrain plan cannot publish");
}

}  // namespace

int main() {
  return patternTranslationMovesTheWholeRelationship() &&
                 scatterSourcesAndExclusionsTranslate() &&
                 patternStalenessAndDependenciesFailAtomically() &&
                 durableTerrainKindsPlanReplayAndApply() &&
                 terrainUnsupportedOverflowAndStalePlansFailClosed()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
