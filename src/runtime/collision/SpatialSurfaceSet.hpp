#pragma once

#include <span>
#include <vector>

#include "content/assets/RoomAsset.hpp"
#include "runtime/collision/CollisionTypes.hpp"

namespace iggy3d {

class SpatialSurfaceSet {
 public:
  SpatialSurfaceSet() = default;

  [[nodiscard]] std::span<const CollisionSurfaceView> surfaces() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::size_t size() const;

 private:
  friend SpatialSurfaceSet buildSpatialSurfaceSet(const RoomAsset& room);

  std::vector<CollisionSurfaceView> surfaces_;
};

SpatialSurfaceSet buildSpatialSurfaceSet(const RoomAsset& room);

}  // namespace iggy3d
