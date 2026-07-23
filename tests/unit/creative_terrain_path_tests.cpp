#include "app/iggy3d/creative/tools/TerrainPath.hpp"
#include "app/iggy3d/creative/recipes/TerrainPathSource.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
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

const cr::CreativeTerrainControlPoint* controlAt(
    const cr::CreativeTerrainPathPlan& plan,
    cr::CreativeTerrainCoord2 coord) {
  const auto found = std::find_if(
      plan.finalControls().begin(), plan.finalControls().end(),
      [coord](const auto& control) { return control.coord == coord; });
  return found == plan.finalControls().end() ? nullptr : &*found;
}

cr::CreativeTerrainPathRequest requestFor(
    const cr::CreativeTerrainField& field,
    std::span<const cr::CreativeTerrainPathPoint> points) {
  cr::CreativeTerrainPathRequest request;
  request.field = &field;
  request.points = points;
  request.elevation = cr::CreativeTerrainPathElevation::Level;
  request.halfWidthCells = 1U;
  request.amplitudeCells = 2U;
  return request;
}

cr::CreativeTerrainHeightField flatHeightField(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t heightCells) {
  cr::CreativeTerrainHeightField field;
  const std::vector<std::uint16_t> heights(
      static_cast<std::size_t>(bounds.widthCells) * bounds.depthCells,
      heightCells);
  static_cast<void>(field.replace(bounds, heights));
  return field;
}

cr::CreativeTerrainPathSourcePoint sourcePoint(
    cr::CreativeTerrainPathSourcePointId id,
    std::int32_t x,
    std::int32_t z,
    std::uint16_t height,
    std::uint16_t halfWidth,
    std::uint16_t amplitude = 0U,
    std::int32_t bankPermille = 0) {
  return {id, {x, z}, height, halfWidth, amplitude, bankPermille};
}

bool heightFieldsEqual(const cr::CreativeTerrainHeightField& lhs,
                       const cr::CreativeTerrainHeightField& rhs) {
  return lhs.bounds() == rhs.bounds() &&
         std::equal(lhs.heights().begin(), lhs.heights().end(),
                    rhs.heights().begin(), rhs.heights().end());
}

bool materialEditsEqual(
    std::span<const cr::CreativeTerrainMaterialEdit> lhs,
    std::span<const cr::CreativeTerrainMaterialEdit> rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                    [](const auto& left, const auto& right) {
                      return left.kind == right.kind &&
                             left.coord == right.coord &&
                             left.material == right.material &&
                             left.weights == right.weights;
                    });
}

bool segmentReceiptsEqual(
    std::span<const cr::CreativeTerrainPathSegmentReceipt> lhs,
    std::span<const cr::CreativeTerrainPathSegmentReceipt> rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                    [](const auto& left, const auto& right) {
                      return left.startPointId == right.startPointId &&
                             left.endPointId == right.endPointId &&
                             left.impactBounds == right.impactBounds &&
                             left.sourceHash == right.sourceHash &&
                             left.centerlineCellCount ==
                                 right.centerlineCellCount &&
                             left.generatedCellCount ==
                                 right.generatedCellCount;
                    });
}

bool optionValuesAndLabelsAreClosed() {
  return expect(cr::creativeTerrainPathHalfWidthCells(
                    cr::CreativeTerrainPathWidth::SevenCells) == 3U &&
                    cr::creativeTerrainPathWidthCells(
                        cr::CreativeTerrainPathWidth::FiveCells) == 5U &&
                    cr::creativeTerrainPathAmplitudeCells(
                        cr::CreativeTerrainPathAmplitude::EightCells) == 8U,
                "path option enums resolve to bounded scalar values") &&
         expect(cr::toString(cr::CreativeTerrainPathKind::Road) == "ROAD" &&
                    cr::toString(cr::CreativeTerrainPathKind::Trench) ==
                        "TRENCH" &&
                    cr::toString(cr::CreativeTerrainPathElevation::Grade) ==
                        "GRADE" &&
                    cr::toString(cr::CreativeTerrainPathWidth::ThreeCells) ==
                        "3 CELLS" &&
                    cr::toString(cr::CreativeTerrainPathKind::Count) ==
                        "INVALID",
                "path enums expose explicit creator labels") &&
         expect(cr::creativeTerrainPathUsesDepth(
                    cr::CreativeTerrainPathKind::River) &&
                    cr::creativeTerrainPathUsesDepth(
                        cr::CreativeTerrainPathKind::Trench) &&
                    !cr::creativeTerrainPathUsesDepth(
                        cr::CreativeTerrainPathKind::Road),
                "depth-bearing path kinds are explicit");
}

bool roadRasterIsCanonicalAndIdempotent() {
  cr::CreativeTerrainField field;
  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 10U},
      cr::CreativeTerrainPathPoint{{4, 0}, 20U},
  };
  cr::CreativeTerrainPathRequest request = requestFor(field, points);
  const cr::CreativeTerrainPathPlan first =
      cr::buildCreativeTerrainPathPlan(request);
  bool canonical = true;
  for (std::size_t index = 1U; index < first.finalControls().size(); ++index) {
    const auto before = first.finalControls()[index - 1U].coord;
    const auto after = first.finalControls()[index].coord;
    canonical = canonical &&
                (before.z < after.z ||
                 (before.z == after.z && before.x < after.x));
  }
  const cr::CreativeTerrainMutationReceipt applied = field.apply(first.items());
  request.field = &field;
  const cr::CreativeTerrainPathPlan repeated =
      cr::buildCreativeTerrainPathPlan(request);
  const cr::CreativeTerrainControlPoint* center = controlAt(first, {2, 0});
  const cr::CreativeTerrainControlPoint* edge = controlAt(first, {2, 1});
  return expect(first.accepted &&
                    first.status == cr::CreativeTerrainPathPlanStatus::Ready &&
                    first.centerlineCount == 5U && first.controlCount == 17U &&
                    first.editCount == 17U && canonical,
                "straight road raster is bounded deduplicated and canonical") &&
         expect(center != nullptr && edge != nullptr &&
                    center->heightCells == 12U && edge->heightCells == 12U &&
                    center->radiusCells == 2U,
                "road emits a flat width-three deck at level plus rise") &&
         expect(applied.changed && field.controlCount() == 17U &&
                    repeated.accepted && repeated.items().empty() &&
                    repeated.status ==
                        cr::CreativeTerrainPathPlanStatus::NoChange,
                "one terrain batch applies and exact replay is idempotent");
}

bool crossSectionsAndElevationModesArePinned() {
  cr::CreativeTerrainField field;
  constexpr std::array flatPoints{
      cr::CreativeTerrainPathPoint{{0, 0}, 20U},
      cr::CreativeTerrainPathPoint{{2, 0}, 20U},
  };
  cr::CreativeTerrainPathRequest request = requestFor(field, flatPoints);
  request.halfWidthCells = 2U;
  request.amplitudeCells = 4U;

  request.kind = cr::CreativeTerrainPathKind::River;
  const cr::CreativeTerrainPathPlan river =
      cr::buildCreativeTerrainPathPlan(request);
  request.kind = cr::CreativeTerrainPathKind::Ridge;
  const cr::CreativeTerrainPathPlan ridge =
      cr::buildCreativeTerrainPathPlan(request);
  request.kind = cr::CreativeTerrainPathKind::Trench;
  const cr::CreativeTerrainPathPlan trench =
      cr::buildCreativeTerrainPathPlan(request);

  constexpr std::array gradedPoints{
      cr::CreativeTerrainPathPoint{{0, 0}, 10U},
      cr::CreativeTerrainPathPoint{{4, 0}, 20U},
  };
  cr::CreativeTerrainPathRequest grade = requestFor(field, gradedPoints);
  grade.kind = cr::CreativeTerrainPathKind::Ridge;
  grade.elevation = cr::CreativeTerrainPathElevation::Grade;
  grade.halfWidthCells = 0U;
  grade.amplitudeCells = 1U;
  const cr::CreativeTerrainPathPlan graded =
      cr::buildCreativeTerrainPathPlan(grade);

  return expect(controlAt(river, {1, 0})->heightCells == 16U &&
                    controlAt(river, {1, 2})->heightCells == 20U,
                "river is a smooth bowl returning to base at its bank") &&
         expect(controlAt(ridge, {1, 0})->heightCells == 24U &&
                    controlAt(ridge, {1, 2})->heightCells == 20U,
                "ridge is the positive smooth cross-section") &&
         expect(controlAt(trench, {1, 0})->heightCells == 16U &&
                    controlAt(trench, {1, 2})->heightCells == 16U,
                "trench keeps a flat full-depth cut across its width") &&
         expect(controlAt(graded, {0, 0})->heightCells == 11U &&
                    controlAt(graded, {2, 0})->heightCells == 16U &&
                    controlAt(graded, {4, 0})->heightCells == 21U,
                "grade interpolates endpoint elevations by path progress");
}

bool bendsDeduplicateAndFollowSamplesLiveTerrain() {
  cr::CreativeTerrainField field;
  const std::array existing{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 8U, 1U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 2}, 14U, 1U}},
  };
  static_cast<void>(field.apply(existing));
  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{2, 0}, 4U},
      cr::CreativeTerrainPathPoint{{2, 2}, 4U},
  };
  cr::CreativeTerrainPathRequest request = requestFor(field, points);
  request.kind = cr::CreativeTerrainPathKind::Road;
  request.elevation = cr::CreativeTerrainPathElevation::Follow;
  request.halfWidthCells = 0U;
  request.amplitudeCells = 1U;
  const cr::CreativeTerrainPathPlan plan =
      cr::buildCreativeTerrainPathPlan(request);
  return expect(plan.accepted && plan.centerlineCount == 5U &&
                    plan.controlCount == 5U,
                "joined segments own their bend cell exactly once") &&
         expect(controlAt(plan, {0, 0})->heightCells == 9U &&
                    controlAt(plan, {2, 2})->heightCells == 15U,
                "follow samples current terrain along the centerline");
}

bool durableRoadRecipeOwnsProfilesMaterialsAndExactOutput() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{0, 0}, 16U, 12U}, 4U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPathSourceRecipe recipe;
  recipe.kind = cr::CreativeTerrainPathKind::Road;
  recipe.elevation = cr::CreativeTerrainPathElevation::Grade;
  recipe.curve = cr::CreativeTerrainPathCurvePolicy::Linear;
  recipe.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  recipe.falloffCells = 2U;
  recipe.material = cr::CreativeTerrainMaterial::Dirt;
  recipe.nextPointId = 3U;
  recipe.points = {sourcePoint(1U, 2, 5, 4U, 1U),
                   sourcePoint(2U, 12, 5, 6U, 2U, 0U, 1000)};

  const cr::CreativeTerrainPathSourceResult first =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe);
  const cr::CreativeTerrainPathSourceResult repeated =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe);
  const bool endpointPainted = std::any_of(
      first.materialEdits.begin(), first.materialEdits.end(),
      [](const cr::CreativeTerrainMaterialEdit& edit) {
        return edit.coord == cr::CreativeTerrainCoord2{12, 5} &&
               edit.material == cr::CreativeTerrainMaterial::Dirt;
      });
  const bool materialEditsRepeat =
      repeated.materialEdits.size() == first.materialEdits.size() &&
      std::equal(repeated.materialEdits.begin(), repeated.materialEdits.end(),
                 first.materialEdits.begin(),
                 [](const cr::CreativeTerrainMaterialEdit& lhs,
                    const cr::CreativeTerrainMaterialEdit& rhs) {
                   return lhs.kind == rhs.kind && lhs.coord == rhs.coord &&
                          lhs.material == rhs.material &&
                          lhs.weights == rhs.weights;
                 });

  return expect(cr::isValidCreativeTerrainPathSourceRecipe(recipe) &&
                    first.receipt.accepted && first.segments.size() == 1U &&
                    first.segments[0].startPointId == 1U &&
                    first.segments[0].endPointId == 2U &&
                    first.segments[0].centerlineCellCount == 11U &&
                    first.segments[0].impactBounds.widthCells > 0U,
                "durable path recipe owns stable points and segment facts") &&
         expect(first.heightField.heightAt({2, 5}) == 4U &&
                    first.heightField.heightAt({12, 5}) == 6U &&
                    first.heightField.heightAt({12, 6}) == 7U &&
                    first.heightField.heightAt({0, 0}) == 4U,
                "road profiles interpolate elevation width and signed bank") &&
         expect(first.receipt.materialEditCount > 0U && endpointPainted,
                "road recipe emits exact bounded surface material edits") &&
         expect(repeated.receipt.accepted &&
                    repeated.receipt.recipeHash == first.receipt.recipeHash &&
                    repeated.receipt.outputHeightHash ==
                        first.receipt.outputHeightHash &&
                    materialEditsRepeat,
                "path source output and provenance hashes are deterministic");
}

bool roadSettingsOwnShouldersGradesAndExactSampling() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{0, 0}, 16U, 12U}, 4U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPathSourceRecipe road;
  road.kind = cr::CreativeTerrainPathKind::Road;
  road.elevation = cr::CreativeTerrainPathElevation::Grade;
  road.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  road.falloffCells = 0U;
  road.material = cr::CreativeTerrainMaterial::Dirt;
  road.nextPointId = 3U;
  road.points = {sourcePoint(1U, 2, 5, 8U, 1U),
                 sourcePoint(2U, 10, 5, 8U, 1U)};

  const cr::CreativeTerrainPathSourceResult travelSurface =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials, road);
  road.road.shoulderWidthCells = 2U;
  const cr::CreativeTerrainPathSourceSamplingResult sampled =
      cr::sampleCreativeTerrainPathSourceRecipe(road);
  const cr::CreativeTerrainPathSourceResult shouldered =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials, road);
  const cr::CreativeTerrainPathSourceSamplingResult repeated =
      cr::sampleCreativeTerrainPathSourceRecipe(road);

  cr::CreativeTerrainPathSourceRecipe exactGrade = road;
  exactGrade.points[0].heightCells = 4U;
  exactGrade.points[1].heightCells = 8U;
  exactGrade.road.maximumGradePermille = 500U;
  const cr::CreativeTerrainPathSourceSamplingResult acceptedGrade =
      cr::sampleCreativeTerrainPathSourceRecipe(exactGrade);
  exactGrade.road.maximumGradePermille = 499U;
  const cr::CreativeTerrainPathSourceSamplingResult rejectedGrade =
      cr::sampleCreativeTerrainPathSourceRecipe(exactGrade);

  const auto paints = [](const cr::CreativeTerrainPathSourceResult& result,
                         cr::CreativeTerrainCoord2 coord) {
    return std::any_of(
        result.materialEdits.begin(), result.materialEdits.end(),
        [coord](const cr::CreativeTerrainMaterialEdit& edit) {
          return edit.coord == coord &&
                 edit.material == cr::CreativeTerrainMaterial::Dirt;
        });
  };

  return expect(travelSurface.receipt.accepted &&
                    travelSurface.heightField.heightAt({6, 8}) == 4U &&
                    !paints(travelSurface, {6, 8}),
                "authored half-width remains the road travel surface") &&
         expect(sampled.accepted && shouldered.receipt.accepted &&
                    sampled.samples == shouldered.samples &&
                    !sampled.samples.empty() &&
                    sampled.samples.front().halfWidthCells == 3.0 &&
                    shouldered.heightField.heightAt({6, 8}) == 8U &&
                    paints(shouldered, {6, 8}),
                "shoulder width expands grading painting and shared samples") &&
         expect(repeated.accepted && repeated.samples == sampled.samples &&
                    segmentReceiptsEqual(repeated.segments,
                                         sampled.segments),
                "public road sampling is deterministic") &&
         expect(acceptedGrade.accepted && !rejectedGrade.accepted &&
                    rejectedGrade.status ==
                        cr::CreativeTerrainPathSourceStatus::InvalidRecipe &&
                    rejectedGrade.reasonCode ==
                        "creative_terrain_path_source_grade_limit_exceeded",
                "road grade accepts its exact limit and rejects steeper sources") &&
         expect(cr::toString(cr::CreativeTerrainRoadEdgeTreatment::None) ==
                        "NONE" &&
                    cr::toString(cr::CreativeTerrainRoadEdgeTreatment::Curb) ==
                        "CURB",
                "road edge treatments expose closed creator labels");
}

bool durableFollowUsesPointHeightWhereTerrainIsAbsent() {
  cr::CreativeTerrainHeightField empty;
  const cr::CreativeTerrainSurfacePlan emptySurface =
      cr::buildCreativeTerrainHeightSurfacePlan(empty);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPathSourceRecipe recipe;
  recipe.kind = cr::CreativeTerrainPathKind::Road;
  recipe.elevation = cr::CreativeTerrainPathElevation::Follow;
  recipe.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  recipe.falloffCells = 0U;
  recipe.nextPointId = 3U;
  recipe.points = {sourcePoint(1U, 0, 0, 4U, 0U),
                   sourcePoint(2U, 4, 0, 6U, 0U)};

  const cr::CreativeTerrainPathSourceResult result =
      cr::buildCreativeTerrainPathSourceRecipe(empty, emptySurface, materials,
                                               recipe);
  return expect(result.receipt.accepted &&
                    result.heightField.heightAt({0, 0}) == 4U &&
                    result.heightField.heightAt({4, 0}) == 6U,
                "follow falls back to authored endpoint heights over empty terrain");
}

bool curvesCrossSectionsAndDirtySegmentsAreExplicit() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{0, 0}, 20U, 16U}, 10U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPathSourceRecipe curve;
  curve.kind = cr::CreativeTerrainPathKind::River;
  curve.elevation = cr::CreativeTerrainPathElevation::Level;
  curve.curve = cr::CreativeTerrainPathCurvePolicy::CatmullRom;
  curve.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  curve.startJoin = cr::CreativeTerrainPathEndpointJoin::Blend;
  curve.endJoin = cr::CreativeTerrainPathEndpointJoin::Bridge;
  curve.falloffCells = 1U;
  curve.material = cr::CreativeTerrainMaterial::Sand;
  curve.nextPointId = 7U;
  curve.points = {
      sourcePoint(1U, 2, 2, 10U, 2U, 2U),
      sourcePoint(2U, 5, 8, 10U, 2U, 2U),
      sourcePoint(3U, 8, 10, 10U, 3U, 2U),
      sourcePoint(4U, 11, 8, 10U, 3U, 2U),
      sourcePoint(5U, 14, 2, 10U, 2U, 2U),
      sourcePoint(6U, 17, 3, 10U, 2U, 2U),
  };
  const cr::CreativeTerrainPathSourceResult curved =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               curve);
  cr::CreativeTerrainPathSourceRecipe linear = curve;
  linear.curve = cr::CreativeTerrainPathCurvePolicy::Linear;
  const cr::CreativeTerrainPathSourceResult straight =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               linear);
  cr::CreativeTerrainPathSourceRecipe edited = curve;
  edited.points[3U].halfWidthCells = 5U;
  const cr::CreativeTerrainPathDirtySegments dirty =
      cr::diffCreativeTerrainPathSourceSegments(curve, edited);
  cr::CreativeTerrainPathSourceRecipe allocatorOnly = curve;
  ++allocatorOnly.nextPointId;
  const cr::CreativeTerrainPathDirtySegments allocatorDirty =
      cr::diffCreativeTerrainPathSourceSegments(curve, allocatorOnly);

  return expect(curved.receipt.accepted && straight.receipt.accepted &&
                    curved.receipt.centerlineCellCount > curve.points.size() &&
                    curved.heightField.heightAt({8, 10}) == 8U,
                "curved channel emits a sampled concave cross-section") &&
         expect(curved.receipt.recipeHash != straight.receipt.recipeHash &&
                    curved.receipt.outputHeightHash !=
                        straight.receipt.outputHeightHash,
                "curve policy changes exact deterministic geometry") &&
         expect(dirty.changed && !dirty.allSegments &&
                    dirty.firstSegment == 1U && dirty.segmentCount == 4U,
                "Catmull-Rom edits invalidate only tangent-adjacent segments") &&
         expect(!allocatorDirty.changed && allocatorDirty.segmentCount == 0U,
                "point id allocator changes do not rebuild geometry") &&
         expect(cr::toString(cr::CreativeTerrainPathCurvePolicy::CatmullRom) ==
                        "CATMULL-ROM" &&
                    cr::toString(cr::CreativeTerrainPathCrossSection::Cut) ==
                        "CUT" &&
                    cr::toString(cr::CreativeTerrainPathEndpointJoin::Bridge) ==
                        "BRIDGE" &&
                    cr::toString(
                        cr::CreativeTerrainPathEndpointJoin::BuildingPad) ==
                        "BUILDING_PAD",
                "durable path policies expose closed creator labels");
}

bool incrementalSegmentCacheIsExactAtomicAndObservable() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{0, 0}, 24U, 16U}, 10U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPathSourceRecipe recipe;
  recipe.kind = cr::CreativeTerrainPathKind::River;
  recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  recipe.curve = cr::CreativeTerrainPathCurvePolicy::CatmullRom;
  recipe.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  recipe.falloffCells = 1U;
  recipe.material = cr::CreativeTerrainMaterial::Sand;
  recipe.nextPointId = 7U;
  recipe.points = {
      sourcePoint(1U, 2, 2, 10U, 2U, 2U),
      sourcePoint(2U, 5, 8, 10U, 2U, 2U),
      sourcePoint(3U, 8, 10, 10U, 3U, 2U),
      sourcePoint(4U, 11, 8, 10U, 3U, 2U),
      sourcePoint(5U, 14, 2, 10U, 2U, 2U),
      sourcePoint(6U, 17, 3, 10U, 2U, 2U),
  };

  cr::CreativeTerrainPathSourceCache cache;
  const cr::CreativeTerrainPathSourceResult first =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe, &cache);
  bool ok = expect(first.receipt.accepted && cache.valid &&
                       first.receipt.rebuiltSegmentCount == 5U &&
                       first.receipt.reusedSegmentCount == 0U &&
                       cache.dirtySegments.changed &&
                       cache.dirtySegments.allSegments &&
                       cache.segments.size() == 5U &&
                       first.receipt.generatedControlCount > 0U &&
                       first.receipt.generatedControlCount ==
                           first.receipt.evaluatedCellCount &&
                       cache.generatedControlCount ==
                           first.receipt.generatedControlCount,
                   "first cached build reports all sampled segments and controls");

  cr::CreativeTerrainPathSourceRecipe edited = recipe;
  edited.points[3U].halfWidthCells = 5U;
  const cr::CreativeTerrainPathSourceResult incremental =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               edited, &cache);
  const cr::CreativeTerrainPathSourceResult clean =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               edited);
  const bool exactParity =
      incremental.receipt.accepted && clean.receipt.accepted &&
      incremental.receipt.outputHeightHash ==
          clean.receipt.outputHeightHash &&
      heightFieldsEqual(incremental.heightField, clean.heightField) &&
      materialEditsEqual(incremental.materialEdits, clean.materialEdits) &&
      segmentReceiptsEqual(incremental.segments, clean.segments);
  ok = expect(cache.dirtySegments.changed &&
                  !cache.dirtySegments.allSegments &&
                  cache.dirtySegments.firstSegment == 1U &&
                  cache.dirtySegments.segmentCount == 4U &&
                  incremental.receipt.rebuiltSegmentCount == 4U &&
                  incremental.receipt.reusedSegmentCount == 1U && exactParity,
              "one middle-point edit rebuilds only tangent-adjacent segments with clean parity") &&
       ok;

  cr::CreativeTerrainPathSourceRecipe allocatorOnly = edited;
  ++allocatorOnly.nextPointId;
  const cr::CreativeTerrainPathSourceResult reused =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               allocatorOnly, &cache);
  ok = expect(reused.receipt.accepted &&
                  reused.receipt.rebuiltSegmentCount == 0U &&
                  reused.receipt.reusedSegmentCount == 5U &&
                  !cache.dirtySegments.changed &&
                  reused.receipt.outputHeightHash ==
                      incremental.receipt.outputHeightHash &&
                  heightFieldsEqual(reused.heightField,
                                    incremental.heightField),
              "allocator-only recipe changes reuse every geometric segment") &&
       ok;

  const cr::CreativeTerrainPathSourceCache stableCache = cache;
  cr::CreativeTerrainPathSourceRecipe rejected = allocatorOnly;
  rejected.crossSection = cr::CreativeTerrainPathCrossSection::Crowned;
  for (auto& point : rejected.points) {
    point.heightCells = cr::kCreativeTerrainMaximumHeightCells;
    point.amplitudeCells = cr::kCreativeTerrainMaximumHeightCells;
  }
  const cr::CreativeTerrainPathSourceResult failed =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               rejected, &cache);
  return expect(!failed.receipt.accepted &&
                    failed.receipt.status ==
                        cr::CreativeTerrainPathSourceStatus::HeightOutOfRange &&
                    cache == stableCache,
                "failed incremental composition preserves the last good cache") &&
         ok;
}

bool endpointJoinsOwnDistinctTerrainSeams() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{-4, -4}, 20U, 12U}, 4U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPathSourceRecipe recipe;
  recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  recipe.crossSection = cr::CreativeTerrainPathCrossSection::Crowned;
  recipe.falloffCells = 0U;
  recipe.material = cr::CreativeTerrainMaterial::Dirt;
  recipe.nextPointId = 3U;
  recipe.points = {sourcePoint(1U, 0, 0, 8U, 2U, 2U),
                   sourcePoint(2U, 8, 0, 8U, 2U, 2U)};

  recipe.startJoin = cr::CreativeTerrainPathEndpointJoin::Open;
  const cr::CreativeTerrainPathSourceResult open =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe);
  recipe.startJoin = cr::CreativeTerrainPathEndpointJoin::Blend;
  const cr::CreativeTerrainPathSourceResult blend =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe);
  recipe.startJoin = cr::CreativeTerrainPathEndpointJoin::Intersection;
  const cr::CreativeTerrainPathSourceResult intersection =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe);
  recipe.startJoin = cr::CreativeTerrainPathEndpointJoin::Bridge;
  const cr::CreativeTerrainPathSourceResult bridge =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe);
  recipe.startJoin = cr::CreativeTerrainPathEndpointJoin::BuildingPad;
  const cr::CreativeTerrainPathSourceResult buildingPad =
      cr::buildCreativeTerrainPathSourceRecipe(base, surface, materials,
                                               recipe);

  const auto paintedAt = [](const cr::CreativeTerrainPathSourceResult& result,
                            cr::CreativeTerrainCoord2 coord) {
    return std::any_of(
        result.materialEdits.begin(), result.materialEdits.end(),
        [coord](const cr::CreativeTerrainMaterialEdit& edit) {
          return edit.kind == cr::CreativeTerrainMaterialEditKind::Set &&
                 edit.coord == coord;
        });
  };

  return expect(open.receipt.accepted && blend.receipt.accepted &&
                    intersection.receipt.accepted && bridge.receipt.accepted &&
                    buildingPad.receipt.accepted,
                "all endpoint join policies generate valid terrain") &&
         expect(open.heightField.heightAt({0, 0}) == 10U &&
                    paintedAt(open, {0, 0}),
                "open join retains the ordinary full-profile cap") &&
         expect(blend.heightField.heightAt({0, 0}) == 4U &&
                    blend.heightField.heightAt({1, 0}) == 7U &&
                    blend.heightField.heightAt({2, 0}) == 10U &&
                    !paintedAt(blend, {0, 0}) && paintedAt(blend, {2, 0}),
                "blend join eases height and material into surrounding terrain") &&
         expect(intersection.heightField.heightAt({0, 0}) == 8U &&
                    intersection.heightField.heightAt({0, 3}) == 8U &&
                    paintedAt(intersection, {0, 3}),
                "intersection join emits a widened level junction pad") &&
         expect(bridge.heightField.heightAt({0, 0}) == 8U &&
                    bridge.heightField.heightAt({0, 1}) == 8U &&
                    bridge.heightField.heightAt({2, 0}) == 10U,
                "bridge join emits a level approach before full profile resumes") &&
         expect(buildingPad.heightField.heightAt({0, 0}) == 8U &&
                    buildingPad.heightField.heightAt({0, 1}) == 8U &&
                    buildingPad.heightField.heightAt({2, 0}) == 10U,
                "building-pad join flattens the profile at the host perimeter");
}

bool durablePathFailuresAreAtomicAndBounded() {
  cr::CreativeTerrainHeightField empty;
  const cr::CreativeTerrainSurfacePlan emptySurface =
      cr::buildCreativeTerrainHeightSurfacePlan(empty);
  cr::CreativeTerrainMaterialField materials;
  cr::CreativeTerrainPathSourceRecipe duplicateIds;
  duplicateIds.nextPointId = 2U;
  duplicateIds.points = {sourcePoint(1U, 0, 0, 4U, 1U),
                         sourcePoint(1U, 4, 0, 4U, 1U)};
  const cr::CreativeTerrainPathSourceResult invalid =
      cr::buildCreativeTerrainPathSourceRecipe(
          empty, emptySurface, materials, duplicateIds);

  cr::CreativeTerrainPathSourceRecipe oversized;
  oversized.nextPointId = 3U;
  oversized.falloffCells =
      cr::kCreativeTerrainPathSourceMaximumFalloffCells;
  oversized.points = {
      sourcePoint(1U, 0, 0, 4U,
                  cr::kCreativeTerrainPathSourceMaximumHalfWidthCells),
      sourcePoint(2U, cr::kCreativeTerrainPathMaximumSegmentCells, 0, 4U,
                  cr::kCreativeTerrainPathSourceMaximumHalfWidthCells),
  };
  const cr::CreativeTerrainPathSourceResult capacity =
      cr::buildCreativeTerrainPathSourceRecipe(empty, emptySurface, materials,
                                               oversized);

  cr::CreativeTerrainPathSourceRecipe unsupported = oversized;
  unsupported.version = 99U;
  const cr::CreativeTerrainPathSourceResult version =
      cr::buildCreativeTerrainPathSourceRecipe(empty, emptySurface, materials,
                                               unsupported);

  return expect(!invalid.receipt.accepted && invalid.materialEdits.empty() &&
                    invalid.heightField.cellCount() == 0U &&
                    invalid.receipt.status ==
                        cr::CreativeTerrainPathSourceStatus::InvalidRecipe,
                "invalid stable point ownership rejects atomically") &&
         expect(!capacity.receipt.accepted &&
                    capacity.materialEdits.empty() &&
                    capacity.heightField.cellCount() == 0U &&
                    capacity.receipt.status ==
                        cr::CreativeTerrainPathSourceStatus::CapacityExceeded,
                "oversized path bounds reject before partial output") &&
         expect(!version.receipt.accepted && version.materialEdits.empty() &&
                    version.receipt.status ==
                        cr::CreativeTerrainPathSourceStatus::UnsupportedVersion,
                "unsupported path source versions fail closed");
}

bool rejectedPlansAreAtomicAndSpecific() {
  cr::CreativeTerrainField empty;
  constexpr std::array duplicate{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
  };
  const auto invalid =
      cr::buildCreativeTerrainPathPlan(requestFor(empty, duplicate));

  constexpr std::array longSegment{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{256, 0}, 4U},
  };
  const auto tooLong =
      cr::buildCreativeTerrainPathPlan(requestFor(empty, longSegment));

  constexpr std::array wideSegment{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{255, 0}, 4U},
  };
  cr::CreativeTerrainPathRequest capacityRequest =
      requestFor(empty, wideSegment);
  capacityRequest.halfWidthCells = 3U;
  const auto capacity =
      cr::buildCreativeTerrainPathPlan(capacityRequest);

  constexpr std::array overflowPoints{
      cr::CreativeTerrainPathPoint{
          {std::numeric_limits<std::int32_t>::max(), 0}, 4U},
      cr::CreativeTerrainPathPoint{
          {std::numeric_limits<std::int32_t>::max() - 1, 0}, 4U},
  };
  cr::CreativeTerrainPathRequest overflowRequest =
      requestFor(empty, overflowPoints);
  overflowRequest.halfWidthCells = 1U;
  const auto overflow =
      cr::buildCreativeTerrainPathPlan(overflowRequest);

  std::vector<cr::CreativeTerrainControlEdit> fullEdits;
  fullEdits.reserve(cr::kCreativeTerrainControlCapacity);
  for (std::size_t index = 0U; index < cr::kCreativeTerrainControlCapacity;
       ++index) {
    fullEdits.push_back(
        {cr::CreativeTerrainEditKind::Upsert,
         {{1000 + static_cast<std::int32_t>(index), 1000}, 4U, 1U}});
  }
  cr::CreativeTerrainField full;
  static_cast<void>(full.apply(fullEdits));
  constexpr std::array shortPath{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{1, 0}, 4U},
  };
  const auto fieldCapacity =
      cr::buildCreativeTerrainPathPlan(requestFor(full, shortPath));

  return expect(!invalid.accepted && invalid.items().empty() &&
                    invalid.status ==
                        cr::CreativeTerrainPathPlanStatus::InvalidRequest,
                "duplicate adjacent points reject atomically") &&
         expect(!tooLong.accepted && tooLong.items().empty() &&
                    tooLong.status ==
                        cr::CreativeTerrainPathPlanStatus::PathTooLong,
                "unbounded segments reject before traversal") &&
         expect(!capacity.accepted && capacity.items().empty() &&
                    capacity.status ==
                        cr::CreativeTerrainPathPlanStatus::CapacityExceeded,
                "wide long corridors reject instead of partially stamping") &&
         expect(!overflow.accepted && overflow.items().empty() &&
                    overflow.status ==
                        cr::CreativeTerrainPathPlanStatus::CoordinateOverflow,
                "corridor offset overflow rejects with zero edits") &&
         expect(!fieldCapacity.accepted && fieldCapacity.items().empty() &&
                    fieldCapacity.status ==
                        cr::CreativeTerrainPathPlanStatus::CapacityExceeded,
                "full authored field rejects a path before partial edits");
}

}  // namespace

int main() {
  bool ok = true;
  ok = optionValuesAndLabelsAreClosed() && ok;
  ok = roadRasterIsCanonicalAndIdempotent() && ok;
  ok = crossSectionsAndElevationModesArePinned() && ok;
  ok = bendsDeduplicateAndFollowSamplesLiveTerrain() && ok;
  ok = durableRoadRecipeOwnsProfilesMaterialsAndExactOutput() && ok;
  ok = roadSettingsOwnShouldersGradesAndExactSampling() && ok;
  ok = durableFollowUsesPointHeightWhereTerrainIsAbsent() && ok;
  ok = curvesCrossSectionsAndDirtySegmentsAreExplicit() && ok;
  ok = incrementalSegmentCacheIsExactAtomicAndObservable() && ok;
  ok = endpointJoinsOwnDistinctTerrainSeams() && ok;
  ok = durablePathFailuresAreAtomicAndBounded() && ok;
  ok = rejectedPlansAreAtomicAndSpecific() && ok;
  return ok ? 0 : 1;
}
