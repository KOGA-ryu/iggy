#include "runtime/RuntimeNpcAiDecisionQueueStep.hpp"

namespace iggy::runtime {

RuntimeNpcAiDecisionQueueResult RuntimeNpcAiDecisionQueueStep::push(
	const RuntimeCommandQueueState &queue,
	const RuntimeCommandQueueConfig &queueConfig,
	const AiMap2D &aiMap,
	const LevelTileMap &levelMap,
	const std::vector<RuntimeNpcAiDecisionQueueNpcInput> &npcs,
	const RuntimeNpcAiDecisionQueueConfig &config) const
{
	RuntimeNpcAiDecisionQueueResult result;
	std::vector<NpcAiMovementProposal2DResult> proposals;
	proposals.reserve(npcs.size());
	result.entries.reserve(npcs.size());

	for (std::size_t index = 0; index < npcs.size(); ++index) {
		RuntimeNpcAiDecisionQueueEntry entry;
		entry.npcIndex = index;
		entry.profile = npcs[index].profile;
		entry.state = npcs[index].state;
		entry.decision = NpcAiDecisionMaker2D {}.decide(aiMap, entry.profile, entry.state, config.decision);
		entry.route = NpcAiRouteRequestBuilder2D {}.build(entry.decision, config.route);
		entry.navigation = NpcAiNavigationRequestBuilder2D {}.build(entry.route, levelMap);
		entry.path = NpcAiPathReporter2D {}.findPath(entry.navigation, levelMap);
		entry.proposal = NpcAiMovementProposalBuilder2D {}.build(entry.path, config.proposal);
		proposals.push_back(entry.proposal);
		result.entries.push_back(entry);
	}

	result.queuePush = RuntimeNpcAiMovementQueueStep {}.push(queue, queueConfig, proposals);
	result.mapping = result.queuePush.mapping;
	result.queue = result.queuePush.queue;
	result.status = result.queuePush.status == RuntimeNpcAiMovementQueueStatus::Queued
		? RuntimeNpcAiDecisionQueueStatus::Queued
		: RuntimeNpcAiDecisionQueueStatus::RejectedFull;
	return result;
}

} // namespace iggy::runtime
