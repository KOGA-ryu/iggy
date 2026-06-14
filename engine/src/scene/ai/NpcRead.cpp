#include "scene/ai/NpcRead.hpp"

#include <algorithm>

namespace iggy {

bool NpcRead::hasRankedEnts() const
{
	return !rankedEnts.empty();
}

bool NpcRead::hasIssues() const
{
	return !issues.empty();
}

NpcRead NpcReader::read(const NpcHand &hand, const NpcReadConfig &config) const
{
	NpcRead read;
	read.hand = hand;
	read.issues = hand.issues;

	for (std::size_t index = 0; index < hand.ents.size(); ++index) {
		const NpcHandEnt &ent = hand.ents[index];
		const float score = ent.weight;
		if (score >= config.minimumScore) {
			read.rankedEnts.push_back({
				ent,
				score,
				index,
			});
		}
	}

	std::stable_sort(
		read.rankedEnts.begin(),
		read.rankedEnts.end(),
		[](const NpcReadEnt &lhs, const NpcReadEnt &rhs) {
			return lhs.score > rhs.score;
		});

	return read;
}

} // namespace iggy
