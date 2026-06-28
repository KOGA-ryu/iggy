#pragma once

#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/collision/CollisionTypes.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

namespace iggy3d {

CollisionQueryResult sampleSurfaceHeight(const SpatialSurfaceSet& surfaces,
                                         Vec3 worldPoint,
                                         float footprintToleranceMeters = 0.001F);

CollisionQueryResult sampleSurfaceHeightAtOrBelow(
    const SpatialSurfaceSet& surfaces,
    Vec3 worldPoint,
    float maxHeightMeters,
    float footprintToleranceMeters = 0.001F);

CollisionQueryResult sampleSurfaceNormal(const SpatialSurfaceSet& surfaces,
                                         Vec3 worldPoint,
                                         float footprintToleranceMeters = 0.001F);

CollisionQueryResult querySegment(const SpatialSurfaceSet& surfaces,
                                  Vec3 start,
                                  Vec3 end,
                                  CollisionQueryKind kind);

CollisionQueryResult queryPointOverlap(const SpatialSurfaceSet& surfaces,
                                       Vec3 point,
                                       CollisionQueryKind kind);

CollisionQueryResult queryAabbOverlap(const SpatialSurfaceSet& surfaces,
                                      const Aabb3& bounds,
                                      CollisionQueryKind kind);

}  // namespace iggy3d
