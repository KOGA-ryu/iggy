#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

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

}  // namespace iggy3d
