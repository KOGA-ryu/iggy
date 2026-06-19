#pragma once

#include "NativePlayMath.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace iggy::native_play {

struct NativeStaticMeshVertex {
	Vec3 position;
	std::array<float, 3> color {};
};

struct NativeStaticMeshAsset {
	std::vector<NativeStaticMeshVertex> vertices;
	std::vector<std::uint16_t> indices;
};

[[nodiscard]] inline bool IsNativeStaticMeshAssetValid(
	const NativeStaticMeshAsset &asset)
{
	if (asset.vertices.empty() || asset.indices.empty())
		return false;
	if (asset.indices.size() > std::numeric_limits<std::uint32_t>::max())
		return false;
	for (const std::uint16_t index : asset.indices) {
		if (static_cast<std::size_t>(index) >= asset.vertices.size())
			return false;
	}
	return true;
}

[[nodiscard]] inline NativeStaticMeshAsset NativeCubeStaticMeshAsset()
{
	NativeStaticMeshAsset asset;
	asset.vertices = {
		{ { -0.5F, -0.5F, -0.5F }, { 0.10F, 0.55F, 0.95F } },
		{ { 0.5F, -0.5F, -0.5F }, { 0.25F, 0.80F, 0.95F } },
		{ { 0.5F, 0.5F, -0.5F }, { 0.95F, 0.75F, 0.25F } },
		{ { -0.5F, 0.5F, -0.5F }, { 0.90F, 0.35F, 0.50F } },
		{ { -0.5F, -0.5F, 0.5F }, { 0.25F, 0.70F, 0.45F } },
		{ { 0.5F, -0.5F, 0.5F }, { 0.70F, 0.45F, 0.95F } },
		{ { 0.5F, 0.5F, 0.5F }, { 0.95F, 0.55F, 0.20F } },
		{ { -0.5F, 0.5F, 0.5F }, { 0.85F, 0.85F, 0.45F } },
	};
	asset.indices = {
		0, 1, 2, 2, 3, 0,
		4, 6, 5, 6, 4, 7,
		0, 4, 5, 5, 1, 0,
		3, 2, 6, 6, 7, 3,
		1, 5, 6, 6, 2, 1,
		0, 3, 7, 7, 4, 0,
	};
	return asset;
}

} // namespace iggy::native_play
