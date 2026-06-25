#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductAsciiRoomActivationResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string roomId = "none";
  std::string packageId = "none";
  std::string scenarioId = "none";
  bool sessionCreated = false;
  bool playerSpawned = false;
  std::size_t playerCount = 0;
  std::size_t entityCount = 0;
  std::size_t objectiveCount = 0;
  std::size_t wallCount = 0;
  std::size_t markerCount = 0;
  std::uint64_t runtimeStateHash = 0;
};

ProductAsciiRoomActivationResult activateProductAsciiRoomPreview(
    std::optional<Session>& activeSession,
    ProductAppWindowState& window);

}  // namespace iggy3d
