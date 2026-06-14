#include "scene/ai/NpcTell.hpp"

namespace {

bool SameHandEnt(const iggy::NpcHandEnt &lhs, const iggy::NpcHandEnt &rhs)
{
	return lhs.source == rhs.source
		&& lhs.entryId == rhs.entryId
		&& lhs.actionTag == rhs.actionTag
		&& lhs.behaviorState == rhs.behaviorState
		&& lhs.weight == rhs.weight
		&& lhs.mapTags == rhs.mapTags
		&& lhs.drawEntryIndex == rhs.drawEntryIndex;
}

bool SameReadEnt(const iggy::NpcReadEnt &lhs, const iggy::NpcReadEnt &rhs)
{
	return SameHandEnt(lhs.ent, rhs.ent)
		&& lhs.score == rhs.score
		&& lhs.handIndex == rhs.handIndex;
}

iggy::NpcTellLine LineForReadEnt(iggy::NpcTellLineOutcome outcome, const iggy::NpcReadEnt &ent)
{
	return {
		outcome,
		ent.ent.source,
		ent.ent.actionTag,
		ent.ent.behaviorState,
		ent.score,
		ent.handIndex,
		ent.ent.drawEntryIndex,
		0,
	};
}

iggy::NpcTellLine LineForIssue(const iggy::NpcHandIssue &issue, std::size_t issueIndex)
{
	return {
		iggy::NpcTellLineOutcome::InvalidDraw,
		issue.source,
		{},
		iggy::NpcBehaviorStateType::None,
		0.0F,
		0,
		0,
		issueIndex,
	};
}

} // namespace

namespace iggy {

bool NpcTell::hasLines() const
{
	return !lines.empty();
}

NpcTell NpcTeller::tell(const NpcPlay &play) const
{
	NpcTell tell;
	tell.play = play;
	tell.handEntCount = play.read.hand.ents.size();
	tell.rankedEntCount = play.read.rankedEnts.size();
	tell.issueCount = play.read.issues.size();
	tell.playedCount = play.hasPlay() ? 1 : 0;

	bool skippedSelected = false;
	if (play.hasPlay()) {
		tell.lines.push_back(LineForReadEnt(NpcTellLineOutcome::Played, play.selected));
	}

	for (const NpcReadEnt &ranked : play.read.rankedEnts) {
		if (play.hasPlay() && !skippedSelected && SameReadEnt(ranked, play.selected)) {
			skippedSelected = true;
			continue;
		}
		tell.lines.push_back(LineForReadEnt(NpcTellLineOutcome::Ranked, ranked));
	}

	for (std::size_t issueIndex = 0; issueIndex < play.read.issues.size(); ++issueIndex) {
		tell.lines.push_back(LineForIssue(play.read.issues[issueIndex], issueIndex));
	}

	return tell;
}

} // namespace iggy
