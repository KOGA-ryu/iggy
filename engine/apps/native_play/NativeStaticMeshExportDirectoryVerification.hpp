#pragma once

#include "NativeStaticMeshAssetLoader.hpp"
#include "NativeStaticMeshFileExport.hpp"
#include "NativeStaticMeshExportManifest.hpp"
#include "NativeStaticMeshExportPackageManifest.hpp"
#include "NativeStaticMeshExportPolicy.hpp"
#include "NativeStaticMeshExportReport.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshExportDirectoryVerificationStatus {
	Verified,
	InvalidPolicy,
	MissingOutputDirectory,
	OutputDirectoryNotDirectory,
	MissingManifest,
	ManifestMismatch,
	MissingAsset,
	AssetLoadFailed,
	GeometryMismatch,
	ManifestBuildFailed,
	MissingPackageManifest,
	PackageManifestReadFailed,
	PackageManifestMismatch,
	PackageManifestBuildFailed,
};

[[nodiscard]] inline const char *NativeStaticMeshExportDirectoryVerificationStatusText(
	NativeStaticMeshExportDirectoryVerificationStatus status)
{
	switch (status) {
	case NativeStaticMeshExportDirectoryVerificationStatus::Verified:
		return "Verified";
	case NativeStaticMeshExportDirectoryVerificationStatus::InvalidPolicy:
		return "InvalidPolicy";
	case NativeStaticMeshExportDirectoryVerificationStatus::MissingOutputDirectory:
		return "MissingOutputDirectory";
	case NativeStaticMeshExportDirectoryVerificationStatus::OutputDirectoryNotDirectory:
		return "OutputDirectoryNotDirectory";
	case NativeStaticMeshExportDirectoryVerificationStatus::MissingManifest:
		return "MissingManifest";
	case NativeStaticMeshExportDirectoryVerificationStatus::ManifestMismatch:
		return "ManifestMismatch";
	case NativeStaticMeshExportDirectoryVerificationStatus::MissingAsset:
		return "MissingAsset";
	case NativeStaticMeshExportDirectoryVerificationStatus::AssetLoadFailed:
		return "AssetLoadFailed";
	case NativeStaticMeshExportDirectoryVerificationStatus::GeometryMismatch:
		return "GeometryMismatch";
	case NativeStaticMeshExportDirectoryVerificationStatus::ManifestBuildFailed:
		return "ManifestBuildFailed";
	case NativeStaticMeshExportDirectoryVerificationStatus::MissingPackageManifest:
		return "MissingPackageManifest";
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestReadFailed:
		return "PackageManifestReadFailed";
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestMismatch:
		return "PackageManifestMismatch";
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestBuildFailed:
		return "PackageManifestBuildFailed";
	}
	return "Unknown";
}

struct NativeStaticMeshExportDirectoryVerificationEntry {
	std::string name;
	std::string filename;
	NativeStaticMeshExportDirectoryVerificationStatus status =
		NativeStaticMeshExportDirectoryVerificationStatus::Verified;
	std::filesystem::path path;
	std::size_t vertexCount = 0;
	std::size_t indexCount = 0;
	std::size_t expectedVertexCount = 0;
	std::size_t expectedIndexCount = 0;
	std::size_t issueCount = 0;
};

struct NativeStaticMeshExportDirectoryVerificationResult {
	NativeStaticMeshExportDirectoryVerificationStatus status =
		NativeStaticMeshExportDirectoryVerificationStatus::Verified;
	std::filesystem::path outputDirectory;
	std::filesystem::path problemPath;
	std::filesystem::path manifestPath;
	std::filesystem::path packageManifestPath;
	std::vector<NativeStaticMeshExportDirectoryVerificationEntry> entries;
	std::vector<NativeStaticMeshExportPackageManifestReadIssue> packageManifestReadIssues;
	std::size_t verifiedCount = 0;
	std::size_t issueCount = 0;
	std::size_t packageManifestReadIssueCount = 0;
	bool manifestVerified = false;
	bool packageManifestVerified = false;

	[[nodiscard]] bool verified() const
	{
		return status == NativeStaticMeshExportDirectoryVerificationStatus::Verified;
	}
};

[[nodiscard]] inline std::string BuildNativeStaticMeshExportDirectoryVerificationSuccessText(
	const NativeStaticMeshExportDirectoryVerificationResult &result)
{
	std::ostringstream stream;
	stream
		<< "static-mesh-export-verify"
		<< " output=" << result.outputDirectory.string()
		<< " verified=" << result.verifiedCount
		<< " manifest=ok"
		<< " packageManifest=ok"
		<< "\n";
	return stream.str();
}

[[nodiscard]] inline std::string NativeStaticMeshExportReadTextFile(
	const std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return {};
	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

inline void NativeStaticMeshExportDirectorySetFailureIfVerified(
	NativeStaticMeshExportDirectoryVerificationResult &result,
	NativeStaticMeshExportDirectoryVerificationStatus status,
	const std::filesystem::path &path)
{
	if (result.status == NativeStaticMeshExportDirectoryVerificationStatus::Verified) {
		result.status = status;
		result.problemPath = path;
	}
}

[[nodiscard]] inline NativeStaticMeshExportDirectoryVerificationResult
VerifyNativeStaticMeshExportDirectory(
	const NativeStaticMeshExportPolicy &policy,
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportDirectoryVerificationResult result;
	result.outputDirectory = directory;

	const NativeStaticMeshExportPolicyValidationResult validation =
		ValidateNativeStaticMeshExportPolicy(policy);
	if (!validation.valid()) {
		result.status = NativeStaticMeshExportDirectoryVerificationStatus::InvalidPolicy;
		result.issueCount = validation.issues.size();
		return result;
	}

	if (!std::filesystem::exists(directory)) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::MissingOutputDirectory;
		result.problemPath = directory;
		return result;
	}
	if (!std::filesystem::is_directory(directory)) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::OutputDirectoryNotDirectory;
		result.problemPath = directory;
		return result;
	}

	const NativeStaticMeshExportManifestResult manifest =
		BuildNativeStaticMeshExportManifestText(policy);
	if (!manifest.written()) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::ManifestBuildFailed;
		result.issueCount = manifest.issueCount;
		return result;
	}

	const std::filesystem::path manifestPath =
		directory / NativeStaticMeshExportManifestFilename;
	result.manifestPath = manifestPath;
	if (!std::filesystem::exists(result.manifestPath)) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::MissingManifest;
		result.problemPath = result.manifestPath;
		return result;
	}
	if (NativeStaticMeshExportReadTextFile(result.manifestPath) != manifest.text) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::ManifestMismatch;
		result.problemPath = result.manifestPath;
		++result.issueCount;
		return result;
	}
	result.manifestVerified = true;

	NativeStaticMeshExportPackagePolicy packagePolicy =
		DefaultNativeStaticMeshExportPackagePolicy();
	packagePolicy.meshPolicy = policy;
	const NativeStaticMeshExportPackageManifestResult packageManifest =
		BuildNativeStaticMeshExportPackageManifestText(packagePolicy);
	if (!packageManifest.written()) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestBuildFailed;
		result.issueCount = packageManifest.issueCount;
		return result;
	}

	const std::filesystem::path packageManifestPath =
		directory / NativeStaticMeshExportPackageManifestSidecarFilename;
	result.packageManifestPath = packageManifestPath;
	if (!std::filesystem::exists(result.packageManifestPath)) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::MissingPackageManifest;
		result.problemPath = result.packageManifestPath;
		return result;
	}
	const NativeStaticMeshExportPackageManifestReadResult readPackageManifest =
		ReadNativeStaticMeshExportPackageManifestFile(result.packageManifestPath);
	if (!readPackageManifest.read()) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestReadFailed;
		result.problemPath = result.packageManifestPath;
		result.packageManifestReadIssues = readPackageManifest.issues;
		result.packageManifestReadIssueCount =
			result.packageManifestReadIssues.size();
		result.issueCount = result.packageManifestReadIssueCount;
		return result;
	}
	if (NativeStaticMeshExportReadTextFile(result.packageManifestPath) != packageManifest.text) {
		result.status =
			NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestMismatch;
		result.problemPath = result.packageManifestPath;
		++result.issueCount;
		return result;
	}
	result.packageManifestVerified = true;

	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(policy);
	for (std::size_t index = 0; index < policy.assets.size(); ++index) {
		const NativeStaticMeshExportAssetRef &assetRef = policy.assets[index];
		const NativeStaticMeshExportReportEntry &reportEntry = report.entries[index];

		NativeStaticMeshExportDirectoryVerificationEntry entry;
		entry.name = assetRef.name;
		entry.filename = assetRef.defaultFilename;
		entry.expectedVertexCount = reportEntry.vertexCount;
		entry.expectedIndexCount = reportEntry.indexCount;
		entry.path = directory / assetRef.defaultFilename;

		if (!std::filesystem::exists(entry.path)) {
			entry.status =
				NativeStaticMeshExportDirectoryVerificationStatus::MissingAsset;
			++entry.issueCount;
			NativeStaticMeshExportDirectorySetFailureIfVerified(
				result,
				entry.status,
				entry.path);
			++result.issueCount;
			result.entries.push_back(entry);
			continue;
		}

		const NativeStaticMeshAssetLoadResult loaded =
			LoadNativeStaticMeshAssetFile(entry.path);
		entry.issueCount = loaded.issues.size();
		entry.vertexCount = loaded.asset.vertices.size();
		entry.indexCount = loaded.asset.indices.size();
		if (!loaded.loaded()) {
			entry.status =
				NativeStaticMeshExportDirectoryVerificationStatus::AssetLoadFailed;
			NativeStaticMeshExportDirectorySetFailureIfVerified(
				result,
				entry.status,
				entry.path);
			result.issueCount += entry.issueCount == 0 ? 1U : entry.issueCount;
			result.entries.push_back(entry);
			continue;
		}

		if (entry.vertexCount != entry.expectedVertexCount ||
				entry.indexCount != entry.expectedIndexCount) {
			entry.status =
				NativeStaticMeshExportDirectoryVerificationStatus::GeometryMismatch;
			++entry.issueCount;
			NativeStaticMeshExportDirectorySetFailureIfVerified(
				result,
				entry.status,
				entry.path);
			++result.issueCount;
			result.entries.push_back(entry);
			continue;
		}

		entry.status = NativeStaticMeshExportDirectoryVerificationStatus::Verified;
		++result.verifiedCount;
		result.entries.push_back(entry);
	}

	return result;
}

} // namespace iggy::native_play
