#include "runtime/RuntimeSessionMutationCommandStep.hpp"

namespace iggy::runtime {

namespace {

RuntimeSessionCommandTickInput CommandTickInput(
	const RuntimeSessionMutationCommandInput &input,
	const RuntimeSessionState &session)
{
	return {
		session,
		input.commandFrame,
		input.fallbackPlayerPosition,
		input.playerCommandConfig,
		input.npcConfig,
	};
}

RuntimeSessionMutationCommandResult MutationFailedResult(RuntimeLevelMutationResult mutation)
{
	RuntimeSessionMutationCommandResult result;
	result.status = RuntimeSessionMutationCommandStatus::MutationFailed;
	result.mutation = mutation;
	result.session = mutation.session;
	return result;
}

} // namespace

RuntimeSessionMutationCommandResult RuntimeSessionMutationCommandStep::run(const RuntimeSessionMutationCommandInput &input) const
{
	RuntimeLevelMutationResult mutation = RuntimeLevelMutationStep {}.apply(input.session, input.levelEdits);
	if (mutation.status == RuntimeLevelMutationStatus::Failed)
		return MutationFailedResult(mutation);

	RuntimeSessionMutationCommandResult result;
	result.status = RuntimeSessionMutationCommandStatus::Ticked;
	result.mutation = mutation;
	result.commandTick = RuntimeSessionCommandTick {}.run(CommandTickInput(input, mutation.session));
	result.session = result.commandTick.session;
	return result;
}

RuntimeSessionMutationCommandResult RuntimeSessionMutationCommandStep::run(
	const RuntimeSessionMutationCommandInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimeLevelMutationResult mutation = RuntimeLevelMutationStep {}.apply(input.session, input.levelEdits);
	if (mutation.status == RuntimeLevelMutationStatus::Failed)
		return MutationFailedResult(mutation);

	RuntimeSessionMutationCommandResult result;
	result.status = RuntimeSessionMutationCommandStatus::Ticked;
	result.mutation = mutation;
	result.commandTick = RuntimeSessionCommandTick {}.run(CommandTickInput(input, mutation.session), explicitWorld);
	result.session = result.commandTick.session;
	return result;
}

} // namespace iggy::runtime
