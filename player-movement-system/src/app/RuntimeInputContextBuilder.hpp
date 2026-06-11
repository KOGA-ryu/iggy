#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeInputContextBuilder {
public:
	explicit RuntimeInputContextBuilder(const GameSession &session);

	[[nodiscard]] RuntimeInputContext build(const RuntimeInputSettings &settings) const;

private:
	const GameSession &session_;
};

} // namespace dev
