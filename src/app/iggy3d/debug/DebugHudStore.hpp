#pragma once

#include "app/iggy3d/debug/DebugHudState.hpp"
#include "app/iggy3d/debug/PositionHud.hpp"

namespace iggy3d {

struct DebugHudStore {
  ProductTopDownMapState topDownMap;
  ProductDevCollisionOverlayState devCollisionOverlay;
  ProductNpcBehaviorDebugHudState npcBehaviorDebugHud;
  PhysicsDebugHud physicsDebugHud;
  PositionHud positionHud;
};

}  // namespace iggy3d
