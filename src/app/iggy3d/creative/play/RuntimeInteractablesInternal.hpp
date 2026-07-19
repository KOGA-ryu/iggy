#pragma once

#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"

namespace iggy3d::creative::runtime_interactables_internal {

[[nodiscard]] bool automaticSourceMode(
    CreativeRuntimeLogicSourceMode mode) noexcept;
[[nodiscard]] bool surfaceOwnedByRoomMesh(
    const RoomSpatialSurface& surface,
    std::string_view targetMesh) noexcept;
[[nodiscard]] bool surfaceOwnedByTarget(
    const RoomSpatialSurface& surface,
    const CreativeRuntimeInteractableState& target) noexcept;

}  // namespace iggy3d::creative::runtime_interactables_internal
