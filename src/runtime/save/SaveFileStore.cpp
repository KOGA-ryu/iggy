#include "runtime/save/SaveFileStore.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string_view>

namespace iggy3d {
namespace {

constexpr std::string_view kSaveFileExtension = ".iggy3d.save";

bool hasSaveFileExtension(const std::filesystem::path& path) {
  const std::string filename = path.filename().string();
  return filename.size() > kSaveFileExtension.size() &&
         filename.ends_with(kSaveFileExtension);
}

std::string idFromPath(const std::filesystem::path& path) {
  std::string filename = path.filename().string();
  if (filename.size() > kSaveFileExtension.size() &&
      filename.ends_with(kSaveFileExtension)) {
    filename.erase(filename.size() - kSaveFileExtension.size());
  }
  return filename.empty() ? "save" : filename;
}

bool validSaveId(std::string_view id) {
  if (id.empty()) {
    return false;
  }
  for (const char c : id) {
    const bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!valid) {
      return false;
    }
  }
  return true;
}

std::string makeSaveId(std::size_t index) {
  std::ostringstream output;
  output << "save_";
  output.width(3);
  output.fill('0');
  output << index;
  return output.str();
}

std::filesystem::path savePathFor(const std::filesystem::path& root, std::string_view id) {
  return root / (std::string(id) + std::string(kSaveFileExtension));
}

SaveFileRecord recordFromEnvelope(const std::filesystem::path& path,
                                  const SaveEnvelope& envelope) {
  SaveFileRecord record;
  record.id = idFromPath(path);
  record.path = path;
  record.packageId = envelope.metadata.packageId;
  record.scenarioId = envelope.metadata.scenarioId;
  record.currentTick = envelope.session.currentTick;
  record.nextCommandId = envelope.session.nextCommandId;
  record.savedStateHash = envelope.metadata.savedStateHash;
  record.savedStateHashHex = envelope.metadata.savedStateHashHex;
  return record;
}

std::string readWholeFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

}  // namespace

std::vector<SaveFileRecord> listSaveFiles(const std::filesystem::path& root) {
  std::vector<SaveFileRecord> records;
  std::error_code error;
  if (!std::filesystem::exists(root, error) || !std::filesystem::is_directory(root, error)) {
    return records;
  }
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(root, error)) {
    if (error || !entry.is_regular_file(error) || !hasSaveFileExtension(entry.path())) {
      continue;
    }
    const SaveFileReadResult read = readSaveFile(entry.path());
    if (read.ok) {
      records.push_back(read.record);
    }
  }
  std::sort(records.begin(), records.end(), [](const SaveFileRecord& lhs,
                                               const SaveFileRecord& rhs) {
    return lhs.id < rhs.id;
  });
  return records;
}

SaveFileWriteResult writeSessionSaveFile(const SaveFileWriteRequest& request) {
  SaveFileWriteResult result;
  if (request.state == nullptr) {
    result.reason = "save_state_missing";
    return result;
  }

  SaveStateResult saved = saveSessionState(*request.state);
  result.saveStatus = saved.status;
  if (saved.status != SaveLoadStatus::Ok) {
    result.reason = "save_encode_failed";
    return result;
  }
  if (request.authoredRoom != nullptr) {
    saved.envelope.authoredRoom = *request.authoredRoom;
  }
  const SaveEncodeResult encoded = encodeSaveEnvelope(saved.envelope);
  saved.codecStatus = encoded.status;
  result.codecStatus = encoded.status;
  if (encoded.status != SaveCodecStatus::Ok) {
    result.saveStatus = SaveLoadStatus::EncodeFailed;
    result.reason = "save_encode_failed";
    return result;
  }
  saved.encodedSaveText = encoded.encodedText;
  saved.savedStateHash = encoded.savedStateHash;

  std::error_code error;
  std::filesystem::create_directories(request.root, error);
  if (error) {
    result.reason = "save_root_create_failed";
    return result;
  }

  std::string id = validSaveId(request.idHint)
                       ? request.idHint
                       : makeSaveId(listSaveFiles(request.root).size() + 1U);
  std::filesystem::path path = savePathFor(request.root, id);
  if (!validSaveId(request.idHint)) {
    std::size_t index = listSaveFiles(request.root).size() + 1U;
    while (std::filesystem::exists(path, error)) {
      ++index;
      id = makeSaveId(index);
      path = savePathFor(request.root, id);
    }
  }

  std::ofstream output(path);
  if (!output) {
    result.reason = "save_file_write_failed";
    return result;
  }
  output << saved.encodedSaveText;
  if (!output) {
    result.reason = "save_file_write_failed";
    return result;
  }

  result.ok = true;
  result.reason = "save_file_written";
  result.record = recordFromEnvelope(path, saved.envelope);
  result.encodedBytes = saved.encodedSaveText.size();
  return result;
}

SaveFileReadResult readSaveFile(const std::filesystem::path& path) {
  SaveFileReadResult result;
  result.encodedText = readWholeFile(path);
  if (result.encodedText.empty()) {
    result.reason = "save_file_read_failed";
    return result;
  }
  const SaveDecodeResult decoded = decodeSaveEnvelope(result.encodedText);
  result.codecStatus = decoded.status;
  if (decoded.status != SaveCodecStatus::Ok) {
    result.reason = "save_file_decode_failed";
    return result;
  }
  result.ok = true;
  result.reason = "save_file_read";
  result.record = recordFromEnvelope(path, decoded.envelope);
  return result;
}

bool deleteSaveFile(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::remove(path, error) && !error;
}

std::filesystem::path defaultSaveFileRoot() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / ".iggy3d" / "saves";
  }
  return std::filesystem::path(".iggy3d") / "saves";
}

}  // namespace iggy3d
