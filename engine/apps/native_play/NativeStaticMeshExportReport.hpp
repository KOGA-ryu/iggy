#pragma once

#include "NativeStaticMeshAssetWriter.hpp"
#include "NativeStaticMeshExportPolicy.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshExportReportStatus {
	Writable,
	WriterFailed,
};

[[nodiscard]] inline const char *NativeStaticMeshExportReportStatusText(
	NativeStaticMeshExportReportStatus status)
{
	switch (status) {
	case NativeStaticMeshExportReportStatus::Writable:
		return "Writable";
	case NativeStaticMeshExportReportStatus::WriterFailed:
		return "WriterFailed";
	}
	return "Unknown";
}

struct NativeStaticMeshExportReportEntry {
	NativeStaticMeshBuiltInExportId id = NativeStaticMeshBuiltInExportId::Cube;
	std::string name;
	std::string defaultFilename;
	NativeStaticMeshExportReportStatus status =
		NativeStaticMeshExportReportStatus::WriterFailed;
	bool writable = false;
	std::size_t issueCount = 0;
	std::size_t vertexCount = 0;
	std::size_t indexCount = 0;
	std::size_t byteCount = 0;
};

struct NativeStaticMeshExportReport {
	std::vector<NativeStaticMeshExportReportEntry> entries;
	std::size_t assetCount = 0;
	std::size_t writableCount = 0;
	std::size_t byteCount = 0;
	std::size_t issueCount = 0;
};

[[nodiscard]] inline NativeStaticMeshExportReport BuildNativeStaticMeshExportReport(
	const NativeStaticMeshExportPolicy &policy)
{
	NativeStaticMeshExportReport report;
	for (const NativeStaticMeshExportAssetRef &ref : policy.assets) {
		const NativeStaticMeshAsset asset = BuiltInNativeStaticMeshExportAsset(ref.id);
		const NativeStaticMeshAssetWriteResult write =
			WriteNativeStaticMeshAssetText(asset);

		NativeStaticMeshExportReportEntry entry;
		entry.id = ref.id;
		entry.name = ref.name;
		entry.defaultFilename = ref.defaultFilename;
		entry.writable = write.written();
		entry.status = entry.writable
			? NativeStaticMeshExportReportStatus::Writable
			: NativeStaticMeshExportReportStatus::WriterFailed;
		entry.issueCount = write.issues.size();
		entry.vertexCount = asset.vertices.size();
		entry.indexCount = asset.indices.size();
		entry.byteCount = entry.writable ? write.text.size() : 0U;

		report.entries.push_back(entry);
		++report.assetCount;
		if (entry.writable)
			++report.writableCount;
		report.byteCount += entry.byteCount;
		report.issueCount += entry.issueCount;
	}
	return report;
}

[[nodiscard]] inline std::string BuildNativeStaticMeshExportReportText(
	const NativeStaticMeshExportReport &report)
{
	std::ostringstream stream;
	stream
		<< "static-mesh-export-report"
		<< " assets=" << report.assetCount
		<< " writable=" << report.writableCount
		<< " bytes=" << report.byteCount
		<< " issues=" << report.issueCount
		<< "\n";
	for (const NativeStaticMeshExportReportEntry &entry : report.entries) {
		stream
			<< "asset=" << entry.name
			<< " filename=" << entry.defaultFilename
			<< " status=" << NativeStaticMeshExportReportStatusText(entry.status)
			<< " vertices=" << entry.vertexCount
			<< " indices=" << entry.indexCount
			<< " bytes=" << entry.byteCount
			<< " issues=" << entry.issueCount
			<< "\n";
	}
	return stream.str();
}

} // namespace iggy::native_play
