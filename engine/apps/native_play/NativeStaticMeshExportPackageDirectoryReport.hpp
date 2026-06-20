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

struct NativeStaticMeshExportPackageDirectoryManifestComparisonResult {
	std::size_t matchCount = 0;
	std::vector<NativeStaticMeshExportPackageDirectoryManifestComparison> comparisons;
};

struct NativeStaticMeshExportPackageDirectoryPathFacts {
	bool exists = false;
	bool regularFile = false;
	std::uintmax_t byteCount = 0;
};

struct NativeStaticMeshExportPackageDirectoryAssetFacts {
	NativeStaticMeshExportPackageDirectoryAsset asset;
	NativeStaticMeshExportPackageDirectoryPathFacts facts;
};

struct NativeStaticMeshExportPackageDirectoryReport {
	NativeStaticMeshExportPackageDirectoryReadResult read;
	bool manifestReadAttempted = false;
	NativeStaticMeshExportManifestReadResult manifestRead;
	NativeStaticMeshExportPackageDirectoryManifestComparisonResult manifestComparison;
	bool packageManifestFactsRecorded = false;
	NativeStaticMeshExportPackageDirectoryPathFacts packageManifestFacts;
	bool manifestFactsRecorded = false;
	NativeStaticMeshExportPackageDirectoryPathFacts manifestFacts;
	std::vector<NativeStaticMeshExportPackageDirectoryAssetFacts> assetFacts;
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

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryPathFacts
ReadNativeStaticMeshExportPackageDirectoryPathFacts(
	const std::filesystem::path &path)
{
	NativeStaticMeshExportPackageDirectoryPathFacts facts;
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
}

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryManifestComparisonResult
CompareNativeStaticMeshExportPackageDirectoryManifestRows(
	const std::vector<NativeStaticMeshExportPackageDirectoryAsset> &packageAssets,
	const std::vector<NativeStaticMeshExportManifestAssetRow> &manifestAssets)
{
	NativeStaticMeshExportPackageDirectoryManifestComparisonResult result;
	for (const NativeStaticMeshExportPackageDirectoryAsset &packageAsset :
			packageAssets) {
		const NativeStaticMeshExportManifestAssetRow *matchingManifestAsset =
			nullptr;
		for (const NativeStaticMeshExportManifestAssetRow &manifestAsset :
				manifestAssets) {
			if (manifestAsset.name == packageAsset.name) {
				matchingManifestAsset = &manifestAsset;
				break;
			}
		}
		if (matchingManifestAsset == nullptr) {
			result.comparisons.push_back({
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromManifest,
				packageAsset.name,
				packageAsset.filename,
				{},
			});
			continue;
		}
		if (matchingManifestAsset->filename != packageAsset.filename) {
			result.comparisons.push_back({
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::FilenameMismatch,
				packageAsset.name,
				packageAsset.filename,
				matchingManifestAsset->filename,
			});
			continue;
		}
		++result.matchCount;
	}

	for (const NativeStaticMeshExportManifestAssetRow &manifestAsset :
			manifestAssets) {
		bool hasPackageAsset = false;
		for (const NativeStaticMeshExportPackageDirectoryAsset &packageAsset :
				packageAssets) {
			if (packageAsset.name == manifestAsset.name) {
				hasPackageAsset = true;
				break;
			}
		}
		if (!hasPackageAsset) {
			result.comparisons.push_back({
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromPackage,
				manifestAsset.name,
				{},
				manifestAsset.filename,
			});
		}
	}

	return result;
}

[[nodiscard]] inline std::string BuildNativeStaticMeshExportPackageDirectoryReportText(
	const NativeStaticMeshExportPackageDirectoryReport &report)
{
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
			<< report.read.packageManifestPath.string()
			<< " packageManifestExists=" << (report.packageManifestFacts.exists ? 1 : 0)
			<< " packageManifestRegularFile=" << (report.packageManifestFacts.regularFile ? 1 : 0)
			<< " packageManifestBytes=" << report.packageManifestFacts.byteCount;
	}
	if (!report.read.manifestPath.empty()) {
		stream
			<< " manifest="
			<< report.read.manifestPath.string()
			<< " manifestExists="
			<< (report.manifestFacts.exists ? 1 : 0)
			<< " manifestRegularFile=" << (report.manifestFacts.regularFile ? 1 : 0)
			<< " manifestBytes=" << report.manifestFacts.byteCount;
		if (report.manifestReadAttempted) {
			stream
				<< " manifestRead=" << (report.manifestRead.read() ? "ok" : "invalid")
				<< " manifestReadIssues=" << report.manifestRead.issues.size();
			if (report.manifestRead.read()) {
				stream
					<< " manifestMatches=" << report.manifestComparison.matchCount
					<< " manifestMismatches=" << report.manifestComparison.comparisons.size()
					<< " manifestComparisonIssues=" << report.manifestComparison.comparisons.size();
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

	if (report.manifestReadAttempted && !report.manifestRead.read()) {
		for (const NativeStaticMeshExportManifestReadIssue &issue :
				report.manifestRead.issues) {
			stream
				<< "manifestReadIssue"
				<< " code=" << NativeStaticMeshExportPackageDirectoryMeshManifestIssueCodeText(
					issue.code)
				<< " line=" << issue.line
				<< " token=" << issue.token
				<< "\n";
		}
	}

	if (report.manifestReadAttempted && report.manifestRead.read()) {
		for (const NativeStaticMeshExportManifestAssetRow &asset :
				report.manifestRead.document.assets) {
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
			report.manifestComparison.comparisons) {
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

	for (const NativeStaticMeshExportPackageDirectoryAssetFacts &assetFacts :
			report.assetFacts) {
		stream
			<< "asset=" << assetFacts.asset.name
			<< " filename=" << assetFacts.asset.filename
			<< " path=" << assetFacts.asset.path.string()
			<< " exists=" << (assetFacts.facts.exists ? 1 : 0)
			<< " regularFile=" << (assetFacts.facts.regularFile ? 1 : 0)
			<< " bytes=" << assetFacts.facts.byteCount
			<< "\n";
	}

	return stream.str();
}

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryReport
BuildNativeStaticMeshExportPackageDirectoryReportData(
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportPackageDirectoryReport report;
	report.read = ReadNativeStaticMeshExportPackageDirectory(directory);

	if (report.read.read() && !report.read.manifestPath.empty()) {
		report.manifestRead = ReadNativeStaticMeshExportManifestFile(
			report.read.manifestPath);
		report.manifestReadAttempted = true;
	}

	if (report.manifestReadAttempted && report.manifestRead.read()) {
		report.manifestComparison = CompareNativeStaticMeshExportPackageDirectoryManifestRows(
			report.read.assets,
			report.manifestRead.document.assets);
	}

	if (!report.read.packageManifestPath.empty()) {
		report.packageManifestFacts =
			ReadNativeStaticMeshExportPackageDirectoryPathFacts(
				report.read.packageManifestPath);
		report.packageManifestFactsRecorded = true;
	}
	if (!report.read.manifestPath.empty()) {
		report.manifestFacts =
			ReadNativeStaticMeshExportPackageDirectoryPathFacts(
				report.read.manifestPath);
		report.manifestFactsRecorded = true;
	}
	for (const NativeStaticMeshExportPackageDirectoryAsset &asset :
			report.read.assets) {
		report.assetFacts.push_back({
			asset,
			ReadNativeStaticMeshExportPackageDirectoryPathFacts(asset.path),
		});
	}

	return report;
}

[[nodiscard]] inline NativeStaticMeshExportPackageDirectoryReport
BuildNativeStaticMeshExportPackageDirectoryReport(
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportPackageDirectoryReport report =
		BuildNativeStaticMeshExportPackageDirectoryReportData(directory);
	report.text = BuildNativeStaticMeshExportPackageDirectoryReportText(report);
	return report;
}

} // namespace iggy::native_play
