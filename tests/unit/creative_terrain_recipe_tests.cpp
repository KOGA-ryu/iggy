#include "app/iggy3d/creative/recipes/TerrainRecipe.hpp"
#include "app/iggy3d/creative/tools/TerrainSeed.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocument makeDocument(cr::CreativeDocumentId id) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Terrain Recipe");
  static_cast<void>(document.assignId(id));
  return document;
}

cr::CreativeAppState makeAppState(cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(makeDocument(id)));
  return appState;
}

bool sameEdit(const cr::CreativeTerrainControlEdit& lhs,
              const cr::CreativeTerrainControlEdit& rhs) {
  return lhs.kind == rhs.kind && lhs.control == rhs.control;
}

const cr::CreativeTerrainControlPoint* controlAt(
    const cr::CreativeTerrainRecipePlan& plan,
    cr::CreativeTerrainCoord2 coord) {
  const auto found = std::find_if(
      plan.controlEdits.begin(), plan.controlEdits.end(),
      [coord](const cr::CreativeTerrainControlEdit& edit) {
        return edit.control.coord == coord;
      });
  return found == plan.controlEdits.end() ? nullptr : &found->control;
}

cr::CreativeTerrainProfileRecipeRequest hillRequest(
    const cr::CreativeDocument& document) {
  cr::CreativeTerrainProfileRecipeRequest request;
  request.document = &document;
  request.kind = cr::CreativeTerrainRecipeKind::Hill;
  request.center = {3, -2};
  request.baseHeightCells = 20U;
  request.radiusCells = 2U;
  request.amplitudeCells = 8U;
  return request;
}

cr::CreativeTerrainPathRecipeRequest riverRequest(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeTerrainPathPoint> points) {
  cr::CreativeTerrainPathRecipeRequest request;
  request.document = &document;
  request.kind = cr::CreativeTerrainRecipeKind::River;
  request.points = points;
  request.elevation = cr::CreativeTerrainPathElevation::Level;
  request.halfWidthCells = 1U;
  request.amplitudeCells = 2U;
  return request;
}

bool profileRecipeIsExactKernelOutput() {
  const cr::CreativeDocument document = makeDocument(101U);
  const cr::CreativeTerrainProfileRecipeRequest request = hillRequest(document);
  const cr::CreativeTerrainRecipeResult recipe =
      cr::buildCreativeTerrainProfileRecipe(request);

  cr::CreativeTerrainProfileRequest kernelRequest;
  kernelRequest.field = &document.terrainField();
  kernelRequest.center = request.center;
  kernelRequest.baseHeightCells = request.baseHeightCells;
  kernelRequest.profile = cr::CreativeTerrainProfileKind::Hill;
  kernelRequest.blend = request.blend;
  kernelRequest.rodPolicy = request.rodPolicy;
  kernelRequest.direction = request.direction;
  kernelRequest.radiusCells = request.radiusCells;
  kernelRequest.amplitudeCells = request.amplitudeCells;
  kernelRequest.spacingCells = request.spacingCells;
  kernelRequest.frequency = request.frequency;
  const cr::CreativeTerrainProfilePlan kernel =
      cr::buildCreativeTerrainProfilePlan(kernelRequest);

  bool editsMatch = recipe.plan.controlEdits.size() == kernel.items().size();
  for (std::size_t index = 0U;
       editsMatch && index < recipe.plan.controlEdits.size(); ++index) {
    editsMatch = sameEdit(recipe.plan.controlEdits[index], kernel.items()[index]);
  }

  return expect(recipe.receipt.accepted && kernel.accepted,
                "profile semantic recipe and kernel accepted") &&
         expect(recipe.receipt.status == cr::CreativeTerrainRecipeStatus::Ready,
                "profile semantic recipe ready") &&
         expect(editsMatch, "profile recipe copies exact kernel edit order") &&
         expect(recipe.plan.minimumCoord == cr::CreativeTerrainCoord2{1, -4} &&
                    recipe.plan.maximumCoord == cr::CreativeTerrainCoord2{5, 0},
                "profile recipe owns exact 2D bounds") &&
         expect(recipe.plan.sourceDocumentId == document.id() &&
                    recipe.plan.sourceDocumentRevision == document.revision() &&
                    recipe.plan.sourceTerrainRevision ==
                        document.terrainField().revision(),
                "profile recipe pins source identity and revision");
}

bool plateauRecipeUsesCanonicalLatticeAndAbsoluteHeight() {
  cr::CreativeDocument document = makeDocument(107U);
  const cr::CreativeTerrainControlEdit existing{
      cr::CreativeTerrainEditKind::Upsert, {{8, 8}, 11U, 2U}};
  static_cast<void>(document.applyTerrainControlEdits(
      std::span{&existing, 1U}));

  cr::CreativeTerrainProfileRecipeRequest request;
  request.document = &document;
  request.kind = cr::CreativeTerrainRecipeKind::Plateau;
  request.center = {8, 8};
  request.baseHeightCells = 6U;
  request.radiusCells = 8U;
  request.spacingCells = 4U;
  const cr::CreativeTerrainRecipeResult recipe =
      cr::buildCreativeTerrainProfileRecipe(request);

  const cr::CreativeTerrainField emptyField;
  const cr::CreativeTerrainSeedPlan lattice =
      cr::buildCreativeTerrainSeedPlan(
          {&emptyField, request.center,
           cr::CreativeTerrainSeedOperation::SeedMissing,
           request.radiusCells, request.spacingCells,
           request.baseHeightCells, 4U});
  bool exactLattice = recipe.plan.controlEdits.size() == lattice.items().size();
  for (std::size_t index = 0U;
       exactLattice && index < recipe.plan.controlEdits.size(); ++index) {
    exactLattice =
        sameEdit(recipe.plan.controlEdits[index], lattice.items()[index]);
  }
  const cr::CreativeTerrainControlPoint* center =
      controlAt(recipe.plan, request.center);
  const cr::CreativeTerrainRecipePreviewResult preview =
      cr::previewCreativeTerrainRecipe(document, recipe.plan);

  return expect(recipe.receipt.accepted && lattice.accepted,
                "plateau recipe and lattice kernel accepted") &&
         expect(recipe.plan.kind == cr::CreativeTerrainRecipeKind::Plateau &&
                    recipe.plan.controlEdits.size() == 13U && exactLattice,
                "plateau recipe owns canonical sparse lattice") &&
         expect(center != nullptr && center->heightCells == 6U &&
                    center->radiusCells == 4U,
                "plateau replaces existing center with absolute flat height") &&
         expect(recipe.plan.minimumCoord == cr::CreativeTerrainCoord2{0, 0} &&
                    recipe.plan.maximumCoord ==
                        cr::CreativeTerrainCoord2{16, 16},
                "plateau reports complete semantic bounds") &&
         expect(preview.accepted && preview.renderPlan.accepted,
                "plateau exact edit plan previews") &&
         expect(document.terrainField().controlAt({8, 8})->heightCells == 11U,
                "plateau planning leaves source terrain unchanged");
}

bool pathRecipeAddsSemanticMaterialAndExactPreview() {
  const cr::CreativeDocument document = makeDocument(102U);
  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 20U},
      cr::CreativeTerrainPathPoint{{3, 0}, 20U},
  };
  const cr::CreativeTerrainRecipeResult recipe =
      cr::buildCreativeTerrainPathRecipe(riverRequest(document, points));
  const cr::CreativeTerrainControlPoint* center =
      controlAt(recipe.plan, {1, 0});
  const bool allSand = !recipe.plan.materialEdits.empty() &&
                       std::all_of(recipe.plan.materialEdits.begin(),
                                   recipe.plan.materialEdits.end(),
                                   [](const auto& edit) {
                                     return edit.kind ==
                                                cr::CreativeTerrainMaterialEditKind::Set &&
                                            edit.material ==
                                                cr::CreativeTerrainMaterial::Sand;
                                   });
  const cr::CreativeTerrainRecipePreviewResult preview =
      cr::previewCreativeTerrainRecipe(document, recipe.plan);
  const bool previewHasSand =
      std::any_of(preview.renderPlan.patches.begin(),
                  preview.renderPlan.patches.end(), [](const auto& patch) {
                    return patch.material == cr::CreativeTerrainMaterial::Sand;
                  });

  return expect(recipe.receipt.accepted &&
                    recipe.receipt.kind == cr::CreativeTerrainRecipeKind::River,
                "river semantic recipe accepted") &&
         expect(center != nullptr && center->heightCells == 18U,
                "river recipe preserves kernel depth") &&
         expect(allSand, "river semantic default paints influenced surface") &&
         expect(preview.accepted && preview.renderPlan.accepted,
                "terrain recipe exact edit plan previews") &&
         expect(previewHasSand, "terrain recipe preview includes planned material") &&
         expect(document.revision() == 0U &&
                    document.terrainField().controlCount() == 0U &&
                    document.terrainMaterialField().overrideCount() == 0U,
                "terrain recipe preview leaves source document unchanged");
}

bool heightAndMaterialCommitAsOneUndoStep() {
  cr::CreativeAppState appState = makeAppState(103U);
  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 12U},
      cr::CreativeTerrainPathPoint{{2, 0}, 12U},
  };
  const cr::CreativeTerrainRecipeResult recipe =
      cr::buildCreativeTerrainPathRecipe(
          riverRequest(appState.facade.document(), points));
  const cr::CreativeTerrainRecipeApplyReceipt applied =
      cr::applyCreativeTerrainRecipeWithHistory(appState, recipe.plan,
                                                "terrain_recipe_test");
  const bool bothCommitted =
      appState.facade.document().terrainField().controlCount() > 0U &&
      appState.facade.document().terrainMaterialField().overrideCount() > 0U;
  const std::uint64_t undoDepthAfterApply =
      cr::creativeUndoDepth(appState.history);
  const cr::CreativeAuthoringOperationRecord* operation =
      cr::creativeHistoryTargetOperation(
          appState.history, cr::CreativeHistoryDirection::Undo);
  const std::optional<cr::CreativeAuthoringOperationRecord> expectedOperation =
      operation != nullptr
          ? std::optional<cr::CreativeAuthoringOperationRecord>{*operation}
          : std::nullopt;
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(applied.accepted && applied.changed &&
                    applied.status == cr::CreativeTerrainRecipeStatus::Applied,
                "terrain recipe atomic apply accepted") &&
         expect(applied.terrainReceipt.changed && applied.materialReceipt.changed,
                "terrain recipe changes height and material") &&
         expect(bothCommitted, "terrain recipe commits both authored fields") &&
         expect(applied.historyReceipt.recorded && undoDepthAfterApply == 1U,
                "terrain recipe records exactly one undo snapshot") &&
         expect(expectedOperation.has_value() &&
                    expectedOperation->family ==
                        cr::CreativeAuthoringFamily::Terrain &&
                    expectedOperation->kind ==
                        cr::CreativeAuthoringOperationKind::Apply &&
                    expectedOperation->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Parametric &&
                    expectedOperation->action == "River" &&
                    expectedOperation->requestFingerprint ==
                        cr::fingerprintCreativeTerrainRecipePlan(recipe.plan) &&
                    expectedOperation->affectedMemberCount ==
                        recipe.plan.controlEdits.size() +
                            recipe.plan.materialEdits.size(),
                "terrain history records the exact semantic plan") &&
         expect(undone.accepted && undone.changed,
                "terrain recipe one-step undo accepted") &&
         expect(undone.targetOperation == expectedOperation,
                "terrain operation metadata survives undo") &&
         expect(appState.facade.document().terrainField().controlCount() == 0U &&
                    appState.facade.document()
                            .terrainMaterialField()
                            .overrideCount() == 0U,
                "terrain recipe undo restores both authored fields");
}

bool staleAndRejectedPlansNeverPartiallyMutate() {
  cr::CreativeAppState staleState = makeAppState(104U);
  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 10U},
      cr::CreativeTerrainPathPoint{{2, 0}, 10U},
  };
  const cr::CreativeTerrainRecipeResult staleRecipe =
      cr::buildCreativeTerrainPathRecipe(
          riverRequest(staleState.facade.document(), points));
  const cr::CreativeTerrainControlEdit external{
      cr::CreativeTerrainEditKind::Upsert, {{50, 50}, 4U, 1U}};
  static_cast<void>(staleState.facade.applyTerrainControlEdits(
      std::span{&external, 1U}));
  const std::uint64_t staleRevisionBefore =
      staleState.facade.document().revision();
  const cr::CreativeTerrainRecipeApplyReceipt stale =
      cr::applyCreativeTerrainRecipeWithHistory(staleState, staleRecipe.plan,
                                                "stale_recipe");

  cr::CreativeAppState atomicState = makeAppState(105U);
  cr::CreativeTerrainRecipeResult atomicRecipe =
      cr::buildCreativeTerrainPathRecipe(
          riverRequest(atomicState.facade.document(), points));
  atomicRecipe.plan.materialEdits.push_back(
      atomicRecipe.plan.materialEdits.front());
  const cr::CreativeTerrainRecipeApplyReceipt rejected =
      cr::applyCreativeTerrainRecipeWithHistory(atomicState, atomicRecipe.plan,
                                                "invalid_recipe");

  return expect(!stale.accepted && !stale.changed &&
                    stale.status == cr::CreativeTerrainRecipeStatus::StalePlan,
                "stale terrain recipe rejected") &&
         expect(staleState.facade.document().revision() == staleRevisionBefore &&
                    staleState.facade.document().terrainField().controlCount() ==
                        1U &&
                    cr::creativeUndoDepth(staleState.history) == 0U,
                "stale terrain recipe preserves live document and history") &&
         expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeTerrainRecipeStatus::MutationRejected,
                "invalid material batch rejects complete terrain recipe") &&
         expect(rejected.terrainReceipt.changed &&
                    !rejected.materialReceipt.accepted,
                "atomic staging reaches material failure after staged height") &&
         expect(atomicState.facade.document().revision() == 0U &&
                    atomicState.facade.document()
                            .terrainField()
                            .controlCount() == 0U &&
                    atomicState.facade.document()
                            .terrainMaterialField()
                            .overrideCount() == 0U &&
                    cr::creativeUndoDepth(atomicState.history) == 0U,
                "failed terrain recipe publishes no partial mutation");
}

bool invalidFamiliesAndEnumsFailClosed() {
  const cr::CreativeDocument document = makeDocument(106U);
  cr::CreativeTerrainProfileRecipeRequest wrongProfile = hillRequest(document);
  wrongProfile.kind = cr::CreativeTerrainRecipeKind::Road;
  const cr::CreativeTerrainRecipeResult profile =
      cr::buildCreativeTerrainProfileRecipe(wrongProfile);

  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{1, 0}, 4U},
  };
  cr::CreativeTerrainPathRecipeRequest wrongPath =
      riverRequest(document, points);
  wrongPath.kind = cr::CreativeTerrainRecipeKind::Hill;
  const cr::CreativeTerrainRecipeResult path =
      cr::buildCreativeTerrainPathRecipe(wrongPath);

  cr::CreativeTerrainPathRecipeRequest invalidMaterial =
      riverRequest(document, points);
  invalidMaterial.material =
      static_cast<cr::CreativeTerrainMaterial>(
          std::numeric_limits<std::uint8_t>::max());
  const cr::CreativeTerrainRecipeResult material =
      cr::buildCreativeTerrainPathRecipe(invalidMaterial);

  return expect(!profile.receipt.accepted && profile.plan.controlEdits.empty() &&
                    profile.receipt.status ==
                        cr::CreativeTerrainRecipeStatus::InvalidKind,
                "path kind rejects in profile recipe") &&
         expect(!path.receipt.accepted && path.plan.controlEdits.empty() &&
                    path.receipt.status ==
                        cr::CreativeTerrainRecipeStatus::InvalidKind,
                "profile kind rejects in path recipe") &&
         expect(!material.receipt.accepted &&
                    material.plan.controlEdits.empty() &&
                    material.receipt.status ==
                        cr::CreativeTerrainRecipeStatus::MaterialPlanRejected,
                "invalid material enum rejects complete path recipe");
}

}  // namespace

int main() {
  const bool ok = profileRecipeIsExactKernelOutput() &&
                  plateauRecipeUsesCanonicalLatticeAndAbsoluteHeight() &&
                  pathRecipeAddsSemanticMaterialAndExactPreview() &&
                  heightAndMaterialCommitAsOneUndoStep() &&
                  staleAndRejectedPlansNeverPartiallyMutate() &&
                  invalidFamiliesAndEnumsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
