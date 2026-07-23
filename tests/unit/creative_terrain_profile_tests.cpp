#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"
#include "app/iggy3d/creative/tools/TerrainProfile.hpp"

#include <algorithm>
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

const cr::CreativeTerrainControlEdit* editAt(
    const cr::CreativeTerrainProfilePlan& plan,
    cr::CreativeTerrainCoord2 coord) {
  const auto found = std::find_if(
      plan.items().begin(), plan.items().end(), [coord](const auto& edit) {
        return edit.control.coord == coord;
      });
  return found == plan.items().end() ? nullptr : &*found;
}

std::uint16_t plannedHeight(const cr::CreativeTerrainProfilePlan& plan,
                            cr::CreativeTerrainCoord2 coord) {
  const cr::CreativeTerrainControlEdit* edit = editAt(plan, coord);
  return edit == nullptr ? 0U : edit->control.heightCells;
}

cr::CreativeTerrainProfileRequest requestFor(
    const cr::CreativeTerrainField& field,
    cr::CreativeTerrainProfileKind profile) {
  cr::CreativeTerrainProfileRequest request;
  request.field = &field;
  request.baseHeightCells = 20U;
  request.profile = profile;
  request.radiusCells = 4U;
  request.amplitudeCells = 8U;
  request.spacingCells = 1U;
  return request;
}

bool fixedPointKernelIsPinned() {
  bool monotonic = true;
  std::int32_t previous = 0;
  for (std::int64_t phase = 0; phase <= 16'384; phase += 256) {
    const std::int32_t value = cr::creativeTerrainSinTurnsQ15(phase);
    monotonic = monotonic && value >= previous;
    previous = value;
  }
  return expect(cr::creativeTerrainIntegerSquareRoot(0U) == 0U &&
                    cr::creativeTerrainIntegerSquareRoot(15U) == 3U &&
                    cr::creativeTerrainIntegerSquareRoot(16U) == 4U,
                "integer square root is floor deterministic") &&
         expect(cr::creativeTerrainRoundDivideSymmetric(5, 2) == 3 &&
                    cr::creativeTerrainRoundDivideSymmetric(-5, 2) == -3 &&
                    cr::creativeTerrainMultiplyQ15(
                        cr::kCreativeTerrainQ15One,
                        cr::kCreativeTerrainQ15One) ==
                        cr::kCreativeTerrainQ15One,
                "signed fixed point rounding is symmetric") &&
         expect(cr::creativeTerrainSinTurnsQ15(0) == 0 &&
                    cr::creativeTerrainSinTurnsQ15(16'384) == 32'767 &&
                    cr::creativeTerrainSinTurnsQ15(32'768) == 0 &&
                    cr::creativeTerrainSinTurnsQ15(49'152) == -32'767 &&
                    cr::creativeTerrainSinTurnsQ15(65'536) == 0 &&
                    cr::creativeTerrainSinTurnsQ15(-16'384) == -32'767,
                "sine anchors periodicity and negative phase are exact") &&
         expect(cr::creativeTerrainCosTurnsQ15(0) == 32'767 &&
                    cr::creativeTerrainCosTurnsQ15(32'768) == -32'767 &&
                    cr::creativeTerrainSinTurnsQ15(256) == 804 && monotonic,
                "cosine offset and quarter table are pinned") &&
         expect(cr::creativeTerrainQuarterWaveChecksum() == 56'797'365U,
                "quarter wave table checksum is pinned") &&
         expect(cr::creativeTerrainCosineBellQ15(0U) == 32'767 &&
                    cr::creativeTerrainCosineBellQ15(32'768U) == 16'384 &&
                    cr::creativeTerrainCosineBellQ15(65'536U) == 0,
                "cosine bell endpoints and midpoint are exact") &&
         expect(cr::creativeTerrainNormalizedDistanceQ16(
                    {0, 0}, {3, 4}, 5U) == 65'536U &&
                    cr::creativeTerrainNormalizedDistanceQ16(
                        {0, 0}, {1, 0}, 2U) == 32'768U,
                "normalized radial distance uses Q16 geometry");
}

bool profileValuesAndNamesAreClosed() {
  return expect(cr::creativeTerrainProfileRadiusCells(
                    cr::CreativeTerrainProfileRadius::EightCells) == 8U &&
                    cr::creativeTerrainProfileAmplitudeCells(
                        cr::CreativeTerrainProfileAmplitude::SixteenCells) ==
                        16U &&
                    cr::creativeTerrainProfileSpacingCells(
                        cr::CreativeTerrainProfileSpacing::FourCells) == 4U &&
                    cr::creativeTerrainProfileFrequencyCycles(
                        cr::CreativeTerrainProfileFrequency::TwoCycles) == 2U,
                "profile option enums resolve to bounded values") &&
         expect(cr::toString(cr::CreativeTerrainProfileKind::Crater) ==
                        "CRATER" &&
                    cr::toString(cr::CreativeTerrainProfileBlend::Add) ==
                        "ADD" &&
                    cr::toString(
                        cr::CreativeTerrainProfileRodPolicy::Existing) ==
                        "EXISTING" &&
                    cr::toString(
                        cr::CreativeTerrainProfileDirection::PositiveXNegativeZ) ==
                        "+X -Z" &&
                    cr::toString(cr::CreativeTerrainProfileKind::Count) ==
                        "INVALID",
                "profile enums have explicit creator labels") &&
         expect(cr::creativeTerrainProfileUsesDirection(
                    cr::CreativeTerrainProfileKind::Ridge) &&
                    cr::creativeTerrainProfileUsesDirection(
                        cr::CreativeTerrainProfileKind::Wave) &&
                    !cr::creativeTerrainProfileUsesDirection(
                        cr::CreativeTerrainProfileKind::Hill) &&
                    cr::creativeTerrainProfileUsesFrequency(
                        cr::CreativeTerrainProfileKind::Ripple),
                "conditional option predicates are explicit");
}

bool hillFillIsCanonicalAndSetIsIdempotent() {
  cr::CreativeTerrainField field;
  cr::CreativeTerrainProfileRequest request =
      requestFor(field, cr::CreativeTerrainProfileKind::Hill);
  request.radiusCells = 2U;
  const cr::CreativeTerrainProfilePlan first =
      cr::buildCreativeTerrainProfilePlan(request);
  bool canonical = true;
  for (std::size_t index = 1U; index < first.items().size(); ++index) {
    const auto before = first.items()[index - 1U].control.coord;
    const auto after = first.items()[index].control.coord;
    canonical = canonical &&
                (before.z < after.z ||
                 (before.z == after.z && before.x < after.x));
  }
  const auto* center = editAt(first, {0, 0});
  const auto* shoulder = editAt(first, {1, 0});
  const auto* edge = editAt(first, {2, 0});
  const cr::CreativeTerrainMutationReceipt applied = field.apply(first.items());
  request.field = &field;
  const cr::CreativeTerrainProfilePlan repeated =
      cr::buildCreativeTerrainProfilePlan(request);
  return expect(first.accepted &&
                    first.status == cr::CreativeTerrainProfilePlanStatus::Ready &&
                    first.candidateCount == 13U && first.editCount == 13U &&
                    canonical,
                "hill fill emits canonical bounded disk") &&
         expect(center != nullptr && center->control.heightCells == 28U &&
                    center->control.radiusCells == 2U && shoulder != nullptr &&
                    shoulder->control.heightCells == 24U && edge != nullptr &&
                    edge->control.heightCells == 20U,
                "hill center shoulder edge match cosine bell") &&
         expect(applied.changed && field.controlCount() == 13U,
                "hill plan applies as one terrain batch") &&
         expect(repeated.accepted && repeated.items().empty() &&
                    repeated.status ==
                        cr::CreativeTerrainProfilePlanStatus::NoChange,
                "set blend is idempotent");
}

bool radialProfilesPinCreatorShapes() {
  cr::CreativeTerrainField field;
  cr::CreativeTerrainProfileRequest basinRequest =
      requestFor(field, cr::CreativeTerrainProfileKind::Basin);
  const cr::CreativeTerrainProfilePlan basin =
      cr::buildCreativeTerrainProfilePlan(basinRequest);
  cr::CreativeTerrainProfileRequest ringRequest =
      requestFor(field, cr::CreativeTerrainProfileKind::Ring);
  const cr::CreativeTerrainProfilePlan ring =
      cr::buildCreativeTerrainProfilePlan(ringRequest);
  cr::CreativeTerrainProfileRequest craterRequest =
      requestFor(field, cr::CreativeTerrainProfileKind::Crater);
  craterRequest.radiusCells = 8U;
  const cr::CreativeTerrainProfilePlan crater =
      cr::buildCreativeTerrainProfilePlan(craterRequest);
  cr::CreativeTerrainProfileRequest rippleRequest =
      requestFor(field, cr::CreativeTerrainProfileKind::Ripple);
  const cr::CreativeTerrainProfilePlan ripple =
      cr::buildCreativeTerrainProfilePlan(rippleRequest);
  return expect(plannedHeight(basin, {0, 0}) == 12U &&
                    plannedHeight(basin, {4, 0}) == 20U,
                "basin depresses center and preserves boundary") &&
         expect(plannedHeight(ring, {0, 0}) == 20U &&
                    plannedHeight(ring, {2, 0}) == 28U &&
                    plannedHeight(ring, {4, 0}) == 20U,
                "ring raises annulus with zero center and edge") &&
         expect(plannedHeight(crater, {0, 0}) == 12U &&
                    plannedHeight(crater, {6, 0}) > 20U &&
                    plannedHeight(crater, {8, 0}) == 20U,
                "crater has depressed bowl raised rim and flat edge") &&
         expect(plannedHeight(ripple, {0, 0}) == 28U &&
                    plannedHeight(ripple, {2, 0}) < 20U &&
                    plannedHeight(ripple, {4, 0}) == 20U,
                "ripple alternates radially inside a fading envelope");
}

bool directionAndBlendSemanticsArePinned() {
  cr::CreativeTerrainField empty;
  cr::CreativeTerrainProfileRequest ridgeRequest =
      requestFor(empty, cr::CreativeTerrainProfileKind::Ridge);
  const cr::CreativeTerrainProfilePlan ridge =
      cr::buildCreativeTerrainProfilePlan(ridgeRequest);
  cr::CreativeTerrainProfileRequest waveRequest =
      requestFor(empty, cr::CreativeTerrainProfileKind::Wave);
  const cr::CreativeTerrainProfilePlan wave =
      cr::buildCreativeTerrainProfilePlan(waveRequest);

  cr::CreativeTerrainField additive;
  const cr::CreativeTerrainControlEdit initial{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 20U, 3U}};
  static_cast<void>(additive.apply(std::span{&initial, 1U}));
  cr::CreativeTerrainProfileRequest addRequest =
      requestFor(additive, cr::CreativeTerrainProfileKind::Hill);
  addRequest.blend = cr::CreativeTerrainProfileBlend::Add;
  addRequest.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Existing;
  const cr::CreativeTerrainProfilePlan firstAdd =
      cr::buildCreativeTerrainProfilePlan(addRequest);
  static_cast<void>(additive.apply(firstAdd.items()));
  const cr::CreativeTerrainProfilePlan secondAdd =
      cr::buildCreativeTerrainProfilePlan(addRequest);

  return expect(plannedHeight(ridge, {2, 0}) > 20U &&
                    plannedHeight(ridge, {0, 2}) == 20U,
                "ridge direction elongates along positive X") &&
         expect(plannedHeight(wave, {-1, 0}) > 20U &&
                    plannedHeight(wave, {1, 0}) < 20U,
                "wave direction controls signed phase") &&
         expect(plannedHeight(firstAdd, {0, 0}) == 28U &&
                    plannedHeight(secondAdd, {0, 0}) == 36U &&
                    editAt(firstAdd, {0, 0})->control.radiusCells == 3U,
                "add compounds from pre-edit height and preserves rod radius");
}

bool everyDirectionAndSamplingCombinationIsExplicit() {
  cr::CreativeTerrainField field;
  constexpr std::array directions{
      cr::CreativeTerrainProfileDirection::PositiveX,
      cr::CreativeTerrainProfileDirection::PositiveXPositiveZ,
      cr::CreativeTerrainProfileDirection::PositiveZ,
      cr::CreativeTerrainProfileDirection::NegativeXPositiveZ,
      cr::CreativeTerrainProfileDirection::NegativeX,
      cr::CreativeTerrainProfileDirection::NegativeXNegativeZ,
      cr::CreativeTerrainProfileDirection::NegativeZ,
      cr::CreativeTerrainProfileDirection::PositiveXNegativeZ,
  };
  constexpr std::array alongOffsets{
      cr::CreativeTerrainCoord2{2, 0},   cr::CreativeTerrainCoord2{2, 2},
      cr::CreativeTerrainCoord2{0, 2},   cr::CreativeTerrainCoord2{-2, 2},
      cr::CreativeTerrainCoord2{-2, 0},  cr::CreativeTerrainCoord2{-2, -2},
      cr::CreativeTerrainCoord2{0, -2},  cr::CreativeTerrainCoord2{2, -2},
  };
  constexpr std::array acrossOffsets{
      cr::CreativeTerrainCoord2{0, 2},   cr::CreativeTerrainCoord2{-2, 2},
      cr::CreativeTerrainCoord2{-2, 0},  cr::CreativeTerrainCoord2{-2, -2},
      cr::CreativeTerrainCoord2{0, -2},  cr::CreativeTerrainCoord2{2, -2},
      cr::CreativeTerrainCoord2{2, 0},   cr::CreativeTerrainCoord2{2, 2},
  };
  bool directionsValid = true;
  for (std::size_t index = 0U; index < directions.size(); ++index) {
    cr::CreativeTerrainProfileRequest ridge =
        requestFor(field, cr::CreativeTerrainProfileKind::Ridge);
    ridge.direction = directions[index];
    const cr::CreativeTerrainProfilePlan plan =
        cr::buildCreativeTerrainProfilePlan(ridge);
    directionsValid = directionsValid && plan.accepted &&
                      plannedHeight(plan, alongOffsets[index]) > 20U &&
                      plannedHeight(plan, acrossOffsets[index]) == 20U;
  }

  constexpr std::array<std::uint16_t, 3U> radii{{2U, 4U, 8U}};
  constexpr std::array<std::uint16_t, 3U> spacings{{1U, 2U, 4U}};
  bool samplingExact = true;
  for (const std::uint16_t radius : radii) {
    for (const std::uint16_t spacing : spacings) {
      for (std::uint8_t frequency = 1U; frequency <= 2U; ++frequency) {
        cr::CreativeTerrainProfileRequest wave =
            requestFor(field, cr::CreativeTerrainProfileKind::Wave);
        wave.radiusCells = radius;
        wave.spacingCells = spacing;
        wave.frequency = frequency;
        const cr::CreativeTerrainProfilePlan wavePlan =
            cr::buildCreativeTerrainProfilePlan(wave);
        const bool waveExpected =
            2U * frequency * spacing <= radius;
        samplingExact = samplingExact &&
                        wavePlan.accepted == waveExpected &&
                        (waveExpected ||
                         wavePlan.status ==
                             cr::CreativeTerrainProfilePlanStatus::UnderSampled);

        cr::CreativeTerrainProfileRequest ripple = wave;
        ripple.profile = cr::CreativeTerrainProfileKind::Ripple;
        const cr::CreativeTerrainProfilePlan ripplePlan =
            cr::buildCreativeTerrainProfilePlan(ripple);
        const bool rippleExpected =
            4U * frequency * spacing <= radius;
        samplingExact = samplingExact &&
                        ripplePlan.accepted == rippleExpected &&
                        (rippleExpected ||
                         ripplePlan.status ==
                             cr::CreativeTerrainProfilePlanStatus::UnderSampled);
      }
    }
  }
  return expect(directionsValid,
                "all eight ridge directions preserve along/across geometry") &&
         expect(samplingExact,
                "every radius spacing frequency combination follows sampling law");
}

bool rejectedPlansAreAtomicAndSpecific() {
  cr::CreativeTerrainField empty;
  cr::CreativeTerrainProfileRequest existing =
      requestFor(empty, cr::CreativeTerrainProfileKind::Hill);
  existing.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Existing;
  const auto noControls = cr::buildCreativeTerrainProfilePlan(existing);

  cr::CreativeTerrainProfileRequest unproven =
      requestFor(empty, cr::CreativeTerrainProfileKind::Wave);
  unproven.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Existing;
  const auto samplingUnproven = cr::buildCreativeTerrainProfilePlan(unproven);

  cr::CreativeTerrainProfileRequest undersampled =
      requestFor(empty, cr::CreativeTerrainProfileKind::Ripple);
  undersampled.radiusCells = 2U;
  const auto underSampled = cr::buildCreativeTerrainProfilePlan(undersampled);

  cr::CreativeTerrainProfileRequest overflow =
      requestFor(empty, cr::CreativeTerrainProfileKind::Hill);
  overflow.center = {std::numeric_limits<std::int32_t>::max(), 0};
  const auto coordinateOverflow = cr::buildCreativeTerrainProfilePlan(overflow);

  std::vector<cr::CreativeTerrainControlEdit> fullEdits;
  fullEdits.reserve(cr::kCreativeTerrainControlCapacity);
  for (std::size_t index = 0U; index < cr::kCreativeTerrainControlCapacity;
       ++index) {
    fullEdits.push_back({cr::CreativeTerrainEditKind::Upsert,
                         {{1000 + static_cast<std::int32_t>(index), 1000},
                          4U, 1U}});
  }
  cr::CreativeTerrainField full;
  static_cast<void>(full.apply(fullEdits));
  cr::CreativeTerrainProfileRequest capacity =
      requestFor(full, cr::CreativeTerrainProfileKind::Hill);
  const auto capacityExceeded = cr::buildCreativeTerrainProfilePlan(capacity);

  cr::CreativeTerrainProfileRequest invalid =
      requestFor(empty, cr::CreativeTerrainProfileKind::Count);
  const auto invalidPlan = cr::buildCreativeTerrainProfilePlan(invalid);

  return expect(!noControls.accepted && noControls.items().empty() &&
                    noControls.status ==
                        cr::CreativeTerrainProfilePlanStatus::NoControlsInBrush,
                "existing policy reports empty authored footprint") &&
         expect(!samplingUnproven.accepted &&
                    samplingUnproven.items().empty() &&
                    samplingUnproven.status ==
                        cr::CreativeTerrainProfilePlanStatus::SamplingUnproven,
                "oscillation rejects unproven sparse spacing") &&
         expect(!underSampled.accepted && underSampled.items().empty() &&
                    underSampled.status ==
                        cr::CreativeTerrainProfilePlanStatus::UnderSampled,
                "ripple enforces four samples per cycle") &&
         expect(!coordinateOverflow.accepted &&
                    coordinateOverflow.items().empty() &&
                    coordinateOverflow.status ==
                        cr::CreativeTerrainProfilePlanStatus::CoordinateOverflow,
                "coordinate overflow rejects before edits") &&
         expect(!capacityExceeded.accepted &&
                    capacityExceeded.items().empty() &&
                    capacityExceeded.status ==
                        cr::CreativeTerrainProfilePlanStatus::CapacityExceeded,
                "full field rejects fill atomically") &&
         expect(!invalidPlan.accepted && invalidPlan.items().empty() &&
                    invalidPlan.status ==
                        cr::CreativeTerrainProfilePlanStatus::InvalidRequest,
                "closed enum validation rejects invalid profile");
}

cr::CreativeTerrainSurfacePlan emptyCanonicalTerrain() {
  return cr::buildCreativeComposedTerrainSurfacePlan(
      cr::CreativeTerrainField{}, cr::CreativeTerrainHeightField{});
}

cr::CreativeTerrainProfileRecipe profileRecipe(
    cr::CreativeTerrainProfileKind kind) {
  cr::CreativeTerrainProfileRecipe recipe;
  recipe.profile = kind;
  recipe.baseHeightCells = 20U;
  recipe.radiusCells = 4U;
  recipe.amplitudeCells = 8U;
  recipe.spacingCells = 1U;
  return recipe;
}

bool denseRecipeOwnsEveryShapeAndSetParity() {
  const cr::CreativeTerrainSurfacePlan canonical = emptyCanonicalTerrain();
  bool everyShapeReady = true;
  for (const cr::CreativeTerrainProfileKind kind :
       {cr::CreativeTerrainProfileKind::Hill,
        cr::CreativeTerrainProfileKind::Basin,
        cr::CreativeTerrainProfileKind::Ring,
        cr::CreativeTerrainProfileKind::Crater,
        cr::CreativeTerrainProfileKind::Ridge,
        cr::CreativeTerrainProfileKind::Wave,
        cr::CreativeTerrainProfileKind::Ripple}) {
    cr::CreativeTerrainProfileRecipe recipe = profileRecipe(kind);
    if (kind == cr::CreativeTerrainProfileKind::Wave ||
        kind == cr::CreativeTerrainProfileKind::Ripple) {
      recipe.radiusCells = 8U;
    }
    const cr::CreativeTerrainProfileRecipeResult result =
        cr::buildCreativeTerrainProfileRecipe(
            cr::CreativeTerrainHeightField{}, canonical, recipe);
    everyShapeReady = everyShapeReady && result.receipt.accepted &&
                      result.receipt.affectedCellCount > 0U &&
                      result.heightField.validateInvariants();
  }

  cr::CreativeTerrainProfileRecipe hill =
      profileRecipe(cr::CreativeTerrainProfileKind::Hill);
  hill.radiusCells = 2U;
  const cr::CreativeTerrainProfileRecipeResult first =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, hill);
  const cr::CreativeTerrainSurfacePlan firstCanonical =
      cr::buildCreativeComposedTerrainSurfacePlan(
          cr::CreativeTerrainField{}, first.heightField);
  const cr::CreativeTerrainProfileRecipeResult repeated =
      cr::buildCreativeTerrainProfileRecipe(first.heightField,
                                             firstCanonical, hill);
  return expect(everyShapeReady,
                "dense durable recipe evaluates every profile kind") &&
         expect(first.receipt.accepted &&
                    first.heightField.bounds() ==
                        cr::CreativeTerrainHeightFieldBounds{{-2, -2}, 5U,
                                                             5U} &&
                    first.receipt.evaluatedCellCount == 13U &&
                    first.heightField.heightAt({0, 0}) == 28U &&
                    first.heightField.heightAt({2, 0}) == 20U,
                "dense hill owns its exact bounded footprint") &&
         expect(repeated.receipt.accepted &&
                    repeated.receipt.modifiedCellCount == 0U &&
                    repeated.receipt.heightHash == first.receipt.heightHash,
                "dense set recipe is idempotent");
}

bool denseRecipeSeedSpacingAndFailureContractsAreExplicit() {
  const cr::CreativeTerrainSurfacePlan canonical = emptyCanonicalTerrain();
  cr::CreativeTerrainProfileRecipe wave =
      profileRecipe(cr::CreativeTerrainProfileKind::Wave);
  wave.radiusCells = 8U;
  wave.frequency = 1U;
  const cr::CreativeTerrainProfileRecipeResult base =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, wave);
  wave.seed = 42U;
  const cr::CreativeTerrainProfileRecipeResult seeded =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, wave);
  wave.seed = 0U;
  wave.spacingCells = 2U;
  const cr::CreativeTerrainProfileRecipeResult spaced =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, wave);

  cr::CreativeTerrainProfileRecipe existing =
      profileRecipe(cr::CreativeTerrainProfileKind::Hill);
  existing.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Existing;
  const cr::CreativeTerrainProfileRecipeResult noSource =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, existing);
  cr::CreativeTerrainProfileRecipe undersampled =
      profileRecipe(cr::CreativeTerrainProfileKind::Ripple);
  undersampled.radiusCells = 2U;
  undersampled.frequency = 2U;
  const cr::CreativeTerrainProfileRecipeResult samplingRejected =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, undersampled);
  cr::CreativeTerrainProfileRecipe overflow =
      profileRecipe(cr::CreativeTerrainProfileKind::Hill);
  overflow.center = {std::numeric_limits<std::int32_t>::max(), 0};
  const cr::CreativeTerrainProfileRecipeResult overflowRejected =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, overflow);
  cr::CreativeTerrainProfileRecipe capacity =
      profileRecipe(cr::CreativeTerrainProfileKind::Hill);
  capacity.radiusCells = cr::kCreativeTerrainProfileMaximumRadiusCells;
  const cr::CreativeTerrainProfileRecipeResult capacityRejected =
      cr::buildCreativeTerrainProfileRecipe(
          cr::CreativeTerrainHeightField{}, canonical, capacity);

  cr::CreativeTerrainProfileKind parsedKind =
      cr::CreativeTerrainProfileKind::Count;
  cr::CreativeTerrainProfileBlend parsedBlend =
      cr::CreativeTerrainProfileBlend::Count;
  cr::CreativeTerrainProfileRodPolicy parsedPolicy =
      cr::CreativeTerrainProfileRodPolicy::Count;
  cr::CreativeTerrainProfileDirection parsedDirection =
      cr::CreativeTerrainProfileDirection::Count;
  return expect(base.receipt.accepted && seeded.receipt.accepted &&
                    base.receipt.heightHash != seeded.receipt.heightHash,
                "seed changes oscillatory phase and durable output") &&
         expect(spaced.receipt.accepted &&
                    spaced.receipt.heightHash != base.receipt.heightHash,
                "spacing changes deterministic sampled shape") &&
         expect(!noSource.receipt.accepted &&
                    noSource.receipt.status ==
                        cr::CreativeTerrainProfileRecipeStatus::
                            NoSourceInFootprint &&
                    noSource.heightField.cellCount() == 0U,
                "existing-only recipe rejects an empty footprint atomically") &&
         expect(!samplingRejected.receipt.accepted &&
                    samplingRejected.receipt.status ==
                        cr::CreativeTerrainProfileRecipeStatus::UnderSampled,
                "durable recipe exposes invalid sampling before commit") &&
         expect(!overflowRejected.receipt.accepted &&
                    overflowRejected.receipt.status ==
                        cr::CreativeTerrainProfileRecipeStatus::
                            CoordinateOverflow,
                "durable recipe exposes coordinate overflow") &&
         expect(!capacityRejected.receipt.accepted &&
                    capacityRejected.receipt.status ==
                        cr::CreativeTerrainProfileRecipeStatus::
                            CapacityExceeded,
                "durable recipe rejects an oversized dense footprint") &&
         expect(cr::parseCreativeTerrainProfileKind("RIPPLE", parsedKind) &&
                    parsedKind == cr::CreativeTerrainProfileKind::Ripple &&
                    cr::parseCreativeTerrainProfileBlend("ADD", parsedBlend) &&
                    parsedBlend == cr::CreativeTerrainProfileBlend::Add &&
                    cr::parseCreativeTerrainProfileRodPolicy(
                        "EXISTING", parsedPolicy) &&
                    parsedPolicy ==
                        cr::CreativeTerrainProfileRodPolicy::Existing &&
                    cr::parseCreativeTerrainProfileDirection(
                        "-X +Z", parsedDirection) &&
                    parsedDirection ==
                        cr::CreativeTerrainProfileDirection::NegativeXPositiveZ,
                "durable profile enums round-trip through persistence labels");
}

}  // namespace

int main() {
  bool ok = true;
  ok = fixedPointKernelIsPinned() && ok;
  ok = profileValuesAndNamesAreClosed() && ok;
  ok = hillFillIsCanonicalAndSetIsIdempotent() && ok;
  ok = radialProfilesPinCreatorShapes() && ok;
  ok = directionAndBlendSemanticsArePinned() && ok;
  ok = everyDirectionAndSamplingCombinationIsExplicit() && ok;
  ok = rejectedPlansAreAtomicAndSpecific() && ok;
  ok = denseRecipeOwnsEveryShapeAndSetParity() && ok;
  ok = denseRecipeSeedSpacingAndFailureContractsAreExplicit() && ok;
  return ok ? 0 : 1;
}
