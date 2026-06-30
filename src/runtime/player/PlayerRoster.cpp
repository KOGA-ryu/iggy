#include "runtime/player/PlayerRoster.hpp"

#include <algorithm>
#include <utility>

namespace iggy3d {

namespace {

std::vector<PlayerSlot>::const_iterator findSlotIterator(
    const std::vector<PlayerSlot>& slots,
    PlayerSlotId slotId) {
  return std::find_if(slots.begin(), slots.end(), [slotId](const PlayerSlot& slot) {
    return slot.id == slotId;
  });
}

std::vector<PlayerSlot>::iterator findSlotIterator(
    std::vector<PlayerSlot>& slots,
    PlayerSlotId slotId) {
  return std::find_if(slots.begin(), slots.end(), [slotId](const PlayerSlot& slot) {
    return slot.id == slotId;
  });
}

bool actorBoundToDifferentSlot(const std::vector<PlayerSlot>& slots,
                               EntityId actor,
                               PlayerSlotId slotIdToIgnore) {
  if (!isValid(actor)) {
    return false;
  }
  for (const PlayerSlot& slot : slots) {
    if (slot.id != slotIdToIgnore && isActorControllingSlotKind(slot.kind) &&
        slot.actor == actor) {
      return true;
    }
  }
  return false;
}

PlayerRosterStatus validateSlotForWrite(const std::vector<PlayerSlot>& slots,
                                        const PlayerSlot& slot,
                                        bool allowExistingSlotId) {
  if (!isValidPlayerSlotId(slot.id)) {
    return PlayerRosterStatus::InvalidSlotId;
  }
  if (slot.kind == PlayerSlotKind::Unknown) {
    return PlayerRosterStatus::InvalidSlotKind;
  }
  if (isActorControllingSlotKind(slot.kind) && !isValid(slot.actor)) {
    return PlayerRosterStatus::InvalidActor;
  }
  const bool hasExistingSlot = findSlotIterator(slots, slot.id) != slots.end();
  if (!allowExistingSlotId && hasExistingSlot) {
    return PlayerRosterStatus::DuplicateSlotId;
  }
  if (actorBoundToDifferentSlot(slots, slot.actor, allowExistingSlotId ? slot.id
                                                                       : kInvalidPlayerSlotId)) {
    return PlayerRosterStatus::ActorAlreadyBound;
  }
  return PlayerRosterStatus::Ok;
}

PlayerRosterResult result(PlayerRosterStatus status, PlayerSlotId slotId, std::size_t index = 0) {
  return {status, slotId, index};
}

}  // namespace

const std::vector<PlayerSlot>& PlayerRoster::slots() const {
  return slots_;
}

bool PlayerRoster::empty() const {
  return slots_.empty();
}

std::size_t PlayerRoster::size() const {
  return slots_.size();
}

PlayerRosterResult PlayerRoster::addSlot(PlayerSlot slot) {
  const PlayerRosterStatus status = validateSlotForWrite(slots_, slot, false);
  if (status != PlayerRosterStatus::Ok) {
    return result(status, slot.id);
  }
  const std::size_t index = slots_.size();
  slots_.push_back(std::move(slot));
  return result(PlayerRosterStatus::Ok, slots_.back().id, index);
}

const PlayerSlot* PlayerRoster::findSlot(PlayerSlotId slotId) const {
  const auto found = findSlotIterator(slots_, slotId);
  return found == slots_.end() ? nullptr : &*found;
}

EntityId PlayerRoster::actorForSlot(PlayerSlotId slotId) const {
  const PlayerSlot* slot = findSlot(slotId);
  if (slot == nullptr || !isActorControllingSlotKind(slot->kind)) {
    return kInvalidEntityId;
  }
  return slot->actor;
}

bool PlayerRoster::slotControlsActor(PlayerSlotId slotId, EntityId actor) const {
  const PlayerSlot* slot = findSlot(slotId);
  return slot != nullptr && isActorControllingSlotKind(slot->kind) && slot->actor == actor;
}

void PlayerRoster::clear() {
  slots_.clear();
}

}  // namespace iggy3d
