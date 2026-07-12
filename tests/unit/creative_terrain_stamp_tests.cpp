#include "app/iggy3d/creative/tools/TerrainStamp.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const cr::CreativeTerrainControlPoint* controlAt(
    const cr::CreativeTerrainStampPlan& plan,
    cr::CreativeTerrainCoord2 coord) {
  for (const cr::CreativeTerrainControlPoint& control : plan.controls()) {
    if (control.coord == coord) {
      return &control;
    }
  }
  return nullptr;
}

bool copyIsRelativeBoundedAndTransactional() {
  constexpr std::array controls{
      cr::CreativeTerrainControlPoint{{9, 19}, 1U, 1U},
      cr::CreativeTerrainControlPoint{{10, 20}, 2U, 2U},
      cr::CreativeTerrainControlPoint{{11, 20}, 3U, 3U},
      cr::CreativeTerrainControlPoint{{10, 22}, 4U, 4U},
  };
  cr::CreativeTerrainStamp stamp;
  const cr::CreativeTerrainStampCopyReceipt copied =
      cr::copyCreativeTerrainRegionToStamp(17U, 23U, controls, {10, 20},
                                           {11, 22}, stamp);
  const cr::CreativeTerrainStamp preserved = stamp;
  const cr::CreativeTerrainStampCopyReceipt empty =
      cr::copyCreativeTerrainRegionToStamp(17U, 23U, controls, {30, 30},
                                           {31, 31}, stamp);

  return expect(copied.accepted && copied.copiedControlCount == 3U &&
                    stamp.controlCount == preserved.controlCount &&
                    preserved.sourceDocumentId == 17U &&
                    preserved.sourceRevision == 23U &&
                    preserved.sourceMinimum == cr::CreativeTerrainCoord2{10, 20} &&
                    preserved.widthCells == 2U && preserved.depthCells == 3U &&
                    preserved.items()[0].coord ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    preserved.items()[1].coord ==
                        cr::CreativeTerrainCoord2{1, 0} &&
                    preserved.items()[2].coord ==
                        cr::CreativeTerrainCoord2{0, 2} &&
                    preserved.contentSignature != 0U &&
                    cr::isValidCreativeTerrainStamp(preserved),
                "copy stores canonical source-local rods and stable identity") &&
         expect(!empty.accepted &&
                    empty.status ==
                        cr::CreativeTerrainStampCopyStatus::EmptyRegion &&
                    stamp.contentSignature == preserved.contentSignature,
                "rejected copy preserves the previous terrain clipboard");
}

bool copyRejectsInvalidInputsWithoutClobberingClipboard() {
  constexpr std::array valid{
      cr::CreativeTerrainControlPoint{{0, 0}, 4U, 2U},
  };
  cr::CreativeTerrainStamp stamp;
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      2U, 3U, valid, {0, 0}, {0, 0}, stamp));
  const cr::CreativeTerrainStamp preserved = stamp;
  constexpr std::array unsorted{
      cr::CreativeTerrainControlPoint{{1, 0}, 4U, 2U},
      cr::CreativeTerrainControlPoint{{0, 0}, 4U, 2U},
  };
  const cr::CreativeTerrainStampCopyReceipt invalidSource =
      cr::copyCreativeTerrainRegionToStamp(2U, 3U, unsorted, {0, 0},
                                           {1, 0}, stamp);
  const cr::CreativeTerrainStampCopyReceipt reversedBounds =
      cr::copyCreativeTerrainRegionToStamp(2U, 3U, valid, {1, 0},
                                           {0, 0}, stamp);
  const cr::CreativeTerrainStampCopyReceipt oversizedBounds =
      cr::copyCreativeTerrainRegionToStamp(
          2U, 3U, valid,
          {std::numeric_limits<std::int32_t>::min(), 0}, {-1, 0}, stamp);

  return expect(!invalidSource.accepted &&
                    invalidSource.status ==
                        cr::CreativeTerrainStampCopyStatus::InvalidSource,
                "copy rejects non-canonical source controls") &&
         expect(!reversedBounds.accepted && !oversizedBounds.accepted &&
                    reversedBounds.status ==
                        cr::CreativeTerrainStampCopyStatus::InvalidBounds &&
                    oversizedBounds.status ==
                        cr::CreativeTerrainStampCopyStatus::InvalidBounds,
                "copy rejects reversed and unrenderable footprint bounds") &&
         expect(stamp.contentSignature == preserved.contentSignature &&
                    stamp.controlCount == preserved.controlCount,
                "every rejected copy preserves the previous clipboard");
}

bool rotationAndMirrorsNormalizeTheFootprint() {
  constexpr std::array controls{
      cr::CreativeTerrainControlPoint{{10, 20}, 2U, 2U},
      cr::CreativeTerrainControlPoint{{11, 20}, 3U, 3U},
      cr::CreativeTerrainControlPoint{{10, 22}, 4U, 4U},
  };
  cr::CreativeTerrainStamp stamp;
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      1U, 1U, controls, {10, 20}, {11, 22}, stamp));
  cr::CreativeTerrainStampRequest request;
  request.stamp = &stamp;
  request.targetMinimum = {100, 200};
  request.quarterTurns = 1U;
  const cr::CreativeTerrainStampPlan rotated =
      cr::buildCreativeTerrainStampPlan(request);

  request.quarterTurns = 0U;
  request.mirrorX = true;
  request.mirrorZ = true;
  const cr::CreativeTerrainStampPlan mirrored =
      cr::buildCreativeTerrainStampPlan(request);

  return expect(rotated.accepted && rotated.finalControlCount == 3U &&
                    rotated.editCount == 3U &&
                    rotated.transformedWidthCells == 3U &&
                    rotated.transformedDepthCells == 2U &&
                    rotated.targetMaximum ==
                        cr::CreativeTerrainCoord2{102, 201} &&
                    controlAt(rotated, {102, 200})->heightCells == 2U &&
                    controlAt(rotated, {102, 201})->heightCells == 3U &&
                    controlAt(rotated, {100, 200})->heightCells == 4U,
                "clockwise rotation swaps dimensions and keeps target at min") &&
         expect(mirrored.accepted &&
                    controlAt(mirrored, {101, 202})->heightCells == 2U &&
                    controlAt(mirrored, {100, 202})->heightCells == 3U &&
                    controlAt(mirrored, {101, 200})->heightCells == 4U,
                "both mirrors remain exact integer-lattice transforms");
}

bool mergeAndReplaceHaveDistinctAtomicSemantics() {
  constexpr std::array source{
      cr::CreativeTerrainControlPoint{{0, 0}, 5U, 2U},
      cr::CreativeTerrainControlPoint{{1, 1}, 7U, 3U},
  };
  cr::CreativeTerrainStamp stamp;
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      1U, 1U, source, {0, 0}, {1, 1}, stamp));
  constexpr std::array destination{
      cr::CreativeTerrainControlPoint{{10, 10}, 1U, 1U},
      cr::CreativeTerrainControlPoint{{11, 10}, 9U, 1U},
      cr::CreativeTerrainControlPoint{{20, 20}, 8U, 2U},
  };
  cr::CreativeTerrainStampRequest request;
  request.stamp = &stamp;
  request.destinationControls = destination;
  request.targetMinimum = {10, 10};
  const cr::CreativeTerrainStampPlan merged =
      cr::buildCreativeTerrainStampPlan(request);
  request.mode = cr::CreativeTerrainStampMode::Replace;
  const cr::CreativeTerrainStampPlan replaced =
      cr::buildCreativeTerrainStampPlan(request);

  cr::CreativeTerrainField field;
  std::array<cr::CreativeTerrainControlEdit, destination.size()> seed{};
  for (std::size_t index = 0U; index < destination.size(); ++index) {
    seed[index] = {cr::CreativeTerrainEditKind::Upsert, destination[index]};
  }
  static_cast<void>(field.apply(seed));
  const cr::CreativeTerrainMutationReceipt applied = field.apply(replaced.items());
  return expect(merged.accepted && merged.editCount == 2U &&
                    merged.insertedControlCount == 1U &&
                    merged.updatedControlCount == 1U &&
                    merged.removedControlCount == 0U,
                "merge updates stamp coordinates and preserves footprint extras") &&
         expect(replaced.accepted && replaced.editCount == 3U &&
                    replaced.insertedControlCount == 1U &&
                    replaced.updatedControlCount == 1U &&
                    replaced.removedControlCount == 1U && applied.accepted &&
                    applied.changed && field.controlAt({10, 10}) != nullptr &&
                    field.controlAt({10, 10})->heightCells == 5U &&
                    field.controlAt({11, 10}) == nullptr &&
                    field.controlAt({11, 11}) != nullptr &&
                    field.controlAt({20, 20}) != nullptr,
                "replace removes only destination-only rods inside its footprint");
}

bool exactEditCeilingAndFinalCapacityFailClosed() {
  std::array<cr::CreativeTerrainControlPoint,
             cr::kCreativeTerrainControlCapacity>
      source{};
  std::array<cr::CreativeTerrainControlPoint,
             cr::kCreativeTerrainControlCapacity>
      destination{};
  for (std::size_t index = 0U; index < source.size(); ++index) {
    source[index] = {{static_cast<std::int32_t>(index * 2U + 1U), 0}, 4U, 1U};
    destination[index] = {
        {static_cast<std::int32_t>(index * 2U), 0}, 3U, 1U};
  }
  cr::CreativeTerrainStamp stamp;
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      1U, 1U, source, {0, 0}, {511, 0}, stamp));
  cr::CreativeTerrainStampRequest request;
  request.stamp = &stamp;
  request.destinationControls = destination;
  request.mode = cr::CreativeTerrainStampMode::Replace;
  const cr::CreativeTerrainStampPlan exact =
      cr::buildCreativeTerrainStampPlan(request);

  cr::CreativeTerrainStamp one;
  constexpr std::array oneSource{
      cr::CreativeTerrainControlPoint{{0, 0}, 4U, 1U},
  };
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      1U, 1U, oneSource, {0, 0}, {0, 0}, one));
  request.stamp = &one;
  request.mode = cr::CreativeTerrainStampMode::Merge;
  request.targetMinimum = {1000, 0};
  const cr::CreativeTerrainStampPlan overflow =
      cr::buildCreativeTerrainStampPlan(request);

  return expect(exact.accepted &&
                    exact.editCount == cr::kCreativeTerrainStampEditCapacity &&
                    exact.insertedControlCount ==
                        cr::kCreativeTerrainControlCapacity &&
                    exact.removedControlCount ==
                        cr::kCreativeTerrainControlCapacity,
                "replace supports the exact 512-edit remove-plus-upsert ceiling") &&
         expect(!overflow.accepted && overflow.editCount == 0U &&
                    overflow.status ==
                        cr::CreativeTerrainStampPlanStatus::CapacityExceeded,
                "merge rejects a 257th final rod without partial edits");
}

bool invalidAndOverflowRequestsDoNotProducePlans() {
  constexpr std::array source{
      cr::CreativeTerrainControlPoint{{0, 0}, 4U, 1U},
  };
  cr::CreativeTerrainStamp stamp;
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      1U, 1U, source, {0, 0}, {1, 0}, stamp));
  cr::CreativeTerrainStampRequest request;
  request.stamp = &stamp;
  request.targetMinimum = {std::numeric_limits<std::int32_t>::max(), 0};
  const cr::CreativeTerrainStampPlan overflow =
      cr::buildCreativeTerrainStampPlan(request);
  cr::CreativeTerrainStamp singleCell;
  static_cast<void>(cr::copyCreativeTerrainRegionToStamp(
      1U, 1U, source, {0, 0}, {0, 0}, singleCell));
  request.stamp = &singleCell;
  const cr::CreativeTerrainStampPlan exclusiveBoundsOverflow =
      cr::buildCreativeTerrainStampPlan(request);
  request.stamp = &stamp;
  request.targetMinimum = {};
  request.quarterTurns = 4U;
  const cr::CreativeTerrainStampPlan invalid =
      cr::buildCreativeTerrainStampPlan(request);
  stamp.contentSignature ^= 1U;
  request.quarterTurns = 0U;
  const cr::CreativeTerrainStampPlan corrupt =
      cr::buildCreativeTerrainStampPlan(request);

  return expect(!overflow.accepted && overflow.editCount == 0U &&
                    overflow.status ==
                        cr::CreativeTerrainStampPlanStatus::CoordinateOverflow,
                "transformed footprint overflow rejects before output") &&
         expect(!invalid.accepted && invalid.editCount == 0U &&
                    invalid.status ==
                        cr::CreativeTerrainStampPlanStatus::InvalidRequest,
                "invalid rotation rejects before output") &&
         expect(!exclusiveBoundsOverflow.accepted &&
                    exclusiveBoundsOverflow.editCount == 0U &&
                    exclusiveBoundsOverflow.status ==
                        cr::CreativeTerrainStampPlanStatus::CoordinateOverflow,
                "exclusive render bound overflow rejects a max-coordinate cell") &&
         expect(!corrupt.accepted && corrupt.editCount == 0U &&
                    corrupt.status ==
                        cr::CreativeTerrainStampPlanStatus::InvalidStamp,
                "stamp signature corruption fails closed");
}

}  // namespace

int main() {
  bool ok = true;
  ok = copyIsRelativeBoundedAndTransactional() && ok;
  ok = copyRejectsInvalidInputsWithoutClobberingClipboard() && ok;
  ok = rotationAndMirrorsNormalizeTheFootprint() && ok;
  ok = mergeAndReplaceHaveDistinctAtomicSemantics() && ok;
  ok = exactEditCeilingAndFinalCapacityFailClosed() && ok;
  ok = invalidAndOverflowRequestsDoNotProducePlans() && ok;
  return ok ? 0 : 1;
}
