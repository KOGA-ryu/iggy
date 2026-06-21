#pragma once

#include "runtime3d/Runtime3DSessionState.hpp"

namespace iggy::runtime3d {

class Runtime3DSession {
public:
	Runtime3DSession() = default;
	explicit Runtime3DSession(Runtime3DSessionState state);

	[[nodiscard]] static Runtime3DSession Create(
		Runtime3DWorldState world,
		Runtime3DSessionLifecycle lifecycle = Runtime3DSessionLifecycle::PlayingRealtime);

	[[nodiscard]] const Runtime3DSessionState &state() const;
	[[nodiscard]] Runtime3DSessionState &state();

private:
	Runtime3DSessionState state_;
};

} // namespace iggy::runtime3d
