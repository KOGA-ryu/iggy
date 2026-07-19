#include "app/iggy3d/creative/world/WorldLayoutTerrainImpact.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
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
  cr::CreativeDocument document = cr::CreativeDocument::create("Impact Test");
  static_cast<void>(document.assignId(id));
  static_cast<void>(
      document.setGridSettings({{10.0, 1.0, -10.0}, 2.0, {64U, 16U, 64U}}));
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

const cr::CreativeTerrainControlPoint* findControl(
    const cr::CreativeWorldLayoutTerrainSourceImpact* impact,
    cr::CreativeTerrainCoord2 coord) {
  if (impact == nullptr) {
    return nullptr;
  }
  const auto found = std::find_if(
      impact->controls.begin(), impact->controls.end(),
      [coord](const cr::CreativeTerrainControlPoint& control) {
        return control.coord == coord;
      });
  return found == impact->controls.end() ? nullptr : &*found;
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

bool profileImpactTracksGenerationDriftAndWorldBounds() {
  cr::CreativeDocument document = makeDocument(9301U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "impact_profile";
  layout.terrainProfiles.push_back(plateau("terrain.plateau", 4U));

  const cr::CreativeWorldLayoutTerrainImpactPlan before =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  const cr::CreativeWorldLayoutTerrainSourceImpact* beforeImpact =
      cr::findCreativeWorldLayoutTerrainSourceImpact(
          before, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  cr::CreativeBounds bounds{};
  const bool framed = beforeImpact != nullptr &&
                      cr::creativeWorldLayoutTerrainImpactWorldBounds(
                          *beforeImpact, document.gridSettings(), bounds);
  const bool generated = applyLayoutTerrain(document, layout);
  const cr::CreativeWorldLayoutTerrainImpactPlan after =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  const cr::CreativeWorldLayoutTerrainSourceImpact* afterImpact =
      cr::findCreativeWorldLayoutTerrainSourceImpact(
          after, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  if (!generated || afterImpact == nullptr || afterImpact->controls.empty()) {
    return expect(false, "profile impact fixture generates terrain");
  }

  cr::CreativeTerrainControlPoint drifted = afterImpact->controls.front();
  ++drifted.heightCells;
  const cr::CreativeTerrainControlEdit driftEdit{
      cr::CreativeTerrainEditKind::Upsert, drifted};
  const cr::CreativeTerrainMutationReceipt driftReceipt =
      document.applyTerrainControlEdits({&driftEdit, 1U});
  const cr::CreativeWorldLayoutTerrainImpactPlan drift =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  const cr::CreativeWorldLayoutTerrainSourceImpact* driftImpact =
      cr::findCreativeWorldLayoutTerrainSourceImpact(
          drift, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);

  return expect(before.accepted && beforeImpact != nullptr &&
                    beforeImpact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Drifted &&
                    !beforeImpact->controls.empty() &&
                    !beforeImpact->influenceCells.empty(),
                "ungenerated profile exposes deterministic intended impact") &&
         expect(framed && bounds.min.x == 6.0 && bounds.max.x == 16.0 &&
                    bounds.min.z == -14.0 && bounds.max.z == -4.0 &&
                    bounds.min.y == 1.0 && bounds.max.y == 9.5,
                "inclusive grid footprint converts to exact world bounds") &&
         expect(after.accepted &&
                    afterImpact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current,
                "generated profile impact becomes current") &&
         expect(driftReceipt.accepted && driftReceipt.changed &&
                    driftImpact != nullptr &&
                    driftImpact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Drifted,
                "live terrain refinement is reported as drift");
}

bool overlapUsesEffectiveLastWriterWithoutNoOpTheft() {
  cr::CreativeDocument document = makeDocument(9302U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "impact_overlap";
  layout.terrainProfiles.push_back(plateau("terrain.first", 4U));
  layout.terrainProfiles.push_back(plateau("terrain.second", 6U));
  const bool generated = applyLayoutTerrain(document, layout);
  const cr::CreativeWorldLayoutTerrainImpactPlan impactPlan =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  const auto* first = cr::findCreativeWorldLayoutTerrainSourceImpact(
      impactPlan, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  const auto* second = cr::findCreativeWorldLayoutTerrainSourceImpact(
      impactPlan, cr::CreativeWorldLayoutTable::TerrainProfile, 1U);

  cr::CreativeWorldLayout noOpLayout;
  noOpLayout.stableKey = "impact_noop";
  noOpLayout.terrainProfiles.push_back(plateau("terrain.owner", 4U));
  noOpLayout.terrainProfiles.push_back(plateau("terrain.noop", 4U));
  cr::CreativeDocument noOpDocument = makeDocument(9303U);
  const bool noOpGenerated = applyLayoutTerrain(noOpDocument, noOpLayout);
  const cr::CreativeWorldLayoutTerrainImpactPlan noOpPlan =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(noOpDocument, noOpLayout);
  const auto* owner = cr::findCreativeWorldLayoutTerrainSourceImpact(
      noOpPlan, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  const auto* noOp = cr::findCreativeWorldLayoutTerrainSourceImpact(
      noOpPlan, cr::CreativeWorldLayoutTable::TerrainProfile, 1U);

  return expect(generated && impactPlan.accepted && first != nullptr &&
                    second != nullptr && first->controls.empty() &&
                    first->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Overridden,
                "later effective profile owns fully replaced controls") &&
         expect(second->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current &&
                    !second->controls.empty(),
                "effective last writer owns current terrain") &&
         expect(noOpGenerated && owner != nullptr && noOp != nullptr &&
                    owner->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current &&
                    noOp->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::NoEffect &&
                    noOp->authoredControlCount > 0U,
                "identical declaration cannot steal terrain ownership");
}

bool terrainOwnershipPreservesOrReplacesExternalControlShape() {
  const cr::CreativeTerrainControlEdit external{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 2U, 7U}};
  cr::CreativeDocument preserved = makeDocument(9305U);
  static_cast<void>(preserved.applyTerrainControlEdits({&external, 1U}));
  cr::CreativeWorldLayout layout;
  layout.stableKey = "impact_existing";
  cr::CreativeWorldLayoutTerrainProfile hill;
  hill.stableKey = "terrain.hill";
  hill.kind = cr::CreativeTerrainRecipeKind::Hill;
  hill.center = {0, 0};
  hill.baseHeightCells = 4U;
  hill.radiusCells = 2U;
  hill.amplitudeCells = 1U;
  hill.spacingCells = 1U;
  hill.blend = cr::CreativeTerrainProfileBlend::Set;
  hill.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  layout.terrainProfiles.push_back(hill);
  const bool preserveGenerated = applyLayoutTerrain(preserved, layout);
  const cr::CreativeWorldLayoutTerrainImpactPlan preservePlan =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(preserved, layout);
  const auto* preserveImpact = cr::findCreativeWorldLayoutTerrainSourceImpact(
      preservePlan, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  const cr::CreativeTerrainControlPoint* preservedCenter =
      findControl(preserveImpact, {0, 0});

  cr::CreativeDocument replaced = makeDocument(9306U);
  static_cast<void>(replaced.applyTerrainControlEdits({&external, 1U}));
  layout.terrainOwnership = cr::CreativeWorldLayoutTerrainOwnership::ReplaceAll;
  const bool replaceGenerated = applyLayoutTerrain(replaced, layout);
  const cr::CreativeWorldLayoutTerrainImpactPlan replacePlan =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(replaced, layout);
  const auto* replaceImpact = cr::findCreativeWorldLayoutTerrainSourceImpact(
      replacePlan, cr::CreativeWorldLayoutTable::TerrainProfile, 0U);
  const cr::CreativeTerrainControlPoint* replacedCenter =
      findControl(replaceImpact, {0, 0});

  return expect(preserveGenerated && preserveImpact != nullptr &&
                    preserveImpact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current &&
                    preservedCenter != nullptr &&
                    preservedCenter->radiusCells == 7U,
                "PreserveExisting impact retains external control radius") &&
         expect(replaceGenerated && replaceImpact != nullptr &&
                    replaceImpact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current &&
                    replacedCenter != nullptr &&
                    replacedCenter->radiusCells == 2U,
                "ReplaceAll impact derives canonical profile radius");
}

bool pathImpactTracksControlsMaterialsAndInvalidSources() {
  cr::CreativeDocument document = makeDocument(9304U);
  cr::CreativeWorldLayout layout;
  layout.stableKey = "impact_path";
  layout.terrainPathPoints = {{{0, 0}, 5U}, {{4, 0}, 5U}};
  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "terrain.road";
  path.kind = cr::CreativeTerrainRecipeKind::Road;
  path.firstPointIndex = 0U;
  path.pointCount = 2U;
  path.elevation = cr::CreativeTerrainPathElevation::Level;
  path.halfWidthCells = 1U;
  path.amplitudeCells = 1U;
  path.paintSurface = true;
  path.material = cr::CreativeTerrainMaterial::Dirt;
  layout.terrainPaths.push_back(path);

  const bool generated = applyLayoutTerrain(document, layout);
  const cr::CreativeWorldLayoutTerrainImpactPlan plan =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  const auto* impact = cr::findCreativeWorldLayoutTerrainSourceImpact(
      plan, cr::CreativeWorldLayoutTable::TerrainPath, 0U);

  layout.terrainPaths[0].elevation = cr::CreativeTerrainPathElevation::Follow;
  const cr::CreativeWorldLayoutTerrainImpactPlan invalid =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(document, layout);
  return expect(generated && plan.accepted && impact != nullptr &&
                    impact->status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current &&
                    !impact->controls.empty() && !impact->materials.empty(),
                "path impact retains exact control and material products") &&
         expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeWorldLayoutTerrainImpactPlanStatus::
                            InvalidLayout &&
                    invalid.failedTable ==
                        cr::CreativeWorldLayoutTable::TerrainPath &&
                    invalid.failedIndex == 0U,
                "unsupported relative path fails closed with exact source");
}

}  // namespace

int main() {
  const bool ok = profileImpactTracksGenerationDriftAndWorldBounds() &&
                  overlapUsesEffectiveLastWriterWithoutNoOpTheft() &&
                  terrainOwnershipPreservesOrReplacesExternalControlShape() &&
                  pathImpactTracksControlsMaterialsAndInvalidSources();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
