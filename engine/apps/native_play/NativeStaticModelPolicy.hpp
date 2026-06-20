#pragma once

#include <string>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticModelSlot {
	Floor,
	Wall,
	NpcActor,
	Player,
};

[[nodiscard]] inline const char *NativeStaticModelSlotText(NativeStaticModelSlot slot)
{
	switch (slot) {
	case NativeStaticModelSlot::Floor:
		return "Floor";
	case NativeStaticModelSlot::Wall:
		return "Wall";
	case NativeStaticModelSlot::NpcActor:
		return "NpcActor";
	case NativeStaticModelSlot::Player:
		return "Player";
	}
	return "Unknown";
}

struct NativeStaticModelAssetRef {
	NativeStaticModelSlot slot = NativeStaticModelSlot::Player;
	std::string meshFilename;
};

struct NativeStaticModelPolicy {
	std::vector<NativeStaticModelAssetRef> models;
};

[[nodiscard]] inline NativeStaticModelPolicy DefaultNativeStaticModelPolicy()
{
	return {
		{
			{ NativeStaticModelSlot::Floor, "floor.igmesh" },
			{ NativeStaticModelSlot::Wall, "wall.igmesh" },
			{ NativeStaticModelSlot::NpcActor, "npc.igmesh" },
			{ NativeStaticModelSlot::Player, "player.igmesh" },
		},
	};
}

[[nodiscard]] inline const NativeStaticModelAssetRef *FindNativeStaticModelAsset(
	const NativeStaticModelPolicy &policy,
	NativeStaticModelSlot slot)
{
	for (const NativeStaticModelAssetRef &model : policy.models) {
		if (model.slot == slot)
			return &model;
	}
	return nullptr;
}

} // namespace iggy::native_play
