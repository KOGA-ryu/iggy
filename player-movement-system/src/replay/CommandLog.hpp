#pragma once

#include <vector>

#include "commands/MovementCommand.hpp"

namespace dev {

class CommandLog {
public:
	void record(const MovementCommand &command);
	void clear();

	[[nodiscard]] const std::vector<MovementCommand> &commands() const;
	[[nodiscard]] bool empty() const;

private:
	std::vector<MovementCommand> commands_;
};

} // namespace dev

