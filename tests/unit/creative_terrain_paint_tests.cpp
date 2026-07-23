#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/TerrainPaint.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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

bool near(double lhs, double rhs, double epsilon = 1.0e-9) {
  return std::abs(lhs - rhs) <= epsilon;
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

bool weightedFieldIsExactDeterministicAndAtomic() {
  cr::CreativeTerrainMaterialField field;
  const cr::CreativeTerrainMaterialWeights balanced{64U, 64U, 64U, 63U};
  const cr::CreativeTerrainMaterialEdit weighted =
      cr::makeCreativeTerrainMaterialWeightEdit({2, 3}, balanced);
  const cr::CreativeTerrainMaterialMutationReceipt applied =
      field.apply(std::span{&weighted, 1U});
  const std::uint64_t revisionAfterApply = field.revision();

  const cr::CreativeTerrainMaterialWeights invalid{64U, 64U, 64U, 64U};
  const cr::CreativeTerrainMaterialEdit invalidEdit =
      cr::makeCreativeTerrainMaterialWeightEdit({9, 9}, invalid);
  const cr::CreativeTerrainMaterialMutationReceipt rejected =
      field.apply(std::span{&invalidEdit, 1U});

  const cr::CreativeTerrainMaterialWeights grass =
      cr::creativeTerrainMaterialSolidWeights(
          cr::CreativeTerrainMaterial::Grass);
  const cr::CreativeTerrainMaterialEdit restore =
      cr::makeCreativeTerrainMaterialWeightEdit({2, 3}, grass);
  const cr::CreativeTerrainMaterialMutationReceipt restored =
      field.apply(std::span{&restore, 1U});

  return expect(cr::isValidCreativeTerrainMaterialWeights(balanced) &&
                    cr::dominantCreativeTerrainMaterial(balanced) ==
                        cr::CreativeTerrainMaterial::Grass,
                "weighted material ties resolve to the first stable layer") &&
         expect(applied.accepted && applied.changed &&
                    field.revision() == revisionAfterApply + 1U &&
                    weighted.material == cr::CreativeTerrainMaterial::Grass,
                "exact weighted override applies and canonical restore mutates") &&
         expect(rejected.status ==
                        cr::CreativeTerrainMaterialMutationStatus::InvalidEdit &&
                    !rejected.accepted && !rejected.changed &&
                    rejected.revisionAfter == revisionAfterApply,
                "invalid weight sum rejects atomically") &&
         expect(restored.accepted && restored.changed &&
                    field.overrideCount() == 0U &&
                    field.weightsAt({2, 3}) == grass &&
                    field.validateInvariants(),
                "canonical all-grass weights erase the sparse override");
}

bool partialReplaceAndAdditivePaintingConserveWeights() {
  const std::array columns{cr::CreativeTerrainColumn{{0, 0}, 4U}};
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = columns;
  request.materialField = &materials;
  request.mode = cr::CreativeTerrainPaintMode::Connected;
  request.center = {0, 0};
  request.material = cr::CreativeTerrainMaterial::Stone;
  request.opacity = cr::CreativeTerrainPaintOpacity::Percent25;

  const cr::CreativeTerrainPaintPlan replaced =
      cr::buildCreativeTerrainPaintPlan(request);
  const cr::CreativeTerrainMaterialWeights expectedReplace{191U, 0U, 64U,
                                                            0U};
  const cr::CreativeTerrainMaterialMutationReceipt firstApply =
      materials.apply(replaced.items());

  request.blend = cr::CreativeTerrainPaintBlend::Additive;
  const cr::CreativeTerrainPaintPlan added =
      cr::buildCreativeTerrainPaintPlan(request);
  const cr::CreativeTerrainMaterialWeights expectedAdd{127U, 0U, 128U, 0U};
  const cr::CreativeTerrainMaterialMutationReceipt secondApply =
      materials.apply(added.items());
  const cr::CreativeVec3 blended =
      cr::creativeTerrainMaterialRenderColor(expectedAdd);

  return expect(replaced.accepted && replaced.items().size() == 1U &&
                    replaced.previews().size() == 1U &&
                    replaced.previews().front().influence == 64U &&
                    replaced.previews().front().afterWeights ==
                        expectedReplace,
                "25 percent replace exposes exact conserved preview weights") &&
         expect(firstApply.accepted && firstApply.changed && added.accepted &&
                    added.previews().front().beforeWeights == expectedReplace &&
                    added.previews().front().afterWeights == expectedAdd,
                "additive paint starts from the persisted weighted layer") &&
         expect(secondApply.accepted && secondApply.changed &&
                    materials.weightsAt({0, 0}) == expectedAdd &&
                    cr::isValidCreativeTerrainMaterialWeights(expectedAdd),
                "repeated additive paint conserves the 255-unit layer total") &&
         expect(near(blended.x, (0.22 * 127.0 + 0.42 * 128.0) / 255.0) &&
                    near(blended.y,
                         (0.52 * 127.0 + 0.44 * 128.0) / 255.0) &&
                    near(blended.z,
                         (0.20 * 127.0 + 0.46 * 128.0) / 255.0),
                "weighted layer color is a deterministic material blend");
}

bool brushMaskHardnessAndFiltersAreExact() {
  std::vector<cr::CreativeTerrainColumn> flat;
  for (std::int32_t z = -2; z <= 2; ++z) {
    for (std::int32_t x = -2; x <= 2; ++x) {
      flat.push_back({{x, z}, 4U});
    }
  }
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = flat;
  request.materialField = &materials;
  request.center = {0, 0};
  request.material = cr::CreativeTerrainMaterial::Stone;
  request.radiusCells = 2U;
  request.hardness = cr::CreativeTerrainPaintHardness::Soft;
  const cr::CreativeTerrainPaintPlan circle =
      cr::buildCreativeTerrainPaintPlan(request);
  request.mask = cr::CreativeTerrainPaintMask::Square;
  const cr::CreativeTerrainPaintPlan square =
      cr::buildCreativeTerrainPaintPlan(request);

  const auto previewAt = [](const cr::CreativeTerrainPaintPlan& plan,
                            cr::CreativeTerrainCoord2 coord) {
    return std::find_if(
        plan.previewCells.begin(), plan.previewCells.end(),
        [coord](const cr::CreativeTerrainPaintCellPreview& preview) {
          return preview.coord == coord;
        });
  };
  const auto circleCenter = previewAt(circle, {0, 0});
  const auto circleEdge = previewAt(circle, {2, 0});
  const auto squareCorner = previewAt(square, {2, 2});

  const std::array filteredColumns{
      cr::CreativeTerrainColumn{{-1, 0}, 1U},
      cr::CreativeTerrainColumn{{0, 0}, 1U},
      cr::CreativeTerrainColumn{{1, 0}, 5U},
      cr::CreativeTerrainColumn{{3, 0}, 12U},
  };
  const cr::CreativeTerrainMaterialEdit dirt{
      cr::CreativeTerrainMaterialEditKind::Set, {3, 0},
      cr::CreativeTerrainMaterial::Dirt};
  static_cast<void>(materials.apply(std::span{&dirt, 1U}));
  request.surfaceColumns = filteredColumns;
  request.mode = cr::CreativeTerrainPaintMode::Region;
  request.minimumCoord = {-1, 0};
  request.maximumCoord = {3, 0};
  request.source = cr::CreativeTerrainPaintSource::Grass;
  request.slopeFilter = cr::CreativeTerrainPaintSlopeFilter::Above45Degrees;
  request.heightFilter = cr::CreativeTerrainPaintHeightFilter::Cells1To8;
  request.mask = cr::CreativeTerrainPaintMask::Circle;
  request.hardness = cr::CreativeTerrainPaintHardness::Solid;
  const cr::CreativeTerrainPaintPlan filtered =
      cr::buildCreativeTerrainPaintPlan(request);

  return expect(circle.accepted && circle.cells().size() == 13U &&
                    square.accepted && square.cells().size() == 25U,
                "circle and square masks own distinct exact footprints") &&
         expect(circleCenter != circle.previewCells.end() &&
                    circleEdge != circle.previewCells.end() &&
                    squareCorner != square.previewCells.end() &&
                    circleCenter->influence == 255U &&
                    circleEdge->influence == 51U &&
                    squareCorner->influence == 51U,
                "soft hardness falls from an exact center to bounded edges") &&
         expect(filtered.accepted && filtered.cells().size() == 2U &&
                    filtered.cells()[0] == cr::CreativeTerrainCoord2{0, 0} &&
                    filtered.cells()[1] == cr::CreativeTerrainCoord2{1, 0} &&
                    filtered.previews().size() == 2U &&
                    filtered.previews()[0].heightCells == 1U &&
                    filtered.previews()[0].slopeDegrees > 45.0,
                "source slope and height filters compose before exact preview");
}

bool invalidPaintEnumsRejectBeforePlanning() {
  const std::array columns{cr::CreativeTerrainColumn{{0, 0}, 1U}};
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = columns;
  request.materialField = &materials;
  const auto invalid = [](const cr::CreativeTerrainPaintRequest& candidate) {
    const cr::CreativeTerrainPaintPlan plan =
        cr::buildCreativeTerrainPaintPlan(candidate);
    return !plan.accepted && plan.cells().empty() && plan.items().empty() &&
           plan.status == cr::CreativeTerrainPaintPlanStatus::InvalidRequest;
  };
  cr::CreativeTerrainPaintRequest candidate = request;
  candidate.hardness = cr::CreativeTerrainPaintHardness::Count;
  const bool hardness = invalid(candidate);
  candidate = request;
  candidate.opacity = cr::CreativeTerrainPaintOpacity::Count;
  const bool opacity = invalid(candidate);
  candidate = request;
  candidate.mask = cr::CreativeTerrainPaintMask::Count;
  const bool mask = invalid(candidate);
  candidate = request;
  candidate.blend = cr::CreativeTerrainPaintBlend::Count;
  const bool blend = invalid(candidate);
  candidate = request;
  candidate.slopeFilter = cr::CreativeTerrainPaintSlopeFilter::Count;
  const bool slope = invalid(candidate);
  candidate = request;
  candidate.heightFilter = cr::CreativeTerrainPaintHeightFilter::Count;
  const bool height = invalid(candidate);
  return expect(hardness && opacity && mask && blend && slope && height,
                "every invalid weighted-paint enum rejects before planning");
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
  const cr::CreativeTerrainMaterialWeights weights{128U, 0U, 127U, 0U};
  const cr::CreativeTerrainMaterialEdit edit =
      cr::makeCreativeTerrainMaterialWeightEdit({0, 0}, weights);
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
                    center->material == cr::CreativeTerrainMaterial::Grass &&
                    center->materialWeights == weights &&
                    near(center->materialColor.x,
                         (0.22 * 128.0 + 0.42 * 127.0) / 255.0) &&
                    near(center->materialColor.y,
                         (0.52 * 128.0 + 0.44 * 127.0) / 255.0) &&
                    near(center->materialColor.z,
                         (0.20 * 128.0 + 0.46 * 127.0) / 255.0) &&
                    painted.sourceMaterialRevision == materials.revision(),
                "render patch carries exact weighted tint and revision");
}

bool toolOptionsOwnTheCompletePaintContractWithoutNewBindings() {
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
  const std::array expectedBrush{
      cr::CreativeToolOptionId::TerrainPaintMode,
      cr::CreativeToolOptionId::TerrainPaintMaterial,
      cr::CreativeToolOptionId::TerrainPaintRadius,
      cr::CreativeToolOptionId::TerrainPaintSource,
      cr::CreativeToolOptionId::TerrainPaintHardness,
      cr::CreativeToolOptionId::TerrainPaintOpacity,
      cr::CreativeToolOptionId::TerrainPaintMask,
      cr::CreativeToolOptionId::TerrainPaintBlend,
      cr::CreativeToolOptionId::TerrainPaintSlopeFilter,
      cr::CreativeToolOptionId::TerrainPaintHeightFilter,
  };
  const std::array expectedArea{
      cr::CreativeToolOptionId::TerrainPaintMode,
      cr::CreativeToolOptionId::TerrainPaintMaterial,
      cr::CreativeToolOptionId::TerrainPaintSource,
      cr::CreativeToolOptionId::TerrainPaintOpacity,
      cr::CreativeToolOptionId::TerrainPaintBlend,
      cr::CreativeToolOptionId::TerrainPaintSlopeFilter,
      cr::CreativeToolOptionId::TerrainPaintHeightFilter,
  };
  const cr::CreativeToolOptionAdjustReceipt hardness =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintHardness, 1);
  const cr::CreativeToolOptionAdjustReceipt opacity =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintOpacity, -1);
  const cr::CreativeToolOptionAdjustReceipt mask =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintMask, 1);
  const cr::CreativeToolOptionAdjustReceipt blend =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintBlend, 1);
  const cr::CreativeToolOptionAdjustReceipt slope =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintSlopeFilter, 1);
  const cr::CreativeToolOptionAdjustReceipt height =
      cr::adjustCreativeToolOption(
          settings, cr::CreativeToolOptionId::TerrainPaintHeightFilter, 1);
  return expect(brushOptions.count == expectedBrush.size() &&
                    std::equal(brushOptions.ids.begin(),
                               brushOptions.ids.begin() + brushOptions.count,
                               expectedBrush.begin(), expectedBrush.end()),
                "brush exposes the complete weighted paint contract") &&
         expect(material.changed && radius.changed && connectedMode.changed &&
                    settings.terrainPaintMaterial ==
                        cr::CreativeTerrainMaterial::Dirt &&
                    settings.terrainPaintRadius ==
                        cr::CreativeTerrainPaintRadius::FourCells,
                "shared option router adjusts paint settings") &&
         expect(connectedOptions.count == expectedArea.size() &&
                    std::equal(connectedOptions.ids.begin(),
                               connectedOptions.ids.begin() +
                                   connectedOptions.count,
                               expectedArea.begin(), expectedArea.end()) &&
                    regionMode.changed &&
                    regionOptions.count == expectedArea.size() &&
                    std::equal(regionOptions.ids.begin(),
                               regionOptions.ids.begin() + regionOptions.count,
                               expectedArea.begin(), expectedArea.end()) &&
                    source.changed &&
                    settings.terrainPaintMode ==
                        cr::CreativeTerrainPaintMode::Region &&
                    settings.terrainPaintSource ==
                        cr::CreativeTerrainPaintSource::Grass,
                "area modes hide only brush-local radius hardness and mask") &&
         expect(hardness.changed && opacity.changed && mask.changed &&
                    blend.changed && slope.changed && height.changed &&
                    settings.terrainPaintHardness ==
                        cr::CreativeTerrainPaintHardness::Soft &&
                    settings.terrainPaintOpacity ==
                        cr::CreativeTerrainPaintOpacity::Percent75 &&
                    settings.terrainPaintMask ==
                        cr::CreativeTerrainPaintMask::Square &&
                    settings.terrainPaintBlend ==
                        cr::CreativeTerrainPaintBlend::Additive &&
                    settings.terrainPaintSlopeFilter ==
                        cr::CreativeTerrainPaintSlopeFilter::UpTo5Degrees &&
                    settings.terrainPaintHeightFilter ==
                        cr::CreativeTerrainPaintHeightFilter::Cells1To8,
                "shared option router adjusts every paint setting") &&
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
                 weightedFieldIsExactDeterministicAndAtomic() &&
                 partialReplaceAndAdditivePaintingConserveWeights() &&
                 brushMaskHardnessAndFiltersAreExact() &&
                 invalidPaintEnumsRejectBeforePlanning() &&
                 brushPlansOnlyPresentSurfaceAndSkipsNoOps() &&
                 connectedAndRegionPlansAreBoundedFilteredAndCanonical() &&
                 maximumConnectedPlanAppliesAtomicallyAtTheBound() &&
                 renderPlanJoinsMaterialWithoutChangingGeometry() &&
                 toolOptionsOwnTheCompletePaintContractWithoutNewBindings()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
