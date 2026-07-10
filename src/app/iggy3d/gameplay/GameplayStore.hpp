#pragma once

#include <cstdint>
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



// Owned gameplay-target state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayTargetState {
  std::string status = "not_requested";
  std::string action = "none";
  std::uint64_t entityId = 0;
  std::string stableName = "none";
  std::string kind = "none";
  float distanceMeters = 0.0F;
  bool supportsCommand = false;
};

// Owned gameplay-outcome state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayOutcomeState {
  std::string status = "not_requested";
  bool targetActiveAfter = false;
  bool inventoryChanged = false;
  std::string itemId = "none";
  std::uint64_t itemCount = 0;
  bool objectiveChanged = false;
  std::uint64_t eventCount = 0;
};

// Owned gameplay-tape (scripted playback) state -- extracted from the ProductAppWindowState
// god-struct (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayTapeState {
  bool requested = false;
  bool loaded = false;
  std::string path = "none";
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::uint64_t lineCount = 0;
  std::uint64_t stepCount = 0;
  std::uint64_t executedStepCount = 0;
  std::uint64_t expectedRejectedStepCount = 0;
  std::uint64_t expectedBlockedStepCount = 0;
  std::string failedStep = "none";
  std::uint64_t failedSourceLine = 0;
  std::string failedAction = "none";
  std::string failedTarget = "none";
  std::string failedRejection = "none";
  std::string failedMovementBlock = "none";
  std::string lastAction = "none";
  std::string lastTarget = "none";
  std::string lastMovementBlock = "none";
  bool keyCollected = false;
  bool secretDoorOpened = false;
  bool treasureCollected = false;
  bool npcTargetable = false;
  bool npcDefeated = false;
  bool exitObjectiveComplete = false;
  bool loopComplete = false;
  bool aiCommandLogged = false;
  bool aiAttackLogged = false;
  bool aiWaitLogged = false;
  bool aiPlayerDamaged = false;
  std::int32_t aiPlayerHpBefore = 0;
  std::int32_t aiPlayerHpAfter = 0;
  std::string aiActorId = "none";
  std::string aiTargetId = "none";
  std::string aiBehavior = "none";
  std::string aiIntent = "none";
};

// Owned menu/gameplay transition state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: menu transitions. Behavior-identical.
struct ProductTransitionState {
  std::string lastAction = "none";
  std::string status = "not_requested";
  bool returnedToGameplay = false;
  bool returnedToTitle = false;
  bool sessionPreserved = false;
};
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
