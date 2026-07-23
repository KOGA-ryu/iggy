#include "app/iggy3d/creative/world/WorldLayoutTerrainReconciliation.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocument makeDocument(cr::CreativeDocumentId id) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Terrain Reconciliation");
  static_cast<void>(document.assignId(id));
  return document;
}

cr::CreativeWorldLayoutTerrainProfile plateau(std::string_view key,
                                               std::uint16_t height) {
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = key;
  profile.kind = cr::CreativeTerrainRecipeKind::Plateau;
  profile.center = {0, 0};
  profile.baseHeightCells = height;
  profile.radiusCells = 2U;
  profile.spacingCells = 1U;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  return profile;
}

cr::CreativeWorldLayoutTerrainProfile terrace(std::string_view key) {
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = key;
  profile.kind = cr::CreativeTerrainRecipeKind::Terrace;
  profile.usesLandformRecipe = true;
  profile.landform.kind = cr::CreativeTerrainLandformKind::Terrace;
  profile.landform.bounds = {{-2, 3}, 8U, 4U};
  profile.landform.baseHeightCells = 2U;
  profile.landform.targetHeightCells = 8U;
  profile.landform.terraceCount = 4U;
  profile.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  profile.landform.edgeWidthCells = 0U;
  profile.landform.material = cr::CreativeTerrainMaterial::Stone;
  return profile;
}

bool applyLayoutTerrain(cr::CreativeDocument& document,
                        const cr::CreativeWorldLayout& layout) {
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  if (!compiled.receipt.accepted) {
    return false;
  }
  const cr::CreativeTerrainMutationReceipt terrain =
      document.applyTerrainControlEdits(compiled.plan.terrainEdits);
  const cr::CreativeTerrainMaterialMutationReceipt materials =
      document.applyTerrainMaterialEdits(compiled.plan.materialEdits);
  return terrain.accepted && materials.accepted;
}

bool applyFullLayoutTerrain(cr::CreativeDocument& document,
                            const cr::CreativeWorldLayout& layout) {
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  if (!compiled.receipt.accepted) {
    return false;
  }
  for (const cr::CreativeTerrainOperationMutationRequest& mutation :
       compiled.plan.terrainOperationMutations) {
    if (!document.applyTerrainOperationMutation(mutation).accepted) {
      return false;
    }
  }
  return document.applyTerrainControlEdits(compiled.plan.terrainEdits).accepted &&
         document.applyTerrainMaterialEdits(compiled.plan.materialEdits)
             .accepted;
}

bool driftFirstOwnedControl(cr::CreativeDocument& document,
                            const cr::CreativeWorldLayout& generated) {
  const cr::CreativeWorldLayoutTerrainImpactPlan impact =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, generated);
  if (!impact.accepted || impact.sources.empty() ||
      impact.sources[0].controls.empty()) {
    return false;
  }
  cr::CreativeTerrainControlPoint drifted = impact.sources[0].controls.front();
  ++drifted.heightCells;
  const cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, drifted};
  const cr::CreativeTerrainMutationReceipt receipt =
      document.applyTerrainControlEdits({&edit, 1U});
  return receipt.accepted && receipt.changed;
}

cr::CreativeWorldLayoutTerrainReconciliationResult reconcile(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& generated,
    const cr::CreativeWorldLayout& desired,
    std::span<const cr::CreativeWorldLayoutTerrainConflictDecision> decisions =
        {}) {
  return cr::reconcileCreativeWorldLayoutTerrain(
      {&document, &generated, &desired, decisions});
}

bool currentTerrainNeedsNoDecision() {
  cr::CreativeDocument document = makeDocument(9401U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "terrain_current";
  layout.terrainProfiles.push_back(plateau("terrain.plateau", 4U));
  const bool generated = applyLayoutTerrain(document, layout);
  const std::uint64_t documentRevision = document.revision();
  const std::uint64_t terrainRevision = document.terrainField().revision();
  const cr::CreativeWorldLayoutTerrainReconciliationResult result =
      reconcile(document, layout, layout);
  return expect(generated && result.requested && result.accepted &&
                    !result.blocked && result.conflicts.empty() &&
                    result.status ==
                        cr::CreativeWorldLayoutTerrainReconciliationStatus::
                            Ready,
                "current generated terrain needs no decision") &&
         expect(document.revision() == documentRevision &&
                    document.terrainField().revision() == terrainRevision,
                "reconciliation is read-only");
}

bool driftRequiresExactRegenerateDecision() {
  cr::CreativeDocument document = makeDocument(9402U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "terrain_drift";
  layout.terrainProfiles.push_back(plateau("terrain.plateau", 4U));
  const bool generated = applyLayoutTerrain(document, layout);
  const bool drifted = driftFirstOwnedControl(document, layout);
  const cr::CreativeWorldLayoutTerrainReconciliationResult blocked =
      reconcile(document, layout, layout);
  if (!generated || !drifted || blocked.conflicts.size() != 1U) {
    return expect(false, "drift reconciliation fixture is valid");
  }
  const cr::CreativeWorldLayoutTerrainConflict& conflict =
      blocked.conflicts[0];
  const cr::CreativeWorldLayoutTerrainConflictDecision decision{
      conflict.generatedTable, conflict.generatedIndex, conflict.stableKey,
      cr::CreativeWorldLayoutTerrainConflictResolution::Regenerate};
  const cr::CreativeWorldLayoutTerrainReconciliationResult resolved =
      reconcile(document, layout, layout, {&decision, 1U});

  cr::CreativeWorldLayoutTerrainConflictDecision stale = decision;
  stale.stableKey = "terrain.stale";
  const cr::CreativeWorldLayoutTerrainReconciliationResult staleResult =
      reconcile(document, layout, layout, {&stale, 1U});
  const std::array duplicate{decision, decision};
  const cr::CreativeWorldLayoutTerrainReconciliationResult duplicateResult =
      reconcile(document, layout, layout, duplicate);

  return expect(blocked.blocked && !blocked.accepted &&
                    blocked.status ==
                        cr::CreativeWorldLayoutTerrainReconciliationStatus::
                            Conflict &&
                    conflict.desiredSourcePresent &&
                    conflict.canDetachAndKeep3D,
                "drift blocks generation and exposes safe choices") &&
         expect(resolved.accepted && !resolved.blocked,
                "exact regenerate decision resolves drift") &&
         expect(!staleResult.accepted && !staleResult.blocked &&
                    staleResult.status ==
                        cr::CreativeWorldLayoutTerrainReconciliationStatus::
                            DecisionInvalid,
                "stale decision fails closed") &&
         expect(!duplicateResult.accepted &&
                    duplicateResult.status ==
                        cr::CreativeWorldLayoutTerrainReconciliationStatus::
                            DecisionInvalid,
                "duplicate decision fails closed");
}

bool sourceRemovalOnlyKeeps3DUnderPreserveExisting() {
  cr::CreativeDocument document = makeDocument(9403U);
  cr::CreativeWorldLayout generatedLayout;
  generatedLayout.stableKey = "terrain_detach";
  generatedLayout.terrainProfiles.push_back(
      plateau("terrain.detached", 4U));
  const bool generated = applyLayoutTerrain(document, generatedLayout);
  const bool drifted = driftFirstOwnedControl(document, generatedLayout);

  cr::CreativeWorldLayout preserved = generatedLayout;
  preserved.terrainProfiles.clear();
  const cr::CreativeWorldLayoutTerrainReconciliationResult detached =
      reconcile(document, generatedLayout, preserved);

  cr::CreativeWorldLayout replaced = preserved;
  replaced.terrainOwnership =
      cr::CreativeWorldLayoutTerrainOwnership::ReplaceAll;
  const cr::CreativeWorldLayoutTerrainReconciliationResult blocked =
      reconcile(document, generatedLayout, replaced);
  if (!generated || !drifted || blocked.conflicts.size() != 1U) {
    return expect(false, "terrain removal reconciliation fixture is valid");
  }
  const cr::CreativeWorldLayoutTerrainConflict& conflict =
      blocked.conflicts[0];
  const cr::CreativeWorldLayoutTerrainConflictDecision decision{
      conflict.generatedTable, conflict.generatedIndex, conflict.stableKey,
      cr::CreativeWorldLayoutTerrainConflictResolution::Regenerate};
  const cr::CreativeWorldLayoutTerrainReconciliationResult replacedResolved =
      reconcile(document, generatedLayout, replaced, {&decision, 1U});

  return expect(detached.accepted && detached.conflicts.empty() &&
                    detached.detachedSourceCount == 1U,
                "removed PreserveExisting source keeps 3D terrain") &&
         expect(blocked.blocked && conflict.desiredSourcePresent == false &&
                    !conflict.canDetachAndKeep3D,
                "ReplaceAll removal cannot pretend to preserve 3D terrain") &&
         expect(replacedResolved.accepted,
                "explicit source-wins decision permits ReplaceAll removal");
}

bool sourceTypeChangeCannotMasqueradeAsRemoval() {
  cr::CreativeDocument document = makeDocument(9404U);
  cr::CreativeWorldLayout generatedLayout;
  generatedLayout.stableKey = "terrain_type_change";
  generatedLayout.terrainProfiles.push_back(
      plateau("terrain.converted", 4U));
  const bool generated = applyLayoutTerrain(document, generatedLayout);
  const bool drifted = driftFirstOwnedControl(document, generatedLayout);

  cr::CreativeWorldLayout desired = generatedLayout;
  desired.terrainProfiles.clear();
  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "terrain.converted";
  path.recipe.kind = cr::CreativeTerrainPathKind::Road;
  path.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  path.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Crowned;
  path.recipe.material = cr::CreativeTerrainMaterial::Dirt;
  path.recipe.nextPointId = 3U;
  path.recipe.points = {
      {1U, {0, 0}, 4U, 1U, 1U, 0},
      {2U, {4, 0}, 4U, 1U, 1U, 0},
  };
  desired.terrainPaths.push_back(path);

  const cr::CreativeWorldLayoutTerrainReconciliationResult result =
      reconcile(document, generatedLayout, desired);
  return expect(generated && drifted && result.blocked &&
                    result.detachedSourceCount == 0U &&
                    result.conflicts.size() == 1U &&
                    result.conflicts[0].desiredSourcePresent &&
                    result.conflicts[0].desiredTable ==
                        cr::CreativeWorldLayoutTable::TerrainPath &&
                    !result.conflicts[0].canDetachAndKeep3D,
                "same-key source type change requires explicit regeneration");
}

bool landformImpactAndDriftUseDurableOperationIdentity() {
  cr::CreativeDocument document = makeDocument(9405U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "terrain_landform_reconcile";
  layout.terrainProfiles.push_back(terrace("terrain.terrace"));
  const bool generated = applyFullLayoutTerrain(document, layout);
  const cr::CreativeWorldLayoutTerrainImpactPlan current =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  const cr::CreativeWorldLayoutTerrainSourceImpact* currentImpact =
      cr::findCreativeWorldLayoutTerrainSourceImpact(
          current, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);

  const std::string sourceKey =
      cr::creativeWorldLayoutTerrainLandformSourceKey(
          layout.stableKey, layout.terrainProfiles[0].stableKey);
  const auto found = std::find_if(
      document.terrainOperationStack().operations.begin(),
      document.terrainOperationStack().operations.end(),
      [&](const cr::CreativeTerrainOperation& operation) {
        return operation.sourceKey == sourceKey;
      });
  bool drifted = false;
  if (found != document.terrainOperationStack().operations.end()) {
    cr::CreativeTerrainOperationMutationRequest update;
    update.kind = cr::CreativeTerrainOperationMutationKind::Update;
    update.operationId = found->id;
    update.owner = cr::CreativeTerrainOperationOwner::WorldLayout;
    update.sourceKey = sourceKey;
    update.operationKind = cr::CreativeTerrainOperationKind::Landform;
    update.landform = found->landform;
    ++update.landform.targetHeightCells;
    const cr::CreativeTerrainOperationMutationReceipt receipt =
        document.applyTerrainOperationMutation(update);
    drifted = receipt.accepted && receipt.changed;
  }
  const cr::CreativeWorldLayoutTerrainImpactPlan driftedImpactPlan =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  const cr::CreativeWorldLayoutTerrainSourceImpact* driftedImpact =
      cr::findCreativeWorldLayoutTerrainSourceImpact(
          driftedImpactPlan, cr::CreativeWorldLayoutTable::TerrainProfile,
          0U);
  const cr::CreativeWorldLayoutTerrainReconciliationResult blocked =
      reconcile(document, layout, layout);

  return expect(generated && current.accepted && currentImpact != nullptr &&
                    currentImpact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current &&
                    currentImpact->hasGridBounds &&
                    currentImpact->minimumCoord ==
                        cr::CreativeTerrainCoord2{-2, 3} &&
                    currentImpact->maximumCoord ==
                        cr::CreativeTerrainCoord2{5, 6} &&
                    currentImpact->minimumHeightCells == 2U &&
                    currentImpact->maximumHeightCells == 8U &&
                    !currentImpact->materials.empty(),
                "landform impact reports exact bounds heights material and current state") &&
         expect(drifted && driftedImpactPlan.accepted &&
                    driftedImpact != nullptr &&
                    driftedImpact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Drifted &&
                    blocked.blocked && blocked.conflicts.size() == 1U &&
                    blocked.conflicts[0].generatedTable ==
                        cr::CreativeWorldLayoutTable::TerrainProfile &&
                    blocked.conflicts[0].stableKey == "terrain.terrace",
                "landform operation drift requires the normal source conflict decision");
}

}  // namespace

int main() {
  const bool ok = currentTerrainNeedsNoDecision() &&
                  driftRequiresExactRegenerateDecision() &&
                  sourceRemovalOnlyKeeps3DUnderPreserveExisting() &&
                  sourceTypeChangeCannotMasqueradeAsRemoval() &&
                  landformImpactAndDriftUseDurableOperationIdentity();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
