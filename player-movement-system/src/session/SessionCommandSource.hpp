#pragma once

#include <cstddef>
#include <vector>

#include "session/SessionCommand.hpp"

namespace dev {

class SessionCommandSource {
public:
	virtual ~SessionCommandSource() = default;

	[[nodiscard]] virtual std::vector<SessionCommand> drain() = 0;
};

class QueuedSessionCommandSource : public SessionCommandSource {
public:
	void enqueue(const SessionCommand &command);
	void clear();

	[[nodiscard]] std::vector<SessionCommand> drain() override;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;

private:
	std::vector<SessionCommand> commands_;
};

} // namespace dev
