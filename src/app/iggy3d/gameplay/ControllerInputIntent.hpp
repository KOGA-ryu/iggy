#pragma once

namespace iggy3d {

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

}  // namespace iggy3d
