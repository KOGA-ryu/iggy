#include "creative_desktop_command_test_runners.hpp"

#ifndef CREATIVE_DESKTOP_COMMAND_TEST_RUNNER
#error "CREATIVE_DESKTOP_COMMAND_TEST_RUNNER must name the focused test runner"
#endif

int main() {
  return CREATIVE_DESKTOP_COMMAND_TEST_RUNNER() ? 0 : 1;
}
