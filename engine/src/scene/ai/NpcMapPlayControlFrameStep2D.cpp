#include "scene/ai/NpcMapPlayControlFrameStep2D.hpp"

namespace iggy {

bool NpcMapPlayControlFrameStep2DResult::changed() const
{
	return report.changed;
}

NpcMapPlayControlFrameStep2DResult NpcMapPlayControlFrameStepper2D::step(
	const NpcActorControlState2DRegistry &controls,
	const std::vector<NpcMapPlayControlFrameStep2DRequest> &requests,
	const NpcMapPlayControlFrameStep2DConfig &config) const
{
	NpcMapPlayControlFrameStep2DResult result;
	result.requestCount = requests.size();

	std::vector<NpcPlayControlFrameProposal2D> proposals;
	for (std::size_t index = 0; index < requests.size(); ++index) {
		const NpcMapPlayControlFrameStep2DRequest &request = requests[index];
		NpcMapPlayControlFrameStep2DEntry entry;
		entry.requestIndex = index;
		entry.request = request;
		entry.mapPlay = NpcMapPlayReporter {}.report(
			request.hand,
			request.map,
			config.rawRead,
			config.mapRead,
			config.fold);
		entry.proposal = NpcPlayControlProjector {}.project(
			entry.mapPlay.fold,
			request.proposalContext,
			config.proposal);
		entry.frameProposal = {
			request.npcId,
			entry.proposal,
		};

		if (entry.proposal.hasProposal())
			++result.proposedCount;
		else
			++result.proposalFailedCount;
		if (entry.mapPlay.mapChangedSelection)
			++result.mapChangedSelectionCount;

		proposals.push_back(entry.frameProposal);
		result.entries.push_back(entry);
	}

	result.proposalCount = proposals.size();
	result.apply = NpcPlayControlFrameApplier2D {}.apply(controls, proposals);
	result.report = NpcPlayControlFrameReporter2D {}.report(result.apply);
	result.registry = result.report.registry;
	result.appliedCount = result.report.appliedCount;
	result.failedApplyCount = result.report.failedCount;

	if (requests.empty())
		result.status = NpcMapPlayControlFrameStep2DStatus::NoRequests;
	else if (!result.report.changed)
		result.status = NpcMapPlayControlFrameStep2DStatus::NoControlsChanged;
	else
		result.status = NpcMapPlayControlFrameStep2DStatus::Ran;

	return result;
}

} // namespace iggy
