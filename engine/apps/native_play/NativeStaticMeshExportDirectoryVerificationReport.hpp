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
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestReadFailed:
		return "PackageManifestReadFailed";
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestMismatch:
		return "PackageManifestMismatch";
	case NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestBuildFailed:
		return "PackageManifestBuildFailed";
	}
	return "Unknown";
}

[[nodiscard]] inline const char *NativeStaticMeshExportPackageManifestReadIssueCodeText(
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
	if (verification.status == NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestReadFailed)
		return "invalid";
	if (verification.status == NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestMismatch)
		return "mismatch";
	return "not-checked";
}

[[nodiscard]] inline std::string BuildNativeStaticMeshExportDirectoryVerificationReportText(
	const NativeStaticMeshExportDirectoryVerificationReport &report)
{
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

	for (const NativeStaticMeshExportPackageManifestReadIssue &issue :
			report.verification.packageManifestReadIssues) {
		stream
			<< "packageManifestReadIssue"
			<< " code=" << NativeStaticMeshExportPackageManifestReadIssueCodeText(
				issue.code)
			<< " line=" << issue.line
			<< " token=" << issue.token
			<< "\n";
	}

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

	return stream.str();
}

[[nodiscard]] inline NativeStaticMeshExportDirectoryVerificationReport
BuildNativeStaticMeshExportDirectoryVerificationReportData(
	const NativeStaticMeshExportPolicy &policy,
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportDirectoryVerificationReport report;
	report.verification = VerifyNativeStaticMeshExportDirectory(policy, directory);
	return report;
}

[[nodiscard]] inline NativeStaticMeshExportDirectoryVerificationReport
BuildNativeStaticMeshExportDirectoryVerificationReport(
	const NativeStaticMeshExportPolicy &policy,
	const std::filesystem::path &directory)
{
	NativeStaticMeshExportDirectoryVerificationReport report =
		BuildNativeStaticMeshExportDirectoryVerificationReportData(policy, directory);
	report.text = BuildNativeStaticMeshExportDirectoryVerificationReportText(report);
	return report;
}

} // namespace iggy::native_play
