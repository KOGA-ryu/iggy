#include "app/iggy3d/creative/tools/TerrainStamp.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
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

cr::CreativeTerrainHeightField makeHeightField(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::initializer_list<std::uint16_t> heights) {
  cr::CreativeTerrainHeightField field;
  const std::vector<std::uint16_t> values(heights);
  static_cast<void>(field.replace(bounds, values));
  return field;
}

cr::CreativeTerrainHeightField makeHeightField(
    cr::CreativeTerrainHeightFieldBounds bounds,
    const std::vector<std::uint16_t>& heights) {
  cr::CreativeTerrainHeightField field;
  static_cast<void>(field.replace(bounds, heights));
  return field;
}

cr::CreativeTerrainMaterialField makeMaterials(
    std::initializer_list<cr::CreativeTerrainMaterialOverride> values) {
  cr::CreativeTerrainMaterialField field;
  std::vector<cr::CreativeTerrainMaterialEdit> edits;
  edits.reserve(values.size());
  for (const cr::CreativeTerrainMaterialOverride& value : values) {
    edits.push_back(
        cr::makeCreativeTerrainMaterialWeightEdit(value.coord, value.weights));
  }
  if (!edits.empty()) {
    static_cast<void>(field.apply(edits));
  }
  return field;
}

cr::CreativeTerrainStamp captureStamp(
    const cr::CreativeTerrainHeightField& height,
    const cr::CreativeTerrainMaterialField& materials,
    cr::CreativeTerrainCoord2 minimum,
    cr::CreativeTerrainCoord2 maximum,
    std::string_view assetId = "terrain-stamp-1",
    std::string_view label = "Test Stamp") {
  cr::CreativeTerrainStamp stamp;
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(height);
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      17U, 23U, surface, materials, minimum, maximum, assetId, label, 3U,
      stamp));
  return stamp;
}

bool copyCapturesExactHeightMaterialAndHolesTransactionally() {
  const cr::CreativeTerrainHeightField source = makeHeightField(
      {{10, 20}, 3U, 2U}, {2U, 0U, 4U, 5U, 6U, 7U});
  const auto dirt = cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Dirt);
  const auto stone = cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Stone);
  const cr::CreativeTerrainMaterialField materials = makeMaterials(
      {{{10, 20}, dirt}, {{12, 21}, stone}});
  cr::CreativeTerrainStamp stamp;
  const cr::CreativeTerrainStampCopyReceipt copied =
      cr::copyCreativeTerrainRegionToStamp(
          17U, 23U, cr::buildCreativeTerrainHeightSurfacePlan(source),
          materials, {10, 20}, {12, 21}, "terrain-stamp-7", "Rock Shelf",
          4U, stamp);
  const cr::CreativeTerrainStamp preserved = stamp;
  const cr::CreativeTerrainStampCopyReceipt empty =
      cr::copyCreativeTerrainRegionToStamp(
          17U, 23U, cr::buildCreativeTerrainHeightSurfacePlan(source),
          materials, {30, 30}, {31, 31}, "terrain-stamp-8", "Empty", 1U,
          stamp);

  return expect(copied.accepted && copied.copiedCellCount == 6U &&
                    copied.copiedPresentCellCount == 5U &&
                    copied.copiedMaterialCellCount == 2U &&
                    stamp.assetId == "terrain-stamp-7" &&
                    stamp.label == "Rock Shelf" && stamp.assetVersion == 4U &&
                    stamp.sourceDocumentId == 17U &&
                    stamp.sourceRevision == 23U &&
                    stamp.sourceMinimum == cr::CreativeTerrainCoord2{10, 20} &&
                    stamp.widthCells == 3U && stamp.depthCells == 2U &&
                    stamp.minimumHeightCells == 2U &&
                    stamp.heights ==
                        std::vector<std::uint16_t>{2U, 0U, 4U, 5U, 6U, 7U} &&
                    stamp.materials[0U] == dirt &&
                    stamp.materials[1U] ==
                        cr::creativeTerrainMaterialSolidWeights(
                            cr::CreativeTerrainMaterial::Grass) &&
                    stamp.materials[5U] == stone &&
                    stamp.contentSignature != 0U &&
                    cr::isValidCreativeTerrainStamp(stamp),
                "copy captures exact dense heights materials holes and identity") &&
         expect(!empty.accepted &&
                    empty.status ==
                        cr::CreativeTerrainStampCopyStatus::EmptyRegion &&
                    stamp.contentSignature == preserved.contentSignature,
                "rejected copy preserves the previous terrain stamp");
}

bool rotationMirrorAndElevationProduceOneExactCandidate() {
  const cr::CreativeTerrainHeightField source =
      makeHeightField({{0, 0}, 2U, 2U}, {2U, 0U, 6U, 4U});
  const auto sand = cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Sand);
  const cr::CreativeTerrainMaterialField sourceMaterials =
      makeMaterials({{{0, 1}, sand}});
  const cr::CreativeTerrainStamp stamp =
      captureStamp(source, sourceMaterials, {0, 0}, {1, 1});

  const cr::CreativeTerrainHeightField destination =
      makeHeightField({{10, 20}, 3U, 3U},
                      {9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U});
  const cr::CreativeTerrainMaterialField destinationMaterials;
  cr::CreativeTerrainStampRequest request;
  request.stamp = stamp;
  request.targetMinimum = {10, 20};
  request.quarterTurns = 1U;
  request.mirrorX = true;
  request.elevationMode = cr::CreativeTerrainStampElevationMode::Surface;
  request.manualHeightOffsetCells = -1;
  const cr::CreativeTerrainStampPlan plan = cr::buildCreativeTerrainStampPlan(
      destination, destinationMaterials,
      cr::buildCreativeTerrainHeightSurfacePlan(destination), request);

  return expect(plan.accepted &&
                    plan.status == cr::CreativeTerrainStampPlanStatus::Ready &&
                    plan.transformedWidthCells == 2U &&
                    plan.transformedDepthCells == 2U &&
                    plan.targetMaximum == cr::CreativeTerrainCoord2{11, 21} &&
                    plan.targetSurfacePresent &&
                    plan.targetSurfaceHeightCells == 9U &&
                    plan.appliedHeightOffsetCells == 6 &&
                    plan.heightField.heightAt({11, 20}).value() == 9U &&
                    plan.heightField.heightAt({10, 20}).value() == 10U &&
                    plan.heightField.heightAt({11, 21}).value() == 8U &&
                    plan.heightField.heightAt({10, 21}).value() == 12U &&
                    plan.materialField.weightsAt({10, 21}) == sand,
                "rotation mirror surface alignment and material transform stay exact");
}

bool mergePreservesHolesAndReplaceClearsThem() {
  const cr::CreativeTerrainHeightField source =
      makeHeightField({{0, 0}, 2U, 2U}, {4U, 0U, 0U, 7U});
  const auto stone = cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Stone);
  const cr::CreativeTerrainMaterialField sourceMaterials =
      makeMaterials({{{1, 1}, stone}});
  const cr::CreativeTerrainStamp stamp =
      captureStamp(source, sourceMaterials, {0, 0}, {1, 1});
  const cr::CreativeTerrainHeightField destination =
      makeHeightField({{10, 10}, 2U, 2U}, {1U, 2U, 3U, 4U});
  const auto dirt = cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Dirt);
  const cr::CreativeTerrainMaterialField destinationMaterials =
      makeMaterials({{{11, 10}, dirt}, {{10, 11}, dirt}});
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(destination);
  cr::CreativeTerrainStampRequest request;
  request.stamp = stamp;
  request.targetMinimum = {10, 10};
  request.elevationMode = cr::CreativeTerrainStampElevationMode::Absolute;
  const cr::CreativeTerrainStampPlan merged = cr::buildCreativeTerrainStampPlan(
      destination, destinationMaterials, surface, request);
  request.mode = cr::CreativeTerrainStampMode::Replace;
  const cr::CreativeTerrainStampPlan replaced =
      cr::buildCreativeTerrainStampPlan(destination, destinationMaterials,
                                        surface, request);

  return expect(merged.accepted &&
                    merged.heightField.heightAt({10, 10}).value() == 4U &&
                    merged.heightField.heightAt({11, 10}).value() == 2U &&
                    merged.heightField.heightAt({10, 11}).value() == 3U &&
                    merged.heightField.heightAt({11, 11}).value() == 7U &&
                    merged.materialField.weightsAt({11, 10}) == dirt,
                "merge writes present cells and preserves stamp holes") &&
         expect(replaced.accepted &&
                    replaced.heightField.heightAt({10, 10}).value() == 4U &&
                    replaced.heightField.heightAt({11, 10}).value() == 0U &&
                    replaced.heightField.heightAt({10, 11}).value() == 0U &&
                    replaced.heightField.heightAt({11, 11}).value() == 7U &&
                    replaced.materialField.weightsAt({11, 10}) ==
                        cr::creativeTerrainMaterialSolidWeights(
                            cr::CreativeTerrainMaterial::Grass) &&
                    replaced.materialField.weightsAt({11, 11}) == stone,
                "replace clears height and material at baked stamp holes");
}

bool invalidOverflowAndCapacityRequestsFailAtomically() {
  const cr::CreativeTerrainHeightField source =
      makeHeightField({{0, 0}, 2U, 1U}, {1U, 64U});
  cr::CreativeTerrainStamp stamp =
      captureStamp(source, {}, {0, 0}, {1, 0});
  const cr::CreativeTerrainHeightField destination =
      makeHeightField({{0, 0}, 1U, 1U}, {4U});
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(destination);
  cr::CreativeTerrainStampRequest request;
  request.stamp = stamp;
  request.targetMinimum = {std::numeric_limits<std::int32_t>::max(), 0};
  const cr::CreativeTerrainStampPlan coordinateOverflow =
      cr::buildCreativeTerrainStampPlan(destination, {}, surface, request);
  request.targetMinimum = {};
  request.quarterTurns = 4U;
  const cr::CreativeTerrainStampPlan invalidRotation =
      cr::buildCreativeTerrainStampPlan(destination, {}, surface, request);
  request.quarterTurns = 0U;
  request.manualHeightOffsetCells = 1;
  const cr::CreativeTerrainStampPlan heightOverflow =
      cr::buildCreativeTerrainStampPlan(destination, {}, surface, request);
  stamp.contentSignature ^= 1U;
  request.stamp = stamp;
  request.manualHeightOffsetCells = 0;
  const cr::CreativeTerrainStampPlan corrupt =
      cr::buildCreativeTerrainStampPlan(destination, {}, surface, request);

  const cr::CreativeTerrainHeightField huge = makeHeightField(
      {{0, 0}, 128U, 64U}, std::vector<std::uint16_t>(8192U, 4U));
  request.stamp = captureStamp(source, {}, {0, 0}, {1, 0});
  request.targetMinimum = {1000, 1000};
  const cr::CreativeTerrainStampPlan unionCapacity =
      cr::buildCreativeTerrainStampPlan(
          huge, {}, cr::buildCreativeTerrainHeightSurfacePlan(huge), request);

  return expect(!coordinateOverflow.accepted &&
                    coordinateOverflow.status ==
                        cr::CreativeTerrainStampPlanStatus::CoordinateOverflow,
                "coordinate overflow rejects before output") &&
         expect(!invalidRotation.accepted &&
                    invalidRotation.status ==
                        cr::CreativeTerrainStampPlanStatus::InvalidRequest,
                "invalid transform rejects before output") &&
         expect(!heightOverflow.accepted &&
                    heightOverflow.status ==
                        cr::CreativeTerrainStampPlanStatus::HeightOutOfRange,
                "height overflow rejects atomically") &&
         expect(!corrupt.accepted &&
                    corrupt.status ==
                        cr::CreativeTerrainStampPlanStatus::InvalidStamp,
                "content corruption rejects atomically") &&
         expect(!unionCapacity.accepted &&
                    unionCapacity.status ==
                        cr::CreativeTerrainStampPlanStatus::CapacityExceeded,
                "output union beyond terrain capacity rejects atomically");
}

}  // namespace

int main() {
  bool ok = true;
  ok = copyCapturesExactHeightMaterialAndHolesTransactionally() && ok;
  ok = rotationMirrorAndElevationProduceOneExactCandidate() && ok;
  ok = mergePreservesHolesAndReplaceClearsThem() && ok;
  ok = invalidOverflowAndCapacityRequestsFailAtomically() && ok;
  return ok ? 0 : 1;
}
