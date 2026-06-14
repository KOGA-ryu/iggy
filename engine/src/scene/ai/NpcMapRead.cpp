#include "scene/ai/NpcMapRead.hpp"

#include <algorithm>

namespace iggy {

namespace {

bool Contains(const std::vector<ResourceId> &values, const ResourceId &value)
{
	return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

bool NpcMapRead::hasRankedEnts() const
{
	return !rankedEnts.empty();
}

bool NpcMapRead::hasIssues() const
{
	return !issues.empty();
}

NpcMapRead NpcMapReader::read(
	const NpcHand &hand,
	const AiMapQuery2DResult &map,
	const NpcMapReadConfig &config) const
{
	NpcMapRead read;
	read.hand = hand;
	read.map = map;
	read.issues = hand.issues;

	for (std::size_t index = 0; index < hand.ents.size(); ++index) {
		const NpcHandEnt &ent = hand.ents[index];
		NpcMapReadEnt ranked;
		ranked.ent = ent;
		ranked.baseWeight = ent.weight;
		ranked.handIndex = index;

		for (const ResourceId &tag : ent.mapTags) {
			if (Contains(map.tags, tag)) {
				if (!Contains(ranked.matchedMapTags, tag))
					ranked.matchedMapTags.push_back(tag);
			} else if (!Contains(ranked.unmatchedMapTags, tag)) {
				ranked.unmatchedMapTags.push_back(tag);
			}
		}

		ranked.mapMatched = !ranked.matchedMapTags.empty();
		if (config.requireMapTagMatch && !ent.mapTags.empty() && !ranked.mapMatched)
			continue;

		ranked.score = ranked.baseWeight
			+ static_cast<float>(ranked.matchedMapTags.size()) * config.matchedMapTagBonus
			- static_cast<float>(ranked.unmatchedMapTags.size()) * config.unmatchedMapTagPenalty;

		if (ranked.score >= config.minimumScore)
			read.rankedEnts.push_back(ranked);
	}

	std::stable_sort(
		read.rankedEnts.begin(),
		read.rankedEnts.end(),
		[](const NpcMapReadEnt &lhs, const NpcMapReadEnt &rhs) {
			return lhs.score > rhs.score;
		});

	return read;
}

} // namespace iggy
