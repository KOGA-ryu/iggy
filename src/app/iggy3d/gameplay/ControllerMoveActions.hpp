#pragma once

#include <string_view>

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ProductAppWindowState;

[[nodiscard]] bool productHorizontalVelocityActive(
    const ProductAppWindowState& window);

void submitProductMove(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       bool sprinting,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces);

}  // namespace iggy3d
