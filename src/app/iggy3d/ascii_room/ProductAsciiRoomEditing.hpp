#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToEditableRoom.hpp"
#include "app/iggy3d/ascii_room/ProductAsciiRoomAuthoring.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"

namespace iggy3d {

struct ProductAsciiRoomEditingResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string failedStage = "not_started";
  AsciiRoomSource source;
  AsciiRoomGridBuildResult grid;
  AsciiRoomToEditableRoomResult editableRoom;
  ProductRoomAuthoringController controller;
  ProductRoomAuthoringSnapshot snapshot;
  std::vector<AsciiRoomDiagnostic> diagnostics;
  std::size_t width = 0;
  std::size_t height = 0;
  std::size_t floorCount = 0;
  std::size_t wallCount = 0;
  std::size_t markerCount = 0;
  std::size_t documentFloorCount = 0;
  std::size_t documentWallCount = 0;
  std::size_t projectedMeshCount = 0;
  std::size_t projectedFloorMeshCount = 0;
  std::size_t projectedWallMeshCount = 0;
};

ProductAsciiRoomEditingResult buildProductAsciiRoomEditing(
    const ProductAsciiRoomAuthoringRequest& request);

}  // namespace iggy3d
