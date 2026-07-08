#pragma once

#include <string_view>

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ProductAppWindowState;
struct RoomAnchorAsset;

bool resetProductPlayerToSpawn(Session& session,
                               ProductAppWindowState& window,
                               std::string_view reason,
                               const RoomAnchorAsset* source);

bool applyProductGameplayResetIfNeeded(Session& session,
                                       ProductAppWindowState& window,
                                       const SpatialSurfaceSet* surfaces);

bool beginProductFallIfUnsupported(Session& session,
                                   ProductAppWindowState& window,
                                   const SpatialSurfaceSet* collisionSurfaces);

}  // namespace iggy3d
