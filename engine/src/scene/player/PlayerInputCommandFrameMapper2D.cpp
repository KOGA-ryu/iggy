#include "scene/player/PlayerInputCommandFrameMapper2D.hpp"

namespace iggy {

bool PlayerInputCommandFrameMapper2DResult::hasIssues() const
{
	return !issues.empty();
}

PlayerInputCommandFrameMapper2DResult PlayerInputCommandFrameMapper2D::map(ResourceId actorId, const std::vector<PlayerInputIntent2D> &intents) const
{
	PlayerInputCommandFrameMapper2DResult result;
	const PlayerInputCommandMapper2D mapper;

	for (std::size_t index = 0; index < intents.size(); ++index) {
		const PlayerInputIntent2D intent = intents[index];
		const PlayerInputCommandMapper2DResult mapped = mapper.map(actorId, intent);
		if (mapped.status == PlayerInputCommandMapper2DStatus::Mapped) {
			result.frame.commands.push_back(mapped.command);
			continue;
		}

		result.issues.push_back({ index, mapped, intent });
	}

	return result;
}

} // namespace iggy
