#pragma once

#include <vector>

#include "session/SessionCommand.hpp"

namespace dev {

class SessionCommandLog {
public:
	void record(const SessionCommand &command);
	void clear();

	[[nodiscard]] const std::vector<SessionCommand> &commands() const;
	[[nodiscard]] bool empty() const;

private:
	std::vector<SessionCommand> commands_;
};

} // namespace dev
