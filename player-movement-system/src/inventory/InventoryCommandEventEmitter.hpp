#pragma once

#include "inventory/InventoryCommand.hpp"
#include "inventory/InventoryEventSink.hpp"

namespace dev {

class InventoryCommandEventEmitter {
public:
	explicit InventoryCommandEventEmitter(InventoryEventSink *eventSink = nullptr);

	void emit(const InventoryCommandResult &result) const;

private:
	InventoryEventSink *eventSink_;
};

} // namespace dev
