#pragma once

#include <array>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorState2D.hpp"

#include "NativePlayMath.hpp"

namespace iggy::native_play {

enum class NativeSceneModelId {
	Floor,
	Wall,
	NpcActor,
	Player,
};

struct NativeSceneDrawItem {
	NativeSceneModelId modelId = NativeSceneModelId::Player;
	Mat4 model;
	std::array<float, 4> tint { 1.0F, 1.0F, 1.0F, 1.0F };
};

struct NativeSceneDrawListInput {
	const runtime::RuntimeGameplayState *state = nullptr;
	float seconds = 0.0F;
};

[[nodiscard]] inline Mat4 NativePlayerCubeModelMatrix(
	const runtime::RuntimeGameplayState *state,
	float seconds)
{
	if (state == nullptr || !state->session.hasPlayer) {
		return Multiply(
			RotationY(seconds * 0.85F),
			RotationX(seconds * 0.35F));
	}

	const auto &session = state->session;
	const auto &map = session.level.map;
	const Vec3 playerPosition {
		session.player.position.x - static_cast<float>(map.width) * 0.5F,
		0.5F,
		session.player.position.y - static_cast<float>(map.height) * 0.5F,
	};
	return Multiply(
		Translation(playerPosition),
		Scale({ 0.75F, 0.75F, 0.75F }));
}

[[nodiscard]] inline Mat4 NativeTileCubeModelMatrix(
	int x,
	int y,
	float mapWidth,
	float mapHeight,
	float verticalCenter,
	Vec3 scale)
{
	const Vec3 position {
		static_cast<float>(x) + 0.5F - mapWidth * 0.5F,
		verticalCenter,
		static_cast<float>(y) + 0.5F - mapHeight * 0.5F,
	};
	return Multiply(Translation(position), Scale(scale));
}

inline void AppendNativeSceneDraw(
	std::vector<NativeSceneDrawItem> &drawItems,
	NativeSceneModelId modelId,
	const Mat4 &model,
	std::array<float, 4> tint)
{
	drawItems.push_back({ modelId, model, tint });
}

[[nodiscard]] inline std::vector<NativeSceneDrawItem>
BuildNativeSceneDrawItems(const NativeSceneDrawListInput &input)
{
	std::vector<NativeSceneDrawItem> drawItems;
	if (input.state == nullptr) {
		AppendNativeSceneDraw(
			drawItems,
			NativeSceneModelId::Player,
			NativePlayerCubeModelMatrix(nullptr, input.seconds),
			{ 0.18F, 0.70F, 1.0F, 1.0F });
		return drawItems;
	}

	const auto &map = input.state->session.level.map;
	const float mapWidth = static_cast<float>(map.width);
	const float mapHeight = static_cast<float>(map.height);
	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			AppendNativeSceneDraw(
				drawItems,
				NativeSceneModelId::Floor,
				NativeTileCubeModelMatrix(
					x,
					y,
					mapWidth,
					mapHeight,
					-0.055F,
					{ 0.96F, 0.10F, 0.96F }),
				{ 0.20F, 0.34F, 0.26F, 1.0F });

			const iggy::LevelTile *tile = map.tileAt(x, y);
			if (tile != nullptr && !tile->walkable) {
				AppendNativeSceneDraw(
					drawItems,
					NativeSceneModelId::Wall,
					NativeTileCubeModelMatrix(
						x,
						y,
						mapWidth,
						mapHeight,
						0.38F,
						{ 0.96F, 0.78F, 0.96F }),
					{ 0.38F, 0.40F, 0.48F, 1.0F });
			}
		}
	}

	for (const iggy::NpcActorState2D &actor : input.state->npcActors.actors) {
		if (!actor.present)
			continue;
		const Vec3 position {
			actor.position.x - mapWidth * 0.5F,
			0.38F,
			actor.position.y - mapHeight * 0.5F,
		};
		AppendNativeSceneDraw(
			drawItems,
			NativeSceneModelId::NpcActor,
			Multiply(
				Translation(position),
				Scale({ 0.62F, 0.62F, 0.62F })),
			{ 1.0F, 0.55F, 0.18F, 1.0F });
	}

	AppendNativeSceneDraw(
		drawItems,
		NativeSceneModelId::Player,
		NativePlayerCubeModelMatrix(input.state, input.seconds),
		{ 0.18F, 0.70F, 1.0F, 1.0F });
	return drawItems;
}

} // namespace iggy::native_play
