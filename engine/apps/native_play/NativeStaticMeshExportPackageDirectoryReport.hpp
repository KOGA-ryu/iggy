#pragma once

#include "NativeStaticMeshExportManifest.hpp"
#include "NativeStaticMeshExportPackageDirectoryReader.hpp"

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshExportPackageDirectoryManifestComparisonCode {
	MissingFromManifest,
	MissingFromPackage,
	FilenameMismatch,
};

struct NativeStaticMeshExportPackageDirectoryManifestComparison {
	NativeStaticMeshExportPackageDirectoryManifestComparisonCode code =
		NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromManifest;
	std::string name;
	std::string packageFilename;
	std::string manifestFilename;
};

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

[[nodiscard]] inline const char *NativeStaticMeshExportPackageDirectoryMeshManifestIssueCodeText(
	NativeStaticMeshExportManifestReadIssueCode code)
{
	switch (code) {
	case NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed:
		return "FileOpenFailed";
	case NativeStaticMeshExportManifestReadIssueCode::EmptyInput:
		return "EmptyInput";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedHeader:
		return "MalformedHeader";
	case NativeStaticMeshExportManifestReadIssueCode::UnsupportedVersion:
		return "UnsupportedVersion";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedAssetCount:
		return "MalformedAssetCount";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedByteCount:
		return "MalformedByteCount";
	case NativeStaticMeshExportManifestReadIssueCode::MissingField:
		return "MissingField";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedAssetRow:
		return "MalformedAssetRow";
	case NativeStaticMeshExportManifestReadIssueCode::AssetCountMismatch:
		return "AssetCountMismatch";
	case NativeStaticMeshExportManifestReadIssueCode::ByteCountMismatch:
		return "ByteCountMismatch";
	case NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetName:
		return "DuplicateAssetName";
	case NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetFilename:
		return "DuplicateAssetFilename";
	case NativeStaticMeshExportManifestReadIssueCode::ExtraToken:
		return "ExtraToken";
	case NativeStaticMeshExportManifestReadIssueCode::UnexpectedLine:
		return "UnexpectedLine";
	}
	return "Unknown";
}

[[nodiscard]] inline const char *NativeStaticMeshExportPackageDirectoryManifestComparisonCodeText(
	NativeStaticMeshExportPackageDirectoryManifestComparisonCode code)
{
	switch (code) {
	case NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromManifest:
		return "MissingFromManifest";
	case NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromPackage:
		return "MissingFromPackage";
	case NativeStaticMeshExportPackageDirectoryManifestComparisonCode::FilenameMismatch:
		return "FilenameMismatch";
	}
	return "Unknown";
}

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryReport
BuildNativeStaticMeshExportPackageDirectoryReport(
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportPackageDirectoryReport report;
	report.read = ReadNativeStaticMeshExportPackageDirectory(directory);

	NativeStaticMeshExportManifestReadResult manifestRead;
	bool hasManifestRead = false;
	if (report.read.read() && !report.read.manifestPath.empty()) {
		manifestRead = ReadNativeStaticMeshExportManifestFile(
			report.read.manifestPath);
		hasManifestRead = true;
	}

	std::size_t manifestMatchCount = 0;
	std::vector<NativeStaticMeshExportPackageDirectoryManifestComparison>
		manifestComparisons;
	if (hasManifestRead && manifestRead.read()) {
		for (const NativeStaticMeshExportPackageDirectoryAsset &packageAsset :
				report.read.assets) {
			const NativeStaticMeshExportManifestAssetRow *matchingManifestAsset =
				nullptr;
			for (const NativeStaticMeshExportManifestAssetRow &manifestAsset :
					manifestRead.document.assets) {
				if (manifestAsset.name == packageAsset.name) {
					matchingManifestAsset = &manifestAsset;
					break;
				}
			}
			if (matchingManifestAsset == nullptr) {
				manifestComparisons.push_back({
					NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromManifest,
					packageAsset.name,
					packageAsset.filename,
					{},
				});
				continue;
			}
			if (matchingManifestAsset->filename != packageAsset.filename) {
				manifestComparisons.push_back({
					NativeStaticMeshExportPackageDirectoryManifestComparisonCode::FilenameMismatch,
					packageAsset.name,
					packageAsset.filename,
					matchingManifestAsset->filename,
				});
				continue;
			}
			++manifestMatchCount;
		}

		for (const NativeStaticMeshExportManifestAssetRow &manifestAsset :
				manifestRead.document.assets) {
			bool hasPackageAsset = false;
			for (const NativeStaticMeshExportPackageDirectoryAsset &packageAsset :
					report.read.assets) {
				if (packageAsset.name == manifestAsset.name) {
					hasPackageAsset = true;
					break;
				}
			}
			if (!hasPackageAsset) {
				manifestComparisons.push_back({
					NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromPackage,
					manifestAsset.name,
					{},
					manifestAsset.filename,
				});
			}
		}
	}

	struct PathFacts {
		bool exists = false;
		bool regularFile = false;
		std::uintmax_t byteCount = 0;
	};

	const auto pathFacts = [](const std::filesystem::path &path) {
		PathFacts facts;
		std::error_code statusError;
		const std::filesystem::file_status status =
			std::filesystem::status(path, statusError);
		if (statusError) {
			return facts;
		}
		facts.exists = std::filesystem::exists(status);
		facts.regularFile = std::filesystem::is_regular_file(status);
		if (facts.regularFile) {
			std::error_code sizeError;
			facts.byteCount = std::filesystem::file_size(path, sizeError);
			if (sizeError) {
				facts.byteCount = 0;
			}
		}
		return facts;
	};

	std::ostringstream stream;
	stream
		<< "static-mesh-export-package-directory-report"
		<< " status=" << NativeStaticMeshExportPackageDirectoryReadStatusText(
			report.read.status)
		<< " directory=" << report.read.directory.string()
		<< " assets=" << report.read.assets.size()
		<< " issues=" << report.read.issueCount;
	if (!report.read.packageManifestPath.empty()) {
		const PathFacts facts = pathFacts(report.read.packageManifestPath);
		stream
			<< " packageManifest="
			<< report.read.packageManifestPath.string()
			<< " packageManifestExists=" << (facts.exists ? 1 : 0)
			<< " packageManifestRegularFile=" << (facts.regularFile ? 1 : 0)
			<< " packageManifestBytes=" << facts.byteCount;
	}
	if (!report.read.manifestPath.empty()) {
		const PathFacts facts = pathFacts(report.read.manifestPath);
		stream
			<< " manifest="
			<< report.read.manifestPath.string()
			<< " manifestExists="
			<< (facts.exists ? 1 : 0)
			<< " manifestRegularFile=" << (facts.regularFile ? 1 : 0)
			<< " manifestBytes=" << facts.byteCount;
		if (hasManifestRead) {
			stream
				<< " manifestRead=" << (manifestRead.read() ? "ok" : "invalid")
				<< " manifestReadIssues=" << manifestRead.issues.size();
			if (manifestRead.read()) {
				stream
					<< " manifestMatches=" << manifestMatchCount
					<< " manifestMismatches=" << manifestComparisons.size();
			}
		}
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

	if (hasManifestRead && !manifestRead.read()) {
		for (const NativeStaticMeshExportManifestReadIssue &issue :
				manifestRead.issues) {
			stream
				<< "manifestReadIssue"
				<< " code=" << NativeStaticMeshExportPackageDirectoryMeshManifestIssueCodeText(
					issue.code)
				<< " line=" << issue.line
				<< " token=" << issue.token
				<< "\n";
		}
	}

	if (hasManifestRead && manifestRead.read()) {
		for (const NativeStaticMeshExportManifestAssetRow &asset :
				manifestRead.document.assets) {
			stream
				<< "manifestAsset=" << asset.name
				<< " filename=" << asset.filename
				<< " vertices=" << asset.vertexCount
				<< " indices=" << asset.indexCount
				<< " bytes=" << asset.byteCount
				<< "\n";
		}
	}

	for (const NativeStaticMeshExportPackageDirectoryManifestComparison &comparison :
			manifestComparisons) {
		stream
			<< "manifestComparison"
			<< " code=" << NativeStaticMeshExportPackageDirectoryManifestComparisonCodeText(
				comparison.code);
		if (comparison.code ==
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromPackage) {
			stream
				<< " manifestAsset=" << comparison.name
				<< " manifestFilename=" << comparison.manifestFilename;
		} else {
			stream
				<< " asset=" << comparison.name
				<< " packageFilename=" << comparison.packageFilename;
			if (comparison.code ==
					NativeStaticMeshExportPackageDirectoryManifestComparisonCode::FilenameMismatch) {
				stream
					<< " manifestFilename=" << comparison.manifestFilename;
			}
		}
		stream << "\n";
	}

	for (const NativeStaticMeshExportPackageDirectoryAsset &asset :
			report.read.assets) {
		const PathFacts facts = pathFacts(asset.path);
		stream
			<< "asset=" << asset.name
			<< " filename=" << asset.filename
			<< " path=" << asset.path.string()
			<< " exists=" << (facts.exists ? 1 : 0)
			<< " regularFile=" << (facts.regularFile ? 1 : 0)
			<< " bytes=" << facts.byteCount
			<< "\n";
	}

	report.text = stream.str();
	return report;
}

} // namespace iggy::native_play
