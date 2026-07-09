#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

class Session;
struct CollisionSurfaceView;
struct ProductAppWindowState;
struct TraversalIntentResult;

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

void clearProductTraversalProof(ProductAppWindowState& window);

void recordProductTraversalProof(ProductAppWindowState& window,
                                 const TraversalIntentResult& result);

void recordProductWallJumpTraversalProof(ProductAppWindowState& window,
                                         const CollisionSurfaceView& surface,
                                         Vec3 start,
                                         Vec3 finalPosition);

}  // namespace iggy3d
