#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

struct SessionState;

using StateHashValue = std::uint64_t;

StateHashValue computeStateHash(const SessionState& state);
std::string formatStateHash(StateHashValue value);

}  // namespace iggy3d
