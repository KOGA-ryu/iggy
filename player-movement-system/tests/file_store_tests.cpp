#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "files/ByteFileStore.hpp"
#include "files/TextFileStore.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestByteFileStoreSavesLoadsAndCleansTempFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_byte_file_store_test";
	const std::filesystem::path path = root / "bytes.bin";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::ByteFileStore store;
	std::vector<uint8_t> bytes { 0, 1, 2, 255 };

	Expect(store.save(path, bytes), "byte file store should save binary bytes");
	std::optional<std::vector<uint8_t>> loaded = store.load(path);

	Expect(loaded.has_value(), "byte file store should load saved bytes");
	Expect(loaded.has_value() && *loaded == bytes, "byte file store should preserve binary byte payload");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "byte file store should remove temp file after save");

	std::filesystem::remove_all(root);
}

void TestByteFileStoreRejectsMissingAndUnwritablePaths()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_byte_file_store_missing_test";
	const std::filesystem::path missing = root / "missing.bin";
	const std::filesystem::path unwritable = root / "missing_directory" / "bytes.bin";
	std::filesystem::remove_all(root);

	dev::ByteFileStore store;

	Expect(!store.load(missing).has_value(), "byte file store should reject missing files");
	Expect(!store.save(unwritable, { 1, 2, 3 }), "byte file store should reject saves when parent directory is missing");
	Expect(!std::filesystem::exists(unwritable.string() + ".tmp"), "byte file store should not leave temp files after failed open");

	std::filesystem::remove_all(root);
}

void TestTextFileStoreSavesLoadsAndCleansTempFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_text_file_store_test";
	const std::filesystem::path path = root / "lines.txt";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::TextFileStore store;
	std::vector<std::string> lines {
		"run frames=1 frameReports=1",
		"frame[0]",
		"inventoryResult[0] type=Applied",
	};

	Expect(store.saveLines(path, lines), "text file store should save text lines");
	std::optional<std::vector<std::string>> loaded = store.loadLines(path);

	Expect(loaded.has_value(), "text file store should load saved lines");
	Expect(loaded.has_value() && *loaded == lines, "text file store should preserve line payloads");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "text file store should remove temp file after save");

	std::filesystem::remove_all(root);
}

void TestTextFileStoreRejectsMissingAndUnwritablePaths()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_text_file_store_missing_test";
	const std::filesystem::path missing = root / "missing.txt";
	const std::filesystem::path unwritable = root / "missing_directory" / "lines.txt";
	std::filesystem::remove_all(root);

	dev::TextFileStore store;

	Expect(!store.loadLines(missing).has_value(), "text file store should reject missing files");
	Expect(!store.saveLines(unwritable, { "line" }), "text file store should reject saves when parent directory is missing");
	Expect(!std::filesystem::exists(unwritable.string() + ".tmp"), "text file store should not leave temp files after failed open");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestByteFileStoreSavesLoadsAndCleansTempFile();
	TestByteFileStoreRejectsMissingAndUnwritablePaths();
	TestTextFileStoreSavesLoadsAndCleansTempFile();
	TestTextFileStoreRejectsMissingAndUnwritablePaths();
	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "file_store_tests passed\n";
	return EXIT_SUCCESS;
}
