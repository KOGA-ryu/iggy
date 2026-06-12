#include "runtime/RuntimeSaveFileIO.hpp"

#include <fstream>

namespace iggy::runtime {
namespace {

std::filesystem::path tempPathFor(const std::filesystem::path &path)
{
	std::filesystem::path tempPath = path;
	tempPath += ".tmp.iggy";
	return tempPath;
}

void removeIfPresent(const std::filesystem::path &path)
{
	std::error_code ignored;
	std::filesystem::remove(path, ignored);
}

} // namespace

RuntimeSaveFileWriteResult RuntimeSaveFileWriter::writeBytes(
	const std::filesystem::path &path,
	const std::vector<std::uint8_t> &bytes) const
{
	RuntimeSaveFileWriteResult result;
	result.path = path;
	if (path.empty()) {
		result.status = RuntimeSaveFileIOStatus::InvalidPath;
		return result;
	}

	result.tempPath = tempPathFor(path);
	removeIfPresent(result.tempPath);

	{
		std::ofstream stream(result.tempPath, std::ios::binary | std::ios::trunc);
		if (!stream.is_open()) {
			result.status = RuntimeSaveFileIOStatus::OpenFailed;
			removeIfPresent(result.tempPath);
			return result;
		}

		if (!bytes.empty())
			stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		stream.flush();
		if (!stream.good()) {
			result.status = RuntimeSaveFileIOStatus::WriteFailed;
			stream.close();
			removeIfPresent(result.tempPath);
			return result;
		}
	}

	std::error_code error;
	if (std::filesystem::exists(path, error)) {
		std::filesystem::remove(path, error);
		if (error) {
			result.status = RuntimeSaveFileIOStatus::ReplaceFailed;
			removeIfPresent(result.tempPath);
			return result;
		}
	}

	std::filesystem::rename(result.tempPath, path, error);
	if (error) {
		result.status = RuntimeSaveFileIOStatus::ReplaceFailed;
		removeIfPresent(result.tempPath);
		return result;
	}

	result.status = RuntimeSaveFileIOStatus::Ok;
	result.bytesWritten = bytes.size();
	return result;
}

RuntimeSaveFileReadResult RuntimeSaveFileReader::readBytes(const std::filesystem::path &path) const
{
	RuntimeSaveFileReadResult result;
	result.path = path;
	if (path.empty()) {
		result.status = RuntimeSaveFileIOStatus::InvalidPath;
		return result;
	}

	std::ifstream stream(path, std::ios::binary);
	if (!stream.is_open()) {
		result.status = RuntimeSaveFileIOStatus::OpenFailed;
		return result;
	}

	stream.seekg(0, std::ios::end);
	const std::streamoff size = stream.tellg();
	if (size < 0) {
		result.status = RuntimeSaveFileIOStatus::ReadFailed;
		return result;
	}
	stream.seekg(0, std::ios::beg);

	result.bytes.resize(static_cast<std::size_t>(size));
	if (!result.bytes.empty())
		stream.read(reinterpret_cast<char *>(result.bytes.data()), static_cast<std::streamsize>(result.bytes.size()));
	if (!stream.good() && !stream.eof()) {
		result.status = RuntimeSaveFileIOStatus::ReadFailed;
		result.bytes.clear();
		return result;
	}

	result.status = RuntimeSaveFileIOStatus::Ok;
	return result;
}

} // namespace iggy::runtime
