#include "app/iggy3d/gameplay/Controller.hpp"

#include <string_view>

#include "app/iggy3d/gameplay/ControllerActionPhases.hpp"
#include "app/iggy3d/gameplay/ControllerInputIntent.hpp"

namespace iggy3d {

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source,
                                 const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayInputIntent intent =
      sampleProductGameplayInputIntent(actions);
  applyProductGameplayActionPhases(
      session, intent, window, source, collisionSurfaces);
}

}  // namespace iggy3d
