#pragma once

#include "app/iggy3d/creative/Commands.hpp"
#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Document.hpp"
#include "app/iggy3d/creative/Metrics.hpp"
#include "app/iggy3d/creative/Object.hpp"
#include "app/iggy3d/creative/State.hpp"

#include <string>

namespace iggy3d::creative {

class Facade {
 public:
  void reset() noexcept;
  void beginFrame(const FramePacket& packet) noexcept;
  void handle(const Packet& packet) noexcept;

  [[nodiscard]] CreativeObjectId createRoom(const CreateRoomCommand& command);
  [[nodiscard]] CreativeObjectId createRoom(std::string name);
  [[nodiscard]] bool renameObject(const RenameObjectCommand& command);
  [[nodiscard]] bool renameObject(CreativeObjectId id, std::string nextName);
  [[nodiscard]] bool removeObject(const RemoveObjectCommand& command);
  [[nodiscard]] bool removeObject(CreativeObjectId id);
  [[nodiscard]] const CreativeObject* findObject(
      CreativeObjectId id) const noexcept;
  [[nodiscard]] const CreativeDocument& document() const noexcept;
  [[nodiscard]] const Stats& stats() const noexcept;

 private:
  State state_;
  CreativeDocument document_;
  Stats stats_;
};

}  // namespace iggy3d::creative
