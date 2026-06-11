#pragma once

#include <cstddef>
#include <vector>

#include "commands/MovementCommand.hpp"

namespace dev {

class MovementCommandSource {
public:
	virtual ~MovementCommandSource() = default;

	[[nodiscard]] virtual std::vector<MovementCommand> drain() = 0;
};

class QueuedMovementCommandSource : public MovementCommandSource {
public:
	void enqueue(const MovementCommand &command);
	void clear();

	[[nodiscard]] std::vector<MovementCommand> drain() override;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;

private:
	std::vector<MovementCommand> commands_;
};

} // namespace dev
