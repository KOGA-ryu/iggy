#include "RuntimeInputSourceRouter.hpp"

#include "app/RuntimeInputContextBuilder.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeRawInputDrainer.hpp"

namespace dev {

RuntimeInputSourceRouter::RuntimeInputSourceRouter(
    const GameSession &session,
    QueuedSessionCommandSource &routedSessionCommands,
    QueuedMovementCommandSource &routedMovementCommands,
    const RuntimeSourceSettings &sources,
    const RuntimeInputSettings &input)
    : session_(session)
    , routedSessionCommands_(routedSessionCommands)
    , routedMovementCommands_(routedMovementCommands)
    , sources_(sources)
    , input_(input)
{
}

RuntimeInputDrainResult RuntimeInputSourceRouter::route()
{
	RuntimeInputRouter router { routedSessionCommands_, routedMovementCommands_, input_.bindings };
	RuntimeRawInputDrainer drainer { router };
	return drainer.drain(sources_.rawInputSources, RuntimeInputContextBuilder { session_ }.build(input_));
}

} // namespace dev
