#pragma once

#include "combat/CombatEvent.hpp"

namespace dev {

class CombatEventSink {
public:
	virtual ~CombatEventSink() = default;

	virtual void emit(const CombatEvent &event) = 0;
};

} // namespace dev

