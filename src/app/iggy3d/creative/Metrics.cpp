#include "app/iggy3d/creative/Metrics.hpp"

namespace iggy3d::creative {

void resetStats(Stats& stats) noexcept {
  stats = Stats{};
}

void recordCommandAttempt(Stats& stats) noexcept {
  ++stats.commandAttempts;
}

void recordCommandSuccess(Stats& stats) noexcept {
  ++stats.commandSuccesses;
}

void recordCommandFailure(Stats& stats) noexcept {
  ++stats.commandFailures;
}

void recordObjectCreated(Stats& stats) noexcept {
  ++stats.objectsCreated;
}

void recordRoomCreated(Stats& stats) noexcept {
  ++stats.roomsCreated;
}

}  // namespace iggy3d::creative
