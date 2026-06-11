#pragma once

#include "inventory/InventoryEvent.hpp"

namespace dev {

class InventoryEventSink {
public:
	virtual ~InventoryEventSink() = default;

	virtual void emit(const InventoryEvent &event) = 0;
};

} // namespace dev
