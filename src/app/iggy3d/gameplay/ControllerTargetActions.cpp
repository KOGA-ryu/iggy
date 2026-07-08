#include "app/iggy3d/gameplay/ControllerTargetActions.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerCommandExecution.hpp"
#include "app/iggy3d/gameplay/ControllerPlayerAccess.hpp"
#include "app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"

#include <string>

namespace iggy3d {
namespace {

TargetQueryResult queryProductGameplayTarget(const Session& session, CommandKind kind) {
  const EntityId actor = productPlayerActor(session);
  return queryTarget(TargetQueryRequest{&session.state().world, actor, false, {},
                                        kind, 0.0F, false, true});
}

}  // namespace

void submitProductTargetCommand(Session& session,
                                ProductAppWindowState& window,
                                CommandKind kind,
                                std::string_view source,
                                const SpatialSurfaceSet* collisionSurfaces) {
  clearProductOutcomeProof(window);
  const EntityId actor = productPlayerActor(session);
  const TargetQueryResult target = queryProductGameplayTarget(session, kind);
  recordProductTargetProof(session, window, kind, target);
  if (!window.gameplay.targetDiscovered) {
    window.gameplay.gameplayInputUsed = true;
    window.gameplay.gameplayInputSource = std::string(source);
    window.gameplay.gameplayCommand.kind = commandKindName(kind);
    window.gameplay.gameplayCommand.status = "no_target";
    window.gameplay.gameplayReachGate = "not_attempted";
    if (kind == CommandKind::Interact) {
      window.gameplay.gameplayOutcome.status = "no_target";
    }
    return;
  }

  const ReachQueryResult reach =
      queryReach(ReachQueryRequest{&session.state().world, actor, target.target, false, {},
                                   session.state().config.interactionRangeMeters, true});
  const CommandRejectionReason reachReason = rejectionReasonForReach(reach);
  window.gameplay.gameplayReachGate = reachGateName(reachReason);

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor;
  command.kind = kind;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target.target;
  if (kind == CommandKind::Attack) {
    command.payload.attackDamage = 3;
  }
  const ProductInteractionOutcomeSnapshot outcomeBefore =
      kind == CommandKind::Interact
          ? makeProductInteractionOutcomeSnapshot(session,
                                                  command.playerSlot,
                                                  target.target)
          : ProductInteractionOutcomeSnapshot{};
  window.gameplay.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
  if (kind == CommandKind::Interact) {
    recordProductInteractionOutcomeProof(session, window, outcomeBefore);
  }
  if (kind == CommandKind::Interact && window.gameplay.gameplayCommand.accepted &&
      window.gameplay.gameplayTickAdvanced) {
    window.gameplay.interactionExecuted = true;
  }
  if (kind == CommandKind::Attack && window.gameplay.gameplayCommand.accepted &&
      window.gameplay.gameplayTickAdvanced) {
    window.gameplay.attackExecuted = true;
  }
}

}  // namespace iggy3d
