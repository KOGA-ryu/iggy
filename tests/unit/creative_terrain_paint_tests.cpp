#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/TerrainPaint.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sparseFieldIsCanonicalAndAtomic() {
  cr::CreativeTerrainMaterialField field;
  const std::array edits{
      cr::CreativeTerrainMaterialEdit{cr::CreativeTerrainMaterialEditKind::Set,
                                      {3, 2}, cr::CreativeTerrainMaterial::Stone},
      cr::CreativeTerrainMaterialEdit{cr::CreativeTerrainMaterialEditKind::Set,
                                      {-1, 0}, cr::CreativeTerrainMaterial::Dirt},
  };
  const cr::CreativeTerrainMaterialMutationReceipt applied = field.apply(edits);
  const std::array duplicate{
      cr::CreativeTerrainMaterialEdit{cr::CreativeTerrainMaterialEditKind::Set,
                                      {9, 9}, cr::CreativeTerrainMaterial::Sand},
      cr::CreativeTerrainMaterialEdit{cr::CreativeTerrainMaterialEditKind::Clear,
                                      {9, 9}, cr::CreativeTerrainMaterial::Grass},
  };
  const cr::CreativeTerrainMaterialMutationReceipt rejected =
      field.apply(duplicate);
  const cr::CreativeTerrainMaterialEdit clear{
      cr::CreativeTerrainMaterialEditKind::Clear, {-1, 0},
      cr::CreativeTerrainMaterial::Grass};
  const cr::CreativeTerrainMaterialMutationReceipt cleared =
      field.apply(std::span{&clear, 1U});

  return expect(applied.accepted && applied.changed &&
                    field.validateInvariants(),
                "valid material overrides apply canonically") &&
         expect(field.overrides().size() == 1U &&
                    field.overrides().front().coord ==
                        cr::CreativeTerrainCoord2{3, 2} &&
                    field.materialAt({3, 2}) ==
                        cr::CreativeTerrainMaterial::Stone &&
                    field.materialAt({0, 0}) ==
                        cr::CreativeTerrainMaterial::Grass,
                "grass is sparse default and clear removes override") &&
         expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeTerrainMaterialMutationStatus::
                            DuplicateCoordinate &&
                    cleared.accepted && cleared.changed,
                "duplicate batch rejects atomically before later clear") &&
         expect(cr::creativeTerrainMaterialRenderRole(
                    cr::CreativeTerrainMaterial::Sand) == "terrain_sand",
                "material owns stable render role");
}

bool fullCapacityMaterialBatchesApplyAndClearLinearly() {
  std::vector<cr::CreativeTerrainMaterialEdit> sets;
  std::vector<cr::CreativeTerrainMaterialEdit> clears;
  sets.reserve(cr::kCreativeTerrainMaterialOverrideCapacity);
  clears.reserve(cr::kCreativeTerrainMaterialOverrideCapacity);
  for (std::size_t index = 0U;
       index < cr::kCreativeTerrainMaterialOverrideCapacity; ++index) {
    const cr::CreativeTerrainCoord2 coord{static_cast<std::int32_t>(index), 0};
    sets.push_back({cr::CreativeTerrainMaterialEditKind::Set, coord,
                    cr::CreativeTerrainMaterial::Sand});
    clears.push_back({cr::CreativeTerrainMaterialEditKind::Clear, coord,
                      cr::CreativeTerrainMaterial::Grass});
  }
  cr::CreativeTerrainMaterialField field;
  const cr::CreativeTerrainMaterialMutationReceipt filled = field.apply(sets);
  const cr::CreativeTerrainMaterialMutationReceipt emptied =
      field.apply(clears);
  sets.push_back({cr::CreativeTerrainMaterialEditKind::Set,
                  {static_cast<std::int32_t>(sets.size()), 0},
                  cr::CreativeTerrainMaterial::Dirt});
  const cr::CreativeTerrainMaterialMutationReceipt overCapacity =
      field.apply(sets);
  return expect(filled.accepted && filled.changed &&
                    filled.changedOverrideCount ==
                        cr::kCreativeTerrainMaterialOverrideCapacity,
                "full-capacity set batch applies") &&
         expect(emptied.accepted && emptied.changed &&
                    field.overrideCount() == 0U,
                "full-capacity clear batch applies") &&
         expect(!overCapacity.accepted && !overCapacity.changed &&
                    overCapacity.status ==
                        cr::CreativeTerrainMaterialMutationStatus::
                            CapacityExceeded,
                "over-capacity edit batch rejects before staging");
}

bool brushPlansOnlyPresentSurfaceAndSkipsNoOps() {
  cr::CreativeTerrainField terrain;
  const cr::CreativeTerrainControlEdit control{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 2U}};
  static_cast<void>(terrain.apply(std::span{&control, 1U}));
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainSurfacePlan(terrain);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = surface.columns;
  request.materialField = &materials;
  request.center = {0, 0};
  request.material = cr::CreativeTerrainMaterial::Stone;
  request.radiusCells = 1U;
  const cr::CreativeTerrainPaintPlan paint =
      cr::buildCreativeTerrainPaintPlan(request);
  const cr::CreativeTerrainMaterialMutationReceipt applied =
      materials.apply(paint.items());
  const cr::CreativeTerrainPaintPlan repeated =
      cr::buildCreativeTerrainPaintPlan(request);
  request.material = cr::CreativeTerrainMaterial::Grass;
  const cr::CreativeTerrainPaintPlan restored =
      cr::buildCreativeTerrainPaintPlan(request);

  return expect(paint.accepted &&
                    paint.status == cr::CreativeTerrainPaintPlanStatus::Ready &&
                    paint.cells().size() == 5U && paint.items().size() == 5U,
                "radius-one brush owns five circular surface cells") &&
         expect(applied.accepted && applied.changed &&
                    materials.overrideCount() == 5U,
                "paint plan applies as one batch") &&
         expect(repeated.accepted && repeated.items().empty() &&
                    repeated.status ==
                        cr::CreativeTerrainPaintPlanStatus::NoChange,
                "revisiting painted surface is a no-op") &&
         expect(restored.accepted && restored.items().size() == 5U &&
                    restored.items().front().kind ==
                        cr::CreativeTerrainMaterialEditKind::Clear,
                "grass brush restores sparse default");
}

bool connectedAndRegionPlansAreBoundedFilteredAndCanonical() {
  const std::array columns{
      cr::CreativeTerrainColumn{{0, 0}, 4U},
      cr::CreativeTerrainColumn{{1, 0}, 4U},
      cr::CreativeTerrainColumn{{3, 0}, 4U},
      cr::CreativeTerrainColumn{{3, 1}, 4U},
  };
  cr::CreativeTerrainMaterialField materials;
  const cr::CreativeTerrainMaterialEdit dirt{
      cr::CreativeTerrainMaterialEditKind::Set, {1, 0},
      cr::CreativeTerrainMaterial::Dirt};
  static_cast<void>(materials.apply(std::span{&dirt, 1U}));

  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = columns;
  request.materialField = &materials;
  request.mode = cr::CreativeTerrainPaintMode::Connected;
  request.center = {0, 0};
  request.material = cr::CreativeTerrainMaterial::Stone;
  const cr::CreativeTerrainPaintPlan connected =
      cr::buildCreativeTerrainPaintPlan(request);

  request.mode = cr::CreativeTerrainPaintMode::Region;
  request.minimumCoord = {0, 0};
  request.maximumCoord = {3, 1};
  request.material = cr::CreativeTerrainMaterial::Sand;
  request.source = cr::CreativeTerrainPaintSource::Grass;
  const cr::CreativeTerrainPaintPlan region =
      cr::buildCreativeTerrainPaintPlan(request);
  request.maxAffectedCellCount = 2U;
  const cr::CreativeTerrainPaintPlan bounded =
      cr::buildCreativeTerrainPaintPlan(request);

  const std::array expectedRegion{
      cr::CreativeTerrainCoord2{0, 0},
      cr::CreativeTerrainCoord2{3, 0},
      cr::CreativeTerrainCoord2{3, 1},
  };
  return expect(connected.accepted && connected.cells().size() == 1U &&
                    connected.cells().front() ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    connected.source == cr::CreativeTerrainPaintSource::Grass,
                "connected fill stops at material and topology boundaries") &&
         expect(region.accepted &&
                    std::equal(region.cells().begin(), region.cells().end(),
                               expectedRegion.begin(), expectedRegion.end()) &&
                    region.items().size() == expectedRegion.size(),
                "region replace filters source and preserves canonical order") &&
         expect(!bounded.accepted && bounded.cells().empty() &&
                    bounded.items().empty() &&
                    bounded.status ==
                        cr::CreativeTerrainPaintPlanStatus::CapacityExceeded,
                "region rejects atomically before exceeding its bound");
}

bool maximumConnectedPlanAppliesAtomicallyAtTheBound() {
  std::vector<cr::CreativeTerrainColumn> columns;
  columns.reserve(cr::kCreativeTerrainPaintCellCapacity);
  for (std::size_t index = 0U;
       index < cr::kCreativeTerrainPaintCellCapacity; ++index) {
    columns.push_back({{static_cast<std::int32_t>(index), 0}, 1U});
  }
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = columns;
  request.materialField = &materials;
  request.mode = cr::CreativeTerrainPaintMode::Connected;
  request.center = {0, 0};
  request.material = cr::CreativeTerrainMaterial::Stone;
  const cr::CreativeTerrainPaintPlan plan =
      cr::buildCreativeTerrainPaintPlan(request);
  const cr::CreativeTerrainMaterialMutationReceipt applied =
      materials.apply(plan.items());

  request.maxAffectedCellCount =
      cr::kCreativeTerrainPaintCellCapacity - 1U;
  const cr::CreativeTerrainPaintPlan rejected =
      cr::buildCreativeTerrainPaintPlan(request);
  return expect(plan.accepted &&
                    plan.cells().size() ==
                        cr::kCreativeTerrainPaintCellCapacity &&
                    plan.items().size() ==
                        cr::kCreativeTerrainPaintCellCapacity,
                "connected planning reaches the full bounded capacity") &&
         expect(applied.accepted && applied.changed &&
                    materials.overrideCount() ==
                        cr::kCreativeTerrainPaintCellCapacity,
                "full connected plan applies as one atomic material batch") &&
         expect(!rejected.accepted && rejected.cells().empty() &&
                    rejected.items().empty() &&
                    rejected.status ==
                        cr::CreativeTerrainPaintPlanStatus::CapacityExceeded,
                "connected overflow exposes no partial plan");
}

bool renderPlanJoinsMaterialWithoutChangingGeometry() {
  cr::CreativeTerrainField terrain;
  const cr::CreativeTerrainControlEdit control{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 1U}};
  static_cast<void>(terrain.apply(std::span{&control, 1U}));
  cr::CreativeTerrainMaterialField materials;
  const cr::CreativeTerrainMaterialEdit edit{
      cr::CreativeTerrainMaterialEditKind::Set, {0, 0},
      cr::CreativeTerrainMaterial::Sand};
  static_cast<void>(materials.apply(std::span{&edit, 1U}));
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainSurfacePlan(terrain);
  const cr::CreativeTerrainRenderPlan plain =
      cr::buildCreativeTerrainRenderPlan(surface, {}, 1.0);
  const cr::CreativeTerrainRenderPlan painted =
      cr::buildCreativeTerrainRenderPlan(surface, materials, {}, 1.0);

  bool geometryMatches = plain.patches.size() == painted.patches.size();
  for (std::size_t index = 0U;
       geometryMatches && index < plain.patches.size(); ++index) {
    geometryMatches =
        plain.patches[index].coord == painted.patches[index].coord &&
        sameVec3(plain.patches[index].center, painted.patches[index].center);
    for (std::size_t corner = 0U;
         geometryMatches && corner < plain.patches[index].corners.size();
         ++corner) {
      geometryMatches = sameVec3(plain.patches[index].corners[corner],
                                 painted.patches[index].corners[corner]);
    }
  }
  const auto center = std::find_if(
      painted.patches.begin(), painted.patches.end(),
      [](const cr::CreativeTerrainSurfacePatch& patch) {
        return patch.coord == cr::CreativeTerrainCoord2{0, 0};
      });
  return expect(plain.accepted && painted.accepted && geometryMatches,
                "material join leaves terrain geometry unchanged") &&
         expect(center != painted.patches.end() &&
                    center->material == cr::CreativeTerrainMaterial::Sand &&
                    painted.sourceMaterialRevision == materials.revision(),
                "render patch carries authored material and revision");
}

bool toolOptionsOwnMaterialAndRadiusWithoutNewBindings() {
  const cr::CreativeToolOptionList brushOptions =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainPaint);
  cr::CreativeToolSettings settings = cr::makeDefaultCreativeToolSettings();
  const cr::CreativeToolOptionAdjustReceipt material =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintMaterial, 1);
  const cr::CreativeToolOptionAdjustReceipt radius =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintRadius, 1);
  const cr::CreativeToolOptionAdjustReceipt connectedMode =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintMode, 1);
  const cr::CreativeToolOptionList connectedOptions =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainPaint, settings);
  const cr::CreativeToolOptionAdjustReceipt regionMode =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintMode, 1);
  const cr::CreativeToolOptionList regionOptions =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainPaint, settings);
  const cr::CreativeToolOptionAdjustReceipt source =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintSource, 1);
  return expect(brushOptions.count == 3U &&
                    brushOptions.ids[0] ==
                        cr::CreativeToolOptionId::TerrainPaintMode &&
                    brushOptions.ids[1] ==
                        cr::CreativeToolOptionId::TerrainPaintMaterial &&
                    brushOptions.ids[2] ==
                        cr::CreativeToolOptionId::TerrainPaintRadius,
                "brush exposes mode material and radius") &&
         expect(material.changed && radius.changed && connectedMode.changed &&
                    settings.terrainPaintMaterial ==
                        cr::CreativeTerrainMaterial::Dirt &&
                    settings.terrainPaintRadius ==
                        cr::CreativeTerrainPaintRadius::FourCells,
                "shared option router adjusts paint settings") &&
         expect(connectedOptions.count == 2U && regionMode.changed &&
                    regionOptions.count == 3U && source.changed &&
                    settings.terrainPaintMode ==
                        cr::CreativeTerrainPaintMode::Region &&
                    settings.terrainPaintSource ==
                        cr::CreativeTerrainPaintSource::Grass,
                "connected hides radius while region exposes source filter") &&
         expect(cr::creativeHeldItemIsTerrainTool(
                    cr::CreativeHeldItemKind::TerrainPaint) &&
                    !cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::TerrainPaint),
                "terrain paint is semantic terrain tooling, not block material");
}

}  // namespace

int main() {
  return sparseFieldIsCanonicalAndAtomic() &&
                 fullCapacityMaterialBatchesApplyAndClearLinearly() &&
                 brushPlansOnlyPresentSurfaceAndSkipsNoOps() &&
                 connectedAndRegionPlansAreBoundedFilteredAndCanonical() &&
                 maximumConnectedPlanAppliesAtomicallyAtTheBound() &&
                 renderPlanJoinsMaterialWithoutChangingGeometry() &&
                 toolOptionsOwnMaterialAndRadiusWithoutNewBindings()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
