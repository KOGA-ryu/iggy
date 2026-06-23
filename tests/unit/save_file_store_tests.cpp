#include "content/PackageLoader.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_save_file_store_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

iggy3d::Session makeFixtureSession() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({"fixtures/demos/movement_playground/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest request;
  request.packageId = package.manifest.packageId;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  return iggy3d::Session::create(request).value;
}

bool writeListReadAndDeleteRoundTrips() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveFileWriteRequest request;
  request.root = root;
  request.state = &session.state();
  const iggy3d::SaveFileWriteResult written = iggy3d::writeSessionSaveFile(request);
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path) : iggy3d::SaveFileReadResult{};
  const bool fileExists = written.ok && std::filesystem::exists(written.record.path);
  const bool deleted =
      written.ok && !listed.empty() && iggy3d::deleteSaveFile(listed.front().path);
  const std::vector<iggy3d::SaveFileRecord> afterDelete = iggy3d::listSaveFiles(root);
  return expect(written.ok, "save file written") &&
         expect(written.reason == "save_file_written", "write reason") &&
         expect(written.record.id == "save_001", "save id") &&
         expect(written.record.packageId == "iggy3d.movement_playground", "package id") &&
         expect(written.record.scenarioId == "movement_playground.runtime_loop",
                "scenario id") &&
         expect(written.encodedBytes > 0U, "encoded bytes") &&
         expect(fileExists, "save file exists") &&
         expect(listed.size() == 1U, "one save listed") &&
         expect(listed.front().savedStateHash == session.stateHash(), "listed hash") &&
         expect(read.ok, "save file read") &&
         expect(read.encodedText.starts_with("iggy3d.save_envelope.v1\n"), "save header") &&
         expect(read.record.savedStateHash == session.stateHash(), "read hash") &&
         expect(deleted, "save file deleted") &&
         expect(afterDelete.empty(), "saves empty after delete");
}

bool missingStateIsRejected() {
  iggy3d::SaveFileWriteRequest request;
  request.root = testRoot();
  const iggy3d::SaveFileWriteResult result = iggy3d::writeSessionSaveFile(request);
  return expect(!result.ok, "missing state rejected") &&
         expect(result.reason == "save_state_missing", "missing state reason");
}

bool idHintOverwritesExistingSave() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveFileWriteRequest first;
  first.root = root;
  first.state = &session.state();
  const iggy3d::SaveFileWriteResult firstWrite = iggy3d::writeSessionSaveFile(first);
  iggy3d::SaveFileWriteRequest second = first;
  second.idHint = "save_001";
  const iggy3d::SaveFileWriteResult secondWrite = iggy3d::writeSessionSaveFile(second);
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  return expect(firstWrite.ok, "first write") && expect(secondWrite.ok, "second write") &&
         expect(secondWrite.record.id == "save_001", "overwrite id") &&
         expect(listed.size() == 1U, "overwrite keeps one save");
}

}  // namespace

int main() {
  const bool ok = writeListReadAndDeleteRoundTrips() && missingStateIsRejected() &&
                  idHintOverwritesExistingSave();
  return ok ? 0 : 1;
}
