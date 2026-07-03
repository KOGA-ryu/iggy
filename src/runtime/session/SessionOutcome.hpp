#pragma once

#include <cstdint>

namespace iggy3d {

// Terminal session outcome. Split into its own tiny header so lower layers (the objective
// outcome table in runtime/objective/) can name it without pulling in SessionState -- and without
// a cyclic include when SessionState carries an ObjectiveOutcomeTable.
enum class SessionOutcome : std::uint8_t {
  None,
  DemoComplete,
  Victory,
  Defeat,
  Failed,
};

}  // namespace iggy3d
