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

  return expect(stats.frames == 0U, "default frame count") &&
         expect(stats.packets == 0U, "default packet count") &&
         expect(stats.handled == 0U, "default handled count") &&
         expect(stats.ignored == 0U, "default ignored count");
}

bool facadeStubsLeaveStatsStable() {
  cr::Facade facade;

  facade.beginFrame({});
  facade.handle({});
  const cr::Stats& stats = facade.stats();

  return expect(stats.frames == 0U, "facade frame count") &&
         expect(stats.packets == 0U, "facade packet count") &&
         expect(stats.handled == 0U, "facade handled count") &&
         expect(stats.ignored == 0U, "facade ignored count");
}

}  // namespace

int main() {
  const bool ok = defaultStateIsStable() &&
                  defaultStatsAreStable() &&
                  facadeStubsLeaveStatsStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
