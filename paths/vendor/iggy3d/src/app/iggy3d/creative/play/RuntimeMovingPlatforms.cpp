#include "app/iggy3d/creative/play/RuntimeMovingPlatforms.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace iggy3d::creative {
namespace {

bool isValidCreativeMovingPlatformSettings(
    const CreativeMovingPlatformSettings& settings) noexcept {
  return std::isfinite(settings.speedMetersPerSecond) &&
         settings.speedMetersPerSecond > 0.0 &&
         settings.speedMetersPerSecond <= 100.0 &&
         static_cast<std::uint8_t>(settings.traversalMode) <
             static_cast<std::uint8_t>(
                 CreativeMovingPlatformTraversalMode::Count);
}

bool isValidCreativePathPoint(const CreativePathPoint& point) noexcept {
  return std::isfinite(point.position.x) &&
         std::isfinite(point.position.y) &&
         std::isfinite(point.position.z) &&
         std::isfinite(point.dwellSeconds) && point.dwellSeconds >= 0.0 &&
         point.dwellSeconds <= kCreativePathPointMaximumDwellSeconds &&
         std::isfinite(point.outgoingSpeedMultiplier) &&
         point.outgoingSpeedMultiplier >=
             kCreativePathPointMinimumOutgoingSpeedMultiplier &&
         point.outgoingSpeedMultiplier <=
             kCreativePathPointMaximumOutgoingSpeedMultiplier;
}

bool isValidCreativeMovingPlatformPath(
    std::span<const CreativePathPoint> pathPoints) noexcept {
  if (pathPoints.size() < 2U ||
      pathPoints.size() > kCreativeMovingPlatformPathPointCapacity) {
    return false;
  }
  double totalLengthMeters = 0.0;
  for (std::size_t index = 0U; index < pathPoints.size(); ++index) {
    if (!isValidCreativePathPoint(pathPoints[index])) {
      return false;
    }
    const Vec3 point = pathPoints[index].position;
    if (index == 0U) {
      continue;
    }
    const Vec3 previous = pathPoints[index - 1U].position;
    totalLengthMeters += std::hypot(point.x - previous.x,
                                    point.y - previous.y,
                                    point.z - previous.z);
  }
  return std::isfinite(totalLengthMeters) && totalLengthMeters > 1.0e-5;
}

constexpr float kRouteEpsilonMeters = 1.0e-5F;
constexpr float kMotionEpsilonMeters = 1.0e-6F;
constexpr double kRoutePhaseEpsilonMeters = 1.0e-9;
constexpr double kRouteTimeEpsilonSeconds = 1.0e-12;
constexpr double kDwellTickRoundingEpsilon = 1.0e-9;

[[nodiscard]] float segmentLength(Vec3 from, Vec3 to) noexcept {
  return length(to - from);
}

[[nodiscard]] bool validDefinition(
    const CreativeRuntimeMovingPlatformDefinition& definition) noexcept {
  if (definition.pathPointCount < 2U ||
      definition.pathPointCount > definition.pathPoints.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < definition.pathPointCount; ++index) {
    if (!std::isfinite(definition.waypointDwellSeconds[index]) ||
        definition.waypointDwellSeconds[index] < 0.0 ||
        definition.waypointDwellSeconds[index] >
            kCreativePathPointMaximumDwellSeconds ||
        !std::isfinite(definition.outgoingSpeedMultipliers[index]) ||
        definition.outgoingSpeedMultipliers[index] <
            kCreativePathPointMinimumOutgoingSpeedMultiplier ||
        definition.outgoingSpeedMultipliers[index] >
            kCreativePathPointMaximumOutgoingSpeedMultiplier) {
      return false;
    }
  }
  if (definition.routeArcCount == 0U ||
      definition.routeArcCount > definition.routeArcStartPhaseMeters.size() ||
      static_cast<std::uint8_t>(definition.traversalMode) >=
          static_cast<std::uint8_t>(
              CreativeMovingPlatformTraversalMode::Count)) {
    return false;
  }
  const double routeLength =
      definition.traversalMode == CreativeMovingPlatformTraversalMode::Loop
          ? static_cast<double>(definition.loopLengthMeters)
          : static_cast<double>(definition.openLengthMeters) * 2.0;
  double expectedStartPhase = 0.0;
  for (std::size_t index = 0U; index < definition.routeArcCount; ++index) {
    if (!std::isfinite(definition.routeArcStartPhaseMeters[index]) ||
        !std::isfinite(definition.routeArcEndPhaseMeters[index]) ||
        definition.routeArcEndPhaseMeters[index] <=
            definition.routeArcStartPhaseMeters[index] ||
        !std::isfinite(definition.routeArcSpeedMultipliers[index]) ||
        definition.routeArcSpeedMultipliers[index] <
            kCreativePathPointMinimumOutgoingSpeedMultiplier ||
        definition.routeArcSpeedMultipliers[index] >
            kCreativePathPointMaximumOutgoingSpeedMultiplier ||
        definition.routeArcStartWaypointIndices[index] >=
            definition.pathPointCount ||
        definition.routeArcEndWaypointIndices[index] >=
            definition.pathPointCount ||
        std::fabs(definition.routeArcStartPhaseMeters[index] -
                  expectedStartPhase) > kRoutePhaseEpsilonMeters ||
        definition.routeArcEndPhaseMeters[index] >
            routeLength + kRoutePhaseEpsilonMeters) {
      return false;
    }
    expectedStartPhase = definition.routeArcEndPhaseMeters[index];
  }
  return definition.openLengthMeters > kRouteEpsilonMeters &&
         std::isfinite(definition.openLengthMeters) &&
         std::isfinite(definition.loopLengthMeters) &&
         definition.loopLengthMeters >= definition.openLengthMeters &&
         definition.speedMetersPerSecond > 0.0F &&
         std::isfinite(definition.speedMetersPerSecond) &&
         definition.cycleTravelTimeSeconds > 0.0 &&
         std::isfinite(definition.cycleTravelTimeSeconds) &&
         std::fabs(expectedStartPhase - routeLength) <=
             kRoutePhaseEpsilonMeters &&
         isFinite(definition.originPositionMeters) &&
         routeLength > kRoutePhaseEpsilonMeters;
}

[[nodiscard]] double wrapPhase(double phase, double period) noexcept {
  const double wrapped = std::fmod(phase, period);
  return wrapped < 0.0 ? wrapped + period : wrapped;
}

[[nodiscard]] Vec3 sampleOpenRoute(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    float distanceMeters) noexcept {
  const float distance =
      std::clamp(distanceMeters, 0.0F, definition.openLengthMeters);
  for (std::size_t index = 1U; index < definition.pathPointCount; ++index) {
    const float segmentEnd = definition.cumulativeOpenMeters[index];
    if (distance > segmentEnd && index + 1U < definition.pathPointCount) {
      continue;
    }
    const float segmentStart = definition.cumulativeOpenMeters[index - 1U];
    const float lengthMeters = segmentEnd - segmentStart;
    if (lengthMeters <= kRouteEpsilonMeters) {
      continue;
    }
    const float alpha =
        std::clamp((distance - segmentStart) / lengthMeters, 0.0F, 1.0F);
    return definition.pathPoints[index - 1U] +
           (definition.pathPoints[index] -
            definition.pathPoints[index - 1U]) *
               alpha;
  }
  return definition.pathPoints[definition.pathPointCount - 1U];
}

[[nodiscard]] Vec3 sampleLoopRoute(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    float distanceMeters) noexcept {
  if (distanceMeters <= definition.openLengthMeters) {
    return sampleOpenRoute(definition, distanceMeters);
  }
  const float closingLength =
      definition.loopLengthMeters - definition.openLengthMeters;
  if (closingLength <= kRouteEpsilonMeters) {
    return definition.pathPoints.front();
  }
  const float alpha = std::clamp(
      (distanceMeters - definition.openLengthMeters) / closingLength, 0.0F,
      1.0F);
  const Vec3 last = definition.pathPoints[definition.pathPointCount - 1U];
  return last + (definition.pathPoints.front() - last) * alpha;
}

[[nodiscard]] double routeCycleLengthMeters(
    const CreativeRuntimeMovingPlatformDefinition& definition) noexcept {
  return definition.traversalMode ==
                 CreativeMovingPlatformTraversalMode::Loop
             ? static_cast<double>(definition.loopLengthMeters)
             : static_cast<double>(definition.openLengthMeters) * 2.0;
}

[[nodiscard]] Vec3 sampleRoutePosition(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    double phaseMeters) noexcept {
  const double cycleLength = routeCycleLengthMeters(definition);
  const double boundedPhase = std::clamp(phaseMeters, 0.0, cycleLength);
  const float sampleDistance =
      definition.traversalMode == CreativeMovingPlatformTraversalMode::Loop
          ? static_cast<float>(boundedPhase >= cycleLength ? 0.0
                                                           : boundedPhase)
          : static_cast<float>(
                boundedPhase <= definition.openLengthMeters
                    ? boundedPhase
                    : cycleLength - boundedPhase);
  const Vec3 sampled =
      definition.traversalMode == CreativeMovingPlatformTraversalMode::Loop
          ? sampleLoopRoute(definition, sampleDistance)
          : sampleOpenRoute(definition, sampleDistance);
  return definition.originPositionMeters +
         (sampled - definition.pathPoints.front());
}

struct ActiveRouteArc {
  bool found = false;
  double distanceMeters = std::numeric_limits<double>::infinity();
  double arrivalPhaseMeters = 0.0;
  float speedMultiplier = 1.0F;
  std::size_t arrivalWaypointIndex = 0U;
};

[[nodiscard]] ActiveRouteArc activeRouteArc(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    double phaseMeters,
    std::int8_t travelSign) noexcept {
  ActiveRouteArc result;
  const double cycleLength = routeCycleLengthMeters(definition);
  const double currentPhase = wrapPhase(phaseMeters, cycleLength);
  std::size_t arcIndex = definition.routeArcCount;
  bool wraps = false;
  if (travelSign > 0) {
    for (std::size_t index = 0U; index < definition.routeArcCount; ++index) {
      if (currentPhase + kRoutePhaseEpsilonMeters <
          definition.routeArcEndPhaseMeters[index]) {
        arcIndex = index;
        break;
      }
    }
    if (arcIndex == definition.routeArcCount) {
      arcIndex = 0U;
      wraps = true;
    }
    const double boundary = definition.routeArcEndPhaseMeters[arcIndex];
    result.distanceMeters = wraps ? cycleLength - currentPhase + boundary
                                  : boundary - currentPhase;
    result.arrivalWaypointIndex =
        definition.routeArcEndWaypointIndices[arcIndex];
    result.arrivalPhaseMeters = wrapPhase(boundary, cycleLength);
  } else {
    for (std::size_t reverse = definition.routeArcCount; reverse > 0U;
         --reverse) {
      const std::size_t index = reverse - 1U;
      if (currentPhase >
          definition.routeArcStartPhaseMeters[index] +
              kRoutePhaseEpsilonMeters) {
        arcIndex = index;
        break;
      }
    }
    if (arcIndex == definition.routeArcCount) {
      arcIndex = definition.routeArcCount - 1U;
      wraps = true;
    }
    const double boundary = definition.routeArcStartPhaseMeters[arcIndex];
    result.distanceMeters = wraps ? currentPhase + cycleLength - boundary
                                  : currentPhase - boundary;
    result.arrivalWaypointIndex =
        definition.routeArcStartWaypointIndices[arcIndex];
    result.arrivalPhaseMeters = wrapPhase(boundary, cycleLength);
  }
  result.found = std::isfinite(result.distanceMeters) &&
                 result.distanceMeters > kRoutePhaseEpsilonMeters;
  result.speedMultiplier = definition.routeArcSpeedMultipliers[arcIndex];
  return result;
}

[[nodiscard]] std::uint64_t waypointDwellTickCount(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    std::size_t waypointIndex,
    std::uint32_t fixedTickRateHz) noexcept {
  const double scaled = definition.waypointDwellSeconds[waypointIndex] *
                        static_cast<double>(fixedTickRateHz);
  const double wholeTicks = std::floor(scaled);
  const double rounded =
      scaled - wholeTicks <= kDwellTickRoundingEpsilon ? wholeTicks
                                                       : wholeTicks + 1.0;
  return static_cast<std::uint64_t>(rounded);
}

[[nodiscard]] bool routeHasDwellTicks(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    std::uint32_t fixedTickRateHz) noexcept {
  for (std::size_t index = 0U; index < definition.pathPointCount; ++index) {
    if (waypointDwellTickCount(definition, index, fixedTickRateHz) > 0U) {
      return true;
    }
  }
  return false;
}

}  // namespace

CreativeRuntimeMovingPlatformBuildResult
buildCreativeRuntimeMovingPlatformDefinition(
    std::span<const CreativePathPoint> pathPoints,
    CreativeMovingPlatformSettings settings,
    Vec3 originPositionMeters) noexcept {
  CreativeRuntimeMovingPlatformBuildResult result;
  if (!isValidCreativeMovingPlatformSettings(settings) ||
      !isFinite(originPositionMeters)) {
    result.status = CreativeRuntimeMovingPlatformBuildStatus::InvalidSettings;
    result.reasonCode = "creative_runtime_moving_platform_settings_invalid";
    return result;
  }
  if (!isValidCreativeMovingPlatformPath(pathPoints)) {
    return result;
  }

  CreativeRuntimeMovingPlatformDefinition& definition = result.definition;
  definition.pathPointCount = pathPoints.size();
  definition.speedMetersPerSecond =
      static_cast<float>(settings.speedMetersPerSecond);
  definition.traversalMode = settings.traversalMode;
  definition.startsActive = settings.startsActive;
  definition.originPositionMeters = originPositionMeters;
  for (std::size_t index = 0U; index < pathPoints.size(); ++index) {
    definition.pathPoints[index] = pathPoints[index].position;
    definition.waypointDwellSeconds[index] = pathPoints[index].dwellSeconds;
    definition.outgoingSpeedMultipliers[index] =
        static_cast<float>(pathPoints[index].outgoingSpeedMultiplier);
    if (index > 0U) {
      const float lengthMeters = segmentLength(
          definition.pathPoints[index - 1U], definition.pathPoints[index]);
      if (!std::isfinite(lengthMeters)) {
        return result;
      }
      definition.cumulativeOpenMeters[index] =
          definition.cumulativeOpenMeters[index - 1U] + lengthMeters;
    }
  }
  definition.openLengthMeters =
      definition.cumulativeOpenMeters[pathPoints.size() - 1U];
  definition.loopLengthMeters =
      definition.openLengthMeters +
      segmentLength(definition.pathPoints[pathPoints.size() - 1U],
                    definition.pathPoints.front());

  const auto appendRouteArc =
      [&definition](double startPhaseMeters,
                    double endPhaseMeters,
                    std::size_t startWaypointIndex,
                    std::size_t endWaypointIndex,
                    float speedMultiplier) {
        if (endPhaseMeters - startPhaseMeters <= kRoutePhaseEpsilonMeters) {
          return true;
        }
        if (definition.routeArcCount >=
            definition.routeArcStartPhaseMeters.size()) {
          return false;
        }
        const std::size_t arcIndex = definition.routeArcCount++;
        definition.routeArcStartPhaseMeters[arcIndex] = startPhaseMeters;
        definition.routeArcEndPhaseMeters[arcIndex] = endPhaseMeters;
        definition.routeArcStartWaypointIndices[arcIndex] =
            static_cast<std::uint8_t>(startWaypointIndex);
        definition.routeArcEndWaypointIndices[arcIndex] =
            static_cast<std::uint8_t>(endWaypointIndex);
        definition.routeArcSpeedMultipliers[arcIndex] = speedMultiplier;
        definition.cycleTravelTimeSeconds +=
            (endPhaseMeters - startPhaseMeters) /
            (static_cast<double>(definition.speedMetersPerSecond) *
             static_cast<double>(speedMultiplier));
        return std::isfinite(definition.cycleTravelTimeSeconds);
      };

  for (std::size_t index = 0U; index + 1U < pathPoints.size(); ++index) {
    if (!appendRouteArc(
            static_cast<double>(definition.cumulativeOpenMeters[index]),
            static_cast<double>(definition.cumulativeOpenMeters[index + 1U]),
            index, index + 1U, definition.outgoingSpeedMultipliers[index])) {
      return result;
    }
  }
  if (definition.traversalMode ==
      CreativeMovingPlatformTraversalMode::Loop) {
    if (!appendRouteArc(
            static_cast<double>(definition.openLengthMeters),
            static_cast<double>(definition.loopLengthMeters),
            pathPoints.size() - 1U, 0U,
            definition.outgoingSpeedMultipliers[pathPoints.size() - 1U])) {
      return result;
    }
  } else {
    const double cycleLength =
        static_cast<double>(definition.openLengthMeters) * 2.0;
    for (std::size_t reverse = pathPoints.size() - 1U; reverse > 0U;
         --reverse) {
      const double startPhase =
          cycleLength - static_cast<double>(
                            definition.cumulativeOpenMeters[reverse]);
      const double endPhase =
          cycleLength - static_cast<double>(
                            definition.cumulativeOpenMeters[reverse - 1U]);
      if (!appendRouteArc(startPhase, endPhase, reverse, reverse - 1U,
                          definition.outgoingSpeedMultipliers[reverse - 1U])) {
        return result;
      }
    }
  }
  if (!validDefinition(definition)) {
    return result;
  }

  result.ok = true;
  result.status = CreativeRuntimeMovingPlatformBuildStatus::Built;
  result.reasonCode = "creative_runtime_moving_platform_built";
  return result;
}

CreativeRuntimeMovingPlatformStepResult planCreativeRuntimeMovingPlatformStep(
    const CreativeRuntimeMovingPlatformStepRequest& request) noexcept {
  CreativeRuntimeMovingPlatformStepResult result;
  if (request.definition == nullptr || request.state == nullptr ||
      request.fixedTickRateHz == 0U ||
      !validDefinition(*request.definition) ||
      !std::isfinite(request.state->phaseMeters) ||
      (request.state->travelSign != -1 && request.state->travelSign != 1) ||
      !isFinite(request.state->positionMeters) ||
      (request.state->dwellTicksRemaining > 0U &&
       request.state->dwellingWaypointIndex >=
           request.definition->pathPointCount)) {
    return result;
  }
  result.nextState = *request.state;
  result.nextState.blocked = false;
  if (!request.active) {
    result.ok = true;
    result.status = CreativeRuntimeMovingPlatformStepStatus::Inactive;
    result.reasonCode = "creative_runtime_moving_platform_inactive";
    return result;
  }

  if (result.nextState.dwellTicksRemaining > 0U) {
    --result.nextState.dwellTicksRemaining;
    result.nextState.movementTickCount =
        request.state->movementTickCount + 1U;
    result.waypointIndex = result.nextState.dwellingWaypointIndex;
    result.ok = true;
    result.status = CreativeRuntimeMovingPlatformStepStatus::Dwelling;
    result.reasonCode = "creative_runtime_moving_platform_step_dwelling";
    return result;
  }

  const CreativeRuntimeMovingPlatformDefinition& definition =
      *request.definition;
  const double routeLength = routeCycleLengthMeters(definition);
  double phaseMeters = wrapPhase(request.state->phaseMeters, routeLength);
  double remainingSeconds =
      1.0 / static_cast<double>(request.fixedTickRateHz);
  if (!routeHasDwellTicks(definition, request.fixedTickRateHz) &&
      remainingSeconds >= definition.cycleTravelTimeSeconds) {
    remainingSeconds =
        std::fmod(remainingSeconds, definition.cycleTravelTimeSeconds);
  }

  bool consumedTime = remainingSeconds <= kRouteTimeEpsilonSeconds;
  result.nextState.dwellingWaypointIndex =
      std::numeric_limits<std::uint8_t>::max();
  for (std::size_t transition = 0U;
       !consumedTime && transition <= definition.routeArcCount;
       ++transition) {
    const ActiveRouteArc arc = activeRouteArc(
        definition, phaseMeters, request.state->travelSign);
    if (!arc.found || !std::isfinite(arc.speedMultiplier)) {
      return {};
    }
    const double speedMetersPerSecond =
        static_cast<double>(definition.speedMetersPerSecond) *
        static_cast<double>(arc.speedMultiplier);
    const double secondsToWaypoint =
        arc.distanceMeters / speedMetersPerSecond;
    if (!std::isfinite(secondsToWaypoint) ||
        secondsToWaypoint <= kRouteTimeEpsilonSeconds) {
      return {};
    }
    if (remainingSeconds + kRouteTimeEpsilonSeconds < secondsToWaypoint) {
      phaseMeters = wrapPhase(
          phaseMeters +
              static_cast<double>(request.state->travelSign) *
                  speedMetersPerSecond * remainingSeconds,
          routeLength);
      remainingSeconds = 0.0;
      consumedTime = true;
      break;
    }

    phaseMeters = arc.arrivalPhaseMeters;
    remainingSeconds = std::max(0.0, remainingSeconds - secondsToWaypoint);
    const std::uint64_t dwellTicks = waypointDwellTickCount(
        definition, arc.arrivalWaypointIndex, request.fixedTickRateHz);
    if (dwellTicks > 0U) {
      result.arrivedAtWaypoint = true;
      result.waypointIndex =
          static_cast<std::uint8_t>(arc.arrivalWaypointIndex);
      result.nextState.dwellTicksRemaining = dwellTicks;
      result.nextState.dwellingWaypointIndex = result.waypointIndex;
      consumedTime = true;
      break;
    }
    consumedTime = remainingSeconds <= kRouteTimeEpsilonSeconds;
  }
  if (!consumedTime) {
    return {};
  }
  result.nextState.phaseMeters = phaseMeters;
  result.nextState.positionMeters =
      sampleRoutePosition(definition, result.nextState.phaseMeters);
  result.displacementMeters =
      result.nextState.positionMeters - request.state->positionMeters;
  result.nextState.movementTickCount = request.state->movementTickCount + 1U;
  result.moved = lengthSquared(result.displacementMeters) >
                 kMotionEpsilonMeters * kMotionEpsilonMeters;
  result.ok = true;
  result.status = result.moved
                      ? CreativeRuntimeMovingPlatformStepStatus::Advanced
                      : CreativeRuntimeMovingPlatformStepStatus::Stationary;
  result.reasonCode = result.moved
                          ? "creative_runtime_moving_platform_step_advanced"
                          : "creative_runtime_moving_platform_step_stationary";
  return result;
}

CreativeRuntimeMovingPlatformSampleResult
sampleCreativeRuntimeMovingPlatformProgress(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    double normalizedProgress) noexcept {
  CreativeRuntimeMovingPlatformSampleResult result;
  if (!validDefinition(definition) || !std::isfinite(normalizedProgress) ||
      normalizedProgress < 0.0 || normalizedProgress > 1.0) {
    return result;
  }

  result.normalizedProgress = normalizedProgress;
  result.state.phaseMeters =
      (definition.traversalMode ==
               CreativeMovingPlatformTraversalMode::Loop
           ? static_cast<double>(definition.loopLengthMeters)
           : static_cast<double>(definition.openLengthMeters)) *
      normalizedProgress;
  result.state.positionMeters =
      sampleRoutePosition(definition, result.state.phaseMeters);
  result.ok = isFinite(result.state.positionMeters);
  if (!result.ok) {
    return result;
  }
  result.status = CreativeRuntimeMovingPlatformSampleStatus::Sampled;
  result.reasonCode = "creative_runtime_moving_platform_sampled";
  return result;
}

}  // namespace iggy3d::creative
