#include "CombatSystem.hpp"

namespace dev {

namespace {

CombatEventType EventTypeFor(CombatResultType result)
{
	switch (result) {
	case CombatResultType::Hit:
		return CombatEventType::Hit;
	case CombatResultType::Defeated:
		return CombatEventType::Defeated;
	case CombatResultType::InvalidTarget:
		return CombatEventType::Rejected;
	}
	return CombatEventType::Rejected;
}

void EmitCombatEvent(CombatEventSink *eventSink, const Target &target, const CombatResult &result)
{
	if (eventSink == nullptr)
		return;
	eventSink->emit({
		.type = EventTypeFor(result.type),
		.target = target,
		.damage = result.damage,
		.remainingHitPoints = result.remainingHitPoints,
		.result = result.type,
	});
}

} // namespace

CombatSystem::CombatSystem(CombatEventSink *eventSink)
    : eventSink_(eventSink)
{
}

CombatRegistry &CombatSystem::registry()
{
	return registry_;
}

const CombatRegistry &CombatSystem::registry() const
{
	return registry_;
}

CombatResult CombatSystem::resolvePlayerAttack(const Player &player, const DestinationAction &action)
{
	if (action.type != DestinationActionType::Attack)
		return {};

	Combatant *target = registry_.find(action.target);
	if (target == nullptr) {
		EmitCombatEvent(eventSink_, action.target, {});
		return {};
	}

	CombatResult result = resolver_.resolveAttack(player.combatStats, *target);
	EmitCombatEvent(eventSink_, action.target, result);
	return result;
}

} // namespace dev
