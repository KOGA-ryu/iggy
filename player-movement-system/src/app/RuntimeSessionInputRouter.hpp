#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "input/RawInput.hpp"
#include "session/SessionCommandSource.hpp"

namespace dev {

class RuntimeSessionInputRouter {
public:
	RuntimeSessionInputRouter(QueuedSessionCommandSource &sessionCommands, RuntimeInputBindings bindings = {});

	[[nodiscard]] RuntimeInputRouteResult route(const RawInputEvent &event, const RuntimeInputContext &context) const;

private:
	QueuedSessionCommandSource &sessionCommands_;
	RuntimeInputBindings bindings_;
};

} // namespace dev
