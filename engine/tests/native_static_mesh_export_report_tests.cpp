#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"
#include "../apps/native_play/NativeStaticMeshExportPolicy.hpp"
#include "../apps/native_play/NativeStaticMeshExportReport.hpp"

#include <cstddef>
#include <cstdlib>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportReport;
using iggy::native_play::BuildNativeStaticMeshExportReportText;
using iggy::native_play::BuiltInNativeStaticMeshExportAsset;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshAssetWriteResult;
using iggy::native_play::NativeStaticMeshBuiltInExportId;
using iggy::native_play::NativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshExportReport;
using iggy::native_play::NativeStaticMeshExportReportEntry;
using iggy::native_play::NativeStaticMeshExportReportStatus;
using iggy::native_play::NativeStaticMeshExportReportStatusText;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;

void ExpectDefaultEntry(
	const NativeStaticMeshExportReportEntry &entry,
	NativeStaticMeshBuiltInExportId id,
	const char *name,
	const char *filename)
{
	Expect(entry.id == id, std::string { name } + " id should match");
	Expect(entry.name == name, std::string { name } + " export name should match");
	Expect(
		entry.defaultFilename == filename,
		std::string { name } + " default filename should match");
}

void TestDefaultReportPreservesPolicyOrder()
{
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(DefaultNativeStaticMeshExportPolicy());

	Expect(report.entries.size() == 3, "default report should contain three entries");
	Expect(report.assetCount == 3, "default report asset count should be three");
	if (report.entries.size() != 3)
		return;

	ExpectDefaultEntry(
		report.entries[0],
		NativeStaticMeshBuiltInExportId::Cube,
		"cube",
		"cube.igmesh");
	ExpectDefaultEntry(
		report.entries[1],
		NativeStaticMeshBuiltInExportId::Bean,
		"bean",
		"bean.igmesh");
	ExpectDefaultEntry(
		report.entries[2],
		NativeStaticMeshBuiltInExportId::NpcMarker,
		"npc-marker",
		"npc-marker.igmesh");
}

void TestDefaultReportEntriesAreWritable()
{
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(DefaultNativeStaticMeshExportPolicy());

	for (const NativeStaticMeshExportReportEntry &entry : report.entries) {
		const NativeStaticMeshAssetWriteResult expectedWrite =
			WriteNativeStaticMeshAssetText(
				BuiltInNativeStaticMeshExportAsset(entry.id));
		const auto expectedAsset = BuiltInNativeStaticMeshExportAsset(entry.id);

		Expect(entry.writable, entry.name + " should be writable");
		Expect(
			entry.status == NativeStaticMeshExportReportStatus::Writable,
			entry.name + " status should be writable");
		Expect(entry.issueCount == 0, entry.name + " should have no write issues");
		Expect(
			entry.vertexCount == expectedAsset.vertices.size(),
			entry.name + " vertex count should match built-in mesh");
		Expect(
			entry.indexCount == expectedAsset.indices.size(),
			entry.name + " index count should match built-in mesh");
		Expect(
			entry.byteCount == expectedWrite.text.size(),
			entry.name + " byte count should match writer output");
	}
}

void TestDefaultReportAggregatesTotals()
{
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(DefaultNativeStaticMeshExportPolicy());

	std::size_t writableCount = 0;
	std::size_t byteCount = 0;
	std::size_t issueCount = 0;
	for (const NativeStaticMeshExportReportEntry &entry : report.entries) {
		if (entry.writable)
			++writableCount;
		byteCount += entry.byteCount;
		issueCount += entry.issueCount;
	}

	Expect(report.assetCount == report.entries.size(), "asset count should match entries");
	Expect(report.writableCount == writableCount, "writable aggregate should match entries");
	Expect(report.byteCount == byteCount, "byte aggregate should match entries");
	Expect(report.issueCount == issueCount, "issue aggregate should match entries");
	Expect(report.writableCount == 3, "all default built-ins should be writable");
	Expect(report.issueCount == 0, "default built-ins should have zero issues");
}

void TestCustomPolicyPreservesOrderAndDuplicateRefs()
{
	const NativeStaticMeshExportPolicy policy {
		{
			{ NativeStaticMeshBuiltInExportId::Bean, "duplicate", "bean-a.igmesh" },
			{ NativeStaticMeshBuiltInExportId::Cube, "duplicate", "cube-b.igmesh" },
		},
	};

	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(policy);

	Expect(report.entries.size() == 2, "custom report should preserve custom entries");
	if (report.entries.size() != 2)
		return;

	Expect(report.entries[0].id == NativeStaticMeshBuiltInExportId::Bean, "first duplicate id should be preserved");
	Expect(report.entries[0].name == "duplicate", "first duplicate name should be preserved");
	Expect(report.entries[0].defaultFilename == "bean-a.igmesh", "first duplicate filename should be preserved");
	Expect(report.entries[1].id == NativeStaticMeshBuiltInExportId::Cube, "second duplicate id should be preserved");
	Expect(report.entries[1].name == "duplicate", "second duplicate name should be preserved");
	Expect(report.entries[1].defaultFilename == "cube-b.igmesh", "second duplicate filename should be preserved");
}

void TestExportReportStatusText()
{
	Expect(
		std::string(NativeStaticMeshExportReportStatusText(
			NativeStaticMeshExportReportStatus::Writable)) == "Writable",
		"writable export report status text should match stable spelling");
	Expect(
		std::string(NativeStaticMeshExportReportStatusText(
			NativeStaticMeshExportReportStatus::WriterFailed)) == "WriterFailed",
		"writer-failed export report status text should match stable spelling");
	Expect(
		std::string(NativeStaticMeshExportReportStatusText(
			static_cast<NativeStaticMeshExportReportStatus>(999))) == "Unknown",
		"export report status text should report unknown fallback");
}

void TestDefaultReportText()
{
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(DefaultNativeStaticMeshExportPolicy());

	const std::string expected =
		"static-mesh-export-report assets=3 writable=3 bytes=33879 issues=0\n"
		"asset=cube filename=cube.igmesh status=Writable vertices=8 indices=36 bytes=523 issues=0\n"
		"asset=bean filename=bean.igmesh status=Writable vertices=234 indices=1296 bytes=23882 issues=0\n"
		"asset=npc-marker filename=npc-marker.igmesh status=Writable vertices=98 indices=504 bytes=9474 issues=0\n";
	Expect(
		BuildNativeStaticMeshExportReportText(report) == expected,
		"default static mesh export report text should match CLI contract");
}

void TestCustomPolicyReportText()
{
	const NativeStaticMeshExportPolicy policy {
		{
			{ NativeStaticMeshBuiltInExportId::Bean, "duplicate", "bean-a.igmesh" },
			{ NativeStaticMeshBuiltInExportId::Cube, "duplicate", "cube-b.igmesh" },
		},
	};

	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(policy);

	const std::string expected =
		"static-mesh-export-report assets=2 writable=2 bytes=24405 issues=0\n"
		"asset=duplicate filename=bean-a.igmesh status=Writable vertices=234 indices=1296 bytes=23882 issues=0\n"
		"asset=duplicate filename=cube-b.igmesh status=Writable vertices=8 indices=36 bytes=523 issues=0\n";
	Expect(
		BuildNativeStaticMeshExportReportText(report) == expected,
		"custom static mesh export report text should preserve duplicate policy row order");
}

} // namespace

int main()
{
	TestDefaultReportPreservesPolicyOrder();
	TestDefaultReportEntriesAreWritable();
	TestDefaultReportAggregatesTotals();
	TestCustomPolicyPreservesOrderAndDuplicateRefs();
	TestExportReportStatusText();
	TestDefaultReportText();
	TestCustomPolicyReportText();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
