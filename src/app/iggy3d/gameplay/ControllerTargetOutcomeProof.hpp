#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/targeting/TargetQuery.hpp"

namespace iggy3d {

class Session;
struct ProductAppWindowState;

struct ProductInteractionOutcomeSnapshot {
  EntityId target;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::string objectiveId;
  std::uint32_t itemCountBefore = 0;
  bool objectiveCompleteBefore = false;
  std::size_t eventCountBefore = 0;
};

std::string commandKindName(CommandKind kind);

std::string commandRejectionReasonName(CommandRejectionReason reason);

std::string reachGateName(CommandRejectionReason reason);

void clearProductTargetProof(ProductAppWindowState& window);

void clearProductOutcomeProof(ProductAppWindowState& window);

ProductInteractionOutcomeSnapshot makeProductInteractionOutcomeSnapshot(
    const Session& session,
    PlayerSlotId playerSlot,
    EntityId target);

void recordProductInteractionOutcomeProof(
    const Session& session,
    ProductAppWindowState& window,
    const ProductInteractionOutcomeSnapshot& before);

void recordProductTargetProof(const Session& session,
                              ProductAppWindowState& window,
                              CommandKind kind,
                              const TargetQueryResult& target);

}  // namespace iggy3d
