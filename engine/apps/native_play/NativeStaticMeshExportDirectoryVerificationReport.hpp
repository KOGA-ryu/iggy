#pragma once

#include "NativeStaticMeshExportDirectoryVerification.hpp"
#include "NativeStaticMeshExportPolicy.hpp"

#include <filesystem>
#include <sstream>
#include <string>

namespace iggy::native_play {

struct NativeStaticMeshExportDirectoryVerificationReport {
	NativeStaticMeshExportDirectoryVerificationResult verification;
	std::string text;

	[[nodiscard]] bool verified() const
	{
		return verification.verified();
	}
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
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestMismatch:
		return "PackageManifestMismatch";
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestBuildFailed:
		return "PackageManifestBuildFailed";
	}
	return "Unknown";
}

[[nodiscard]] inline const char *NativeStaticMeshExportDirectoryVerificationManifestState(
	const NativeStaticMeshExportDirectoryVerificationResult &verification)
{
	if (verification.manifestVerified)
		return "ok";
	if (verification.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingManifest)
		return "missing";
	if (verification.status == NativeStaticMeshExportDirectoryVerificationStatus::ManifestMismatch)
		return "mismatch";
	return "not-checked";
}

[[nodiscard]] inline const char *NativeStaticMeshExportDirectoryVerificationPackageManifestState(
	const NativeStaticMeshExportDirectoryVerificationResult &verification)
{
	if (verification.packageManifestVerified)
		return "ok";
	if (verification.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingPackageManifest)
		return "missing";
	if (verification.status == NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestMismatch)
		return "mismatch";
	return "not-checked";
}

[[nodiscard]] inline NativeStaticMeshExportDirectoryVerificationReport
BuildNativeStaticMeshExportDirectoryVerificationReport(
	const NativeStaticMeshExportPolicy &policy,
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportDirectoryVerificationReport report;
	report.verification = VerifyNativeStaticMeshExportDirectory(policy, directory);

	std::ostringstream stream;
	stream
		<< "static-mesh-export-verification-report"
		<< " status=" << NativeStaticMeshExportDirectoryVerificationStatusText(
			report.verification.status)
		<< " output=" << report.verification.outputDirectory.string()
		<< " verified=" << report.verification.verifiedCount
		<< " issues=" << report.verification.issueCount
		<< " manifest=" << NativeStaticMeshExportDirectoryVerificationManifestState(
			report.verification)
		<< " packageManifest=" << NativeStaticMeshExportDirectoryVerificationPackageManifestState(
			report.verification);
	if (!report.verification.problemPath.empty())
		stream << " problem=" << report.verification.problemPath.string();
	stream << "\n";

	for (const NativeStaticMeshExportDirectoryVerificationEntry &entry :
			report.verification.entries) {
		stream
			<< "asset=" << entry.name
			<< " filename=" << entry.filename
			<< " status=" << NativeStaticMeshExportDirectoryVerificationStatusText(
				entry.status)
			<< " vertices=" << entry.vertexCount
			<< " expectedVertices=" << entry.expectedVertexCount
			<< " indices=" << entry.indexCount
			<< " expectedIndices=" << entry.expectedIndexCount
			<< " issues=" << entry.issueCount
			<< "\n";
	}

	report.text = stream.str();
	return report;
}

} // namespace iggy::native_play
