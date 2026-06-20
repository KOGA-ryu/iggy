#include "../apps/native_play/NativeStaticMeshExportManifest.hpp"
#include "../apps/native_play/NativeStaticMeshExportPolicy.hpp"
#include "../apps/native_play/NativeStaticMeshExportReport.hpp"

#include <cstdlib>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportManifestText;
using iggy::native_play::BuildNativeStaticMeshExportReport;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshBuiltInExportId;
using iggy::native_play::NativeStaticMeshExportManifestResult;
using iggy::native_play::NativeStaticMeshExportManifestStatus;
using iggy::native_play::NativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshExportReport;
using iggy::native_play::NativeStaticMeshExportReportEntry;
using iggy::test::Expect;
using iggy::test::Failures;

std::string ExpectedManifestText(const NativeStaticMeshExportReport &report)
{
	std::string text =
		"static-mesh-export-manifest version=1 assets=" +
		std::to_string(report.assetCount) +
		" bytes=" + std::to_string(report.byteCount) + "\n";
	for (const NativeStaticMeshExportReportEntry &entry : report.entries) {
		text +=
			"asset=" + entry.name +
			" filename=" + entry.defaultFilename +
			" vertices=" + std::to_string(entry.vertexCount) +
			" indices=" + std::to_string(entry.indexCount) +
			" bytes=" + std::to_string(entry.byteCount) + "\n";
	}
	return text;
}

void TestDefaultManifestMatchesReport()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(policy);
	const NativeStaticMeshExportManifestResult result =
		BuildNativeStaticMeshExportManifestText(policy);

	Expect(result.written(), "default manifest should write");
	Expect(
		result.status == NativeStaticMeshExportManifestStatus::Built,
		"default manifest status should be built");
	Expect(result.issueCount == 0, "default manifest should report no issues");
	Expect(result.text == ExpectedManifestText(report), "default manifest should match export report counts");
}

void TestDefaultManifestContainsPolicyOrderAndFilenames()
{
	const NativeStaticMeshExportManifestResult result =
		BuildNativeStaticMeshExportManifestText(DefaultNativeStaticMeshExportPolicy());

	Expect(result.text.find("asset=cube filename=cube.igmesh") != std::string::npos, "manifest should contain cube row");
	Expect(result.text.find("asset=bean filename=bean.igmesh") != std::string::npos, "manifest should contain bean row");
	Expect(result.text.find("asset=npc-marker filename=npc-marker.igmesh") != std::string::npos, "manifest should contain NPC marker row");
	Expect(
		result.text.find("asset=cube") < result.text.find("asset=bean") &&
			result.text.find("asset=bean") < result.text.find("asset=npc-marker"),
		"manifest should preserve default policy order");
}

void TestDefaultManifestIsDeterministic()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshExportManifestResult first =
		BuildNativeStaticMeshExportManifestText(policy);
	const NativeStaticMeshExportManifestResult second =
		BuildNativeStaticMeshExportManifestText(policy);

	Expect(first.written(), "first manifest should write");
	Expect(second.written(), "second manifest should write");
	Expect(first.text == second.text, "manifest output should be deterministic");
}

void TestInvalidPolicyDoesNotWriteManifest()
{
	const NativeStaticMeshExportPolicy policy {
		{
			{ NativeStaticMeshBuiltInExportId::Cube, "cube", "same.igmesh" },
			{ NativeStaticMeshBuiltInExportId::Bean, "bean", "same.igmesh" },
		},
	};

	const NativeStaticMeshExportManifestResult result =
		BuildNativeStaticMeshExportManifestText(policy);

	Expect(!result.written(), "invalid policy should not produce successful manifest");
	Expect(
		result.status == NativeStaticMeshExportManifestStatus::InvalidPolicy,
		"invalid policy should report invalid policy status");
	Expect(result.issueCount > 0, "invalid policy manifest should surface issue count");
	Expect(result.text.empty(), "invalid policy manifest should not emit text");
}

} // namespace

int main()
{
	TestDefaultManifestMatchesReport();
	TestDefaultManifestContainsPolicyOrderAndFilenames();
	TestDefaultManifestIsDeterministic();
	TestInvalidPolicyDoesNotWriteManifest();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
