#include "app/iggy3d/creative/tools/TerrainRegion.hpp"

#include <array>
#include <cstdint>
#include <iostream>
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

const cr::CreativeTerrainControlEdit* editAt(
    const cr::CreativeTerrainRegionPlan& plan,
    cr::CreativeTerrainCoord2 coord) {
  for (const cr::CreativeTerrainControlEdit& edit : plan.items()) {
    if (edit.control.coord == coord) {
      return &edit;
    }
  }
  return nullptr;
}

bool optionsAndLabelsAreClosed() {
  return expect(cr::creativeTerrainRegionAmountCells(
                    cr::CreativeTerrainRegionAmount::EightCells) == 8U,
                "region amount resolves to bounded cells") &&
         expect(cr::toString(cr::CreativeTerrainRegionOperation::Raise) ==
                        "RAISE" &&
                    cr::toString(cr::CreativeTerrainRegionOperation::Erase) ==
                        "ERASE" &&
                    cr::toString(cr::CreativeTerrainRegionAmount::TwoCells) ==
                        "2 CELLS" &&
                    cr::toString(cr::CreativeTerrainRegionOperation::Count) ==
                        "INVALID",
                "region option enums expose explicit labels") &&
         expect(cr::creativeTerrainRegionUsesAmount(
                    cr::CreativeTerrainRegionOperation::Smooth) &&
                    !cr::creativeTerrainRegionUsesAmount(
                        cr::CreativeTerrainRegionOperation::Flatten) &&
                    cr::creativeTerrainRegionUsesTargetHeight(
                        cr::CreativeTerrainRegionOperation::Flatten),
                "operation-dependent settings are explicit");
}

bool raiseLowerAndBoundsAreDeterministic() {
  constexpr std::array controls{
      cr::CreativeTerrainControlPoint{{-1, 0}, 1U, 1U},
      cr::CreativeTerrainControlPoint{{0, 0}, 4U, 2U},
      cr::CreativeTerrainControlPoint{{2, 0}, 63U, 3U},
      cr::CreativeTerrainControlPoint{{0, 2}, 8U, 2U},
  };
  cr::CreativeTerrainRegionRequest request;
  request.controls = controls;
  request.minimumCoord = {0, 0};
  request.maximumCoord = {2, 0};
  request.operation = cr::CreativeTerrainRegionOperation::Raise;
  request.amountCells = 4U;
  const cr::CreativeTerrainRegionPlan raised =
      cr::buildCreativeTerrainRegionPlan(request);
  request.operation = cr::CreativeTerrainRegionOperation::Lower;
  request.amountCells = 8U;
  const cr::CreativeTerrainRegionPlan lowered =
      cr::buildCreativeTerrainRegionPlan(request);

  return expect(raised.accepted && raised.affectedControlCount == 2U &&
                    raised.editCount == 2U &&
                    editAt(raised, {0, 0})->control.heightCells == 8U &&
                    editAt(raised, {2, 0})->control.heightCells == 64U,
                "raise edits only inclusive XZ members and clamps high") &&
         expect(lowered.accepted && lowered.editCount == 2U &&
                    editAt(lowered, {0, 0})->control.heightCells == 1U &&
                    editAt(lowered, {2, 0})->control.heightCells == 55U,
                "lower preserves order and clamps low") &&
         expect(editAt(raised, {-1, 0}) == nullptr &&
                    editAt(raised, {0, 2}) == nullptr,
                "controls outside either region axis remain untouched");
}

bool flattenSmoothAndEraseUseOneSnapshot() {
  constexpr std::array controls{
      cr::CreativeTerrainControlPoint{{0, 0}, 2U, 2U},
      cr::CreativeTerrainControlPoint{{1, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{2, 0}, 20U, 2U},
  };
  cr::CreativeTerrainRegionRequest request;
  request.controls = controls;
  request.minimumCoord = {0, 0};
  request.maximumCoord = {1, 0};
  request.operation = cr::CreativeTerrainRegionOperation::Flatten;
  request.targetHeightCells = 7U;
  const cr::CreativeTerrainRegionPlan flattened =
      cr::buildCreativeTerrainRegionPlan(request);

  request.operation = cr::CreativeTerrainRegionOperation::Smooth;
  request.amountCells = 8U;
  const cr::CreativeTerrainRegionPlan smoothed =
      cr::buildCreativeTerrainRegionPlan(request);

  request.operation = cr::CreativeTerrainRegionOperation::Erase;
  const cr::CreativeTerrainRegionPlan erased =
      cr::buildCreativeTerrainRegionPlan(request);

  return expect(flattened.accepted && flattened.editCount == 2U &&
                    editAt(flattened, {0, 0})->control.heightCells == 7U &&
                    editAt(flattened, {1, 0})->control.heightCells == 7U,
                "flatten sets every selected rod to the exact target") &&
         expect(smoothed.accepted && smoothed.editCount == 2U &&
                    editAt(smoothed, {0, 0})->control.heightCells == 10U &&
                    editAt(smoothed, {1, 0})->control.heightCells == 11U,
                "smooth reads selected and surrounding rods from one snapshot") &&
         expect(erased.accepted && erased.editCount == 2U &&
                    editAt(erased, {0, 0})->kind ==
                        cr::CreativeTerrainEditKind::Remove &&
                    editAt(erased, {1, 0})->kind ==
                        cr::CreativeTerrainEditKind::Remove,
                "erase emits one canonical remove per selected rod");
}

bool noChangeEmptyAndInvalidPlansFailSafely() {
  constexpr std::array controls{
      cr::CreativeTerrainControlPoint{{0, 0}, 64U, 1U},
      cr::CreativeTerrainControlPoint{{2, 0}, 4U, 1U},
  };
  cr::CreativeTerrainRegionRequest request;
  request.controls = controls;
  request.minimumCoord = {0, 0};
  request.maximumCoord = {0, 0};
  request.operation = cr::CreativeTerrainRegionOperation::Raise;
  request.amountCells = 1U;
  const cr::CreativeTerrainRegionPlan noChange =
      cr::buildCreativeTerrainRegionPlan(request);

  request.minimumCoord = {8, 8};
  request.maximumCoord = {9, 9};
  const cr::CreativeTerrainRegionPlan empty =
      cr::buildCreativeTerrainRegionPlan(request);

  request.minimumCoord = {2, 0};
  request.maximumCoord = {1, 0};
  const cr::CreativeTerrainRegionPlan invalidBounds =
      cr::buildCreativeTerrainRegionPlan(request);
  request.minimumCoord = {0, 0};
  request.maximumCoord = {2, 0};
  request.operation = cr::CreativeTerrainRegionOperation::Count;
  const cr::CreativeTerrainRegionPlan invalidOperation =
      cr::buildCreativeTerrainRegionPlan(request);

  return expect(noChange.accepted && noChange.items().empty() &&
                    noChange.status ==
                        cr::CreativeTerrainRegionPlanStatus::NoChange,
                "clamped region edit is an accepted no-op") &&
         expect(!empty.accepted && empty.items().empty() &&
                    empty.status ==
                        cr::CreativeTerrainRegionPlanStatus::NoControlsInRegion,
                "empty region does not fabricate edits") &&
         expect(!invalidBounds.accepted && invalidBounds.items().empty() &&
                    invalidBounds.status ==
                        cr::CreativeTerrainRegionPlanStatus::InvalidRequest,
                "reversed bounds reject atomically") &&
         expect(!invalidOperation.accepted &&
                    invalidOperation.items().empty(),
                "invalid operation rejects with no partial output");
}

bool fixedCapacityBoundaryIsExact() {
  std::array<cr::CreativeTerrainControlPoint,
             cr::kCreativeTerrainControlCapacity>
      controls{};
  for (std::size_t index = 0U; index < controls.size(); ++index) {
    controls[index] = {{static_cast<std::int32_t>(index), 0}, 4U, 1U};
  }
  cr::CreativeTerrainRegionRequest request;
  request.controls = controls;
  request.minimumCoord = {0, 0};
  request.maximumCoord = {
      static_cast<std::int32_t>(cr::kCreativeTerrainControlCapacity - 1U), 0};
  request.operation = cr::CreativeTerrainRegionOperation::Erase;
  const cr::CreativeTerrainRegionPlan exact =
      cr::buildCreativeTerrainRegionPlan(request);

  std::array<cr::CreativeTerrainControlPoint,
             cr::kCreativeTerrainControlCapacity + 1U>
      oversized{};
  for (std::size_t index = 0U; index < oversized.size(); ++index) {
    oversized[index] = {{static_cast<std::int32_t>(index), 0}, 4U, 1U};
  }
  request.controls = oversized;
  request.maximumCoord.x = static_cast<std::int32_t>(oversized.size() - 1U);
  const cr::CreativeTerrainRegionPlan rejected =
      cr::buildCreativeTerrainRegionPlan(request);

  return expect(exact.accepted &&
                    exact.affectedControlCount ==
                        cr::kCreativeTerrainControlCapacity &&
                    exact.editCount == cr::kCreativeTerrainControlCapacity,
                "all 256 canonical rods fit one atomic region plan") &&
         expect(!rejected.accepted && rejected.editCount == 0U &&
                    rejected.status ==
                        cr::CreativeTerrainRegionPlanStatus::InvalidRequest,
                "a 257th source rod rejects before emitting edits");
}

}  // namespace

int main() {
  bool ok = true;
  ok = optionsAndLabelsAreClosed() && ok;
  ok = raiseLowerAndBoundsAreDeterministic() && ok;
  ok = flattenSmoothAndEraseUseOneSnapshot() && ok;
  ok = noChangeEmptyAndInvalidPlansFailSafely() && ok;
  ok = fixedCapacityBoundaryIsExact() && ok;
  return ok ? 0 : 1;
}
