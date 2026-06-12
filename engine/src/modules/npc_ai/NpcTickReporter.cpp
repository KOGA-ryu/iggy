#include "modules/npc_ai/NpcTickReporter.hpp"

namespace iggy::npc_ai {

namespace {

void AddEvent(NpcTickReport &report, NpcTickEventType type)
{
	report.events.push_back({ type });
}

void AddAwarenessEvent(NpcTickReport &report, AwarenessEventType type)
{
	switch (type) {
	case AwarenessEventType::PlayerSeen:
		AddEvent(report, NpcTickEventType::TargetSeen);
		break;
	case AwarenessEventType::PlayerLost:
		AddEvent(report, NpcTickEventType::TargetLost);
		break;
	case AwarenessEventType::AlertExpired:
		AddEvent(report, NpcTickEventType::AlertExpired);
		break;
	case AwarenessEventType::None:
		break;
	}
}

void AddNavigationEvent(NpcTickReport &report, NpcNavigationStatus status)
{
	switch (status) {
	case NpcNavigationStatus::Moving:
		AddEvent(report, NpcTickEventType::NavigationAdvanced);
		break;
	case NpcNavigationStatus::Arrived:
		AddEvent(report, NpcTickEventType::NavigationArrived);
		break;
	case NpcNavigationStatus::PathNotFound:
	case NpcNavigationStatus::RequestRejected:
		AddEvent(report, NpcTickEventType::NavigationFailed);
		break;
	case NpcNavigationStatus::NoMovement:
		AddEvent(report, NpcTickEventType::NoMovement);
		break;
	}
}

} // namespace

NpcTickReport NpcTickReporter::report(const NpcAgentState &previousState, const NpcAgentTickResult &tickResult) const
{
	NpcTickReport report;
	report.previousPosition = previousState.position;
	report.nextPosition = tickResult.state.position;
	report.awarenessEvent = tickResult.brain.awarenessEvent;
	report.intent = tickResult.brain.intent;
	report.movementPlan = tickResult.brain.movementPlan;
	report.navigation = tickResult.brain.navigation;

	AddAwarenessEvent(report, report.awarenessEvent.type);
	AddEvent(report, NpcTickEventType::IntentSelected);
	AddNavigationEvent(report, report.navigation.status);
	if (!(report.previousPosition == report.nextPosition))
		AddEvent(report, NpcTickEventType::PositionChanged);

	return report;
}

} // namespace iggy::npc_ai
