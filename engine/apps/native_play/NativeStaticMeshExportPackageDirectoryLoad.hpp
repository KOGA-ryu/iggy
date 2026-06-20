#pragma once

#include "NativeStaticMeshAssetLoader.hpp"
#include "NativeStaticMeshExportPackageDirectoryReport.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshExportPackageDirectoryLoadStatus {
	Loaded,
	PackageDirectoryReadFailed,
	ManifestReadFailed,
	ManifestComparisonFailed,
	MissingAsset,
	AssetLoadFailed,
	GeometryMismatch,
};

enum class NativeStaticMeshExportPackageDirectoryLoadedAssetStatus {
	Loaded,
	MissingAsset,
	AssetLoadFailed,
	GeometryMismatch,
};

struct NativeStaticMeshExportPackageDirectoryLoadedAsset {
	NativeStaticMeshExportPackageDirectoryLoadedAssetStatus status =
		NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::Loaded;
	NativeStaticMeshExportPackageDirectoryAsset packageAsset;
	NativeStaticMeshExportManifestAssetRow manifestAsset;
	std::size_t expectedVertexCount = 0;
	std::size_t expectedIndexCount = 0;
	std::size_t actualVertexCount = 0;
	std::size_t actualIndexCount = 0;
	std::size_t issueCount = 0;
	std::vector<NativeStaticMeshAssetLoadIssue> loadIssues;
	NativeStaticMeshAsset asset;

	[[nodiscard]] bool loaded() const
	{
		return status == NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::Loaded;
	}
};

struct NativeStaticMeshExportPackageDirectoryLoadResult {
	NativeStaticMeshExportPackageDirectoryLoadStatus status =
		NativeStaticMeshExportPackageDirectoryLoadStatus::PackageDirectoryReadFailed;
	std::filesystem::path directory;
	NativeStaticMeshExportPackageDirectoryReadResult read;
	NativeStaticMeshExportManifestReadResult manifestRead;
	NativeStaticMeshExportPackageDirectoryManifestComparisonResult manifestComparison;
	std::vector<NativeStaticMeshExportPackageDirectoryLoadedAsset> assets;
	std::size_t loadedCount = 0;
	std::size_t issueCount = 0;

	[[nodiscard]] bool loaded() const
	{
		return status == NativeStaticMeshExportPackageDirectoryLoadStatus::Loaded;
	}
};

[[nodiscard]] inline const NativeStaticMeshExportManifestAssetRow *
FindNativeStaticMeshExportPackageDirectoryLoadManifestAsset(
	const std::vector<NativeStaticMeshExportManifestAssetRow> &manifestAssets,
	const NativeStaticMeshExportPackageDirectoryAsset &packageAsset)
{
	for (const NativeStaticMeshExportManifestAssetRow &manifestAsset :
			manifestAssets) {
		if (manifestAsset.name == packageAsset.name &&
				manifestAsset.filename == packageAsset.filename) {
			return &manifestAsset;
		}
	}
	return nullptr;
}

inline void SetNativeStaticMeshExportPackageDirectoryLoadFailureStatus(
	NativeStaticMeshExportPackageDirectoryLoadResult &result,
	NativeStaticMeshExportPackageDirectoryLoadStatus status)
{
	if (result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::Loaded) {
		result.status = status;
	}
}

[[nodiscard]] inline bool NativeStaticMeshAssetLoadHasIssue(
	const NativeStaticMeshAssetLoadResult &load,
	NativeStaticMeshAssetLoadIssueCode code)
{
	for (const NativeStaticMeshAssetLoadIssue &issue : load.issues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryLoadResult
LoadNativeStaticMeshExportPackageDirectory(const std::filesystem::path &directory)
{
	NativeStaticMeshExportPackageDirectoryLoadResult result;
	result.directory = directory;
	result.read = ReadNativeStaticMeshExportPackageDirectory(directory);
	if (!result.read.read()) {
		result.status =
			NativeStaticMeshExportPackageDirectoryLoadStatus::PackageDirectoryReadFailed;
		result.issueCount = result.read.issueCount;
		return result;
	}

	result.manifestRead = ReadNativeStaticMeshExportManifestFile(result.read.manifestPath);
	if (!result.manifestRead.read()) {
		result.status =
			NativeStaticMeshExportPackageDirectoryLoadStatus::ManifestReadFailed;
		result.issueCount = result.manifestRead.issues.size();
		return result;
	}

	result.manifestComparison = CompareNativeStaticMeshExportPackageDirectoryManifestRows(
		result.read.assets,
		result.manifestRead.document.assets);
	if (!result.manifestComparison.comparisons.empty()) {
		result.status =
			NativeStaticMeshExportPackageDirectoryLoadStatus::ManifestComparisonFailed;
		result.issueCount = result.manifestComparison.comparisons.size();
		return result;
	}

	result.status = NativeStaticMeshExportPackageDirectoryLoadStatus::Loaded;
	for (const NativeStaticMeshExportPackageDirectoryAsset &packageAsset :
			result.read.assets) {
		NativeStaticMeshExportPackageDirectoryLoadedAsset loadedAsset;
		loadedAsset.packageAsset = packageAsset;
		if (const NativeStaticMeshExportManifestAssetRow *manifestAsset =
				FindNativeStaticMeshExportPackageDirectoryLoadManifestAsset(
					result.manifestRead.document.assets,
					packageAsset)) {
			loadedAsset.manifestAsset = *manifestAsset;
			loadedAsset.expectedVertexCount = manifestAsset->vertexCount;
			loadedAsset.expectedIndexCount = manifestAsset->indexCount;
		}

		const NativeStaticMeshAssetLoadResult load =
			LoadNativeStaticMeshAssetFile(packageAsset.path);
		loadedAsset.actualVertexCount = load.asset.vertices.size();
		loadedAsset.actualIndexCount = load.asset.indices.size();
		if (!load.loaded()) {
			loadedAsset.loadIssues = load.issues;
			loadedAsset.issueCount = load.issues.size();
			if (NativeStaticMeshAssetLoadHasIssue(
					load,
					NativeStaticMeshAssetLoadIssueCode::FileOpenFailed)) {
				loadedAsset.status =
					NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::MissingAsset;
				SetNativeStaticMeshExportPackageDirectoryLoadFailureStatus(
					result,
					NativeStaticMeshExportPackageDirectoryLoadStatus::MissingAsset);
			} else {
				loadedAsset.status =
					NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::AssetLoadFailed;
				SetNativeStaticMeshExportPackageDirectoryLoadFailureStatus(
					result,
					NativeStaticMeshExportPackageDirectoryLoadStatus::AssetLoadFailed);
			}
			result.issueCount += loadedAsset.issueCount;
			result.assets.push_back(loadedAsset);
			continue;
		}

		loadedAsset.asset = load.asset;
		if (loadedAsset.actualVertexCount != loadedAsset.expectedVertexCount ||
				loadedAsset.actualIndexCount != loadedAsset.expectedIndexCount) {
			loadedAsset.status =
				NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::GeometryMismatch;
			loadedAsset.issueCount = 1;
			SetNativeStaticMeshExportPackageDirectoryLoadFailureStatus(
				result,
				NativeStaticMeshExportPackageDirectoryLoadStatus::GeometryMismatch);
			++result.issueCount;
			result.assets.push_back(loadedAsset);
			continue;
		}

		++result.loadedCount;
		result.assets.push_back(loadedAsset);
	}

	return result;
}

} // namespace iggy::native_play
