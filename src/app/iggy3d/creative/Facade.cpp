#include "app/iggy3d/creative/Facade.hpp"

#include <utility>

namespace iggy3d::creative {

void Facade::reset() noexcept {
  state_ = State{};
  document_.reset();
  resetStats(stats_);
}

void Facade::beginFrame(const FramePacket& packet) noexcept {
  state_.frame = packet.frame;
  state_.flags = packet.flags;
}

void Facade::handle(const Packet& packet) noexcept {
  state_.frame = packet.frame;
  state_.hovered = packet.target;
  state_.flags = packet.flags;
}

CreativeObjectId Facade::createRoom(const CreateRoomCommand& command) {
  recordCommandAttempt(stats_);

  const CreativeObjectId id = document_.createRoom(command.name,
                                                   command.transform,
                                                   command.bounds,
                                                   command.layerId,
                                                   command.visible,
                                                   command.locked,
                                                   command.tags,
                                                   command.parentId);
  if (id == kInvalidObjectId) {
    recordCommandFailure(stats_);
    return kInvalidObjectId;
  }

  recordCommandSuccess(stats_);
  recordObjectCreated(stats_);
  recordRoomCreated(stats_);
  return id;
}

CreativeObjectId Facade::createRoom(std::string name) {
  return createRoom(makeCreateRoomCommand(std::move(name)));
}

bool Facade::renameObject(const RenameObjectCommand& command) {
  recordCommandAttempt(stats_);
  const bool renamed = document_.renameObject(command.id, command.name);
  if (renamed) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return renamed;
}

bool Facade::renameObject(CreativeObjectId id, std::string nextName) {
  RenameObjectCommand command;
  command.id = id;
  command.name = std::move(nextName);
  return renameObject(command);
}

bool Facade::removeObject(const RemoveObjectCommand& command) {
  recordCommandAttempt(stats_);
  const bool removed = document_.removeObject(command.id);
  if (removed) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return removed;
}

bool Facade::removeObject(CreativeObjectId id) {
  RemoveObjectCommand command;
  command.id = id;
  return removeObject(command);
}

const CreativeObject* Facade::findObject(CreativeObjectId id) const noexcept {
  return document_.findObject(id);
}

const CreativeDocument& Facade::document() const noexcept {
  return document_;
}

const Stats& Facade::stats() const noexcept {
  return stats_;
}

}  // namespace iggy3d::creative
