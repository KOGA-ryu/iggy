#pragma once

#include <string_view>

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ActionState;
struct ProductAppWindowState;

struct ProductGameplayInputIntent {
  float moveX = 0.0F;
  float moveY = 0.0F;
  bool sprinting = false;
  bool jumpPressed = false;
  bool jumpReleased = false;
  bool dashPressed = false;
  bool interactPressed = false;
  bool attackPressed = false;
  bool resetPressed = false;
};

ProductGameplayInputIntent sampleProductGameplayInputIntent(
    const ActionState& actions);

bool productGameplayIntentHasMovement(
    const ProductGameplayInputIntent& intent,
    const ProductAppWindowState& window);

void applyProductGameplayActionPhases(Session& session,
                                      const ProductGameplayInputIntent& intent,
                                      ProductAppWindowState& window,
                                      std::string_view source,
                                      const SpatialSurfaceSet* collisionSurfaces);

}  // namespace iggy3d
