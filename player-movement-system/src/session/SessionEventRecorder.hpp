#pragma once

#include <vector>

#include "session/SessionEventSink.hpp"

namespace dev {

class SessionEventRecorder : public SessionEventSink {
public:
	void emit(const SessionEvent &event) override;
	void clear();

	[[nodiscard]] const std::vector<SessionEvent> &events() const;

private:
	std::vector<SessionEvent> events_;
};

} // namespace dev
