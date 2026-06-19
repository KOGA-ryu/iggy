#pragma once

#include "NativeStaticMeshAssetWriter.hpp"
#include "NativeStaticMeshExportPolicy.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

namespace iggy::native_play {

enum class NativeStaticMeshFileExportStatus {
	Exported,
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

[[nodiscard]] inline NativeStaticMeshFileExportResult ExportNativeStaticMeshAssetToDirectory(
	const NativeStaticMeshExportPolicy &policy,
	const std::string &name,
	const std::filesystem::path &directory)
{
	NativeStaticMeshFileExportResult result;

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

} // namespace iggy::native_play
