#pragma once

#include "NativeStaticMeshExportPackagePolicy.hpp"

#include <cstddef>
#include <sstream>
#include <string>

namespace iggy::native_play {

enum class NativeStaticMeshExportPackageManifestStatus {
	Built,
	InvalidPolicy,
};

struct NativeStaticMeshExportPackageManifestResult {
	NativeStaticMeshExportPackageManifestStatus status =
		NativeStaticMeshExportPackageManifestStatus::InvalidPolicy;
	std::string text;
	std::size_t issueCount = 0;

	[[nodiscard]] bool written() const
	{
		return status == NativeStaticMeshExportPackageManifestStatus::Built &&
			!text.empty();
	}
};

[[nodiscard]] inline NativeStaticMeshExportPackageManifestResult
BuildNativeStaticMeshExportPackageManifestText(
	const NativeStaticMeshExportPackagePolicy &policy)
{
	NativeStaticMeshExportPackageManifestResult result;
	const NativeStaticMeshExportPackagePolicyValidationResult validation =
		ValidateNativeStaticMeshExportPackagePolicy(policy);
	if (!validation.valid()) {
		result.status = NativeStaticMeshExportPackageManifestStatus::InvalidPolicy;
		result.issueCount = validation.issues.size();
		return result;
	}

	std::ostringstream stream;
	stream
		<< "static-mesh-export-package-manifest"
		<< " format=" << policy.formatId
		<< " version=" << policy.version
		<< " manifest=" << policy.manifestFilename
		<< " assets=" << policy.meshPolicy.assets.size()
		<< "\n";
	for (const NativeStaticMeshExportAssetRef &asset : policy.meshPolicy.assets) {
		stream
			<< "asset=" << asset.name
			<< " filename=" << asset.defaultFilename
			<< "\n";
	}

	result.status = NativeStaticMeshExportPackageManifestStatus::Built;
	result.text = stream.str();
	return result;
}

} // namespace iggy::native_play
