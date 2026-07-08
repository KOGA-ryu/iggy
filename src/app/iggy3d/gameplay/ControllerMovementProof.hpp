#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

class Session;
struct ProductAppWindowState;

void clearProductMovementDebug(ProductAppWindowState& window);

[[nodiscard]] float productHorizontalMovementSpeedMetersPerSecond(
    const ProductAppWindowState& window);

void updateProductMovementStateProof(ProductAppWindowState& window);
void recordProductMovementProfile(ProductAppWindowState& window,
                                  bool sprinting);

void recordProductAirborneMovementDebug(ProductAppWindowState& window,
                                        Vec3 start,
                                        Vec3 finalPosition);

void recordProductLedgeFallMovementDebug(ProductAppWindowState& window,
                                         Vec3 start,
                                         Vec3 finalPosition);

[[nodiscard]] bool productMovementDebugChangedPosition(
    const ProductAppWindowState& window);

void recordProductMovementDebug(const Session& session,
                                ProductAppWindowState& window);

}  // namespace iggy3d
