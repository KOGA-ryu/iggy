#pragma once

#include <cstddef>
#include <string>

#include "content/authoring/EditableRoomDocument.hpp"
#include "runtime/save/SaveEnvelope.hpp"

namespace iggy3d {

struct EditableRoomToAuthoredRoomResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  SaveAuthoredRoomSection authoredRoom;
  std::size_t floorCount = 0;
  std::size_t wallCount = 0;
  std::size_t markerCount = 0;
};

EditableRoomToAuthoredRoomResult buildAuthoredRoomFromEditableRoomDocument(
    const EditableRoomDocument& document);

}  // namespace iggy3d
