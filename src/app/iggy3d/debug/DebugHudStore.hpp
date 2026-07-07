#pragma once

#include "app/iggy3d/debug/DevCollisionOverlayState.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHudState.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/debug/PositionHud.hpp"
#include "app/iggy3d/debug/TopDownMapState.hpp"

namespace iggy3d {

struct DebugHudStore {
  ProductTopDownMapState topDownMap;
  ProductDevCollisionOverlayState devCollisionOverlay;
  ProductNpcBehaviorDebugHudState npcBehaviorDebugHud;
  PhysicsDebugHud physicsDebugHud;
  PositionHud positionHud;
};

}  // namespace iggy3d
