#include "scene/npc/NpcActorMovementFrameIntent2D.hpp"

namespace {

void CountStatus(
	iggy::NpcActorMovementFrameIntent2DResult &result,
	iggy::NpcActorMovementIntent2DStatus status)
{
	switch (status) {
	case iggy::NpcActorMovementIntent2DStatus::Ready:
		++result.readyCount;
		return;
	case iggy::NpcActorMovementIntent2DStatus::NoMovement:
		++result.noMovementCount;
		return;
	case iggy::NpcActorMovementIntent2DStatus::MissingControl:
		++result.missingControlCount;
		return;
	case iggy::NpcActorMovementIntent2DStatus::ActorNotPresent:
		++result.actorNotPresentCount;
		return;
	case iggy::NpcActorMovementIntent2DStatus::UnsupportedBehavior:
		++result.unsupportedBehaviorCount;
		return;
	case iggy::NpcActorMovementIntent2DStatus::InvalidMoveMode:
		++result.invalidMoveModeCount;
		return;
	}
}

} // namespace

namespace iggy {

bool NpcActorMovementFrameIntent2DResult::hasReadyMovement() const
{
	return readyCount > 0;
}

NpcActorMovementFrameIntent2DResult NpcActorMovementFrameIntentProjector2D::project(
	const NpcActorFrameState2DProjectionResult &frameState,
	const NpcActorMovementIntent2DConfig &config) const
{
	NpcActorMovementFrameIntent2DResult result;
	result.frameState = frameState;
	result.issues = frameState.issues;

	if (frameState.entries.empty()) {
		result.status = NpcActorMovementFrameIntent2DStatus::NoFrameEntries;
		return result;
	}

	NpcActorMovementIntentProjector2D projector;
	for (std::size_t frameIndex = 0; frameIndex < frameState.entries.size(); ++frameIndex) {
		const NpcActorFrameState2D &frame = frameState.entries[frameIndex];
		NpcActorMovementIntent2D intent = projector.project(frame, config);
		CountStatus(result, intent.status);
		result.entries.push_back({ frameIndex, frame, intent });
	}

	result.entryCount = result.entries.size();
	result.status = result.readyCount > 0
		? NpcActorMovementFrameIntent2DStatus::Projected
		: NpcActorMovementFrameIntent2DStatus::NoReadyMovement;
	return result;
}

} // namespace iggy
