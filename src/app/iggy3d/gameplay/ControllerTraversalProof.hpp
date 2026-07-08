#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct CollisionSurfaceView;
struct ProductAppWindowState;
struct TraversalIntentResult;

void clearProductTraversalProof(ProductAppWindowState& window);

void recordProductTraversalProof(ProductAppWindowState& window,
                                 const TraversalIntentResult& result);

void recordProductWallJumpTraversalProof(ProductAppWindowState& window,
                                         const CollisionSurfaceView& surface,
                                         Vec3 start,
                                         Vec3 finalPosition);

}  // namespace iggy3d
