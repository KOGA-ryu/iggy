#pragma once

#include "app/iggy3d/debug/DebugHudState.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

namespace iggy3d {

struct DebugProjectionResult;

struct InteractionModeHudRequest {
  ProductInteractionMode mode = ProductInteractionMode::Player;
  bool gameplayActive = false;
  bool roomEditingReady = false;
};

InteractionModeHud buildInteractionModeHud(
    InteractionModeHudRequest request);

NpcBehaviorDebugHud buildNpcBehaviorDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled);

PhysicsDebugHud buildPhysicsDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled);

}  // namespace iggy3d
