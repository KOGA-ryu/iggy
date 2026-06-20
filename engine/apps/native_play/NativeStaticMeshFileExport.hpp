#pragma once

#include "NativeStaticMeshAssetWriter.hpp"
#include "NativeStaticMeshExportManifest.hpp"
#include "NativeStaticMeshExportPackageManifest.hpp"
#include "NativeStaticMeshExportPolicy.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace iggy::native_play {

inline constexpr const char *NativeStaticMeshExportManifestFilename =
	"static-mesh-export-manifest.txt";

enum class NativeStaticMeshFileExportStatus {
	Exported,
	InvalidPolicy,
	UnknownAsset,
	MissingOutputDirectory,
	OutputDirectoryNotDirectory,
	TargetAlreadyExists,
	WriterFailed,
	FileOpenFailed,
	WriteFailed,
};

struct NativeStaticMeshFileExportResult {
	NativeStaticMeshFileExportStatus status =
		NativeStaticMeshFileExportStatus::UnknownAsset;
	std::filesystem::path outputPath;
	std::size_t byteCount = 0;
	std::size_t issueCount = 0;
};

struct NativeStaticMeshFileExportBatchEntry {
	std::string name;
	NativeStaticMeshFileExportResult result;
};

struct NativeStaticMeshFileExportBatchResult {
	NativeStaticMeshFileExportStatus status =
		NativeStaticMeshFileExportStatus::Exported;
	std::filesystem::path outputDirectory;
	std::filesystem::path manifestOutputPath;
	std::filesystem::path packageManifestOutputPath;
	std::vector<NativeStaticMeshFileExportBatchEntry> entries;
	std::size_t exportedCount = 0;
	std::size_t byteCount = 0;
	std::size_t manifestByteCount = 0;
	std::size_t packageManifestByteCount = 0;
	std::size_t issueCount = 0;
};

[[nodiscard]] inline NativeStaticMeshFileExportResult ExportNativeStaticMeshAssetToDirectory(
	const NativeStaticMeshExportPolicy &policy,
	const std::string &name,
	const std::filesystem::path &directory)
{
	NativeStaticMeshFileExportResult result;

	const NativeStaticMeshExportPolicyValidationResult validation =
		ValidateNativeStaticMeshExportPolicy(policy);
	if (!validation.valid()) {
		result.status = NativeStaticMeshFileExportStatus::InvalidPolicy;
		result.issueCount = validation.issues.size();
		return result;
	}

	const NativeStaticMeshExportAssetRef *assetRef =
		FindNativeStaticMeshExportAsset(policy, name);
	if (assetRef == nullptr) {
		result.status = NativeStaticMeshFileExportStatus::UnknownAsset;
		return result;
	}

	result.outputPath = directory / assetRef->defaultFilename;
	if (!std::filesystem::exists(directory)) {
		result.status = NativeStaticMeshFileExportStatus::MissingOutputDirectory;
		return result;
	}
	if (!std::filesystem::is_directory(directory)) {
		result.status = NativeStaticMeshFileExportStatus::OutputDirectoryNotDirectory;
		return result;
	}
	if (std::filesystem::exists(result.outputPath)) {
		result.status = NativeStaticMeshFileExportStatus::TargetAlreadyExists;
		return result;
	}

	const NativeStaticMeshAssetWriteResult write =
		WriteNativeStaticMeshAssetText(BuiltInNativeStaticMeshExportAsset(assetRef->id));
	result.issueCount = write.issues.size();
	if (!write.written()) {
		result.status = NativeStaticMeshFileExportStatus::WriterFailed;
		return result;
	}

	std::ofstream file(result.outputPath, std::ios::binary);
	if (!file.is_open()) {
		result.status = NativeStaticMeshFileExportStatus::FileOpenFailed;
		return result;
	}
	file << write.text;
	file.close();
	if (!file) {
		result.status = NativeStaticMeshFileExportStatus::WriteFailed;
		return result;
	}

	result.status = NativeStaticMeshFileExportStatus::Exported;
	result.byteCount = write.text.size();
	return result;
}

[[nodiscard]] inline NativeStaticMeshFileExportBatchResult
ExportNativeStaticMeshPolicyToDirectory(
	const NativeStaticMeshExportPolicy &policy,
	const std::filesystem::path &directory)
{
	NativeStaticMeshFileExportBatchResult result;
	result.outputDirectory = directory;

	const NativeStaticMeshExportPolicyValidationResult validation =
		ValidateNativeStaticMeshExportPolicy(policy);
	if (!validation.valid()) {
		result.status = NativeStaticMeshFileExportStatus::InvalidPolicy;
		result.issueCount = validation.issues.size();
		return result;
	}

	if (!std::filesystem::exists(directory)) {
		result.status = NativeStaticMeshFileExportStatus::MissingOutputDirectory;
		return result;
	}
	if (!std::filesystem::is_directory(directory)) {
		result.status = NativeStaticMeshFileExportStatus::OutputDirectoryNotDirectory;
		return result;
	}

	result.manifestOutputPath = directory / NativeStaticMeshExportManifestFilename;
	if (std::filesystem::exists(result.manifestOutputPath)) {
		result.status = NativeStaticMeshFileExportStatus::TargetAlreadyExists;
		return result;
	}
	result.packageManifestOutputPath =
		directory / NativeStaticMeshExportPackageManifestSidecarFilename;
	if (std::filesystem::exists(result.packageManifestOutputPath)) {
		result.status = NativeStaticMeshFileExportStatus::TargetAlreadyExists;
		return result;
	}

	for (const NativeStaticMeshExportAssetRef &assetRef : policy.assets) {
		NativeStaticMeshFileExportBatchEntry entry;
		entry.name = assetRef.name;
		entry.result.outputPath = directory / assetRef.defaultFilename;
		if (std::filesystem::exists(entry.result.outputPath)) {
			entry.result.status = NativeStaticMeshFileExportStatus::TargetAlreadyExists;
			result.status = NativeStaticMeshFileExportStatus::TargetAlreadyExists;
			result.entries.push_back(std::move(entry));
			return result;
		}
		result.entries.push_back(std::move(entry));
	}

	const NativeStaticMeshExportManifestResult manifest =
		BuildNativeStaticMeshExportManifestText(policy);
	result.issueCount += manifest.issueCount;
	if (!manifest.written()) {
		result.status = NativeStaticMeshFileExportStatus::WriterFailed;
		return result;
	}
	NativeStaticMeshExportPackagePolicy packagePolicy =
		DefaultNativeStaticMeshExportPackagePolicy();
	packagePolicy.meshPolicy = policy;
	const NativeStaticMeshExportPackageManifestResult packageManifest =
		BuildNativeStaticMeshExportPackageManifestText(packagePolicy);
	result.issueCount += packageManifest.issueCount;
	if (!packageManifest.written()) {
		result.status = NativeStaticMeshFileExportStatus::WriterFailed;
		return result;
	}

	std::vector<std::string> texts;
	texts.reserve(policy.assets.size());
	for (std::size_t index = 0; index < policy.assets.size(); ++index) {
		const NativeStaticMeshAssetWriteResult write =
			WriteNativeStaticMeshAssetText(
				BuiltInNativeStaticMeshExportAsset(policy.assets[index].id));
		result.entries[index].result.issueCount = write.issues.size();
		result.issueCount += write.issues.size();
		if (!write.written()) {
			result.entries[index].result.status =
				NativeStaticMeshFileExportStatus::WriterFailed;
			result.status = NativeStaticMeshFileExportStatus::WriterFailed;
			return result;
		}
		texts.push_back(write.text);
	}

	for (std::size_t index = 0; index < result.entries.size(); ++index) {
		NativeStaticMeshFileExportResult &entryResult = result.entries[index].result;
		std::ofstream file(entryResult.outputPath, std::ios::binary);
		if (!file.is_open()) {
			entryResult.status = NativeStaticMeshFileExportStatus::FileOpenFailed;
			result.status = NativeStaticMeshFileExportStatus::FileOpenFailed;
			return result;
		}
		file << texts[index];
		file.close();
		if (!file) {
			entryResult.status = NativeStaticMeshFileExportStatus::WriteFailed;
			result.status = NativeStaticMeshFileExportStatus::WriteFailed;
			return result;
		}

		entryResult.status = NativeStaticMeshFileExportStatus::Exported;
		entryResult.byteCount = texts[index].size();
		result.byteCount += entryResult.byteCount;
		++result.exportedCount;
	}

	std::ofstream manifestFile(result.manifestOutputPath, std::ios::binary);
	if (!manifestFile.is_open()) {
		result.status = NativeStaticMeshFileExportStatus::FileOpenFailed;
		return result;
	}
	manifestFile << manifest.text;
	manifestFile.close();
	if (!manifestFile) {
		result.status = NativeStaticMeshFileExportStatus::WriteFailed;
		return result;
	}
	result.manifestByteCount = manifest.text.size();

	std::ofstream packageManifestFile(result.packageManifestOutputPath, std::ios::binary);
	if (!packageManifestFile.is_open()) {
		result.status = NativeStaticMeshFileExportStatus::FileOpenFailed;
		return result;
	}
	packageManifestFile << packageManifest.text;
	packageManifestFile.close();
	if (!packageManifestFile) {
		result.status = NativeStaticMeshFileExportStatus::WriteFailed;
		return result;
	}
	result.packageManifestByteCount = packageManifest.text.size();

	result.status = NativeStaticMeshFileExportStatus::Exported;
	return result;
}

} // namespace iggy::native_play
