#include "app/iggy3d/creative/Commands.hpp"

#include <utility>

namespace iggy3d::creative {

CreateRoomCommand makeCreateRoomCommand(std::string name) {
  CreateRoomCommand command;
  command.name = std::move(name);
  return command;
}

}  // namespace iggy3d::creative
