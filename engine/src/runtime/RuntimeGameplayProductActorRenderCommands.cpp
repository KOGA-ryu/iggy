#include "runtime/RuntimeGameplayProductActorRenderCommands.hpp"

#include <cmath>

#include "core/math/Aabb2.hpp"

namespace {

iggy::Aabb2 BoundsForActor(iggy::Vec2 position, iggy::Vec2 size, iggy::Vec2 anchor)
{
	const iggy::Vec2 effectiveSize { std::fabs(size.x), std::fabs(size.y) };
	const iggy::Vec2 min {
		position.x - (effectiveSize.x * anchor.x),
		position.y - (effectiveSize.y * anchor.y),
	};
	return { min, min + effectiveSize };
}

} // namespace

namespace iggy::runtime {

RuntimeGameplayProductActorRenderCommandResult
RuntimeGameplayProductActorRenderCommands::build(
	const RuntimeGameplayState &state,
	const RuntimeGameplayProductActorRenderCommandConfig &config) const
{
	RuntimeGameplayProductActorRenderCommandResult result;
	const render::RenderCommandListBuilder2D builder;

	if (config.includePlayer && state.session.hasPlayer) {
		builder.addQuad(
			result.commands,
			BoundsForActor(
				state.session.player.position,
				config.playerSize,
				config.playerAnchor),
			config.playerMaterialId,
			config.playerLayer);
		result.emittedPlayer = true;
	}

	if (config.includeNpcActors) {
		for (const NpcActorState2D &actor : state.npcActors.actors) {
			if (!actor.present)
				continue;
			builder.addQuad(
				result.commands,
				BoundsForActor(
					actor.position,
					config.npcActorSize,
					config.npcActorAnchor),
				config.npcActorMaterialId,
				config.npcActorLayer);
			++result.emittedNpcActorCount;
		}
	}

	result.commandCount = result.commands.commands.size();
	return result;
}

} // namespace iggy::runtime
