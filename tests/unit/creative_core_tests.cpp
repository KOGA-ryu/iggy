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

bool defaultFacadeEditorStateIsStable() {
  const cr::Facade facade;

  return expect(facade.toolState().activeTool == cr::Tool::Select,
                "default tool") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "default pointer target") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "default selected target") &&
         expect(facade.selectionState().candidateTarget.value == cr::kInvalidId,
                "default candidate target");
}

bool defaultStatsAreStable() {
  const cr::Stats stats;

  return expect(stats.commandAttempts == 0U, "default command attempts") &&
         expect(stats.commandSuccesses == 0U, "default command successes") &&
         expect(stats.commandFailures == 0U, "default command failures") &&
         expect(stats.objectsCreated == 0U, "default objects created") &&
         expect(stats.roomsCreated == 0U, "default rooms created");
}

bool facadeDefaultsLeaveStatsStable() {
  const cr::Facade facade;
  const cr::Stats& stats = facade.stats();

  return expect(stats.commandAttempts == 0U, "facade command attempts") &&
         expect(stats.commandSuccesses == 0U, "facade command successes") &&
         expect(stats.commandFailures == 0U, "facade command failures") &&
         expect(stats.objectsCreated == 0U, "facade objects created") &&
         expect(stats.roomsCreated == 0U, "facade rooms created");
}

}  // namespace

int main() {
  const bool ok = defaultFacadeEditorStateIsStable() &&
                  defaultStatsAreStable() &&
                  facadeDefaultsLeaveStatsStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
