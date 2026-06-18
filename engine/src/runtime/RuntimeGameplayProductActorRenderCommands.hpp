#pragma once

#include <cstddef>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayState.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy::runtime {

struct RuntimeGameplayProductActorRenderCommandConfig {
	bool includePlayer = true;
	bool includeNpcActors = true;
	ResourceId playerMaterialId { "material:player" };
	ResourceId npcActorMaterialId { "material:npc_actor" };
	Vec2 playerSize { 1.0F, 1.0F };
	Vec2 playerAnchor { 0.5F, 0.5F };
	Vec2 npcActorSize { 1.0F, 1.0F };
	Vec2 npcActorAnchor { 0.5F, 0.5F };
	int playerLayer = 20;
	int npcActorLayer = 20;
};

struct RuntimeGameplayProductActorRenderCommandResult {
	render::RenderCommandList2D commands;
	bool emittedPlayer = false;
	std::size_t emittedNpcActorCount = 0;
	std::size_t commandCount = 0;
};

class RuntimeGameplayProductActorRenderCommands {
public:
	[[nodiscard]] RuntimeGameplayProductActorRenderCommandResult build(
		const RuntimeGameplayState &state,
		const RuntimeGameplayProductActorRenderCommandConfig &config) const;
};

} // namespace iggy::runtime
