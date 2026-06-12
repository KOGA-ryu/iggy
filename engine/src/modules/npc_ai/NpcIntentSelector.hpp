#pragma once

#include "modules/npc_ai/AwarenessState.hpp"
#include "modules/npc_ai/NpcIntent.hpp"

namespace iggy::npc_ai {

struct NpcIntentSelectorConfig {
	bool returnToPostWhenUnaware = false;
};

class NpcIntentSelector {
public:
	explicit NpcIntentSelector(NpcIntentSelectorConfig config = {});

	[[nodiscard]] NpcIntent select(const AwarenessState &awareness) const;

private:
	NpcIntentSelectorConfig config_;
};

} // namespace iggy::npc_ai
