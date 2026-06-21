#include "runtime3d/Runtime3DSession.hpp"

#include <utility>

namespace iggy::runtime3d {

Runtime3DSession::Runtime3DSession(Runtime3DSessionState state)
	: state_(std::move(state))
{
}

Runtime3DSession Runtime3DSession::Create(Runtime3DWorldState world, Runtime3DSessionLifecycle lifecycle)
{
	Runtime3DSessionState state;
	state.lifecycle = lifecycle;
	state.world = std::move(world);
	return Runtime3DSession(std::move(state));
}

const Runtime3DSessionState &Runtime3DSession::state() const
{
	return state_;
}

Runtime3DSessionState &Runtime3DSession::state()
{
	return state_;
}

} // namespace iggy::runtime3d
