#pragma once

#include <string>

#include "app/iggy3d/gameplay/CollisionState.hpp"
#include "app/iggy3d/gameplay/CommandState.hpp"
#include "app/iggy3d/gameplay/DashState.hpp"
#include "app/iggy3d/gameplay/GameplayMovementInfo.hpp"
#include "app/iggy3d/gameplay/JumpState.hpp"
#include "app/iggy3d/gameplay/PhysicsMovementPlannerState.hpp"
#include "app/iggy3d/gameplay/ResetState.hpp"
#include "app/iggy3d/gameplay/TraversalState.hpp"
#include "app/iggy3d/gameplay/WallRunState.hpp"

namespace iggy3d {

struct GameplayStore {
  bool runtimeSessionCreated = false;
  bool gameplayActive = false;
  bool gameplayInputUsed = false;
  std::string gameplayInputSource = "none";
  bool gameplayTickAdvanced = false;
  bool playerPositionChanged = false;
  ProductGameplayCommandState gameplayCommand;
  ProductGameplayMovementInfo gameplayMovement;
  ProductWallRunState gameplayWallRun;
  ProductGameplayJumpState gameplayJump;
  ProductGameplayResetState gameplayReset;
  ProductGameplayTraversalState gameplayTraversal;
  ProductGameplayDashState gameplayDash;
  ProductGameplayCollisionState gameplayCollision;
  std::string gameplayTickReasonCode = "not_requested";
  ProductPhysicsMovementPlannerState physicsMovementPlanner;
};

}  // namespace iggy3d
