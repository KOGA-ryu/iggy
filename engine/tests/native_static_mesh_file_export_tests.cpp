#include "../apps/native_play/NativeStaticMeshAssetLoader.hpp"
#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuiltInNativeStaticMeshExportAsset;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::ExportNativeStaticMeshAssetToDirectory;
using iggy::native_play::FindNativeStaticMeshExportAsset;
using iggy::native_play::LoadNativeStaticMeshAssetFile;
using iggy::native_play::NativeStaticMeshAssetLoadResult;
using iggy::native_play::NativeStaticMeshAssetWriteResult;
using iggy::native_play::NativeStaticMeshExportAssetRef;
using iggy::native_play::NativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshFileExportResult;
using iggy::native_play::NativeStaticMeshFileExportStatus;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"iggy_native_static_mesh_file_export_tests";
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

void CleanupTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

void WriteText(const std::filesystem::path &path, const std::string &text)
{
	std::ofstream file(path, std::ios::binary);
	file << text;
}

void TestExportsDefaultPolicyAssetsAndReloads()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();

	for (const NativeStaticMeshExportAssetRef &asset : policy.assets) {
		const NativeStaticMeshFileExportResult result =
			ExportNativeStaticMeshAssetToDirectory(policy, asset.name, TempRoot());

		Expect(
			result.status == NativeStaticMeshFileExportStatus::Exported,
			asset.name + " should export");
		Expect(result.outputPath == TempRoot() / asset.defaultFilename, asset.name + " should use default filename");
		Expect(result.byteCount > 0, asset.name + " should report written bytes");
		Expect(result.issueCount == 0, asset.name + " should report no writer issues");
		Expect(std::filesystem::exists(result.outputPath), asset.name + " file should exist");

		const NativeStaticMeshAssetLoadResult loaded =
			LoadNativeStaticMeshAssetFile(result.outputPath);
		Expect(loaded.loaded(), asset.name + " exported file should reload");

		const NativeStaticMeshAssetWriteResult expected =
			WriteNativeStaticMeshAssetText(BuiltInNativeStaticMeshExportAsset(asset.id));
		Expect(expected.written(), asset.name + " expected writer output should write");
		Expect(result.byteCount == expected.text.size(), asset.name + " byte count should match writer text size");
	}

	CleanupTempRoot();
}

void TestUnknownAssetDoesNotCreateFiles()
{
	ResetTempRoot();
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"nope",
			TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::UnknownAsset,
		"unknown asset should be rejected");
	Expect(std::filesystem::is_empty(TempRoot()), "unknown asset should not create files");
	CleanupTempRoot();
}

void TestMissingOutputDirectoryRejectedWithoutCreatingParents()
{
	CleanupTempRoot();
	const std::filesystem::path missing = TempRoot() / "missing";
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"cube",
			missing);

	Expect(
		result.status == NativeStaticMeshFileExportStatus::MissingOutputDirectory,
		"missing output directory should be rejected");
	Expect(!std::filesystem::exists(TempRoot()), "export should not create parent directories");
}

void TestOutputPathMustBeDirectory()
{
	ResetTempRoot();
	const std::filesystem::path filePath = TempRoot() / "not-a-directory";
	WriteText(filePath, "not a directory");

	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"cube",
			filePath);

	Expect(
		result.status == NativeStaticMeshFileExportStatus::OutputDirectoryNotDirectory,
		"file output path should be rejected as not directory");
	Expect(!std::filesystem::exists(filePath / "cube.igmesh"), "not-directory export should not create target");
	CleanupTempRoot();
}

void TestTargetAlreadyExistsIsRejected()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshExportAssetRef *cube =
		FindNativeStaticMeshExportAsset(policy, "cube");
	Expect(cube != nullptr, "cube policy ref should exist");
	if (cube == nullptr)
		return;

	const std::filesystem::path target = TempRoot() / cube->defaultFilename;
	WriteText(target, "existing");
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(policy, "cube", TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::TargetAlreadyExists,
		"existing target should be rejected");
	const NativeStaticMeshAssetLoadResult loaded =
		LoadNativeStaticMeshAssetFile(target);
	Expect(!loaded.loaded(), "existing target should not be overwritten with valid mesh text");
	CleanupTempRoot();
}

} // namespace

int main()
{
	TestExportsDefaultPolicyAssetsAndReloads();
	TestUnknownAssetDoesNotCreateFiles();
	TestMissingOutputDirectoryRejectedWithoutCreatingParents();
	TestOutputPathMustBeDirectory();
	TestTargetAlreadyExistsIsRejected();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
