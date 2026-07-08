#pragma once

#include <string_view>

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ProductAppWindowState;
struct ProductGameplayInputIntent;

void applyProductGameplayActionPhases(Session& session,
                                      const ProductGameplayInputIntent& intent,
                                      ProductAppWindowState& window,
                                      std::string_view source,
                                      const SpatialSurfaceSet* collisionSurfaces);

}  // namespace iggy3d
