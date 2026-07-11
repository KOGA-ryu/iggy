#include "app/iggy3d/creative/Facade.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool defaultStateIsStable() {
  const cr::State state;

  return expect(!state.flags.enabled, "state disabled") &&
         expect(!state.flags.active, "state inactive") &&
         expect(!state.flags.dirty, "state clean") &&
         expect(state.tool == cr::Tool::Select, "default tool") &&
         expect(state.frame.value == 0U, "default frame") &&
         expect(state.hovered.value == cr::kInvalidId, "default hovered") &&
         expect(state.selected.value == cr::kInvalidId, "default selected");
}

bool defaultStatsAreStable() {
  const cr::Stats stats;

  return expect(stats.commandAttempts == 0U, "default command attempts") &&
         expect(stats.commandSuccesses == 0U, "default command successes") &&
         expect(stats.commandFailures == 0U, "default command failures") &&
         expect(stats.objectsCreated == 0U, "default objects created") &&
         expect(stats.roomsCreated == 0U, "default rooms created");
}

bool facadeStubsLeaveStatsStable() {
  cr::Facade facade;

  facade.beginFrame({});
  facade.handle({});
  const cr::Stats& stats = facade.stats();

  return expect(stats.commandAttempts == 0U, "facade command attempts") &&
         expect(stats.commandSuccesses == 0U, "facade command successes") &&
         expect(stats.commandFailures == 0U, "facade command failures") &&
         expect(stats.objectsCreated == 0U, "facade objects created") &&
         expect(stats.roomsCreated == 0U, "facade rooms created");
}

}  // namespace

int main() {
  const bool ok = defaultStateIsStable() &&
                  defaultStatsAreStable() &&
                  facadeStubsLeaveStatsStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
