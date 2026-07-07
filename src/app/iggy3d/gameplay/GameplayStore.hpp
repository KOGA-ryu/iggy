#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/gameplay/CollisionState.hpp"
#include "app/iggy3d/gameplay/CommandState.hpp"
#include "app/iggy3d/gameplay/DashState.hpp"
#include "app/iggy3d/gameplay/GameplayMovementInfo.hpp"
#include "app/iggy3d/gameplay/JumpState.hpp"
#include "app/iggy3d/gameplay/OutcomeState.hpp"
#include "app/iggy3d/gameplay/PhysicsMovementPlannerState.hpp"
#include "app/iggy3d/gameplay/ResetState.hpp"
#include "app/iggy3d/gameplay/TapeState.hpp"
#include "app/iggy3d/gameplay/TargetState.hpp"
#include "app/iggy3d/gameplay/TraversalState.hpp"
#include "app/iggy3d/gameplay/WallRunState.hpp"
#include "app/iggy3d/menu/ProductTransitionState.hpp"

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
  bool targetDiscovered = false;
  ProductGameplayTargetState gameplayTarget;
  ProductGameplayOutcomeState gameplayOutcome;
  std::string sessionOutcome = "None";
  ProductGameplayTapeState gameplayTape;
  bool interactionExecuted = false;
  bool attackExecuted = false;
  ProductTransitionState productTransition;
  std::string gameplayReachGate = "not_attempted";
  std::string gameplayLastRejection = "none";
  std::uint64_t sceneItemCount = 0;
  std::uint64_t debugItemCount = 0;
  bool playerVisible = false;
  bool roomVisible = false;
  bool objectiveVisible = false;
  bool rendererMutatedRuntime = false;
  bool scriptedGameplaySmoke = false;
};

}  // namespace iggy3d
