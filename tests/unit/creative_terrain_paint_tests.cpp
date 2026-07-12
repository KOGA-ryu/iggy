#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/TerrainPaint.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

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
                    paint.surfaceCellCount == 5U && paint.editCount == 5U,
                "radius-one brush owns five circular surface cells") &&
         expect(applied.accepted && applied.changed &&
                    materials.overrideCount() == 5U,
                "paint plan applies as one batch") &&
         expect(repeated.accepted && repeated.editCount == 0U &&
                    repeated.status ==
                        cr::CreativeTerrainPaintPlanStatus::NoChange,
                "revisiting painted surface is a no-op") &&
         expect(restored.accepted && restored.editCount == 5U &&
                    restored.items().front().kind ==
                        cr::CreativeTerrainMaterialEditKind::Clear,
                "grass brush restores sparse default");
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
  const cr::CreativeToolOptionList options =
      cr::creativeToolOptionsForHeldItem(
          cr::CreativeHeldItemKind::TerrainPaint);
  cr::CreativeToolSettings settings = cr::makeDefaultCreativeToolSettings();
  const cr::CreativeToolOptionAdjustReceipt material =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintMaterial, 1);
  const cr::CreativeToolOptionAdjustReceipt radius =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintRadius, 1);
  return expect(options.count == 2U &&
                    options.ids[0] ==
                        cr::CreativeToolOptionId::TerrainPaintMaterial &&
                    options.ids[1] ==
                        cr::CreativeToolOptionId::TerrainPaintRadius,
                "terrain paint exposes only material and radius") &&
         expect(material.changed && radius.changed &&
                    settings.terrainPaintMaterial ==
                        cr::CreativeTerrainMaterial::Dirt &&
                    settings.terrainPaintRadius ==
                        cr::CreativeTerrainPaintRadius::FourCells,
                "shared option router adjusts paint settings") &&
         expect(cr::creativeHeldItemIsTerrainTool(
                    cr::CreativeHeldItemKind::TerrainPaint) &&
                    !cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::TerrainPaint),
                "terrain paint is semantic terrain tooling, not block material");
}

}  // namespace

int main() {
  return sparseFieldIsCanonicalAndAtomic() &&
                 brushPlansOnlyPresentSurfaceAndSkipsNoOps() &&
                 renderPlanJoinsMaterialWithoutChangingGeometry() &&
                 toolOptionsOwnMaterialAndRadiusWithoutNewBindings()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
