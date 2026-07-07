#include "app/iggy3d/gameplay/ScriptedDriver.hpp"

#include <cmath>

#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/input/ActionState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"

namespace iggy3d {
namespace {

float horizontalDistanceMeters(Vec3 lhs, Vec3 rhs) {
  const float dx = rhs.x - lhs.x;
  const float dz = rhs.z - lhs.z;
  return std::sqrt(dx * dx + dz * dz);
}

EntityId productPlayerActor(const Session& session) {
  return session.state().players.actorForSlot(0);
}

TargetQueryResult queryProductGameplayTarget(const Session& session, CommandKind kind) {
  const EntityId actor = productPlayerActor(session);
  return queryTarget(TargetQueryRequest{&session.state().world, actor, false, {},
                                        kind, 0.0F, false, true});
}

void approachProductGameplayTarget(Session& session, ProductAppWindowState& window) {
  constexpr int kMaxApproachSteps = 32;
  for (int step = 0; step < kMaxApproachSteps; ++step) {
    const EntityId actor = productPlayerActor(session);
    const TargetQueryResult target = queryProductGameplayTarget(session, CommandKind::Attack);
    if (target.status != TargetQueryStatus::Found) {
      return;
    }

    const ReachQueryResult reach =
        queryReach(ReachQueryRequest{&session.state().world, actor, target.target, false, {},
                                     session.state().config.interactionRangeMeters, true});
    const CommandRejectionReason reachReason = rejectionReasonForReach(reach);
    if (reachReason == CommandRejectionReason::None) {
      window.gameplay.targetDiscovered = true;
      window.gameplay.gameplayReachGate = "pass";
      return;
    }
    if (reach.status != ReachQueryStatus::OutOfRange) {
      return;
    }

    const float horizontalDistance =
        horizontalDistanceMeters(reach.actorPoint, reach.targetPoint);
    if (horizontalDistance <= 0.0001F) {
      return;
    }

    ActionState actions;
    recordAction(actions,
                 InputAction::PlayerMoveX,
                 true,
                 false,
                 false,
                 (reach.targetPoint.x - reach.actorPoint.x) / horizontalDistance);
    recordAction(actions,
                 InputAction::PlayerMoveY,
                 true,
                 false,
                 false,
                 (reach.targetPoint.z - reach.actorPoint.z) / horizontalDistance);
    applyProductGameplayActions(
        session, actions, window, "scripted",
        productActiveRoomCollisionSurfaces(activeRoomCollision(window)));
    if (!window.gameplay.gameplayCommand.accepted) {
      return;
    }
  }
}

}  // namespace

void runScriptedProductGameplaySmoke(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  if (!activeSession.has_value()) {
    window.gameplay.gameplayCommand.status = "missing_session";
    return;
  }

  window.gameplay.scriptedGameplaySmoke = true;
  if (queryProductGameplayTarget(*activeSession, CommandKind::Attack).status ==
      TargetQueryStatus::Found) {
    approachProductGameplayTarget(*activeSession, window);
  }

  ActionState actions;
  recordAction(actions, InputAction::PlayerAttack, true, true, false, 1.0F);
  applyProductGameplayActions(
      *activeSession, actions, window, "scripted",
      productActiveRoomCollisionSurfaces(activeRoomCollision(window)));
}

}  // namespace iggy3d
