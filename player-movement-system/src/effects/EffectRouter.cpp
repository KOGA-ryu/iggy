#include "EffectRouter.hpp"

namespace dev {

namespace {

constexpr float HitStopDurationSeconds = 0.08F;

} // namespace

EffectRouter::EffectRouter(EffectSink &sink)
    : sink_(sink)
{
}

void EffectRouter::route(const MovementEvent &event) const
{
	switch (event.type) {
	case MovementEventType::StepCommitted:
		emit({
		    .type = EffectRequestType::Footstep,
		    .tile = event.tile,
		});
		break;
	case MovementEventType::PathBlocked:
	case MovementEventType::CommandRejected:
	case MovementEventType::ActionRejected:
		emit({
		    .type = EffectRequestType::BlockedFeedback,
		    .tile = event.tile,
		});
		break;
	case MovementEventType::ActionExecuted:
		emit({
		    .type = EffectRequestType::ActionCue,
		    .tile = event.tile,
		});
		break;
	case MovementEventType::CommandAccepted:
	case MovementEventType::PathStarted:
	case MovementEventType::DestinationActionReady:
	case MovementEventType::AnimationLocked:
	case MovementEventType::AnimationUnlocked:
	case MovementEventType::EnemyPursuitStopped:
		break;
	}
}

void EffectRouter::route(const CombatEvent &event) const
{
	switch (event.type) {
	case CombatEventType::Hit:
		emit({
		    .type = EffectRequestType::DamageNumber,
		    .tile = event.target.tile,
		    .target = event.target,
		    .damage = event.damage,
		});
		emit({
		    .type = EffectRequestType::HitImpact,
		    .tile = event.target.tile,
		    .target = event.target,
		    .damage = event.damage,
		});
		emit({
		    .type = EffectRequestType::HitStop,
		    .tile = event.target.tile,
		    .target = event.target,
		    .durationSeconds = HitStopDurationSeconds,
		});
		break;
	case CombatEventType::Defeated:
		emit({
		    .type = EffectRequestType::DamageNumber,
		    .tile = event.target.tile,
		    .target = event.target,
		    .damage = event.damage,
		});
		emit({
		    .type = EffectRequestType::DefeatCue,
		    .tile = event.target.tile,
		    .target = event.target,
		    .damage = event.damage,
		});
		emit({
		    .type = EffectRequestType::HitStop,
		    .tile = event.target.tile,
		    .target = event.target,
		    .durationSeconds = HitStopDurationSeconds,
		});
		break;
	case CombatEventType::Rejected:
		break;
	}
}

void EffectRouter::emit(EffectRequest request) const
{
	sink_.emit(request);
}

} // namespace dev
