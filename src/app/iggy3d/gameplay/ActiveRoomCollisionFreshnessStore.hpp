#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

class Session;
struct ProductAppWindowState;

struct ProductActiveRoomCollisionFreshnessResult {
  bool rebaked = false;
  std::uint64_t observedRoomRevision = 0;
  std::uint64_t observedSessionHash = 0;
  std::string reasonCode = "skipped_fresh";
};

ProductActiveRoomCollisionFreshnessResult ensureActiveRoomCollisionFresh(
    ProductAppWindowState& window,
    const Session* session);

}  // namespace iggy3d
