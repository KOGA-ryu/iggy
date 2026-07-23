#include "app/iggy3d/creative/recipes/TerrainLandform.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainHeightField flatField(std::uint16_t width,
                                         std::uint16_t depth,
                                         std::uint16_t height) {
  cr::CreativeTerrainHeightField field;
  const std::vector<std::uint16_t> heights(
      static_cast<std::size_t>(width) * depth, height);
  static_cast<void>(field.replace({{0, 0}, width, depth}, heights));
  return field;
}

bool hasHardEdge(std::span<const cr::CreativeTerrainHardEdge> edges,
                 cr::CreativeTerrainCoord2 first,
                 cr::CreativeTerrainCoord2 second) {
  const cr::CreativeTerrainHardEdge expected =
      cr::canonicalCreativeTerrainHardEdge(first, second);
  return std::find(edges.begin(), edges.end(), expected) != edges.end();
}

bool plateauOwnsExactHeightMaterialAndEdgePolicy() {
  const cr::CreativeTerrainHeightField source = flatField(8U, 8U, 2U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(source);
  cr::CreativeTerrainLandformRecipe retaining;
  retaining.bounds = {{1, 1}, 6U, 6U};
  retaining.baseHeightCells = 2U;
  retaining.targetHeightCells = 8U;
  retaining.edge = cr::CreativeTerrainLandformEdge::Retaining;
  retaining.edgeWidthCells = 0U;
  retaining.material = cr::CreativeTerrainMaterial::Stone;
  const cr::CreativeTerrainLandformResult sharp =
      cr::buildCreativeTerrainLandform(source, surface, {}, retaining);

  cr::CreativeTerrainLandformRecipe sloped = retaining;
  sloped.edge = cr::CreativeTerrainLandformEdge::Slope;
  sloped.edgeWidthCells = 2U;
  sloped.paintSurface = false;
  const cr::CreativeTerrainLandformResult soft =
      cr::buildCreativeTerrainLandform(source, surface, {}, sloped);
  return expect(sharp.receipt.accepted &&
                    sharp.heightField.heightAt({1, 1}) == 8U &&
                    sharp.heightField.heightAt({3, 3}) == 8U &&
                    sharp.heightField.heightAt({0, 0}) == 2U,
                "retaining plateau is flat with one sharp authored edge") &&
         expect(sharp.receipt.hardEdgeCount == 24U &&
                    sharp.hardEdges.size() == 24U &&
                    cr::validateCreativeTerrainHardEdges(sharp.hardEdges) &&
                    hasHardEdge(sharp.hardEdges, {1, 1}, {0, 1}) &&
                    hasHardEdge(sharp.hardEdges, {6, 6}, {7, 6}),
                "retaining plateau emits one canonical outer topology ring") &&
         expect(sharp.materialEdits.size() == 36U &&
                    sharp.materialEdits.front().material ==
                        cr::CreativeTerrainMaterial::Stone,
                "plateau owns deterministic surface material edits") &&
         expect(soft.receipt.accepted &&
                    soft.heightField.heightAt({1, 1}) == 2U &&
                    soft.heightField.heightAt({2, 2}) == 5U &&
                    soft.heightField.heightAt({3, 3}) == 8U,
                "slope width blends the perimeter while preserving flat top") &&
         expect(soft.receipt.hardEdgeCount == 0U && soft.hardEdges.empty(),
                "sloped plateau does not fabricate retaining topology");
}

bool terraceAndCliffShareOneDirectionalContract() {
  const cr::CreativeTerrainHeightField source = flatField(8U, 4U, 2U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(source);
  cr::CreativeTerrainLandformRecipe terrace;
  terrace.kind = cr::CreativeTerrainLandformKind::Terrace;
  terrace.bounds = {{0, 0}, 8U, 4U};
  terrace.baseHeightCells = 2U;
  terrace.targetHeightCells = 8U;
  terrace.terraceCount = 4U;
  terrace.direction = cr::CreativeTerrainLandformDirection::PositiveX;
  terrace.edge = cr::CreativeTerrainLandformEdge::Retaining;
  terrace.edgeWidthCells = 0U;
  terrace.paintSurface = false;
  const cr::CreativeTerrainLandformResult steps =
      cr::buildCreativeTerrainLandform(source, surface, {}, terrace);

  cr::CreativeTerrainLandformRecipe cliff = terrace;
  cliff.kind = cr::CreativeTerrainLandformKind::Cliff;
  cliff.direction = cr::CreativeTerrainLandformDirection::NegativeX;
  cliff.erosion = cr::CreativeTerrainLandformErosion::Weathered;
  cliff.erosionReliefCells = 1U;
  cliff.seed = 77U;
  const cr::CreativeTerrainLandformResult first =
      cr::buildCreativeTerrainLandform(source, surface, {}, cliff);
  const cr::CreativeTerrainLandformResult repeated =
      cr::buildCreativeTerrainLandform(source, surface, {}, cliff);
  cr::CreativeTerrainLandformRecipe slopedCliff = cliff;
  slopedCliff.edge = cr::CreativeTerrainLandformEdge::Slope;
  slopedCliff.edgeWidthCells = 2U;
  const cr::CreativeTerrainLandformResult softCliff =
      cr::buildCreativeTerrainLandform(source, surface, {}, slopedCliff);
  return expect(steps.receipt.accepted &&
                    steps.heightField.heightAt({0, 1}) == 2U &&
                    steps.heightField.heightAt({2, 1}) == 4U &&
                    steps.heightField.heightAt({4, 1}) == 6U &&
                    steps.heightField.heightAt({6, 1}) == 8U,
                "terraces are exact directional constant-height bands") &&
         expect(steps.receipt.hardEdgeCount == 36U &&
                    cr::validateCreativeTerrainHardEdges(steps.hardEdges) &&
                    hasHardEdge(steps.hardEdges, {1, 1}, {2, 1}) &&
                    hasHardEdge(steps.hardEdges, {3, 1}, {4, 1}) &&
                    hasHardEdge(steps.hardEdges, {5, 1}, {6, 1}),
                "terraces emit exact internal risers plus their retaining ring") &&
         expect(first.receipt.accepted &&
                    first.receipt.transitionCellCount > 0U &&
                    first.receipt.weatheredCellCount > 0U &&
                    first.receipt.heightHash == repeated.receipt.heightHash &&
                    first.heightField.heights().size() ==
                        repeated.heightField.heights().size() &&
                    std::equal(first.heightField.heights().begin(),
                               first.heightField.heights().end(),
                               repeated.heightField.heights().begin()),
                "weathered cliff transition is bounded and deterministic") &&
         expect(!first.hardEdges.empty() &&
                    first.hardEdges == repeated.hardEdges &&
                    cr::validateCreativeTerrainHardEdges(first.hardEdges),
                "retaining cliff topology is canonical and deterministic") &&
         expect(softCliff.receipt.accepted && softCliff.hardEdges.empty(),
                "sloped cliff owns no hard seam or outer retaining ring");
}

bool invalidAndCapacityInputsFailClosed() {
  const cr::CreativeTerrainHeightField source = flatField(2U, 2U, 2U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(source);
  cr::CreativeTerrainLandformRecipe invalid;
  invalid.kind = cr::CreativeTerrainLandformKind::Count;
  const cr::CreativeTerrainLandformResult bad =
      cr::buildCreativeTerrainLandform(source, surface, {}, invalid);
  cr::CreativeTerrainLandformRecipe capacity;
  capacity.bounds = {{0, 0}, std::numeric_limits<std::uint16_t>::max(), 2U};
  const cr::CreativeTerrainLandformResult tooLarge =
      cr::buildCreativeTerrainLandform(source, surface, {}, capacity);
  return expect(!bad.receipt.accepted && bad.heightField.cellCount() == 0U &&
                    bad.materialEdits.empty(),
                "invalid enum returns no partial landform output") &&
         expect(!tooLarge.receipt.accepted &&
                    tooLarge.receipt.status ==
                        cr::CreativeTerrainLandformStatus::InvalidRecipe &&
                    tooLarge.heightField.cellCount() == 0U,
                "over-capacity bounds reject before evaluation");
}

bool enumVocabularyRoundTrips() {
  cr::CreativeTerrainLandformKind kind{};
  cr::CreativeTerrainLandformDirection direction{};
  cr::CreativeTerrainLandformEdge edge{};
  cr::CreativeTerrainLandformErosion erosion{};
  return expect(cr::parseCreativeTerrainLandformKind("TERRACE", kind) &&
                    kind == cr::CreativeTerrainLandformKind::Terrace &&
                    cr::parseCreativeTerrainLandformDirection("NEGATIVE_Z",
                                                              direction) &&
                    direction ==
                        cr::CreativeTerrainLandformDirection::NegativeZ &&
                    cr::parseCreativeTerrainLandformEdge("RETAINING", edge) &&
                    edge == cr::CreativeTerrainLandformEdge::Retaining &&
                    cr::parseCreativeTerrainLandformErosion("WEATHERED",
                                                            erosion) &&
                    erosion ==
                        cr::CreativeTerrainLandformErosion::Weathered,
                "landform vocabulary round-trips without ordinal assumptions");
}

}  // namespace

int main() {
  const bool ok = plateauOwnsExactHeightMaterialAndEdgePolicy() &&
                  terraceAndCliffShareOneDirectionalContract() &&
                  invalidAndCapacityInputsFailClosed() &&
                  enumVocabularyRoundTrips();
  if (ok) {
    std::cout << "creative terrain landform tests passed\n";
  }
  return ok ? 0 : 1;
}
