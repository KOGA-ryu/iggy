#pragma once

#include "NativeStaticMeshAsset.hpp"

#include <string>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshBuiltInExportId {
	Cube,
	Bean,
	NpcMarker,
};

struct NativeStaticMeshExportAssetRef {
	NativeStaticMeshBuiltInExportId id = NativeStaticMeshBuiltInExportId::Cube;
	std::string name;
	std::string defaultFilename;
};

struct NativeStaticMeshExportPolicy {
	std::vector<NativeStaticMeshExportAssetRef> assets;
};

[[nodiscard]] inline NativeStaticMeshExportPolicy DefaultNativeStaticMeshExportPolicy()
{
	return {
		{
			{ NativeStaticMeshBuiltInExportId::Cube, "cube", "cube.igmesh" },
			{ NativeStaticMeshBuiltInExportId::Bean, "bean", "bean.igmesh" },
			{ NativeStaticMeshBuiltInExportId::NpcMarker, "npc-marker", "npc-marker.igmesh" },
		},
	};
}

[[nodiscard]] inline const NativeStaticMeshExportAssetRef *FindNativeStaticMeshExportAsset(
	const NativeStaticMeshExportPolicy &policy,
	const std::string &name)
{
	for (const NativeStaticMeshExportAssetRef &asset : policy.assets) {
		if (asset.name == name)
			return &asset;
	}
	return nullptr;
}

[[nodiscard]] inline NativeStaticMeshAsset BuiltInNativeStaticMeshExportAsset(
	NativeStaticMeshBuiltInExportId id)
{
	switch (id) {
	case NativeStaticMeshBuiltInExportId::Cube:
		return NativeCubeStaticMeshAsset();
	case NativeStaticMeshBuiltInExportId::Bean:
		return NativeBeanStaticMeshAsset();
	case NativeStaticMeshBuiltInExportId::NpcMarker:
		return NativeNpcMarkerStaticMeshAsset();
	}
	return NativeCubeStaticMeshAsset();
}

} // namespace iggy::native_play
