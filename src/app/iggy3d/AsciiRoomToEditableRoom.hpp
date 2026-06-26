#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "app/iggy3d/AsciiRoomToAuthoredRoom.hpp"
#include "content/authoring/EditableRoomDocument.hpp"

namespace iggy3d {

struct AsciiRoomToEditableRoomResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  EditableRoomDocument document;
  AsciiRoomAuthoredRoomResult authoredRoom;
  std::vector<AsciiRoomDiagnostic> diagnostics;
  std::size_t floorCount = 0;
  std::size_t wallCount = 0;
  std::size_t markerCount = 0;
};

AsciiRoomToEditableRoomResult buildEditableRoomFromAsciiRoom(
    const AsciiRoomGrid& grid,
    const AsciiRoomCompileConfig& config = {});

EditableRoomDocument buildEditableRoomDocumentFromAuthoredRoom(
    const SaveAuthoredRoomSection& authoredRoom);

}  // namespace iggy3d
