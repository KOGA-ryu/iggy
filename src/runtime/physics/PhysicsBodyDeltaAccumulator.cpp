#include "runtime/physics/PhysicsBodyDeltaAccumulator.hpp"

#include <array>
#include <cmath>
#include <limits>

namespace iggy3d {
namespace {

constexpr std::size_t kMissingIndex = std::numeric_limits<std::size_t>::max();

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1093
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsBodyDeltaAccumulatorResult accumulatorResult(
    PhysicsBodyDeltaStatus status,
    bool ok) {
  PhysicsBodyDeltaAccumulatorResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsBodyDeltaStatusName(status);
  return result;
}

PhysicsBodyDeltaApplyResult applyResult(PhysicsBodyDeltaStatus status,
                                        bool ok) {
  PhysicsBodyDeltaApplyResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsBodyDeltaStatusName(status);
  return result;
}

bool finiteNonNegative(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

float magnitudeSquared(Vec3 value) {
  return lengthSquared(value);
}

bool zeroDelta(Vec3 value) {
  return nearlyEqual(value, {});
}

bool finiteDelta(Vec3 value) {
  return isFinite(value);
}

bool bodyStoreVectorShapeMatches(const PhysicsBodyStore& store) {
  const std::size_t size = store.ids().size();
  return store.motions().size() == size && store.positions().size() == size &&
         store.velocities().size() == size && store.masses().size() == size &&
         store.inverseMasses().size() == size;
}

PhysicsBodyDeltaAccumulatorResult invalidStoreResult(std::size_t index,
                                                     PhysicsBodyId id) {
  PhysicsBodyDeltaAccumulatorResult result =
      accumulatorResult(PhysicsBodyDeltaStatus::InvalidStoreState, false);
  result.invalidBodyIndex = index;
  result.invalidBodyId = id;
  return result;
}

PhysicsBodyDeltaApplyResult invalidStoreApplyResult(std::size_t index,
                                                    PhysicsBodyId id) {
  PhysicsBodyDeltaApplyResult result =
      applyResult(PhysicsBodyDeltaStatus::InvalidStoreState, false);
  result.invalidBodyIndex = index;
  result.invalidBodyId = id;
  return result;
}

bool validBodyStoreRow(const PhysicsBodyStore& store, std::size_t index) {
  return isValidPhysicsBodyId(store.ids()[index]) &&
         isFinite(store.positions()[index]) &&
         isFinite(store.velocities()[index]) &&
         std::isfinite(store.masses()[index]) &&
         finiteNonNegative(store.inverseMasses()[index]);
}

PhysicsBodyDeltaAccumulatorResult validateStore(
    const PhysicsBodyStore& store) {
  // branch-gate: BG-1093
  if (!bodyStoreVectorShapeMatches(store)) {
    return invalidStoreResult(0U, {});
  }
  for (std::size_t index = 0U; index < store.ids().size(); ++index) {
    // branch-gate: BG-1093
    if (!validBodyStoreRow(store, index)) {
      return invalidStoreResult(index, store.ids()[index]);
    }
  }
  return accumulatorResult(PhysicsBodyDeltaStatus::AccumulatorBuilt, true);
}

PhysicsBodyDeltaApplyResult validateStoreForApply(
    const PhysicsBodyStore& store) {
  // branch-gate: BG-1093
  if (!bodyStoreVectorShapeMatches(store)) {
    return invalidStoreApplyResult(0U, {});
  }
  for (std::size_t index = 0U; index < store.ids().size(); ++index) {
    // branch-gate: BG-1093
    if (!validBodyStoreRow(store, index)) {
      return invalidStoreApplyResult(index, store.ids()[index]);
    }
  }
  return applyResult(PhysicsBodyDeltaStatus::DeltasApplied, true);
}

bool accumulatorShapeValid(const PhysicsBodyDeltaAccumulator& accumulator) {
  const std::size_t size = accumulator.bodyIds.size();
  return accumulator.positionDeltasMeters.size() == size &&
         accumulator.velocityDeltasMetersPerSecond.size() == size &&
         accumulator.contributingPlanCounts.size() == size;
}

std::size_t findBodyIndex(const PhysicsBodyDeltaAccumulator& accumulator,
                          PhysicsBodyId id) {
  // branch-gate: BG-1093
  if (!isValidPhysicsBodyId(id)) {
    return kMissingIndex;
  }
  for (std::size_t index = 0U; index < accumulator.bodyIds.size(); ++index) {
    // branch-gate: BG-1093
    if (accumulator.bodyIds[index].value == id.value) {
      return index;
    }
  }
  return kMissingIndex;
}

PhysicsBodyDeltaAccumulatorResult invalidPlanResult(PhysicsBodyId id,
                                                    std::size_t index) {
  PhysicsBodyDeltaAccumulatorResult result =
      accumulatorResult(PhysicsBodyDeltaStatus::InvalidPlan, false);
  result.invalidBodyId = id;
  result.invalidBodyIndex = index;
  return result;
}

PhysicsBodyDeltaAccumulatorResult invalidDeltaAccumulatorResult(
    PhysicsBodyId id,
    std::size_t index) {
  PhysicsBodyDeltaAccumulatorResult result =
      accumulatorResult(PhysicsBodyDeltaStatus::InvalidDelta, false);
  result.invalidBodyId = id;
  result.invalidBodyIndex = index;
  return result;
}

bool planHasSolvingDelta(const PhysicsAabbContactSolvePlan& plan) {
  return plan.ok && plan.solveContact &&
         (plan.positionCorrectionApplied || plan.velocityImpulseApplied ||
          plan.frictionImpulseApplied);
}

bool planDeltasFinite(const PhysicsAabbContactSolvePlan& plan) {
  return finiteDelta(plan.firstPositionDeltaMeters) &&
         finiteDelta(plan.secondPositionDeltaMeters) &&
         finiteDelta(plan.firstVelocityDeltaMetersPerSecond) &&
         finiteDelta(plan.secondVelocityDeltaMetersPerSecond);
}

bool accumulatorDeltasFinite(const PhysicsBodyDeltaAccumulator& accumulator) {
  for (Vec3 value : accumulator.positionDeltasMeters) {
    // branch-gate: BG-1093
    if (!finiteDelta(value)) {
      return false;
    }
  }
  for (Vec3 value : accumulator.velocityDeltasMetersPerSecond) {
    // branch-gate: BG-1093
    if (!finiteDelta(value)) {
      return false;
    }
  }
  return true;
}

bool accumulatorAllZero(const PhysicsBodyDeltaAccumulator& accumulator) {
  for (Vec3 value : accumulator.positionDeltasMeters) {
    // branch-gate: BG-1093
    if (!zeroDelta(value)) {
      return false;
    }
  }
  for (Vec3 value : accumulator.velocityDeltasMetersPerSecond) {
    // branch-gate: BG-1093
    if (!zeroDelta(value)) {
      return false;
    }
  }
  return true;
}

PhysicsBodyDeltaApplyResult invalidDeltaApplyResult(std::size_t index,
                                                    PhysicsBodyId id) {
  PhysicsBodyDeltaApplyResult result =
      applyResult(PhysicsBodyDeltaStatus::InvalidDelta, false);
  result.invalidBodyIndex = index;
  result.invalidBodyId = id;
  return result;
}

PhysicsBodyDeltaApplyResult validateApplyDeltas(
    const PhysicsBodyStore& store,
    const PhysicsBodyDeltaAccumulator& accumulator,
    const PhysicsBodyDeltaApplyConfig& config) {
  for (std::size_t storeIndex = 0U; storeIndex < store.ids().size();
       ++storeIndex) {
    const PhysicsBodyId id = store.ids()[storeIndex];
    const std::size_t accumulatorIndex = findBodyIndex(accumulator, id);
    // branch-gate: BG-1093
    if (accumulatorIndex == kMissingIndex) {
      PhysicsBodyDeltaApplyResult result =
          applyResult(PhysicsBodyDeltaStatus::BodyNotFound, false);
      result.invalidBodyId = id;
      result.invalidBodyIndex = storeIndex;
      return result;
    }
    const Vec3 positionDelta =
        accumulator.positionDeltasMeters[accumulatorIndex];
    const Vec3 velocityDelta =
        accumulator.velocityDeltasMetersPerSecond[accumulatorIndex];
    // branch-gate: BG-1093
    if ((config.applyPositionDeltas && !finiteDelta(positionDelta)) ||
        (config.applyVelocityDeltas && !finiteDelta(velocityDelta))) {
      return invalidDeltaApplyResult(storeIndex, id);
    }
    // branch-gate: BG-1093
    if (config.applyPositionDeltas &&
        magnitudeSquared(positionDelta) >
            config.maxPositionDeltaMeters * config.maxPositionDeltaMeters) {
      return invalidDeltaApplyResult(storeIndex, id);
    }
    // branch-gate: BG-1093
    if (config.applyVelocityDeltas &&
        magnitudeSquared(velocityDelta) >
            config.maxVelocityDeltaMetersPerSecond *
                config.maxVelocityDeltaMetersPerSecond) {
      return invalidDeltaApplyResult(storeIndex, id);
    }
  }
  return applyResult(PhysicsBodyDeltaStatus::DeltasApplied, true);
}

}  // namespace

std::string_view physicsBodyDeltaStatusName(PhysicsBodyDeltaStatus status) {
  static constexpr std::array<std::string_view, 12> kNames{
      "physics_body_delta_accumulator_built",
      "physics_body_delta_accumulated",
      "physics_body_delta_applied",
      "physics_body_delta_no_op",
      "physics_body_delta_missing_store",
      "physics_body_delta_missing_accumulator",
      "physics_body_delta_missing_plan",
      "physics_body_delta_invalid_store_state",
      "physics_body_delta_invalid_plan",
      "physics_body_delta_body_not_found",
      "physics_body_delta_invalid_delta",
      "physics_body_delta_invalid_config",
  };
  return enumName(status, kNames, "physics_body_delta_invalid_plan");
}

bool isValidPhysicsBodyDeltaApplyConfig(
    const PhysicsBodyDeltaApplyConfig& config) {
  return finiteNonNegative(config.maxPositionDeltaMeters) &&
         finiteNonNegative(config.maxVelocityDeltaMetersPerSecond);
}

PhysicsBodyDeltaAccumulatorResult buildPhysicsBodyDeltaAccumulator(
    const PhysicsBodyStore* store) {
  // branch-gate: BG-1093
  if (store == nullptr) {
    return accumulatorResult(PhysicsBodyDeltaStatus::MissingStore, false);
  }

  PhysicsBodyDeltaAccumulatorResult validation = validateStore(*store);
  // branch-gate: BG-1093
  if (!validation.ok) {
    return validation;
  }

  validation.accumulator.bodyIds = store->ids();
  validation.accumulator.positionDeltasMeters.assign(store->size(), {});
  validation.accumulator.velocityDeltasMetersPerSecond.assign(store->size(),
                                                              {});
  validation.accumulator.contributingPlanCounts.assign(store->size(), 0U);
  validation.bodyCount = store->size();
  return validation;
}

PhysicsBodyDeltaAccumulatorResult accumulatePhysicsAabbContactSolvePlan(
    PhysicsBodyDeltaAccumulator* accumulator,
    const PhysicsAabbContactSolvePlan* plan) {
  // branch-gate: BG-1093
  if (accumulator == nullptr) {
    return accumulatorResult(PhysicsBodyDeltaStatus::MissingAccumulator,
                             false);
  }
  // branch-gate: BG-1093
  if (plan == nullptr) {
    return accumulatorResult(PhysicsBodyDeltaStatus::MissingPlan, false);
  }
  // branch-gate: BG-1093
  if (!accumulatorShapeValid(*accumulator)) {
    return accumulatorResult(PhysicsBodyDeltaStatus::MissingAccumulator,
                             false);
  }
  // branch-gate: BG-1093
  if (!plan->ok) {
    return invalidPlanResult(plan->firstBodyId, 0U);
  }
  // branch-gate: BG-1093
  if (!planHasSolvingDelta(*plan)) {
    PhysicsBodyDeltaAccumulatorResult result =
        accumulatorResult(PhysicsBodyDeltaStatus::NoOp, true);
    result.bodyCount = accumulator->bodyIds.size();
    return result;
  }
  // branch-gate: BG-1093
  if (!isValidPhysicsBodyId(plan->firstBodyId) ||
      !isValidPhysicsBodyId(plan->secondBodyId)) {
    return invalidPlanResult(plan->firstBodyId, 0U);
  }
  // branch-gate: BG-1093
  if (!planDeltasFinite(*plan)) {
    return invalidDeltaAccumulatorResult(plan->firstBodyId, 0U);
  }

  const std::size_t firstIndex = findBodyIndex(*accumulator, plan->firstBodyId);
  // branch-gate: BG-1093
  if (firstIndex == kMissingIndex) {
    PhysicsBodyDeltaAccumulatorResult result =
        accumulatorResult(PhysicsBodyDeltaStatus::BodyNotFound, false);
    result.invalidBodyId = plan->firstBodyId;
    return result;
  }
  const std::size_t secondIndex =
      findBodyIndex(*accumulator, plan->secondBodyId);
  // branch-gate: BG-1093
  if (secondIndex == kMissingIndex) {
    PhysicsBodyDeltaAccumulatorResult result =
        accumulatorResult(PhysicsBodyDeltaStatus::BodyNotFound, false);
    result.invalidBodyId = plan->secondBodyId;
    return result;
  }

  accumulator->positionDeltasMeters[firstIndex] =
      accumulator->positionDeltasMeters[firstIndex] +
      plan->firstPositionDeltaMeters;
  accumulator->positionDeltasMeters[secondIndex] =
      accumulator->positionDeltasMeters[secondIndex] +
      plan->secondPositionDeltaMeters;
  accumulator->velocityDeltasMetersPerSecond[firstIndex] =
      accumulator->velocityDeltasMetersPerSecond[firstIndex] +
      plan->firstVelocityDeltaMetersPerSecond;
  accumulator->velocityDeltasMetersPerSecond[secondIndex] =
      accumulator->velocityDeltasMetersPerSecond[secondIndex] +
      plan->secondVelocityDeltaMetersPerSecond;
  accumulator->contributingPlanCounts[firstIndex] += 1U;
  accumulator->contributingPlanCounts[secondIndex] += 1U;

  PhysicsBodyDeltaAccumulatorResult result =
      accumulatorResult(PhysicsBodyDeltaStatus::DeltaAccumulated, true);
  result.bodyCount = accumulator->bodyIds.size();
  result.accumulatedPlanCount = 1U;
  return result;
}

PhysicsBodyDeltaApplyResult applyPhysicsBodyDeltas(
    PhysicsBodyStore* store,
    const PhysicsBodyDeltaAccumulator& accumulator,
    const PhysicsBodyDeltaApplyConfig& config) {
  // branch-gate: BG-1093
  if (store == nullptr) {
    return applyResult(PhysicsBodyDeltaStatus::MissingStore, false);
  }
  // branch-gate: BG-1093
  if (!isValidPhysicsBodyDeltaApplyConfig(config)) {
    return applyResult(PhysicsBodyDeltaStatus::InvalidConfig, false);
  }
  // branch-gate: BG-1093
  if (!accumulatorShapeValid(accumulator)) {
    return applyResult(PhysicsBodyDeltaStatus::MissingAccumulator, false);
  }
  PhysicsBodyDeltaApplyResult storeValidation = validateStoreForApply(*store);
  // branch-gate: BG-1093
  if (!storeValidation.ok) {
    return storeValidation;
  }
  // branch-gate: BG-1093
  if (!accumulatorDeltasFinite(accumulator)) {
    return invalidDeltaApplyResult(0U, {});
  }
  // branch-gate: BG-1093
  if (!config.applyPositionDeltas && !config.applyVelocityDeltas) {
    PhysicsBodyDeltaApplyResult result =
        applyResult(PhysicsBodyDeltaStatus::NoOp, true);
    result.bodyCount = store->size();
    return result;
  }
  // branch-gate: BG-1093
  if (accumulatorAllZero(accumulator)) {
    PhysicsBodyDeltaApplyResult result =
        applyResult(PhysicsBodyDeltaStatus::NoOp, true);
    result.bodyCount = store->size();
    return result;
  }

  PhysicsBodyDeltaApplyResult deltaValidation =
      validateApplyDeltas(*store, accumulator, config);
  // branch-gate: BG-1093
  if (!deltaValidation.ok) {
    return deltaValidation;
  }

  PhysicsBodyDeltaApplyResult result =
      applyResult(PhysicsBodyDeltaStatus::DeltasApplied, true);
  result.bodyCount = store->size();
  for (std::size_t storeIndex = 0U; storeIndex < store->ids_.size();
       ++storeIndex) {
    const std::size_t accumulatorIndex =
        findBodyIndex(accumulator, store->ids_[storeIndex]);
    const Vec3 positionDelta =
        accumulator.positionDeltasMeters[accumulatorIndex];
    const Vec3 velocityDelta =
        accumulator.velocityDeltasMetersPerSecond[accumulatorIndex];
    // branch-gate: BG-1093
    if (config.applyPositionDeltas && !zeroDelta(positionDelta)) {
      store->positions_[storeIndex] = store->positions_[storeIndex] +
                                      positionDelta;
      ++result.appliedPositionCount;
    }
    // branch-gate: BG-1093
    if (config.applyVelocityDeltas && !zeroDelta(velocityDelta)) {
      store->velocities_[storeIndex] = store->velocities_[storeIndex] +
                                       velocityDelta;
      ++result.appliedVelocityCount;
    }
  }
  // branch-gate: BG-1093
  if (result.appliedPositionCount == 0U &&
      result.appliedVelocityCount == 0U) {
    result.status = PhysicsBodyDeltaStatus::NoOp;
    result.reasonCode = physicsBodyDeltaStatusName(result.status);
  }
  return result;
}

}  // namespace iggy3d
