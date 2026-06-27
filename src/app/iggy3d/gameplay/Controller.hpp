#pragma once

#include <string_view>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {

struct ActionState;
class Session;
class SpatialSurfaceSet;

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source,
                                 const SpatialSurfaceSet* collisionSurfaces = nullptr);

}  // namespace iggy3d
