#include "scene/player/PlayerCommandFramePlanner2D.hpp"

#include "scene/player/PlayerCommandPlanner2D.hpp"

namespace iggy {

namespace {

bool IsInvalidCommandIndex(const runtime::GameplayCommandFrame2DValidationResult &validation, std::size_t index)
{
	for (const runtime::GameplayCommandFrame2DInvalidCommand &invalid : validation.invalidCommands) {
		if (invalid.index == index)
			return true;
	}
	return false;
}

std::vector<std::size_t> AcceptedCommandIndexes(const runtime::GameplayCommandFrame2D &frame, const runtime::GameplayCommandFrame2DValidationResult &validation)
{
	std::vector<std::size_t> indexes;
	indexes.reserve(validation.acceptedFrame.commands.size());

	for (std::size_t index = 0; index < frame.commands.size(); ++index) {
		if (!IsInvalidCommandIndex(validation, index))
			indexes.push_back(index);
	}

	return indexes;
}

} // namespace

PlayerCommandFramePlan2DResult PlayerCommandFramePlanner2D::plan(const PlayerAgentState &player, const runtime::GameplayCommandFrame2D &frame) const
{
	PlayerCommandFramePlan2DResult result;
	result.validation = runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	const std::vector<std::size_t> acceptedIndexes = AcceptedCommandIndexes(frame, result.validation);
	result.plans.reserve(result.validation.acceptedFrame.commands.size());

	for (std::size_t acceptedIndex = 0; acceptedIndex < result.validation.acceptedFrame.commands.size(); ++acceptedIndex) {
		const PlayerCommandPlan2D plan = PlayerCommandPlanner2D {}.plan(player, result.validation.acceptedFrame.commands[acceptedIndex]);
		const std::size_t planIndex = result.plans.size();
		result.plans.push_back(plan);

		if (plan.type == PlayerCommandPlan2DType::Rejected)
			result.rejectedPlans.push_back({ acceptedIndexes[acceptedIndex], planIndex, plan });
	}

	return result;
}

} // namespace iggy
