#pragma once

#include <cstddef>
#include <vector>

#include "commands/MovementCommandDispatchResult.hpp"

namespace dev {

struct CommandReplayReport {
	std::vector<MovementCommandDispatchResult> results;

	[[nodiscard]] std::size_t acceptedCount() const;
	[[nodiscard]] std::size_t rejectedCount() const;
	[[nodiscard]] bool allAccepted() const;
};

} // namespace dev
