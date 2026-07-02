#pragma once

#include "app/iggy3d/creative/Object.hpp"

#include <optional>
#include <string>
#include <vector>

namespace iggy3d::creative {

struct CreateRoomCommand {
  std::string name;
  CreativeTransform transform;
  CreativeBounds bounds;
  CreativeLayerId layerId = kDefaultLayerId;
  bool visible = true;
  bool locked = false;
  std::vector<std::string> tags;
  std::optional<CreativeObjectId> parentId;
};

struct RenameObjectCommand {
  CreativeObjectId id = kInvalidObjectId;
  std::string name;
};

struct RemoveObjectCommand {
  CreativeObjectId id = kInvalidObjectId;
};

[[nodiscard]] CreateRoomCommand makeCreateRoomCommand(std::string name);

}  // namespace iggy3d::creative
