#pragma once

#include "runtime/command/Command.hpp"

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ProductAppWindowState;

void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces);

}  // namespace iggy3d
