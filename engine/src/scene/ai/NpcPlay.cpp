#include "scene/ai/NpcPlay.hpp"

namespace iggy {

bool NpcPlay::hasPlay() const
{
	return status == NpcPlayStatus::Played;
}

NpcPlay NpcPlaySelector::play(const NpcRead &read) const
{
	NpcPlay play;
	play.read = read;

	if (read.rankedEnts.empty()) {
		play.status = NpcPlayStatus::NoPlayableEnt;
		return play;
	}

	play.status = NpcPlayStatus::Played;
	play.selected = read.rankedEnts.front();
	return play;
}

} // namespace iggy
