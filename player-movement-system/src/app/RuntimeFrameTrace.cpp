#include "RuntimeFrameTrace.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(SessionCommandResultType type)
{
	switch (type) {
	case SessionCommandResultType::Applied:
		return "Applied";
	case SessionCommandResultType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

const char *ToString(SessionCommandType type)
{
	switch (type) {
	case SessionCommandType::StartNewGame:
		return "StartNewGame";
	case SessionCommandType::SaveSlot:
		return "SaveSlot";
	case SessionCommandType::LoadSlot:
		return "LoadSlot";
	case SessionCommandType::SetMode:
		return "SetMode";
	}
	return "Unknown";
}

const char *ToString(SessionEventType type)
{
	switch (type) {
	case SessionEventType::GameStarted:
		return "GameStarted";
	case SessionEventType::SaveCompleted:
		return "SaveCompleted";
	case SessionEventType::SaveFailed:
		return "SaveFailed";
	case SessionEventType::LoadCompleted:
		return "LoadCompleted";
	case SessionEventType::LoadFailed:
		return "LoadFailed";
	case SessionEventType::ModeChanged:
		return "ModeChanged";
	case SessionEventType::ModeChangeRejected:
		return "ModeChangeRejected";
	}
	return "Unknown";
}

const char *ToString(InventoryScriptRunStatus status)
{
	switch (status) {
	case InventoryScriptRunStatus::LoadFailed:
		return "LoadFailed";
	case InventoryScriptRunStatus::NoActivePlayer:
		return "NoActivePlayer";
	case InventoryScriptRunStatus::Completed:
		return "Completed";
	}
	return "Unknown";
}

const char *ToString(InventoryCommandResultType type)
{
	switch (type) {
	case InventoryCommandResultType::Applied:
		return "Applied";
	case InventoryCommandResultType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

const char *ToString(InventoryCommandType type)
{
	switch (type) {
	case InventoryCommandType::EquipItem:
		return "EquipItem";
	case InventoryCommandType::UnequipSlot:
		return "UnequipSlot";
	}
	return "Unknown";
}

const char *ToString(EquipmentResultType type)
{
	switch (type) {
	case EquipmentResultType::Equipped:
		return "Equipped";
	case EquipmentResultType::Unequipped:
		return "Unequipped";
	case EquipmentResultType::MissingItem:
		return "MissingItem";
	case EquipmentResultType::NotEquippable:
		return "NotEquippable";
	case EquipmentResultType::WrongSlot:
		return "WrongSlot";
	case EquipmentResultType::InventoryFull:
		return "InventoryFull";
	case EquipmentResultType::EmptySlot:
		return "EmptySlot";
	}
	return "Unknown";
}

const char *ToString(EquipmentSlot slot)
{
	switch (slot) {
	case EquipmentSlot::Weapon:
		return "Weapon";
	case EquipmentSlot::Armor:
		return "Armor";
	case EquipmentSlot::Accessory:
		return "Accessory";
	}
	return "Unknown";
}

const char *ToString(InventoryEventType type)
{
	switch (type) {
	case InventoryEventType::Equipped:
		return "Equipped";
	case InventoryEventType::Unequipped:
		return "Unequipped";
	case InventoryEventType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

const char *ToString(MovementEventType type)
{
	switch (type) {
	case MovementEventType::CommandAccepted:
		return "CommandAccepted";
	case MovementEventType::CommandRejected:
		return "CommandRejected";
	case MovementEventType::PathStarted:
		return "PathStarted";
	case MovementEventType::PathBlocked:
		return "PathBlocked";
	case MovementEventType::StepCommitted:
		return "StepCommitted";
	case MovementEventType::DestinationActionReady:
		return "DestinationActionReady";
	case MovementEventType::ActionExecuted:
		return "ActionExecuted";
	case MovementEventType::ActionRejected:
		return "ActionRejected";
	case MovementEventType::AnimationLocked:
		return "AnimationLocked";
	case MovementEventType::AnimationUnlocked:
		return "AnimationUnlocked";
	case MovementEventType::EnemyPursuitStopped:
		return "EnemyPursuitStopped";
	}
	return "Unknown";
}

const char *ToString(EnemyPursuitStopReason reason)
{
	switch (reason) {
	case EnemyPursuitStopReason::BudgetSpent:
		return "BudgetSpent";
	case EnemyPursuitStopReason::Blocked:
		return "Blocked";
	case EnemyPursuitStopReason::AlreadyAtTarget:
		return "AlreadyAtTarget";
	case EnemyPursuitStopReason::AttackRangeReached:
		return "AttackRangeReached";
	}
	return "Unknown";
}

const char *ToString(MovementCommandType type)
{
	switch (type) {
	case MovementCommandType::WalkTo:
		return "WalkTo";
	case MovementCommandType::MoveThenAct:
		return "MoveThenAct";
	case MovementCommandType::StandAndAct:
		return "StandAndAct";
	case MovementCommandType::Stop:
		return "Stop";
	}
	return "Unknown";
}

const char *ToString(CombatEventType type)
{
	switch (type) {
	case CombatEventType::Hit:
		return "Hit";
	case CombatEventType::Defeated:
		return "Defeated";
	case CombatEventType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

const char *ToString(EffectRequestType type)
{
	switch (type) {
	case EffectRequestType::Footstep:
		return "Footstep";
	case EffectRequestType::BlockedFeedback:
		return "BlockedFeedback";
	case EffectRequestType::ActionCue:
		return "ActionCue";
	case EffectRequestType::DamageNumber:
		return "DamageNumber";
	case EffectRequestType::HitImpact:
		return "HitImpact";
	case EffectRequestType::HitStop:
		return "HitStop";
	case EffectRequestType::DefeatCue:
		return "DefeatCue";
	}
	return "Unknown";
}

std::string PointText(Point point)
{
	std::ostringstream line;
	line << "(" << point.x << "," << point.y << ")";
	return line.str();
}

} // namespace

std::vector<std::string> RuntimeFrameTrace::format(const RuntimeFrameReport &report) const
{
	std::vector<std::string> lines;
	{
		std::ostringstream line;
		line << "frame rawInput=" << report.rawInputEventsRouted
		     << " sessionResults=" << report.sessionCommandResults.size()
		     << " inventoryScripts=" << report.inventoryScriptResults.size()
		     << " inventoryResults=" << report.inventoryCommandResults.size()
		     << " movementQueued=" << report.movementCommandsQueued
		     << " movementEvents=" << report.frameEvents.movementEvents().size()
		     << " combatEvents=" << report.frameEvents.combatEvents().size()
		     << " effects=" << report.frameEvents.effectRequests().size()
		     << " sessionEvents=" << report.sessionEvents.size()
		     << " inventoryEvents=" << report.inventoryEvents.size();
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.sessionCommandResults.size(); ++i) {
		const SessionCommandResult &result = report.sessionCommandResults[i];
		std::ostringstream line;
		line << "sessionResult[" << i << "] type=" << ToString(result.type)
		     << " command=" << ToString(result.command.type);
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.inventoryScriptResults.size(); ++i) {
		const InventoryScriptRunResult &result = report.inventoryScriptResults[i];
		std::ostringstream line;
		line << "inventoryScript[" << i << "] status=" << ToString(result.status)
		     << " results=" << result.commandResults.size();
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.inventoryCommandResults.size(); ++i) {
		const InventoryCommandResult &result = report.inventoryCommandResults[i];
		std::ostringstream line;
		line << "inventoryResult[" << i << "] type=" << ToString(result.type)
		     << " command=" << ToString(result.command.type)
		     << " equipment=" << ToString(result.equipmentResult.type)
		     << " item=" << result.equipmentResult.itemId
		     << " slot=" << ToString(result.equipmentResult.slot);
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.sessionEvents.size(); ++i) {
		const SessionEvent &event = report.sessionEvents[i];
		std::ostringstream line;
		line << "sessionEvent[" << i << "] type=" << ToString(event.type)
		     << " command=" << ToString(event.commandType);
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.inventoryEvents.size(); ++i) {
		const InventoryEvent &event = report.inventoryEvents[i];
		std::ostringstream line;
		line << "inventoryEvent[" << i << "] type=" << ToString(event.type)
		     << " command=" << ToString(event.commandType)
		     << " result=" << ToString(event.commandResult)
		     << " equipment=" << ToString(event.equipmentResult)
		     << " item=" << event.itemId
		     << " slot=" << ToString(event.slot);
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.frameEvents.movementEvents().size(); ++i) {
		const MovementEvent &event = report.frameEvents.movementEvents()[i];
		std::ostringstream line;
		line << "movementEvent[" << i << "] type=" << ToString(event.type)
		     << " player=" << static_cast<int>(event.playerId)
		     << " tile=" << PointText(event.tile);
		if (event.commandType.has_value())
			line << " command=" << ToString(*event.commandType);
		if (event.enemyId.has_value())
			line << " enemy=" << *event.enemyId;
		if (event.enemyPursuitStopReason.has_value())
			line << " pursuitStop=" << ToString(*event.enemyPursuitStopReason);
		if (event.enemyPursuitStepsCommitted.has_value())
			line << " pursuitSteps=" << *event.enemyPursuitStepsCommitted;
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.frameEvents.combatEvents().size(); ++i) {
		const CombatEvent &event = report.frameEvents.combatEvents()[i];
		std::ostringstream line;
		line << "combatEvent[" << i << "] type=" << ToString(event.type)
		     << " damage=" << event.damage
		     << " remainingHp=" << event.remainingHitPoints;
		lines.push_back(line.str());
	}

	for (std::size_t i = 0; i < report.frameEvents.effectRequests().size(); ++i) {
		const EffectRequest &request = report.frameEvents.effectRequests()[i];
		std::ostringstream line;
		line << "effect[" << i << "] type=" << ToString(request.type)
		     << " tile=" << PointText(request.tile);
		lines.push_back(line.str());
	}

	return lines;
}

} // namespace dev
