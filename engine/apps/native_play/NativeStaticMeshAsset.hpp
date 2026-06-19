#pragma once

#include "NativePlayMath.hpp"

#include <array>
#include <cmath>
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

[[nodiscard]] inline NativeStaticMeshAsset NativeBeanStaticMeshAsset()
{
	constexpr float Pi = 3.14159265358979323846F;
	constexpr std::uint16_t StackCount = 12;
	constexpr std::uint16_t SegmentCount = 18;

	NativeStaticMeshAsset asset;
	asset.vertices.reserve(
		static_cast<std::size_t>(StackCount + 1U) *
		static_cast<std::size_t>(SegmentCount));
	for (std::uint16_t stack = 0; stack <= StackCount; ++stack) {
		const float t = static_cast<float>(stack) / static_cast<float>(StackCount);
		const float vertical = t * 2.0F - 1.0F;
		const float crossSection = 1.0F - vertical * vertical;
		const float ringScale = crossSection > 0.0F ? std::sqrt(crossSection) : 0.0F;
		const float centerX = 0.13F * std::sin(Pi * (t * 1.15F - 0.20F));
		const float centerZ = 0.07F * std::sin(Pi * (t * 2.0F + 0.35F));
		const float xRadius =
			0.38F * ringScale * (1.0F + 0.10F * std::sin(Pi * t * 2.0F));
		const float zRadius =
			0.28F * ringScale * (1.0F - 0.20F * std::sin(Pi * (t - 0.15F)));
		for (std::uint16_t segment = 0; segment < SegmentCount; ++segment) {
			const float angle =
				2.0F * Pi * static_cast<float>(segment) /
				static_cast<float>(SegmentCount);
			const float lobe = 1.0F + 0.12F * std::sin(angle + t * Pi);
			const float colorBand =
				0.5F + 0.5F * std::sin(angle * 0.75F + t * Pi);
			asset.vertices.push_back({
				{
					centerX + xRadius * lobe * std::cos(angle),
					0.56F * vertical,
					centerZ + zRadius * (1.0F - 0.10F * std::cos(angle)) * std::sin(angle),
				},
				{
					0.78F + 0.10F * colorBand,
					0.58F + 0.12F * t,
					0.42F + 0.08F * (1.0F - colorBand),
				},
			});
		}
	}

	asset.indices.reserve(
		static_cast<std::size_t>(StackCount) *
		static_cast<std::size_t>(SegmentCount) *
		6U);
	for (std::uint16_t stack = 0; stack < StackCount; ++stack) {
		for (std::uint16_t segment = 0; segment < SegmentCount; ++segment) {
			const std::uint16_t nextSegment =
				static_cast<std::uint16_t>((segment + 1U) % SegmentCount);
			const std::uint16_t current =
				static_cast<std::uint16_t>(stack * SegmentCount + segment);
			const std::uint16_t next =
				static_cast<std::uint16_t>(stack * SegmentCount + nextSegment);
			const std::uint16_t upper =
				static_cast<std::uint16_t>((stack + 1U) * SegmentCount + segment);
			const std::uint16_t upperNext =
				static_cast<std::uint16_t>((stack + 1U) * SegmentCount + nextSegment);
			asset.indices.insert(
				asset.indices.end(),
				{ current, upper, next, next, upper, upperNext });
		}
	}
	return asset;
}

} // namespace iggy::native_play
