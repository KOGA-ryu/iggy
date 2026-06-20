#pragma once

#include "NativeStaticMeshAsset.hpp"

#include <cstddef>
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

enum class NativeStaticMeshExportPolicyValidationIssueCode {
	EmptyName,
	DuplicateName,
	EmptyDefaultFilename,
	DefaultFilenameContainsSeparator,
	DuplicateDefaultFilename,
};

struct NativeStaticMeshExportPolicyValidationIssue {
	NativeStaticMeshExportPolicyValidationIssueCode code =
		NativeStaticMeshExportPolicyValidationIssueCode::EmptyName;
	std::size_t assetIndex = 0;
	std::size_t previousAssetIndex = 0;
	std::string value;
};

struct NativeStaticMeshExportPolicyValidationResult {
	std::vector<NativeStaticMeshExportPolicyValidationIssue> issues;

	[[nodiscard]] bool valid() const
	{
		return issues.empty();
	}
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

inline void AddNativeStaticMeshExportPolicyValidationIssue(
	NativeStaticMeshExportPolicyValidationResult &result,
	NativeStaticMeshExportPolicyValidationIssueCode code,
	std::size_t assetIndex,
	const std::string &value,
	std::size_t previousAssetIndex = 0)
{
	result.issues.push_back({ code, assetIndex, previousAssetIndex, value });
}

[[nodiscard]] inline bool NativeStaticMeshExportFilenameContainsSeparator(
	const std::string &filename)
{
	return filename.find('/') != std::string::npos ||
		filename.find('\\') != std::string::npos;
}

[[nodiscard]] inline NativeStaticMeshExportPolicyValidationResult
ValidateNativeStaticMeshExportPolicy(const NativeStaticMeshExportPolicy &policy)
{
	NativeStaticMeshExportPolicyValidationResult result;
	for (std::size_t index = 0; index < policy.assets.size(); ++index) {
		const NativeStaticMeshExportAssetRef &asset = policy.assets[index];
		if (asset.name.empty()) {
			AddNativeStaticMeshExportPolicyValidationIssue(
				result,
				NativeStaticMeshExportPolicyValidationIssueCode::EmptyName,
				index,
				asset.name);
		}
		if (asset.defaultFilename.empty()) {
			AddNativeStaticMeshExportPolicyValidationIssue(
				result,
				NativeStaticMeshExportPolicyValidationIssueCode::EmptyDefaultFilename,
				index,
				asset.defaultFilename);
		}
		if (NativeStaticMeshExportFilenameContainsSeparator(asset.defaultFilename)) {
			AddNativeStaticMeshExportPolicyValidationIssue(
				result,
				NativeStaticMeshExportPolicyValidationIssueCode::DefaultFilenameContainsSeparator,
				index,
				asset.defaultFilename);
		}

		for (std::size_t previous = 0; previous < index; ++previous) {
			const NativeStaticMeshExportAssetRef &previousAsset =
				policy.assets[previous];
			if (!asset.name.empty() && asset.name == previousAsset.name) {
				AddNativeStaticMeshExportPolicyValidationIssue(
					result,
					NativeStaticMeshExportPolicyValidationIssueCode::DuplicateName,
					index,
					asset.name,
					previous);
			}
			if (!asset.defaultFilename.empty() &&
					asset.defaultFilename == previousAsset.defaultFilename) {
				AddNativeStaticMeshExportPolicyValidationIssue(
					result,
					NativeStaticMeshExportPolicyValidationIssueCode::DuplicateDefaultFilename,
					index,
					asset.defaultFilename,
					previous);
			}
		}
	}
	return result;
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
