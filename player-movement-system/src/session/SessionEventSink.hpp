#pragma once

#include "session/SessionEvent.hpp"

namespace dev {

class SessionEventSink {
public:
	virtual ~SessionEventSink() = default;

	virtual void emit(const SessionEvent &event) = 0;
};

} // namespace dev
