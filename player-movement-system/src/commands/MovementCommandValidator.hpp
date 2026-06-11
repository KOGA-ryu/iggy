#pragma once

#include "commands/MovementCommand.hpp"

namespace dev {

class MovementCommandValidator {
public:
	[[nodiscard]] bool accepts(const MovementCommand &command) const;
};

} // namespace dev
