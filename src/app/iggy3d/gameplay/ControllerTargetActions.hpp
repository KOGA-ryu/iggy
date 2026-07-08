#pragma once

#include "runtime/command/Command.hpp"

#include <string_view>

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ProductAppWindowState;

void submitProductTargetCommand(Session& session,
                                ProductAppWindowState& window,
                                CommandKind kind,
                                std::string_view source,
                                const SpatialSurfaceSet* collisionSurfaces);

}  // namespace iggy3d
