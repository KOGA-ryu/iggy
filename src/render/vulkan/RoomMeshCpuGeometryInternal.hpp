#pragma once

#include "render/vulkan/BufferImageResources.hpp"

#include <cstdint>

namespace iggy3d::vulkan::room_mesh_detail {

[[nodiscard]] std::uint64_t roomGeometrySignature(
    const SceneRoomProjection& room);

[[nodiscard]] std::uint64_t creativeWireframeDebugGeometrySignature(
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug);

void appendCreativeWireframeDebugReceiptFields(
    RenderReceipt& receipt, const FirstRoomGeometryResources& geometry);

}  // namespace iggy3d::vulkan::room_mesh_detail
