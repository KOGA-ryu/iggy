#pragma once

#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "core/math/Vec3.hpp"

#include <string_view>

namespace iggy3d {

[[nodiscard]] Vec3 productManualFirstPersonDirection(float moveX,
                                                     float moveY,
                                                     float yawDegrees);

[[nodiscard]] float productManualFirstPersonMaxSpeedMetersPerSecond(
    const ProductGameplayMovementTuning& tuning,
    bool sprinting);

[[nodiscard]] std::string_view productManualFirstPersonMovementProfile(
    const ProductGameplayMovementTuning& tuning,
    bool sprinting);

[[nodiscard]] Vec3 productManualFirstPersonMoveDelta(
    float moveX,
    float moveY,
    float yawDegrees,
    bool sprinting,
    const ProductGameplayMovementTuning& tuning,
    float responseMultiplier);

[[nodiscard]] Vec3 productManualFirstPersonDesiredVelocity(
    float moveX,
    float moveY,
    float yawDegrees,
    bool sprinting,
    const ProductGameplayMovementTuning& tuning);

[[nodiscard]] Vec3 moveProductHorizontalVelocityToward(Vec3 current,
                                                       Vec3 target,
                                                       float maxDelta);

[[nodiscard]] Vec3 clampProductHorizontalVelocity(Vec3 velocity,
                                                  float maxSpeed);

}  // namespace iggy3d
