#include "scene/player/PlayerInputGatedCommandFrameMapper2D.hpp"

namespace iggy {

bool PlayerInputGatedCommandFrameMapper2DResult::hasIssues() const
{
	return !gateIssues.empty() || mapping.hasIssues();
}

PlayerInputGatedCommandFrameMapper2DResult PlayerInputGatedCommandFrameMapper2D::map(
	ResourceId actorId,
	const PlayerInputContext2D &context,
	const std::vector<PlayerInputIntent2D> &intents) const
{
	PlayerInputGatedCommandFrameMapper2DResult result;
	const PlayerInputIntentGate2D gate;
	std::vector<PlayerInputIntent2D> acceptedIntents;

	for (std::size_t index = 0; index < intents.size(); ++index) {
		const PlayerInputIntent2D intent = intents[index];
		const PlayerInputIntentGate2DResult gated = gate.evaluate(context, intent);
		if (gated.accepted) {
			acceptedIntents.push_back(intent);
			continue;
		}

		result.gateIssues.push_back({ index, gated });
	}

	result.mapping = PlayerInputCommandFrameMapper2D {}.map(actorId, acceptedIntents);
	result.frame = result.mapping.frame;
	return result;
}

} // namespace iggy
