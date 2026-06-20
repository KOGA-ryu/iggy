#pragma once

#include "NativeStaticMeshExportPackageManifest.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshExportPackageDirectoryReadStatus {
	Read,
	MissingDirectory,
	DirectoryNotDirectory,
	PackageManifestReadFailed,
};

[[nodiscard]] inline const char *NativeStaticMeshExportPackageDirectoryReadStatusText(
	NativeStaticMeshExportPackageDirectoryReadStatus status)
{
	switch (status) {
	case NativeStaticMeshExportPackageDirectoryReadStatus::Read:
		return "Read";
	case NativeStaticMeshExportPackageDirectoryReadStatus::MissingDirectory:
		return "MissingDirectory";
	case NativeStaticMeshExportPackageDirectoryReadStatus::DirectoryNotDirectory:
		return "DirectoryNotDirectory";
	case NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed:
		return "PackageManifestReadFailed";
	}
	return "Unknown";
}

struct NativeStaticMeshExportPackageDirectoryAsset {
	std::string name;
	std::string filename;
	std::filesystem::path path;
};

struct NativeStaticMeshExportPackageDirectoryReadResult {
	NativeStaticMeshExportPackageDirectoryReadStatus status =
		NativeStaticMeshExportPackageDirectoryReadStatus::MissingDirectory;
	std::filesystem::path directory;
	std::filesystem::path packageManifestPath;
	std::filesystem::path manifestPath;
	NativeStaticMeshExportPackageManifestDocument document;
	std::vector<NativeStaticMeshExportPackageManifestReadIssue> packageManifestReadIssues;
	std::vector<NativeStaticMeshExportPackageDirectoryAsset> assets;
	std::size_t issueCount = 0;

	[[nodiscard]] bool read() const
	{
		return status == NativeStaticMeshExportPackageDirectoryReadStatus::Read;
	}
};

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryReadResult
ReadNativeStaticMeshExportPackageDirectory(const std::filesystem::path &directory)
{
	NativeStaticMeshExportPackageDirectoryReadResult result;
	result.directory = directory;

	if (!std::filesystem::exists(directory)) {
		result.status = NativeStaticMeshExportPackageDirectoryReadStatus::MissingDirectory;
		return result;
	}
	if (!std::filesystem::is_directory(directory)) {
		result.status =
			NativeStaticMeshExportPackageDirectoryReadStatus::DirectoryNotDirectory;
		return result;
	}

	result.packageManifestPath =
		directory / NativeStaticMeshExportPackageManifestSidecarFilename;
	const NativeStaticMeshExportPackageManifestReadResult manifest =
		ReadNativeStaticMeshExportPackageManifestFile(result.packageManifestPath);
	if (!manifest.read()) {
		result.status =
			NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed;
		result.packageManifestReadIssues = manifest.issues;
		result.issueCount = result.packageManifestReadIssues.size();
		return result;
	}

	result.document = manifest.document;
	result.manifestPath = directory / result.document.manifestFilename;
	for (const NativeStaticMeshExportPackageManifestAssetRow &asset :
			result.document.assets) {
		result.assets.push_back({
			asset.name,
			asset.filename,
			directory / asset.filename,
		});
	}

	result.status = NativeStaticMeshExportPackageDirectoryReadStatus::Read;
	return result;
}

} // namespace iggy::native_play
