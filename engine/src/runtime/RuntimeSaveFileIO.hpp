#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace iggy::runtime {

enum class RuntimeSaveFileIOStatus {
	Ok,
	InvalidPath,
	OpenFailed,
	WriteFailed,
	ReadFailed,
	ReplaceFailed,
};

struct RuntimeSaveFileWriteResult {
	RuntimeSaveFileIOStatus status = RuntimeSaveFileIOStatus::Ok;
	std::filesystem::path path;
	std::filesystem::path tempPath;
	std::size_t bytesWritten = 0;
};

struct RuntimeSaveFileReadResult {
	RuntimeSaveFileIOStatus status = RuntimeSaveFileIOStatus::Ok;
	std::filesystem::path path;
	std::vector<std::uint8_t> bytes;
};

class RuntimeSaveFileWriter {
public:
	[[nodiscard]] RuntimeSaveFileWriteResult writeBytes(
		const std::filesystem::path &path,
		const std::vector<std::uint8_t> &bytes) const;
};

class RuntimeSaveFileReader {
public:
	[[nodiscard]] RuntimeSaveFileReadResult readBytes(const std::filesystem::path &path) const;
};

} // namespace iggy::runtime
