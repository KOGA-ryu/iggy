#pragma once

#include "app/iggy3d/creative/document/Object.hpp"
#include "content/assets/RoomAsset.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace iggy3d::creative {

struct RoomBakeGreedyFloorInput {
  CreativeObjectId objectId{kInvalidObjectId};
  std::size_t documentIndex = 0;
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 size;
  std::string singleMeshId;
};

struct RoomBakeGreedyFloorPolicy {
  float cellSizeMeters = 1.0F;
  std::int64_t maxCells = 1'000'000;
  std::string meshId = "creative_floor_rect";
  std::string materialId = "creative_floor";
  std::string role = "floor";
};

struct RoomBakeGreedyFloorSource {
  CreativeObjectId objectId{kInvalidObjectId};
  std::size_t documentIndex = 0;
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 size;
};

struct RoomBakeGreedyFloorMeshPlan {
  RoomStaticMeshAsset mesh;
  std::vector<RoomBakeGreedyFloorSource> sources;
  std::size_t firstDocumentIndex = 0;
};

[[nodiscard]] std::vector<RoomBakeGreedyFloorMeshPlan>
buildRoomBakeGreedyFloorPlan(
    std::span<const RoomBakeGreedyFloorInput> inputs,
    const RoomBakeGreedyFloorPolicy& policy = {});

}  // namespace iggy3d::creative
