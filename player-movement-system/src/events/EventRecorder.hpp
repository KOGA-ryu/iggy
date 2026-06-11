#pragma once

#include <vector>

#include "events/MovementEventSink.hpp"

namespace dev {

class EventRecorder : public MovementEventSink {
public:
	void emit(const MovementEvent &event) override;
	void clear();

	[[nodiscard]] const std::vector<MovementEvent> &events() const;

private:
	std::vector<MovementEvent> events_;
};

} // namespace dev

