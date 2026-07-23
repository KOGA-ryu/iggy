#include "EditorAssetScatter.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorPattern.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"
#include "app/iggy3d/creative/tools/AssetScatter.hpp"
#include "core/math/Mat4.hpp"

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

[[nodiscard]] bool sameCandidate(
    const cr::CreativeAssetScatterCandidate& lhs,
    const cr::CreativeAssetScatterCandidate& rhs) noexcept {
  return cr::creativeVec3ExactlyEqual(lhs.position, rhs.position) &&
         lhs.yawOffsetRadians == rhs.yawOffsetRadians &&
         lhs.uniformScale == rhs.uniformScale;
}

cr::CreativeDocumentCreateRequest scatterCreateRequest(
    std::string_view name,
    cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Prop;
  request.name = std::string{name};
  request.assetId = "test/scatter_prop";
  request.assetContentHash = 17U;
  request.bounds = {position,
                    {position.x + 1.0, position.y + 1.0,
                     position.z + 1.0}};
  request.hasBoundsOverride = true;
  return request;
}

cr::CreativeAssetScatterRecipe scatterMutationRecipe() {
  cr::CreativeAssetScatterRecipe recipe;
  recipe.objectKind = cr::CreativeObjectKind::Prop;
  recipe.assetId = "test/scatter_prop";
  recipe.assetContentHash = 17U;
  recipe.assetSourceBounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  recipe.paintCenters.push_back({0.0, 0.0, 0.0});
  recipe.maxGeneratedObjects = 8U;
  return recipe;
}

bool scatterRecipeMutationsPublishOneTruthfulRevision() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Scatter Publication");
  static_cast<void>(document.assignId(8900U));
  const auto nestedRangeMatches =
      [](const cr::CreativeAssetScatterRecipeMutationReceipt& receipt) {
        return receipt.patternMutationReceipt.requested &&
               receipt.patternMutationReceipt.revisionBefore ==
                   receipt.revisionBefore &&
               receipt.patternMutationReceipt.revisionAfter ==
                   receipt.revisionAfter;
      };

  const cr::CreativeAssetScatterRecipe recipe = scatterMutationRecipe();
  const std::array createRequests{
      scatterCreateRequest("Initial A", {0.0, 0.0, 0.0}),
      scatterCreateRequest("Initial B", {2.0, 0.0, 0.0})};
  const cr::CreativeAssetScatterRecipeMutationReceipt created =
      cr::createCreativeAssetScatterRecipeAtomically(
          document, createRequests,
          std::span<const cr::CreativeObjectId>{}, recipe);
  if (!expect(created.accepted && created.generatedObjectIds.size() == 2U,
              "scatter publication create setup")) {
    return false;
  }
  const std::vector<cr::CreativeObjectId> initialIds =
      created.generatedObjectIds;
  const cr::CreativePatternRecipeId recipeId = created.patternRecipeId;
  const std::array updateRequests{
      scatterCreateRequest("Updated A", {4.0, 0.0, 0.0}),
      scatterCreateRequest("Updated B", {6.0, 0.0, 0.0})};
  const cr::CreativeAssetScatterRecipeMutationReceipt updated =
      cr::updateCreativeAssetScatterRecipeAtomically(
          document, recipeId, updateRequests, recipe);
  if (!expect(updated.accepted && updated.generatedObjectIds.size() == 2U,
              "scatter publication update setup")) {
    return false;
  }
  const bool initialOutputsRetired = std::all_of(
      initialIds.begin(), initialIds.end(),
      [&document](cr::CreativeObjectId objectId) {
        return document.findObject(objectId) == nullptr;
      });

  const cr::CreativePatternRecipe* current = cr::findCreativePatternRecipe(
      document.patternRecipeStore(), recipeId);
  if (!expect(current != nullptr, "scatter publication recipe survives update")) {
    return false;
  }
  cr::CreativeAssetScatterRecipe extendedRecipe = current->scatter;
  extendedRecipe.paintCenters.push_back({8.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateRequest extensionRequest =
      scatterCreateRequest("Extended", {8.0, 0.0, 0.0});
  const cr::CreativeAssetScatterRecipeMutationReceipt extended =
      cr::extendCreativeAssetScatterRecipeAtomically(
          document, recipeId, std::span{&extensionRequest, 1U},
          extendedRecipe);
  if (!expect(extended.accepted, "scatter publication extension setup")) {
    return false;
  }

  current =
      cr::findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (!expect(current != nullptr && current->generatedObjectIds.size() == 3U,
              "scatter publication exclusion setup")) {
    return false;
  }
  const cr::CreativeObjectId excludedObjectId =
      current->generatedObjectIds.front();
  cr::CreativeAssetScatterRecipe excludedRecipe = current->scatter;
  excludedRecipe.exclusions.push_back({{4.0, 0.0, 0.0}, 1.0});
  const cr::CreativeAssetScatterRecipeMutationReceipt excluded =
      cr::excludeCreativeAssetScatterOutputAtomically(
          document, recipeId, excludedObjectId, excludedRecipe);
  const bool excludedOutputRetired =
      document.findObject(excludedObjectId) == nullptr;
  const cr::CreativeAssetScatterRecipeMutationReceipt removed =
      cr::removeCreativeAssetScatterRecipeAtomically(document, recipeId);

  return expect(created.revisionAfter == created.revisionBefore + 1U &&
                    nestedRangeMatches(created),
                "scatter create publishes one truthful revision") &&
         expect(updated.revisionAfter == updated.revisionBefore + 1U &&
                    nestedRangeMatches(updated) &&
                    updated.replacedGeneratedObjectCount == 2U &&
                    initialOutputsRetired,
                "scatter update publishes one truthful revision") &&
         expect(extended.revisionAfter == extended.revisionBefore + 1U &&
                    nestedRangeMatches(extended),
                "scatter extension publishes one truthful revision") &&
         expect(excluded.accepted &&
                    excluded.revisionAfter == excluded.revisionBefore + 1U &&
                    nestedRangeMatches(excluded) &&
                    excluded.replacedGeneratedObjectCount == 1U &&
                    excludedOutputRetired,
                "scatter exclusion publishes one truthful revision") &&
         expect(removed.accepted &&
                    removed.revisionAfter == removed.revisionBefore + 1U &&
                    nestedRangeMatches(removed) &&
                    document.patternRecipeStore().recipes.empty() &&
                    document.objectCount() == 0U,
                "scatter removal publishes one truthful revision");
}

bool plannerIsDeterministicBoundedAndSpaced() {
  cr::CreativeAssetScatterRequest request;
  request.center = {10.0, 2.0, -5.0};
  request.radiusMeters = 8.0;
  request.spacingMeters = 1.0;
  request.densityFraction = 1.0;
  request.yaw = cr::CreativeAssetScatterYaw::Full;
  request.scaleVariation = 0.25;
  request.seed = 42U;
  const cr::CreativeAssetScatterPlan first =
      cr::planCreativeAssetScatter(request);
  const cr::CreativeAssetScatterPlan second =
      cr::planCreativeAssetScatter(request);
  bool same = first.candidateCount == second.candidateCount;
  for (std::size_t index = 0U; same && index < first.candidateCount; ++index) {
    same = sameCandidate(first.candidates[index], second.candidates[index]);
  }

  bool validCandidates = first.accepted && first.candidateCount > 1U &&
                         first.candidateCount <=
                             cr::kCreativeAssetScatterCandidateCapacity;
  for (std::size_t index = 0U;
       validCandidates && index < first.candidateCount; ++index) {
    const cr::CreativeAssetScatterCandidate& candidate =
        first.candidates[index];
    const double dx = candidate.position.x - request.center.x;
    const double dz = candidate.position.z - request.center.z;
    validCandidates = cr::isFiniteCreativeVec3(candidate.position) &&
                      dx * dx + dz * dz <= 64.0 + 1.0e-9 &&
                      candidate.yawOffsetRadians >= 0.0 &&
                      candidate.yawOffsetRadians < std::numbers::pi * 2.0 &&
                      candidate.uniformScale >= 0.75 &&
                      candidate.uniformScale <= 1.25;
    for (std::size_t other = 0U;
         validCandidates && other < index; ++other) {
      const double sx =
          candidate.position.x - first.candidates[other].position.x;
      const double sz =
          candidate.position.z - first.candidates[other].position.z;
      validCandidates = sx * sx + sz * sz >= 1.0 - 1.0e-9;
    }
  }
  std::size_t readyEvaluations = 0U;
  std::size_t densityEvaluations = 0U;
  std::size_t spacingEvaluations = 0U;
  bool validEvaluations = first.evaluationCount >= first.candidateCount;
  for (const cr::CreativeAssetScatterEvaluation& evaluation :
       first.evaluatedItems()) {
    const double dx = evaluation.candidate.position.x - request.center.x;
    const double dz = evaluation.candidate.position.z - request.center.z;
    validEvaluations = validEvaluations &&
                       cr::isFiniteCreativeVec3(evaluation.candidate.position) &&
                       dx * dx + dz * dz <= 64.0 + 1.0e-9;
    switch (evaluation.status) {
      case cr::CreativeAssetScatterEvaluationStatus::Ready:
        ++readyEvaluations;
        break;
      case cr::CreativeAssetScatterEvaluationStatus::DensityRejected:
        ++densityEvaluations;
        break;
      case cr::CreativeAssetScatterEvaluationStatus::SpacingRejected:
        ++spacingEvaluations;
        break;
      case cr::CreativeAssetScatterEvaluationStatus::CapacityRejected:
      case cr::CreativeAssetScatterEvaluationStatus::Count:
        break;
    }
  }
  validEvaluations =
      validEvaluations && readyEvaluations == first.candidateCount &&
      densityEvaluations == first.densityRejectedCount &&
      spacingEvaluations == first.spacingRejectedCount;

  cr::CreativeAssetScatterRequest sparseRequest = request;
  sparseRequest.radiusMeters = 4.0;
  sparseRequest.densityFraction = 0.35;
  const cr::CreativeAssetScatterPlan sparse =
      cr::planCreativeAssetScatter(sparseRequest);
  const std::size_t sparseDensityEvaluations = static_cast<std::size_t>(
      std::count_if(
          sparse.evaluatedItems().begin(), sparse.evaluatedItems().end(),
          [](const cr::CreativeAssetScatterEvaluation& evaluation) {
            return evaluation.status ==
                   cr::CreativeAssetScatterEvaluationStatus::DensityRejected;
          }));
  sparseRequest.densityFraction = 1.0;
  const cr::CreativeAssetScatterPlan dense =
      cr::planCreativeAssetScatter(sparseRequest);
  sparseRequest.seed = 43U;
  const cr::CreativeAssetScatterPlan otherSeed =
      cr::planCreativeAssetScatter(sparseRequest);

  return expect(same, "same scatter seed produces byte-stable candidates") &&
         expect(validCandidates,
                "scatter candidates stay finite, bounded, spaced, and tuned") &&
         expect(validEvaluations,
                "scatter planner reports every admitted and rejected grid candidate") &&
         expect(dense.candidateCount >= sparse.candidateCount,
                "density monotonically increases candidate count") &&
         expect(sparse.densityRejectedCount > 0U &&
                    sparseDensityEvaluations == sparse.densityRejectedCount,
                "density rejection positions remain available to preview") &&
         expect(otherSeed.candidateCount > 1U &&
                    !sameCandidate(dense.candidates[1],
                                   otherSeed.candidates[1]),
                "different seed changes deterministic layout");
}

bool plannerRejectsInvalidAndOversizedRequests() {
  cr::CreativeAssetScatterRequest invalid;
  invalid.center.x = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeAssetScatterPlan nonFinite =
      cr::planCreativeAssetScatter(invalid);
  invalid = {};
  invalid.radiusMeters = 64.0;
  invalid.spacingMeters = 1.0;
  const cr::CreativeAssetScatterPlan oversized =
      cr::planCreativeAssetScatter(invalid);
  invalid = {};
  invalid.maxCandidateCount =
      cr::kCreativeAssetScatterCandidateCapacity + 1U;
  const cr::CreativeAssetScatterPlan badCapacity =
      cr::planCreativeAssetScatter(invalid);
  return expect(nonFinite.status ==
                    cr::CreativeAssetScatterStatus::InvalidRequest,
                "non-finite scatter request rejected") &&
         expect(oversized.status ==
                    cr::CreativeAssetScatterStatus::CapacityExceeded,
                "oversized scatter grid rejected before iteration") &&
         expect(badCapacity.status ==
                    cr::CreativeAssetScatterStatus::InvalidRequest,
                "caller cannot exceed fixed output capacity");
}

bool terrainSurfacePoseMatchesFlatRenderedPatch() {
  cr::CreativeTerrainField field;
  const cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 4U}};
  const cr::CreativeTerrainMutationReceipt applied =
      field.apply(std::span{&edit, 1U});
  const cr::CreativeTerrainSurfacePose pose =
      cr::sampleCreativeTerrainSurfacePose(
          {&field, {0.5, 0.0, 0.5}, {}, 1.0});
  const cr::CreativeTerrainSurfacePose missing =
      cr::sampleCreativeTerrainSurfacePose(
          {&field, {100.5, 0.0, 100.5}, {}, 1.0});
  const cr::CreativeTerrainSurfacePose invalid =
      cr::sampleCreativeTerrainSurfacePose(
          {&field, {0.5, 0.0, 0.5}, {}, 0.0});
  return expect(applied.accepted && applied.changed,
                "terrain control installed") &&
         expect(pose.accepted && pose.present &&
                    pose.status == cr::CreativeTerrainSurfacePoseStatus::Ready &&
                    std::fabs(pose.position.y - 4.0) < 1.0e-12 &&
                    std::fabs(pose.normal.y - 1.0) < 1.0e-12 &&
                    std::fabs(pose.slopeRadians) < 1.0e-12,
                "surface pose matches flat four-corner terrain patch") &&
         expect(missing.accepted && !missing.present,
                "missing terrain is distinguished from invalid query") &&
         expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeTerrainSurfacePoseStatus::InvalidRequest,
                "invalid surface request fails closed");
}

[[nodiscard]] bool optionsContain(const cr::CreativeToolOptionList& options,
                                  cr::CreativeToolOptionId id) noexcept {
  for (const cr::CreativeToolOptionId option : options.items()) {
    if (option == id) {
      return true;
    }
  }
  return false;
}

app::CreativeEditorState scatterEditor() {
  app::CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  cr::CreativeHotbarEntry& held = editor.interaction.hotbar.entries[0];
  held = {cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Prop};
  static_cast<void>(cr::setCreativeHotbarAsset(
      held, "scatter_tree", {{-0.5, -0.5, -0.5}, {0.5, 1.5, 0.5}}));
  editor.toolSettings.assetPlacementMode =
      cr::CreativeAssetPlacementMode::Scatter;
  editor.toolSettings.assetScatterRadius =
      cr::CreativeAssetScatterRadius::FourCells;
  editor.toolSettings.assetScatterSpacing =
      cr::CreativeAssetScatterSpacing::TwoCells;
  editor.toolSettings.assetScatterDensity =
      cr::CreativeAssetScatterDensity::Dense;
  editor.toolSettings.assetScatterYaw = cr::CreativeAssetScatterYaw::Full;
  editor.placeCellSize = 1.0;
  editor.interaction.target.valid = true;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.faceNormal = {0.0, 1.0, 0.0};
  editor.interaction.target.grid.placerForward = {0.0, 0.0, -1.0};
  editor.interaction.target.grid.placementAnchor = {0.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  return editor;
}

[[nodiscard]] bool sameHorizontalPosition(cr::CreativeVec3 lhs,
                                          cr::CreativeVec3 rhs) noexcept {
  return std::fabs(lhs.x - rhs.x) < 1.0e-9 &&
         std::fabs(lhs.z - rhs.z) < 1.0e-9;
}

bool recipePlannerHonorsMasksAndExclusions() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter Masks");
  static_cast<void>(document.assignId(9010U));
  app::CreativeEditorState editor = scatterEditor();
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  cr::CreativeAssetScatterRecipe circleRecipe =
      app::makeCreativeEditorAssetScatterRecipe(editor, held);
  circleRecipe.avoidCollisions = false;
  circleRecipe.mask = cr::CreativeAssetScatterRecipeMask::Circle;
  const app::CreativeEditorAssetScatterPlan circle =
      app::buildCreativeEditorAssetScatterRecipePlan(document, editor,
                                                     circleRecipe);
  cr::CreativeAssetScatterRecipe boxRecipe = circleRecipe;
  boxRecipe.mask = cr::CreativeAssetScatterRecipeMask::Box;
  const app::CreativeEditorAssetScatterPlan box =
      app::buildCreativeEditorAssetScatterRecipePlan(document, editor,
                                                     boxRecipe);
  const cr::CreativeVec3 center = circleRecipe.paintCenters.front();
  bool circleInside = true;
  for (const app::CreativeEditorAssetScatterCandidate& candidate :
       circle.items()) {
    if (!candidate.placeable) {
      continue;
    }
    const double dx = candidate.surfacePosition.x - center.x;
    const double dz = candidate.surfacePosition.z - center.z;
    circleInside = circleInside &&
                   dx * dx + dz * dz <=
                       circleRecipe.radiusMeters * circleRecipe.radiusMeters +
                           1.0e-9;
  }
  bool boxHasCorner = false;
  bool boxInside = true;
  for (const app::CreativeEditorAssetScatterCandidate& candidate : box.items()) {
    if (!candidate.placeable) {
      continue;
    }
    const double dx = std::fabs(candidate.surfacePosition.x - center.x);
    const double dz = std::fabs(candidate.surfacePosition.z - center.z);
    boxInside = boxInside && dx <= boxRecipe.radiusMeters + 1.0e-9 &&
                dz <= boxRecipe.radiusMeters + 1.0e-9;
    boxHasCorner = boxHasCorner ||
                   dx * dx + dz * dz >
                       boxRecipe.radiusMeters * boxRecipe.radiusMeters +
                           1.0e-9;
  }

  const auto excludedSource = std::find_if(
      circle.items().begin(), circle.items().end(),
      [](const app::CreativeEditorAssetScatterCandidate& candidate) {
        return candidate.placeable;
      });
  cr::CreativeAssetScatterRecipe excludedRecipe = circleRecipe;
  if (excludedSource != circle.items().end()) {
    excludedRecipe.exclusions.push_back(
        {excludedSource->surfacePosition, 0.25});
  }
  const app::CreativeEditorAssetScatterPlan excluded =
      app::buildCreativeEditorAssetScatterRecipePlan(document, editor,
                                                     excludedRecipe);
  const bool targetExcluded =
      excludedSource != circle.items().end() &&
      std::any_of(
          excluded.items().begin(), excluded.items().end(),
          [&excludedSource](
              const app::CreativeEditorAssetScatterCandidate& candidate) {
            return sameHorizontalPosition(candidate.surfacePosition,
                                          excludedSource->surfacePosition) &&
                   candidate.status ==
                       app::CreativeEditorAssetScatterCandidateStatus::Excluded;
          });

  cr::CreativeDocumentCreateRequest selectionRequest;
  selectionRequest.kind = cr::CreativeObjectKind::Floor;
  selectionRequest.name = "Scatter Selection Mask";
  selectionRequest.hasBoundsOverride = true;
  selectionRequest.bounds = {{-1.0, -0.2, -1.0}, {2.0, 0.0, 2.0}};
  const cr::CreativeDocumentCreateReceipt selectionCreated =
      document.createObject(selectionRequest);
  cr::CreativeAssetScatterRecipe selectionRecipe = circleRecipe;
  selectionRecipe.mask = cr::CreativeAssetScatterRecipeMask::Selection;
  const app::CreativeEditorAssetScatterPlan missingSelection =
      app::buildCreativeEditorAssetScatterRecipePlan(document, editor,
                                                     selectionRecipe);
  const std::array selectionIds{selectionCreated.objectId};
  const app::CreativeEditorAssetScatterPlan selection =
      app::buildCreativeEditorAssetScatterRecipePlan(
          document, editor, selectionRecipe, selectionIds, selectionIds);
  bool selectionInside = selection.placeableCount > 0U;
  bool selectionRejectedOutside = false;
  for (const app::CreativeEditorAssetScatterCandidate& candidate :
       selection.items()) {
    if (candidate.placeable) {
      selectionInside =
          selectionInside && candidate.surfacePosition.x >= -1.0 - 1.0e-9 &&
          candidate.surfacePosition.x <= 2.0 + 1.0e-9 &&
          candidate.surfacePosition.z >= -1.0 - 1.0e-9 &&
          candidate.surfacePosition.z <= 2.0 + 1.0e-9;
    }
    selectionRejectedOutside =
        selectionRejectedOutside ||
        candidate.status ==
            app::CreativeEditorAssetScatterCandidateStatus::OutsideMask;
  }

  return expect(circle.accepted && box.accepted && circleInside && boxInside &&
                    boxHasCorner && box.placeableCount > circle.placeableCount,
                "circle and box masks produce distinct bounded footprints") &&
         expect(targetExcluded &&
                    excluded.placeableCount < circle.placeableCount,
                "exclusion masks reject their deterministic candidate") &&
         expect(selectionCreated.accepted &&
                    missingSelection.kernelStatus ==
                        cr::CreativeAssetScatterStatus::InvalidRequest &&
                    selection.accepted && selectionInside &&
                    selectionRejectedOutside,
                "selection mask requires and clips to its durable source ids");
}

bool recipePlannerHonorsCollisionPolicy() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter Collision");
  static_cast<void>(document.assignId(9011U));
  app::CreativeEditorState editor = scatterEditor();
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  cr::CreativeAssetScatterRecipe recipe =
      app::makeCreativeEditorAssetScatterRecipe(editor, held);
  recipe.avoidCollisions = false;
  const app::CreativeEditorAssetScatterPlan unobstructed =
      app::buildCreativeEditorAssetScatterRecipePlan(document, editor, recipe);
  const auto source = std::find_if(
      unobstructed.items().begin(), unobstructed.items().end(),
      [](const app::CreativeEditorAssetScatterCandidate& candidate) {
        return candidate.placeable;
      });
  if (!expect(source != unobstructed.items().end(),
              "collision policy fixture has a placeable candidate")) {
    return false;
  }
  const cr::CreativeDocumentCreateRequest blocker = app::buildBrushCreateRequest(
      source->placement, 1U, "scatter_policy_blocker");
  const cr::CreativeDocumentCreateReceipt blockerCreated =
      document.createObject(blocker);
  recipe.avoidCollisions = true;
  const app::CreativeEditorAssetScatterPlan avoid =
      app::buildCreativeEditorAssetScatterRecipePlan(document, editor, recipe);
  recipe.avoidCollisions = false;
  const app::CreativeEditorAssetScatterPlan allow =
      app::buildCreativeEditorAssetScatterRecipePlan(document, editor, recipe);
  const auto statusAtSource = [&source](
                                  const app::CreativeEditorAssetScatterPlan& plan) {
    const auto found = std::find_if(
        plan.items().begin(), plan.items().end(),
        [&source](const app::CreativeEditorAssetScatterCandidate& candidate) {
          return sameHorizontalPosition(candidate.surfacePosition,
                                        source->surfacePosition);
        });
    return found != plan.items().end()
               ? found->status
               : app::CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
  };

  return expect(blockerCreated.accepted &&
                    statusAtSource(avoid) ==
                        app::CreativeEditorAssetScatterCandidateStatus::Obstructed,
                "avoid policy rejects a candidate with blocker provenance") &&
         expect(statusAtSource(allow) ==
                    app::CreativeEditorAssetScatterCandidateStatus::Ready,
                "allow policy admits the same overlapping candidate");
}

bool assetOnlyToolOptionsAreContextual() {
  cr::CreativeToolSettings settings;
  const cr::CreativeHotbarEntry ordinary{
      cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Prop};
  cr::CreativeHotbarEntry asset = ordinary;
  const bool assetSet = cr::setCreativeHotbarAsset(
      asset, "tree", {{-0.5, 0.0, -0.5}, {0.5, 2.0, 0.5}});
  const cr::CreativeToolOptionList ordinaryOptions =
      app::creativeEditorToolOptionsForEntry(ordinary, settings);
  const cr::CreativeToolOptionList singleOptions =
      app::creativeEditorToolOptionsForEntry(asset, settings);
  cr::CreativeToolSettings alignmentSettings = settings;
  const cr::CreativeToolOptionAdjustReceipt alignmentAdjusted =
      cr::adjustCreativeToolOption(
          alignmentSettings, cr::CreativeToolOptionId::AssetAlignmentMode, 1);
  cr::CreativeToolSettings invalidAlignment = alignmentSettings;
  invalidAlignment.assetAlignmentMode = cr::CreativeAssetAlignmentMode::Count;
  settings.assetPlacementMode = cr::CreativeAssetPlacementMode::Scatter;
  settings.placementAnchor = cr::CreativePlacementAnchor::Corner;
  const cr::CreativeToolOptionList scatterOptions =
      app::creativeEditorToolOptionsForEntry(asset, settings);
  cr::CreativeAppState appState;
  app::CreativeEditorState scatterState = scatterEditor();
  scatterState.toolSettings.placementAnchor =
      cr::CreativePlacementAnchor::Corner;
  const cr::CreativePlacementGridFrame scatterGrid =
      app::creativeEditorPlacementGridFrame(appState.facade.document(),
                                            scatterState);
  app::CreativeEditorState editor;
  editor.interaction.hotbar.entries[0] = ordinary;
  app::syncCreativeEditorQuickEdit(editor);
  editor.quickEdit.selectedIndex = 1U;
  editor.interaction.hotbar.entries[0] = asset;
  app::syncCreativeEditorQuickEdit(editor);

  return expect(assetSet, "test imported asset is valid") &&
         expect(!optionsContain(ordinaryOptions,
                                cr::CreativeToolOptionId::AssetPlacementMode),
                "ordinary materials do not expose asset controls") &&
         expect(optionsContain(singleOptions,
                               cr::CreativeToolOptionId::AssetPlacementMode) &&
                    optionsContain(singleOptions,
                                   cr::CreativeToolOptionId::AssetAlignmentMode) &&
                    optionsContain(singleOptions,
                                   cr::CreativeToolOptionId::PlacementYaw) &&
                    optionsContain(singleOptions,
                                   cr::CreativeToolOptionId::PlacementAnchor) &&
                    !optionsContain(singleOptions,
                                    cr::CreativeToolOptionId::AssetScatterRadius) &&
                    singleOptions.count == 9U &&
                    !singleOptions.capacityExceeded,
                "single asset mode exposes alignment and every normal placement control") &&
         expect(scatterOptions.count == 9U &&
                    optionsContain(scatterOptions,
                                   cr::CreativeToolOptionId::AssetScatterMask) &&
                    optionsContain(scatterOptions,
                                   cr::CreativeToolOptionId::AssetScatterSlope) &&
                    optionsContain(
                        scatterOptions,
                        cr::CreativeToolOptionId::AssetScatterCollision) &&
                    !optionsContain(scatterOptions,
                                    cr::CreativeToolOptionId::PlacementYaw) &&
                    !optionsContain(scatterOptions,
                                    cr::CreativeToolOptionId::PlacementAnchor) &&
                    !optionsContain(scatterOptions,
                                    cr::CreativeToolOptionId::AssetAlignmentMode) &&
                    !scatterOptions.capacityExceeded,
                "scatter mode exposes nine bounded quick-edit settings") &&
         expect(alignmentAdjusted.accepted && alignmentAdjusted.changed &&
                    alignmentSettings.assetAlignmentMode ==
                        cr::CreativeAssetAlignmentMode::Floor &&
                    cr::creativeToolOptionValueLabel(
                        alignmentSettings,
                        cr::CreativeToolOptionId::AssetAlignmentMode) ==
                        "FLOOR" &&
                    !cr::isValidCreativeToolSettings(invalidAlignment),
                "alignment is a generic cyclic tool option") &&
         expect(scatterGrid.valid &&
                    scatterGrid.anchorKind ==
                        cr::CreativePlacementAnchorKind::BaseCenter,
                "scatter remains center-based when single placement retains another anchor mode") &&
         expect(editor.quickEdit.selectedIndex == 0U &&
                    editor.quickEdit.options.ids[0] ==
                        cr::CreativeToolOptionId::AssetPlacementMode,
                "asset identity change resets quick edit to mode row");
}

bool terrainScatterRejectsSteepAndMissingSurface() {
  cr::CreativeAppState appState;
  const std::array edits{
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 2U, 1U}},
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{1, 0}, 6U, 1U}},
  };
  const cr::CreativeTerrainMutationReceipt applied =
      appState.facade.applyTerrainControlEdits(edits);
  app::CreativeEditorState editor = scatterEditor();
  editor.interaction.target.terrainHit = true;
  editor.toolSettings.assetScatterSlope =
      cr::CreativeAssetScatterSlope::Degrees15;
  const cr::CreativeTerrainSurfacePose centerPose =
      cr::sampleCreativeTerrainSurfacePose(
          {&appState.facade.document().terrainField(), {0.5, 0.0, 0.5},
           appState.facade.document().gridSettings().origin,
           appState.facade.document().gridSettings().cellSizeMeters});
  editor.interaction.target.grid.placementAnchor = centerPose.position;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const app::CreativeEditorAssetScatterPlan plan =
      app::buildCreativeEditorAssetScatterPlan(
          appState.facade.document(), editor, held);
  bool rejectedSurface = false;
  for (const app::CreativeEditorAssetScatterCandidate& candidate :
       plan.items()) {
    rejectedSurface =
        rejectedSurface ||
        candidate.status ==
            app::CreativeEditorAssetScatterCandidateStatus::SlopeRejected ||
        candidate.status ==
            app::CreativeEditorAssetScatterCandidateStatus::MissingSurface;
  }
  return expect(applied.accepted && centerPose.present &&
                    centerPose.slopeRadians >
                        cr::creativeAssetScatterMaximumSlopeRadians(
                            cr::CreativeAssetScatterSlope::Degrees15),
                "test terrain contains a slope above the configured limit") &&
         expect(plan.accepted && rejectedSurface,
                "terrain scatter marks steep or absent candidates invalid");
}

bool previewIsTransientAndSolidGhostIsSuppressed() {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor = scatterEditor();
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const app::CreativeEditorAssetScatterPlan plan =
      app::buildCreativeEditorAssetScatterPlan(
          appState.facade.document(), editor, held);
  editor.interaction.assetScatter.preview = plan;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t edges = app::appendCreativeEditorAssetScatterWireframes(
      editor, 0.05F, lines);
  const bool kernelRejectionVisible = std::any_of(
      plan.rejectedItems().begin(), plan.rejectedItems().end(),
      [](const app::CreativeEditorAssetScatterRejectedCandidate& candidate) {
        return candidate.valid &&
               (candidate.status ==
                    app::CreativeEditorAssetScatterCandidateStatus::
                        DensityRejected ||
                candidate.status ==
                    app::CreativeEditorAssetScatterCandidateStatus::
                        SpacingRejected);
      });
  bool grounded = true;
  for (const app::CreativeEditorAssetScatterCandidate& candidate :
       plan.items()) {
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeTransformedBounds(candidate.placement.authoredBounds,
                                             candidate.placement.transform);
    grounded = grounded && bounds.valid &&
               std::fabs(bounds.worldBounds.min.y -
                         candidate.surfacePosition.y) < 1.0e-9;
  }
  iggy3d::FrameInput frame;
  frame.camera.clipFromWorld = iggy3d::identityMat4();
  frame.camera.clipFromView = iggy3d::identityMat4();
  app::attachCreativeEditorPlacementPreviews(
      editor, false, frame, &appState.facade.document());

  return expect(plan.accepted && plan.placeableCount > 0U,
                "asset scatter preview plan is admitted") &&
         expect(plan.rejectedCandidateCount > 0U && kernelRejectionVisible &&
                    edges == (plan.candidateCount +
                              plan.rejectedCandidateCount) *
                                 12U &&
                    lines.size() == edges,
                "every accepted and rejected scatter candidate has one exact bounds wireframe") &&
         expect(grounded,
                "random scale preserves imported asset ground alignment") &&
         expect(frame.creativePreview.itemCount == 1U &&
                    frame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::Held,
                "scatter keeps held asset and suppresses single target ghost") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    appState.facade.document().objectCount() == 0U,
                "scatter aiming is transient");
}

void setSecondary(cr::CreativeWorldActionFrame& actions,
                  bool down,
                  bool pressed,
                  bool released) {
  const std::size_t index =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Secondary);
  actions.down[index] = down;
  actions.pressed[index] = pressed;
  actions.released[index] = released;
}

void setPrimary(cr::CreativeWorldActionFrame& actions,
                bool down,
                bool pressed,
                bool released) {
  const std::size_t index =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Primary);
  actions.down[index] = down;
  actions.pressed[index] = pressed;
  actions.released[index] = released;
}

bool gestureIsAtomicDeduplicatedAndOneUndoStep() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter");
  static_cast<void>(document.assignId(9001U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "scatter document installed")) {
    return false;
  }
  app::CreativeEditorState editor = scatterEditor();
  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, press, 0U);
  const std::size_t placed = appState.facade.document().objectCount();
  const cr::CreativePatternRecipeStore& storeAfterPress =
      appState.facade.document().patternRecipeStore();
  const cr::CreativePatternRecipeId recipeId =
      storeAfterPress.recipes.empty()
          ? cr::kInvalidCreativePatternRecipeId
          : storeAfterPress.recipes.front().id;
  const std::vector<cr::CreativeObjectId> initialOutputIds =
      storeAfterPress.recipes.empty()
          ? std::vector<cr::CreativeObjectId>{}
          : storeAfterPress.recipes.front().generatedObjectIds;
  const bool initialRecipeValid =
      storeAfterPress.recipes.size() == 1U &&
      storeAfterPress.recipes.front().kind ==
          cr::CreativePatternRecipeKind::AssetScatter &&
      storeAfterPress.recipes.front().scatter.paintCenters.size() == 1U &&
      storeAfterPress.recipes.front().generatedObjectIds.size() == placed;
  const std::uint64_t revisionAfterPress =
      appState.facade.document().revision();

  cr::CreativeWorldActionFrame beforeRepeat;
  setSecondary(beforeRepeat, true, false, false);
  app::processCreativeAssetScatterFrame(
      appState, editor, beforeRepeat, 199'000'000ULL);
  const bool beforeStable =
      appState.facade.document().revision() == revisionAfterPress;
  app::processCreativeAssetScatterFrame(
      appState, editor, beforeRepeat, 200'000'000ULL);
  const bool stationaryStable =
      appState.facade.document().objectCount() == placed &&
      appState.facade.document().revision() == revisionAfterPress;

  editor.interaction.target.grid.placementAnchor = {10.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{10.0, 0.0, 0.0}, {11.0, 1.0, 1.0}};
  app::processCreativeAssetScatterFrame(
      appState, editor, beforeRepeat, 400'000'000ULL);
  const cr::CreativePatternRecipeStore& storeAfterMove =
      appState.facade.document().patternRecipeStore();
  const cr::CreativePatternRecipe* movedRecipe =
      cr::findCreativePatternRecipe(storeAfterMove, recipeId);
  const std::size_t movedObjectCount =
      appState.facade.document().objectCount();
  const std::uint64_t revisionAfterMove =
      appState.facade.document().revision();
  const bool movedRecipeValid =
      storeAfterMove.recipes.size() == 1U && movedRecipe != nullptr &&
      movedRecipe->scatter.paintCenters.size() == 2U &&
      movedRecipe->generatedObjectIds.size() == movedObjectCount &&
      movedObjectCount > placed &&
      std::all_of(initialOutputIds.begin(), initialOutputIds.end(),
                  [&appState, movedRecipe](cr::CreativeObjectId objectId) {
                    return appState.facade.findObject(objectId) != nullptr &&
                           std::find(movedRecipe->generatedObjectIds.begin(),
                                     movedRecipe->generatedObjectIds.end(),
                                     objectId) !=
                               movedRecipe->generatedObjectIds.end();
                  });
  const std::uint64_t movedRecipeFingerprint =
      movedRecipe != nullptr
          ? cr::fingerprintCreativePatternRecipeSource(*movedRecipe)
          : 0U;
  app::processCreativeAssetScatterFrame(
      appState, editor, beforeRepeat, 600'000'000ULL);
  const bool movedCenterDeduplicated =
      appState.facade.document().revision() == revisionAfterMove &&
      appState.facade.document().objectCount() == movedObjectCount;

  cr::CreativeWorldActionFrame release;
  setSecondary(release, false, false, true);
  app::processCreativeAssetScatterFrame(
      appState, editor, release, 601'000'000ULL);
  const std::size_t undoDepthBefore = cr::creativeUndoDepth(appState.history);
  const cr::CreativeAuthoringOperationRecord* operation =
      cr::creativeHistoryTargetOperation(
          appState.history, cr::CreativeHistoryDirection::Undo);
  const std::optional<cr::CreativeAuthoringOperationRecord> expectedOperation =
      operation != nullptr
          ? std::optional<cr::CreativeAuthoringOperationRecord>{*operation}
          : std::nullopt;
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(placed > 1U && initialRecipeValid,
                "first press atomically creates one scatter recipe and its outputs") &&
         expect(beforeStable,
                "repeat kernel does not mutate before 200 milliseconds") &&
         expect(stationaryStable,
                "stationary repeat cannot stack duplicate assets") &&
         expect(movedRecipeValid &&
                    editor.placedCount == movedObjectCount,
                "moving the held stroke appends locally without rebuilding prior outputs") &&
         expect(movedCenterDeduplicated,
                "a revisited paint center cannot rebuild the recipe") &&
         expect(expectedOperation.has_value() &&
                    movedRecipeFingerprint != 0U &&
                    expectedOperation->family ==
                        cr::CreativeAuthoringFamily::AssetScatter &&
                    expectedOperation->kind ==
                        cr::CreativeAuthoringOperationKind::Apply &&
                    expectedOperation->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Parametric &&
                    expectedOperation->action == "AssetScatter.Paint" &&
                    expectedOperation->requestFingerprint ==
                        movedRecipeFingerprint &&
                    expectedOperation->affectedMemberCount ==
                        movedObjectCount,
                "scatter stroke records its final durable recipe source") &&
         expect(undoDepthBefore == 1U &&
                    undo.accepted && undo.objectCountAfter == 0U &&
                    undo.targetOperation == expectedOperation &&
                    appState.facade.document().patternRecipeStore().recipes.empty(),
                "release records one undo step for recipe and outputs together");
}

bool activeStrokeCachesOneDirtyRegionPreview() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Scatter Preview Cache");
  static_cast<void>(document.assignId(9006U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "scatter preview cache document installed")) {
    return false;
  }
  app::CreativeEditorState editor = scatterEditor();
  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, press, 0U);
  if (!expect(editor.interaction.assetScatter.recipeActive &&
                  editor.interaction.assetScatter.previewBuildCount == 1U,
              "first scatter publication performs one preview plan")) {
    return false;
  }

  editor.interaction.target.grid.placementAnchor = {12.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{12.0, 0.0, 0.0}, {13.0, 1.0, 1.0}};
  cr::CreativeWorldActionFrame held;
  setSecondary(held, true, false, false);
  app::processCreativeAssetScatterFrame(
      appState, editor, held, 100'000'000ULL);
  const std::uint64_t revisionBeforeIdle =
      appState.facade.document().revision();
  const std::size_t objectCountBeforeIdle =
      appState.facade.document().objectCount();
  const std::uint64_t previewBuildsBeforeIdle =
      editor.interaction.assetScatter.previewBuildCount;
  const std::size_t pendingCandidateCount =
      editor.interaction.assetScatter.preview.candidateCount;

  for (std::size_t frame = 0U; frame < 300U; ++frame) {
    app::processCreativeAssetScatterFrame(
        appState, editor, held, 100'000'000ULL);
  }

  return expect(pendingCandidateCount > 0U &&
                    pendingCandidateCount <=
                        cr::kCreativeAssetScatterCandidateCapacity &&
                    previewBuildsBeforeIdle == 2U,
                "active scatter previews only one pending paint region") &&
         expect(editor.interaction.assetScatter.previewBuildCount ==
                        previewBuildsBeforeIdle &&
                    appState.facade.document().revision() ==
                        revisionBeforeIdle &&
                    appState.facade.document().objectCount() ==
                        objectCountBeforeIdle,
                "three hundred idle frames reuse the pending-region preview");
}

bool generatedEraseAddsExclusionAndKeepsRecipeEditable() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter Erase");
  static_cast<void>(document.assignId(9004U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "scatter erase document installed")) {
    return false;
  }
  app::CreativeEditorState editor = scatterEditor();
  cr::CreativeWorldActionFrame place;
  setSecondary(place, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, place, 0U);
  cr::CreativeWorldActionFrame placeRelease;
  setSecondary(placeRelease, false, false, true);
  app::processCreativeAssetScatterFrame(appState, editor, placeRelease, 1U);

  const cr::CreativePatternRecipeStore& initialStore =
      appState.facade.document().patternRecipeStore();
  if (!expect(initialStore.recipes.size() == 1U &&
                  initialStore.recipes.front().generatedObjectIds.size() > 1U,
              "scatter erase fixture owns multiple generated outputs")) {
    return false;
  }
  const cr::CreativePatternRecipeId recipeId = initialStore.recipes.front().id;
  const std::vector<cr::CreativeObjectId> initialOutputs =
      initialStore.recipes.front().generatedObjectIds;
  const cr::CreativeObjectId erasedObjectId = initialOutputs.front();
  static_cast<void>(appState.facade.selectTargets(
      std::span{&erasedObjectId, 1U}, erasedObjectId));
  editor.interaction.target.objectHit = true;
  editor.interaction.target.voxelHit = false;
  editor.interaction.target.objectId = erasedObjectId;
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Prop;

  cr::CreativeWorldActionFrame erase;
  setPrimary(erase, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, erase,
                                        1'000'000'000ULL);
  const cr::CreativePatternRecipe* edited = cr::findCreativePatternRecipe(
      appState.facade.document().patternRecipeStore(), recipeId);
  const std::size_t editedObjectCount =
      appState.facade.document().objectCount();
  const bool editedRecipeValid =
      edited != nullptr && edited->scatter.exclusions.size() == 1U &&
      edited->generatedObjectIds.size() == editedObjectCount &&
      editedObjectCount < initialOutputs.size();
  const bool onlyErasedOutputRetired =
      appState.facade.findObject(erasedObjectId) == nullptr &&
      std::all_of(initialOutputs.begin() + 1, initialOutputs.end(),
                  [&appState](cr::CreativeObjectId objectId) {
                    return appState.facade.findObject(objectId) != nullptr;
                  });
  std::vector<cr::CreativeObjectId> expectedSurvivors{initialOutputs.begin() + 1,
                                                      initialOutputs.end()};
  const bool survivorIdsStable =
      edited != nullptr && edited->generatedObjectIds == expectedSurvivors;
  const bool staleSelectionCleared =
      appState.facade.selectionState().selectedTarget.value == cr::kInvalidId;
  cr::CreativeWorldActionFrame eraseRelease;
  setPrimary(eraseRelease, false, false, true);
  app::processCreativeAssetScatterFrame(appState, editor, eraseRelease,
                                        1'000'000'001ULL);
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(appState.history);
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativePatternRecipe* restored = cr::findCreativePatternRecipe(
      appState.facade.document().patternRecipeStore(), recipeId);

  return expect(editedRecipeValid,
                "erasing one generated item adds an exclusion to the same recipe") &&
         expect(onlyErasedOutputRetired && survivorIdsStable &&
                    staleSelectionCleared,
                "local exclusion preserves survivor ids and clears only stale selection") &&
         expect(undoDepthBefore == 2U && undo.accepted &&
                    restored != nullptr && restored->scatter.exclusions.empty() &&
                    restored->generatedObjectIds == initialOutputs,
                "one undo restores the pre-erase recipe and outputs exactly");
}

bool erasingLastGeneratedItemRemovesRecipeAtomically() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter Last");
  static_cast<void>(document.assignId(9005U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "single-output scatter document installed")) {
    return false;
  }
  app::CreativeEditorState editor = scatterEditor();
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  cr::CreativeAssetScatterRecipe recipe =
      app::makeCreativeEditorAssetScatterRecipe(editor, held);
  recipe.radiusMeters = 0.1;
  recipe.spacingMeters = 2.0;
  recipe.maxGeneratedObjects = 1U;
  const app::CreativeEditorAssetScatterPlan plan =
      app::buildCreativeEditorAssetScatterRecipePlan(
          appState.facade.document(), editor, recipe);
  const auto candidate = std::find_if(
      plan.items().begin(), plan.items().end(),
      [](const app::CreativeEditorAssetScatterCandidate& item) {
        return item.placeable;
      });
  if (!expect(candidate != plan.items().end(),
              "single-output scatter candidate planned")) {
    return false;
  }
  const cr::CreativeDocumentCreateRequest request = app::buildBrushCreateRequest(
      candidate->placement, 1U, recipe.assetId, recipe.assetContentHash,
      recipe.assetMaterialVariant);
  const cr::CreativeAssetScatterRecipeMutationReceipt created =
      appState.facade.createAssetScatterRecipe(
          std::span{&request, 1U}, std::span<const cr::CreativeObjectId>{},
          recipe);
  if (!expect(created.accepted && created.generatedObjectIds.size() == 1U,
              "single-output scatter recipe created")) {
    return false;
  }
  const cr::CreativeObjectId outputId = created.generatedObjectIds.front();
  editor.interaction.target.objectHit = true;
  editor.interaction.target.voxelHit = false;
  editor.interaction.target.objectId = outputId;
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Prop;
  cr::CreativeWorldActionFrame erase;
  setPrimary(erase, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, erase,
                                        2'000'000'000ULL);

  return expect(appState.facade.document().objectCount() == 0U &&
                    appState.facade.document()
                        .patternRecipeStore()
                        .recipes.empty(),
                "erasing the final output removes recipe and owned object together");
}

bool selectedScatterRegeneratesAndBakesWithHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Scatter Edit");
  static_cast<void>(document.assignId(9006U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "scatter edit document installed")) {
    return false;
  }
  app::CreativeEditorState editor = scatterEditor();
  cr::CreativeWorldActionFrame place;
  setSecondary(place, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, place, 0U);
  cr::CreativeWorldActionFrame release;
  setSecondary(release, false, false, true);
  app::processCreativeAssetScatterFrame(appState, editor, release, 1U);
  const cr::CreativePatternRecipeStore& initialStore =
      appState.facade.document().patternRecipeStore();
  if (!expect(initialStore.recipes.size() == 1U,
              "scatter edit fixture owns one recipe")) {
    return false;
  }
  const cr::CreativePatternRecipeId recipeId = initialStore.recipes.front().id;
  const std::uint64_t initialSeed = initialStore.recipes.front().scatter.seed;
  const std::vector<cr::CreativeObjectId> initialOutputs =
      initialStore.recipes.front().generatedObjectIds;
  const cr::CreativeObjectId selectedOutput = initialOutputs.back();
  static_cast<void>(appState.facade.selectTargets(
      std::span{&selectedOutput, 1U}, selectedOutput));
  const cr::CreativePatternRecipe* selectedRecipe =
      app::creativeEditorSelectedPatternRecipe(appState);
  const bool selectedRecipeResolved =
      selectedRecipe != nullptr && selectedRecipe->id == recipeId;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const app::CreativeEditorToolOptionsCommandList commands =
      app::creativeEditorToolOptionCommandsForEntry(
          held, cr::CreativeObjectKind::Prop, recipeId,
          cr::CreativePatternRecipeKind::AssetScatter);
  const bool regenerateCommand = std::find(
                                     commands.ids.begin(),
                                     commands.ids.begin() + commands.count,
                                     app::CreativeEditorToolOptionsCommandId::
                                         RegeneratePatternRecipe) !=
                                 commands.ids.begin() + commands.count;
  const bool bakeCommand =
      std::find(commands.ids.begin(), commands.ids.begin() + commands.count,
                app::CreativeEditorToolOptionsCommandId::DetachPatternRecipe) !=
      commands.ids.begin() + commands.count;

  const cr::CreativeAssetScatterRecipeMutationReceipt regenerated =
      app::regenerateCreativeEditorAssetScatterRecipeWithHistory(
          appState, editor, recipeId, nullptr, "test_regenerate_scatter");
  const cr::CreativePatternRecipe* changed = cr::findCreativePatternRecipe(
      appState.facade.document().patternRecipeStore(), recipeId);
  const std::vector<cr::CreativeObjectId> regeneratedOutputs =
      changed != nullptr ? changed->generatedObjectIds
                         : std::vector<cr::CreativeObjectId>{};
  const bool oldOutputsRetired = std::none_of(
      initialOutputs.begin(), initialOutputs.end(),
      [&appState](cr::CreativeObjectId objectId) {
        return appState.facade.findObject(objectId) != nullptr;
      });
  const cr::TargetRef regeneratedSelection =
      appState.facade.selectionState().selectedTarget;
  const bool selectedRegeneratedOutput =
      regeneratedSelection.value != cr::kInvalidId &&
      std::find(regeneratedOutputs.begin(), regeneratedOutputs.end(),
                static_cast<cr::CreativeObjectId>(
                    regeneratedSelection.value)) != regeneratedOutputs.end();
  const bool regenerationApplied =
      regenerated.accepted && regenerated.updatedExistingRecipe &&
      changed != nullptr && changed->id == recipeId &&
      changed->scatter.seed != initialSeed && oldOutputsRetired &&
      selectedRegeneratedOutput;
  const std::uint64_t changedFingerprint =
      changed != nullptr
          ? cr::fingerprintCreativePatternRecipeSource(*changed)
          : 0U;
  const cr::CreativeAuthoringOperationRecord* regenerateOperation =
      cr::creativeHistoryTargetOperation(
          appState.history, cr::CreativeHistoryDirection::Undo);
  const std::optional<cr::CreativeAuthoringOperationRecord>
      expectedRegenerateOperation =
          regenerateOperation != nullptr
              ? std::optional<cr::CreativeAuthoringOperationRecord>{
                    *regenerateOperation}
              : std::nullopt;
  const cr::CreativeHistoryApplyReceipt undoRegenerate =
      cr::applyCreativeHistory(appState.facade, appState.history,
                               cr::CreativeHistoryDirection::Undo);
  const cr::CreativePatternRecipe* restored = cr::findCreativePatternRecipe(
      appState.facade.document().patternRecipeStore(), recipeId);
  const bool regenerateUndoRestored =
      undoRegenerate.accepted && restored != nullptr &&
      restored->scatter.seed == initialSeed &&
      restored->generatedObjectIds == initialOutputs;

  const std::size_t objectCountBeforeBake =
      appState.facade.document().objectCount();
  const cr::CreativePatternRecipeMutationReceipt baked =
      app::detachCreativeEditorPatternRecipeWithHistory(
          appState, recipeId, "test_bake_scatter_instances");
  const cr::CreativeAuthoringOperationRecord* detachOperation =
      cr::creativeHistoryTargetOperation(
          appState.history, cr::CreativeHistoryDirection::Undo);
  const std::optional<cr::CreativeAuthoringOperationRecord>
      expectedDetachOperation =
          detachOperation != nullptr
              ? std::optional<cr::CreativeAuthoringOperationRecord>{
                    *detachOperation}
              : std::nullopt;
  const bool bakedInstancesRemain =
      baked.accepted && baked.changed &&
      appState.facade.document().patternRecipeStore().recipes.empty() &&
      appState.facade.document().objectCount() == objectCountBeforeBake;
  const cr::CreativeHistoryApplyReceipt undoBake = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(selectedRecipeResolved && regenerateCommand && bakeCommand,
                "selected scatter output resolves regenerate and bake commands") &&
         expect(regenerationApplied,
                "regenerate keeps recipe identity and selects a replacement output") &&
         expect(expectedRegenerateOperation.has_value() &&
                    expectedRegenerateOperation->family ==
                        cr::CreativeAuthoringFamily::AssetScatter &&
                    expectedRegenerateOperation->kind ==
                        cr::CreativeAuthoringOperationKind::Reconcile &&
                    expectedRegenerateOperation->action ==
                        "AssetScatter.Regenerate" &&
                    expectedRegenerateOperation->requestFingerprint ==
                        changedFingerprint &&
                    undoRegenerate.targetOperation ==
                        expectedRegenerateOperation &&
                    regenerateUndoRestored,
                "regenerate is one independently undoable recipe edit") &&
         expect(bakedInstancesRemain && expectedDetachOperation.has_value() &&
                    expectedDetachOperation->family ==
                        cr::CreativeAuthoringFamily::AssetScatter &&
                    expectedDetachOperation->kind ==
                        cr::CreativeAuthoringOperationKind::Destructive &&
                    expectedDetachOperation->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Destructive &&
                    expectedDetachOperation->action ==
                        "AssetScatter.Detach" &&
                    undoBake.accepted &&
                    undoBake.targetOperation == expectedDetachOperation &&
                    cr::findCreativePatternRecipe(
                        appState.facade.document().patternRecipeStore(),
                        recipeId) != nullptr,
                "bake keeps instances and is independently undoable");
}

bool scatterEraseRejectsSourceOwnedOutput() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Source Owned");
  static_cast<void>(document.assignId(9003U));
  app::CreativeEditorState editor = scatterEditor();
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = "Generated Crate";
  request.tags = {"creative_world_layout:source_owned"};
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(request);
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(document));
  editor.frameIndex = 77U;
  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = created.objectId;
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Crate;

  cr::CreativeWorldActionFrame remove;
  setPrimary(remove, true, true, false);
  app::processCreativeAssetScatterFrame(appState, editor, remove, 0U);
  const app::CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  const app::CreativeEditorPlacementFeedbackViewModel view =
      app::creativeEditorPlacementFeedbackViewModel(
          feedback, editor.frameIndex, &appState.facade.document());

  return expect(created.accepted && installed.accepted,
                "source-owned scatter erase fixture is valid") &&
         expect(appState.facade.findObject(created.objectId) != nullptr &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "scatter erase cannot destroy direct World Layout output") &&
         expect(feedback.rejectionReason ==
                        app::CreativeEditorPlacementRejectionReason::
                            SemanticSourceOwned &&
                    view.label.view() == "Edit generated source",
                "scatter erase explains the semantic owner");
}

bool scatterPreservesObstructionFeedback() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Blocked");
  static_cast<void>(document.assignId(9002U));
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(document));
  app::CreativeEditorState editor = scatterEditor();
  editor.frameIndex = 41U;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const app::CreativeEditorAssetScatterPlan initial =
      app::buildCreativeEditorAssetScatterPlan(
          appState.facade.document(), editor, held);

  std::vector<cr::CreativeDocumentCreateRequest> blockers;
  blockers.reserve(initial.placeableCount);
  for (const app::CreativeEditorAssetScatterCandidate& candidate :
       initial.items()) {
    if (!candidate.placeable) {
      continue;
    }
    blockers.push_back(app::buildBrushCreateRequest(
        candidate.placement, blockers.size() + 1U, "scatter_blocker"));
  }
  const cr::CreativeFacadeDocumentBatchCreateReceipt created =
      appState.facade.createDocumentObjectsAtomically(blockers);
  app::CreativePlacementClearanceCache cache;
  const bool cacheReady = app::refreshCreativePlacementClearanceCache(
      cache, appState.facade.document());
  const app::CreativeEditorAssetScatterPlan blocked =
      app::buildCreativeEditorAssetScatterPlan(
          appState.facade.document(), editor, held, &cache);
  const auto obstruction = std::find_if(
      blocked.items().begin(), blocked.items().end(),
      [](const app::CreativeEditorAssetScatterCandidate& candidate) {
        return candidate.status ==
               app::CreativeEditorAssetScatterCandidateStatus::Obstructed;
      });

  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  const std::size_t objectCountBefore =
      appState.facade.document().objectCount();
  app::processCreativeAssetScatterFrame(appState, editor, press, 0U, &cache);
  const app::CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;

  return expect(installed.accepted && initial.placeableCount > 0U &&
                    blockers.size() == initial.placeableCount &&
                    created.accepted &&
                    created.appliedCreateCount == blockers.size() &&
                    cacheReady,
                "scatter obstruction fixture and cache are valid") &&
         expect(blocked.placeableCount == 0U &&
                    obstruction != blocked.items().end() &&
                    obstruction->placement.clearance.status ==
                        cr::CreativePlacementClearanceStatus::
                            AuthoredObjectBlocked &&
                    obstruction->placement.clearance.blockingObjectId !=
                        cr::kInvalidObjectId,
                "scatter candidates retain exact obstruction provenance") &&
         expect(appState.facade.document().objectCount() == objectCountBefore &&
                    feedback.status ==
                        app::CreativeEditorPlacementFeedbackStatus::Rejected &&
                    feedback.clearance.status ==
                        cr::CreativePlacementClearanceStatus::
                            AuthoredObjectBlocked &&
                    feedback.clearance.blockingObjectId !=
                        cr::kInvalidObjectId,
                "blocked scatter gesture reports its blocker without mutation");
}

bool sharedVisitedKernelIsBounded() {
  cr::CreativeWorldGestureVisitedKeys visited;
  bool inserted = true;
  for (std::uint64_t key = 1U;
       key <= cr::kCreativeWorldGestureVisitedKeyCapacity; ++key) {
    inserted = inserted &&
               cr::rememberCreativeWorldGestureKey(visited, key) ==
                   cr::CreativeWorldGestureVisitStatus::Inserted;
  }
  const cr::CreativeWorldGestureVisitStatus duplicate =
      cr::rememberCreativeWorldGestureKey(visited, 1U);
  const cr::CreativeWorldGestureVisitStatus overflow =
      cr::rememberCreativeWorldGestureKey(visited, 999U);
  return expect(inserted && visited.count == visited.keys.size(),
                "visited kernel accepts its fixed capacity") &&
         expect(duplicate ==
                    cr::CreativeWorldGestureVisitStatus::AlreadyPresent &&
                    overflow ==
                        cr::CreativeWorldGestureVisitStatus::CapacityExceeded,
                "visited kernel distinguishes duplicate from capacity");
}

}  // namespace

int main() {
  const bool ok = scatterRecipeMutationsPublishOneTruthfulRevision() &&
                  plannerIsDeterministicBoundedAndSpaced() &&
                  plannerRejectsInvalidAndOversizedRequests() &&
                  terrainSurfacePoseMatchesFlatRenderedPatch() &&
                  recipePlannerHonorsMasksAndExclusions() &&
                  recipePlannerHonorsCollisionPolicy() &&
                  assetOnlyToolOptionsAreContextual() &&
                  terrainScatterRejectsSteepAndMissingSurface() &&
                  previewIsTransientAndSolidGhostIsSuppressed() &&
                  gestureIsAtomicDeduplicatedAndOneUndoStep() &&
                  activeStrokeCachesOneDirtyRegionPreview() &&
                  generatedEraseAddsExclusionAndKeepsRecipeEditable() &&
                  erasingLastGeneratedItemRemovesRecipeAtomically() &&
                  selectedScatterRegeneratesAndBakesWithHistory() &&
                  scatterEraseRejectsSourceOwnedOutput() &&
                  scatterPreservesObstructionFeedback() &&
                  sharedVisitedKernelIsBounded();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
