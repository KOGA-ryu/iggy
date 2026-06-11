#pragma once

#include <vector>

#include "combat/CombatEventSink.hpp"

namespace dev {

class CombatEventRecorder : public CombatEventSink {
public:
	void emit(const CombatEvent &event) override;
	void clear();

	[[nodiscard]] const std::vector<CombatEvent> &events() const;

private:
	std::vector<CombatEvent> events_;
};

} // namespace dev

