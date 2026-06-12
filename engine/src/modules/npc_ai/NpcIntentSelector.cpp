#include "modules/npc_ai/NpcIntentSelector.hpp"

namespace iggy::npc_ai {

NpcIntentSelector::NpcIntentSelector(NpcIntentSelectorConfig config)
    : config_(config)
{
}

NpcIntent NpcIntentSelector::select(const AwarenessState &awareness) const
{
	if (awareness.playerVisible)
		return { NpcIntentType::PursueVisibleTarget, awareness.lastSeenTile };
	if (awareness.alerted || awareness.alertTicksRemaining > 0)
		return { NpcIntentType::InvestigateLastSeen, awareness.lastSeenTile };
	if (config_.returnToPostWhenUnaware)
		return { NpcIntentType::ReturnToPost, {} };
	return { NpcIntentType::Idle, {} };
}

} // namespace iggy::npc_ai
