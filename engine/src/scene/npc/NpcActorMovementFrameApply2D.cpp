#include "scene/npc/NpcActorMovementFrameApply2D.hpp"

namespace {

std::optional<std::size_t> FindActorIndex(
	const iggy::NpcActorState2DRegistry &registry,
	const iggy::ResourceId &npcId)
{
	for (std::size_t index = 0; index < registry.actors.size(); ++index) {
		if (registry.actors[index].npcId == npcId) {
			return index;
		}
	}
	return std::nullopt;
}

void CountExecutorStatus(
	iggy::NpcActorMovementFrameApply2DResult &result,
	const iggy::NpcActorMovementExecutor2DResult &executor)
{
	switch (executor.status) {
	case iggy::NpcActorMovementExecutor2DStatus::Moved:
		++result.movedCount;
		++result.appliedCount;
		break;
	case iggy::NpcActorMovementExecutor2DStatus::BlockedByNpc:
		++result.blockedCount;
		break;
	case iggy::NpcActorMovementExecutor2DStatus::NoMovement:
		++result.noMovementCount;
		break;
	case iggy::NpcActorMovementExecutor2DStatus::Rejected:
	case iggy::NpcActorMovementExecutor2DStatus::ActorMismatch:
	case iggy::NpcActorMovementExecutor2DStatus::ActorNotPresent:
		++result.rejectedCount;
		break;
	}
}

} // namespace

namespace iggy {

bool NpcActorMovementFrameApply2DResult::applied() const
{
	return status == NpcActorMovementFrameApply2DStatus::Applied;
}

bool NpcActorMovementFrameApply2DResult::hasChanges() const
{
	return changed;
}

NpcActorMovementFrameApply2DResult NpcActorMovementFrameApplier2D::apply(
	const NpcActorState2DRegistry &registry,
	const std::vector<NpcActorMovementFrameApply2DRequest> &requests) const
{
	NpcActorMovementFrameApply2DResult result;
	result.inputRegistry = registry;
	result.registry = registry;
	result.requestCount = requests.size();

	for (std::size_t requestIndex = 0; requestIndex < requests.size(); ++requestIndex) {
		const NpcActorMovementFrameApply2DRequest &request = requests[requestIndex];
		NpcActorMovementFrameApply2DEntry entry;
		entry.requestIndex = requestIndex;
		entry.request = request;

		entry.actorIndex = FindActorIndex(result.registry, request.filter.step.npcId);
		if (!entry.actorIndex.has_value()) {
			entry.hasIssue = true;
			entry.issue = NpcActorMovementFrameApply2DIssueCode::ActorNotFound;
			++result.missingActorCount;
			result.entries.push_back(entry);
			continue;
		}

		entry.executor = NpcActorMovementExecutor2D {}.execute(
			result.registry.actors[*entry.actorIndex],
			request.filter);
		CountExecutorStatus(result, entry.executor);

		if (entry.executor.moved()) {
			result.registry.actors[*entry.actorIndex] = entry.executor.resultActor;
			++result.changedCount;
			result.changed = true;
		}

		result.entries.push_back(entry);
	}

	if (result.changed) {
		result.status = NpcActorMovementFrameApply2DStatus::Applied;
	}
	return result;
}

} // namespace iggy
