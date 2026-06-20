#pragma once

#include "NativeStaticMeshExportPolicy.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace iggy::native_play {

inline constexpr const char *NativeStaticMeshExportPackageFormatId =
	"iggy:native-static-mesh-export-package";
inline constexpr int NativeStaticMeshExportPackageFormatVersion = 1;
inline constexpr const char *NativeStaticMeshExportPackageManifestFilename =
	"static-mesh-export-manifest.txt";

struct NativeStaticMeshExportPackagePolicy {
	std::string formatId = NativeStaticMeshExportPackageFormatId;
	int version = NativeStaticMeshExportPackageFormatVersion;
	std::string manifestFilename = NativeStaticMeshExportPackageManifestFilename;
	NativeStaticMeshExportPolicy meshPolicy;
};

enum class NativeStaticMeshExportPackagePolicyValidationIssueCode {
	EmptyFormatId,
	UnsupportedFormatId,
	UnsupportedVersion,
	EmptyManifestFilename,
	ManifestFilenameContainsSeparator,
	ManifestFilenameCollidesWithAssetFilename,
	InvalidMeshExportPolicy,
};

struct NativeStaticMeshExportPackagePolicyValidationIssue {
	NativeStaticMeshExportPackagePolicyValidationIssueCode code =
		NativeStaticMeshExportPackagePolicyValidationIssueCode::EmptyFormatId;
	std::size_t assetIndex = 0;
	std::size_t nestedIssueCount = 0;
	std::string value;
};

struct NativeStaticMeshExportPackagePolicyValidationResult {
	std::vector<NativeStaticMeshExportPackagePolicyValidationIssue> issues;

	[[nodiscard]] bool valid() const
	{
		return issues.empty();
	}
};

[[nodiscard]] inline NativeStaticMeshExportPackagePolicy
DefaultNativeStaticMeshExportPackagePolicy()
{
	NativeStaticMeshExportPackagePolicy policy;
	policy.meshPolicy = DefaultNativeStaticMeshExportPolicy();
	return policy;
}

inline void AddNativeStaticMeshExportPackagePolicyValidationIssue(
	NativeStaticMeshExportPackagePolicyValidationResult &result,
	NativeStaticMeshExportPackagePolicyValidationIssueCode code,
	const std::string &value,
	std::size_t assetIndex = 0,
	std::size_t nestedIssueCount = 0)
{
	result.issues.push_back({ code, assetIndex, nestedIssueCount, value });
}

[[nodiscard]] inline NativeStaticMeshExportPackagePolicyValidationResult
ValidateNativeStaticMeshExportPackagePolicy(
	const NativeStaticMeshExportPackagePolicy &policy)
{
	NativeStaticMeshExportPackagePolicyValidationResult result;

	if (policy.formatId.empty()) {
		AddNativeStaticMeshExportPackagePolicyValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::EmptyFormatId,
			policy.formatId);
	} else if (policy.formatId != NativeStaticMeshExportPackageFormatId) {
		AddNativeStaticMeshExportPackagePolicyValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::UnsupportedFormatId,
			policy.formatId);
	}

	if (policy.version != NativeStaticMeshExportPackageFormatVersion) {
		AddNativeStaticMeshExportPackagePolicyValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::UnsupportedVersion,
			std::to_string(policy.version));
	}

	if (policy.manifestFilename.empty()) {
		AddNativeStaticMeshExportPackagePolicyValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::EmptyManifestFilename,
			policy.manifestFilename);
	} else {
		if (NativeStaticMeshExportFilenameContainsSeparator(policy.manifestFilename)) {
			AddNativeStaticMeshExportPackagePolicyValidationIssue(
				result,
				NativeStaticMeshExportPackagePolicyValidationIssueCode::ManifestFilenameContainsSeparator,
				policy.manifestFilename);
		}
		for (std::size_t index = 0; index < policy.meshPolicy.assets.size(); ++index) {
			const NativeStaticMeshExportAssetRef &asset =
				policy.meshPolicy.assets[index];
			if (asset.defaultFilename == policy.manifestFilename) {
				AddNativeStaticMeshExportPackagePolicyValidationIssue(
					result,
					NativeStaticMeshExportPackagePolicyValidationIssueCode::ManifestFilenameCollidesWithAssetFilename,
					policy.manifestFilename,
					index);
			}
		}
	}

	const NativeStaticMeshExportPolicyValidationResult meshValidation =
		ValidateNativeStaticMeshExportPolicy(policy.meshPolicy);
	if (!meshValidation.valid()) {
		AddNativeStaticMeshExportPackagePolicyValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::InvalidMeshExportPolicy,
			{},
			0,
			meshValidation.issues.size());
	}

	return result;
}

} // namespace iggy::native_play
