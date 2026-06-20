#include "../apps/native_play/NativeStaticMeshExportPackageManifest.hpp"

#include <cstdlib>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportPackageManifestText;
using iggy::native_play::DefaultNativeStaticMeshExportPackagePolicy;
using iggy::native_play::NativeStaticMeshExportPackageManifestResult;
using iggy::native_play::NativeStaticMeshExportPackageManifestStatus;
using iggy::native_play::NativeStaticMeshExportPackagePolicy;
using iggy::test::Expect;
using iggy::test::Failures;

const char *ExpectedDefaultManifestText()
{
	return
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=3\n"
		"asset=cube filename=cube.igmesh\n"
		"asset=bean filename=bean.igmesh\n"
		"asset=npc-marker filename=npc-marker.igmesh\n";
}

void TestDefaultPackageManifestBuilds()
{
	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());

	Expect(result.written(), "default package manifest should write");
	Expect(
		result.status == NativeStaticMeshExportPackageManifestStatus::Built,
		"default package manifest status should be built");
	Expect(result.issueCount == 0, "default package manifest should report no issues");
	Expect(
		result.text == ExpectedDefaultManifestText(),
		"default package manifest text should be exact");
}

void TestDefaultPackageManifestIncludesStableSummary()
{
	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());

	Expect(
		result.text.find("format=iggy:native-static-mesh-export-package") !=
			std::string::npos,
		"default package manifest should include format id");
	Expect(
		result.text.find("version=1") != std::string::npos,
		"default package manifest should include version");
	Expect(
		result.text.find("manifest=static-mesh-export-manifest.txt") !=
			std::string::npos,
		"default package manifest should include nested manifest filename");
	Expect(
		result.text.find("assets=3") != std::string::npos,
		"default package manifest should include asset count");
}

void TestAssetRowsFollowNestedMeshPolicyOrder()
{
	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());

	const std::size_t cube = result.text.find("asset=cube filename=cube.igmesh");
	const std::size_t bean = result.text.find("asset=bean filename=bean.igmesh");
	const std::size_t npc =
		result.text.find("asset=npc-marker filename=npc-marker.igmesh");
	Expect(cube != std::string::npos, "cube row should be present");
	Expect(bean != std::string::npos, "bean row should be present");
	Expect(npc != std::string::npos, "NPC marker row should be present");
	Expect(cube < bean && bean < npc, "asset rows should follow policy order");
}

void TestRepeatedCallsAreDeterministic()
{
	const NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();

	const NativeStaticMeshExportPackageManifestResult first =
		BuildNativeStaticMeshExportPackageManifestText(policy);
	const NativeStaticMeshExportPackageManifestResult second =
		BuildNativeStaticMeshExportPackageManifestText(policy);

	Expect(first.written(), "first deterministic package manifest should write");
	Expect(second.written(), "second deterministic package manifest should write");
	Expect(first.text == second.text, "package manifest output should be deterministic");
}

void TestInvalidPolicyReturnsFailureWithoutText()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.version = 2;

	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(policy);

	Expect(!result.written(), "invalid package policy should not write manifest");
	Expect(
		result.status == NativeStaticMeshExportPackageManifestStatus::InvalidPolicy,
		"invalid package policy should report invalid-policy status");
	Expect(result.issueCount > 0, "invalid package policy should report issues");
	Expect(result.text.empty(), "invalid package policy should produce no text");
}

void TestBuilderDoesNotNeedFilesystemState()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.manifestFilename = "not-present-on-disk.txt";

	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(policy);

	Expect(result.written(), "package manifest builder should not touch filesystem");
	Expect(
		result.text.find("manifest=not-present-on-disk.txt") != std::string::npos,
		"package manifest should use supplied manifest filename value");
}

} // namespace

int main()
{
	TestDefaultPackageManifestBuilds();
	TestDefaultPackageManifestIncludesStableSummary();
	TestAssetRowsFollowNestedMeshPolicyOrder();
	TestRepeatedCallsAreDeterministic();
	TestInvalidPolicyReturnsFailureWithoutText();
	TestBuilderDoesNotNeedFilesystemState();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
