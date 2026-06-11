#pragma once

#include <vector>

#include "inventory/InventoryEventSink.hpp"

namespace dev {

class InventoryEventRecorder : public InventoryEventSink {
public:
	void emit(const InventoryEvent &event) override;
	void clear();

	[[nodiscard]] const std::vector<InventoryEvent> &events() const;

private:
	std::vector<InventoryEvent> events_;
};

} // namespace dev
