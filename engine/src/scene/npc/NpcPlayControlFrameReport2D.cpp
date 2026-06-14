#include "scene/npc/NpcPlayControlFrameReport2D.hpp"

namespace {

bool HasEarlierMatchingNonEmptyNpcId(
	const std::vector<iggy::NpcPlayControlFrameApplyEntry2D> &entries,
	std::size_t currentIndex)
{
	const iggy::ResourceId &npcId = entries[currentIndex].request.npcId;
	if (npcId.empty())
		return false;

	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].request.npcId == npcId)
			return true;
	}
	return false;
}

void CountFailure(iggy::NpcPlayControlFrameReport2D &report, iggy::NpcPlayControlApplyStatus status)
{
	switch (status) {
	case iggy::NpcPlayControlApplyStatus::Applied:
		break;
	case iggy::NpcPlayControlApplyStatus::NoProposal:
		++report.noProposalCount;
		break;
	case iggy::NpcPlayControlApplyStatus::MissingNpcId:
		++report.missingNpcIdCount;
		break;
	case iggy::NpcPlayControlApplyStatus::InvalidObjective:
		++report.invalidObjectiveCount;
		break;
	case iggy::NpcPlayControlApplyStatus::InvalidBehavior:
		++report.invalidBehaviorCount;
		break;
	}
}

} // namespace

namespace iggy {

bool NpcPlayControlFrameReport2D::hasEvents() const
{
	return !events.empty();
}

NpcPlayControlFrameReport2D NpcPlayControlFrameReporter2D::report(
	const NpcPlayControlFrameApply2DResult &result) const
{
	NpcPlayControlFrameReport2D report;
	report.apply = result;
	report.registry = result.registry;
	report.proposalCount = result.entries.size();
	report.changed = result.changed;

	for (const NpcPlayControlFrameApplyEntry2D &entry : result.entries) {
		if (entry.apply.applied()) {
			++report.appliedCount;
			report.events.push_back(NpcPlayControlFrameEvent2D::ProposalApplied);
		} else {
			++report.failedCount;
			CountFailure(report, entry.apply.status);
			report.events.push_back(NpcPlayControlFrameEvent2D::ProposalFailed);
		}
	}

	for (const NpcPlayControlFrameApplyEntry2D &entry : result.entries) {
		if (!entry.apply.applied())
			continue;

		if (entry.apply.appended) {
			++report.appendedCount;
			report.events.push_back(NpcPlayControlFrameEvent2D::ProposalAppended);
		} else {
			++report.updatedCount;
			report.events.push_back(NpcPlayControlFrameEvent2D::ProposalUpdated);
		}
	}

	for (std::size_t index = 0; index < result.entries.size(); ++index) {
		if (HasEarlierMatchingNonEmptyNpcId(result.entries, index)) {
			++report.duplicateNpcProposalCount;
			report.events.push_back(NpcPlayControlFrameEvent2D::DuplicateNpcProposalObserved);
		}
	}

	report.events.push_back(result.changed
			? NpcPlayControlFrameEvent2D::ControlChanged
			: NpcPlayControlFrameEvent2D::NoControlChanged);
	return report;
}

} // namespace iggy
