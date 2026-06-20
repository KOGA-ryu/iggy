#pragma once

#include "NativeStaticMeshExportPackageDirectoryReader.hpp"

#include <filesystem>
#include <sstream>
#include <string>

namespace iggy::native_play {

struct NativeStaticMeshExportPackageDirectoryReport {
	NativeStaticMeshExportPackageDirectoryReadResult read;
	std::string text;

	[[nodiscard]] bool readOk() const
	{
		return read.read();
	}
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

[[nodiscard]] inline const char *NativeStaticMeshExportPackageDirectoryManifestIssueCodeText(
	NativeStaticMeshExportPackageManifestReadIssueCode code)
{
	switch (code) {
	case NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed:
		return "FileOpenFailed";
	case NativeStaticMeshExportPackageManifestReadIssueCode::EmptyInput:
		return "EmptyInput";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader:
		return "MalformedHeader";
	case NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedFormatId:
		return "UnsupportedFormatId";
	case NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedVersion:
		return "UnsupportedVersion";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetCount:
		return "MalformedAssetCount";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MissingField:
		return "MissingField";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetRow:
		return "MalformedAssetRow";
	case NativeStaticMeshExportPackageManifestReadIssueCode::AssetCountMismatch:
		return "AssetCountMismatch";
	case NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetName:
		return "DuplicateAssetName";
	case NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetFilename:
		return "DuplicateAssetFilename";
	case NativeStaticMeshExportPackageManifestReadIssueCode::ExtraToken:
		return "ExtraToken";
	case NativeStaticMeshExportPackageManifestReadIssueCode::UnexpectedLine:
		return "UnexpectedLine";
	}
	return "Unknown";
}

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryReport
BuildNativeStaticMeshExportPackageDirectoryReport(
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportPackageDirectoryReport report;
	report.read = ReadNativeStaticMeshExportPackageDirectory(directory);

	std::ostringstream stream;
	stream
		<< "static-mesh-export-package-directory-report"
		<< " status=" << NativeStaticMeshExportPackageDirectoryReadStatusText(
			report.read.status)
		<< " directory=" << report.read.directory.string()
		<< " assets=" << report.read.assets.size()
		<< " issues=" << report.read.issueCount;
	if (!report.read.packageManifestPath.empty()) {
		stream
			<< " packageManifest="
			<< report.read.packageManifestPath.string();
	}
	if (!report.read.manifestPath.empty()) {
		stream
			<< " manifest="
			<< report.read.manifestPath.string()
			<< " manifestExists="
			<< (std::filesystem::exists(report.read.manifestPath) ? 1 : 0);
	}
	stream << "\n";

	for (const NativeStaticMeshExportPackageManifestReadIssue &issue :
			report.read.packageManifestReadIssues) {
		stream
			<< "packageManifestReadIssue"
			<< " code=" << NativeStaticMeshExportPackageDirectoryManifestIssueCodeText(
				issue.code)
			<< " line=" << issue.line
			<< " token=" << issue.token
			<< "\n";
	}

	for (const NativeStaticMeshExportPackageDirectoryAsset &asset :
			report.read.assets) {
		stream
			<< "asset=" << asset.name
			<< " filename=" << asset.filename
			<< " path=" << asset.path.string()
			<< " exists=" << (std::filesystem::exists(asset.path) ? 1 : 0)
			<< "\n";
	}

	report.text = stream.str();
	return report;
}

} // namespace iggy::native_play
