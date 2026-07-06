#pragma once

#include "app/iggy3d/creative/adapters/RoomBake.hpp"

namespace iggy3d::creative {

[[nodiscard]] CreativeRoomBakeReachabilityReceipt
initialCreativeRoomBakeReachabilityReceipt(
    const CreativeRoomBakeRequest& request);

[[nodiscard]] CreativeRoomBakeReachabilityReceipt
validateCreativeRoomBakeReachability(
    const RoomAsset& room,
    const CreativeRoomBakeRequest& request);

}  // namespace iggy3d::creative
