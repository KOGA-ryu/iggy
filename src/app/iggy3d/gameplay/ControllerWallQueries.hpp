#pragma once

#include "core/math/Vec3.hpp"

#include <string>

namespace iggy3d {

class SpatialSurfaceSet;
struct CollisionSurfaceView;
struct ProductAppWindowState;
struct ProductGameplayMovementTuning;

const CollisionSurfaceView* findWallJumpSurface(
    const SpatialSurfaceSet& surfaces,
    Vec3 position,
    Vec3& awayNormal,
    const ProductGameplayMovementTuning& tuning);

const CollisionSurfaceView* findWallRunSurface(
    const SpatialSurfaceSet& surfaces,
    Vec3 position,
    Vec3& awayNormal,
    const ProductGameplayMovementTuning& tuning);

std::string wallRunSideName(Vec3 awayNormal, float yawDegrees);

bool productMovementDebugAlongWall(const ProductAppWindowState& window,
                                   Vec3 awayNormal);

bool wallRunProofNormal(const ProductAppWindowState& window, Vec3& normal);

bool wallRunTangentDirectionFromNormal(const ProductAppWindowState& window,
                                       Vec3 normal,
                                       float moveX,
                                       float moveY,
                                       Vec3& direction);

bool wallRunTangentDirection(const ProductAppWindowState& window,
                             float moveX,
                             float moveY,
                             Vec3& direction);

}  // namespace iggy3d
