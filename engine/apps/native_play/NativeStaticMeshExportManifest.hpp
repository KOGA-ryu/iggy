#pragma once

#include "NativeStaticMeshExportPolicy.hpp"
#include "NativeStaticMeshExportReport.hpp"

#include <cstddef>
#include <sstream>
#include <string>

namespace iggy::native_play {

enum class NativeStaticMeshExportManifestStatus {
	Built,
	InvalidPolicy,
	WriterFailed,
};

struct NativeStaticMeshExportManifestResult {
	NativeStaticMeshExportManifestStatus status =
		NativeStaticMeshExportManifestStatus::InvalidPolicy;
	std::string text;
	std::size_t issueCount = 0;

	[[nodiscard]] bool written() const
	{
		return status == NativeStaticMeshExportManifestStatus::Built &&
			!text.empty();
	}
};

[[nodiscard]] inline NativeStaticMeshExportManifestResult
BuildNativeStaticMeshExportManifestText(
	const NativeStaticMeshExportPolicy &policy)
{
	NativeStaticMeshExportManifestResult result;
	const NativeStaticMeshExportPolicyValidationResult validation =
		ValidateNativeStaticMeshExportPolicy(policy);
	if (!validation.valid()) {
		result.status = NativeStaticMeshExportManifestStatus::InvalidPolicy;
		result.issueCount = validation.issues.size();
		return result;
	}

	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(policy);
	if (report.issueCount != 0 || report.writableCount != report.assetCount) {
		result.status = NativeStaticMeshExportManifestStatus::WriterFailed;
		result.issueCount = report.issueCount;
		return result;
	}

	std::ostringstream stream;
	stream
		<< "static-mesh-export-manifest"
		<< " version=1"
		<< " assets=" << report.assetCount
		<< " bytes=" << report.byteCount
		<< "\n";
	for (const NativeStaticMeshExportReportEntry &entry : report.entries) {
		stream
			<< "asset=" << entry.name
			<< " filename=" << entry.defaultFilename
			<< " vertices=" << entry.vertexCount
			<< " indices=" << entry.indexCount
			<< " bytes=" << entry.byteCount
			<< "\n";
	}

	result.status = NativeStaticMeshExportManifestStatus::Built;
	result.text = stream.str();
	return result;
}

} // namespace iggy::native_play
