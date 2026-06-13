#include "runtime/RuntimeInteractionCommandStep.hpp"

namespace {

iggy::runtime::RuntimeInteractionCommandStatus StatusForPlan(iggy::InteractionPlan2DStatus status)
{
	switch (status) {
	case iggy::InteractionPlan2DStatus::Ready:
		return iggy::runtime::RuntimeInteractionCommandStatus::Ready;
	case iggy::InteractionPlan2DStatus::MissingTargetId:
		return iggy::runtime::RuntimeInteractionCommandStatus::MissingTargetId;
	case iggy::InteractionPlan2DStatus::TargetNotFound:
		return iggy::runtime::RuntimeInteractionCommandStatus::TargetNotFound;
	case iggy::InteractionPlan2DStatus::TargetDisabled:
		return iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled;
	case iggy::InteractionPlan2DStatus::OutOfRange:
		return iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange;
	}
	return iggy::runtime::RuntimeInteractionCommandStatus::TargetNotFound;
}

} // namespace

namespace iggy::runtime {

bool RuntimeInteractionCommandResult::ready() const
{
	return status == RuntimeInteractionCommandStatus::Ready;
}

RuntimeInteractionCommandResult RuntimeInteractionCommandStep::evaluate(
	const RuntimeSessionState &session,
	const InteractionTarget2DRegistry &registry,
	const GameplayCommand2D &command,
	const InteractionReach2DConfig &reachConfig) const
{
	RuntimeInteractionCommandResult result;
	result.command = command;

	if (command.type != GameplayCommand2DType::Interact) {
		result.status = RuntimeInteractionCommandStatus::NotInteractCommand;
		return result;
	}

	if (!session.hasPlayer) {
		result.status = RuntimeInteractionCommandStatus::MissingPlayer;
		return result;
	}

	result.plan = InteractionPlan2D {}.plan(registry, command.targetId, session.player.position, reachConfig);
	result.status = StatusForPlan(result.plan.status);
	return result;
}

} // namespace iggy::runtime
