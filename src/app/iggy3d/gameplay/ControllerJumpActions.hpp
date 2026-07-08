#pragma once

#include <string_view>

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ProductAppWindowState;

void advanceProductJump(Session& session,
                        ProductAppWindowState& window,
                        const SpatialSurfaceSet* collisionSurfaces);

void submitProductJump(Session& session,
                       ProductAppWindowState& window,
                       std::string_view source);

}  // namespace iggy3d
