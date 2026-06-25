#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "content/assets/RoomAsset.hpp"
#include "runtime/save/SaveEnvelope.hpp"

namespace iggy3d {

struct ProductAsciiRoomAuthoringRequest;
struct ProductAsciiRoomAuthoringResult;

struct ProductActiveRoomState {
  bool loaded = false;
  std::string status = "not_loaded";
  std::string reasonCode = "not_loaded";
  std::string source = "none";
  std::string roomId = "none";
  std::string sourceName = "none";
  std::string sourceSubset = "none";
  bool hasAuthoredRoom = false;
  std::uint64_t authoredFloorCount = 0;
  std::uint64_t authoredWallCount = 0;
  std::uint64_t authoredMarkerCount = 0;
  std::uint64_t staticMeshCount = 0;
  std::uint64_t anchorCount = 0;
  std::uint64_t openingCount = 0;
  std::uint64_t spatialSurfaceCount = 0;
  std::uint64_t walkableSurfaceCount = 0;
  std::uint64_t actorBlockerSurfaceCount = 0;
  std::uint64_t projectileBlockerSurfaceCount = 0;
  RoomAsset room;
  SaveAuthoredRoomSection authoredRoom;
};

ProductActiveRoomState buildProductActiveRoomFromAsciiAuthoring(
    const ProductAsciiRoomAuthoringRequest& request,
    const ProductAsciiRoomAuthoringResult& authoring);

ProductActiveRoomState buildProductActiveRoomFromPackageRoom(
    const RoomAsset& room,
    std::string_view packageId,
    std::string_view scenarioId);

ProductActiveRoomState buildProductActiveRoomFromSavedAuthoredRoom(
    const SaveAuthoredRoomSection& authoredRoom);

}  // namespace iggy3d
