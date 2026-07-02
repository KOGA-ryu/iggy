#pragma once

#include <cstdint>

namespace iggy3d::creative {

struct Stats {
  std::uint64_t frames = 0;
  std::uint64_t packets = 0;
  std::uint64_t handled = 0;
  std::uint64_t ignored = 0;
  std::uint64_t commandAttempts = 0;
  std::uint64_t commandSuccesses = 0;
  std::uint64_t commandFailures = 0;
  std::uint64_t objectsCreated = 0;
  std::uint64_t roomsCreated = 0;
};

void resetStats(Stats& stats) noexcept;
void recordCommandAttempt(Stats& stats) noexcept;
void recordCommandSuccess(Stats& stats) noexcept;
void recordCommandFailure(Stats& stats) noexcept;
void recordObjectCreated(Stats& stats) noexcept;
void recordRoomCreated(Stats& stats) noexcept;

}  // namespace iggy3d::creative
