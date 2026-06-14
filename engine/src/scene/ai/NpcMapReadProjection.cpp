#include "scene/ai/NpcMapReadProjection.hpp"

namespace iggy {

NpcRead NpcMapReadProjection::toRead(const NpcMapRead &mapRead) const
{
	NpcRead read;
	read.hand = mapRead.hand;
	read.issues = mapRead.issues;

	for (const NpcMapReadEnt &ent : mapRead.rankedEnts) {
		read.rankedEnts.push_back({
			ent.ent,
			ent.score,
			ent.handIndex,
		});
	}

	return read;
}

} // namespace iggy
