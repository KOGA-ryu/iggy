#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace iggy3d {

struct WorldSlotRecord {
  std::string id;
  std::string name;
  std::filesystem::path path;
  std::filesystem::path packagePath;
};

struct WorldSlotCreateRequest {
  std::filesystem::path root;
  std::string name = "New World";
  std::filesystem::path packagePath;
};

struct WorldSlotCreateResult {
  bool ok = false;
  std::string reason = "not_requested";
  WorldSlotRecord slot;
};

std::vector<WorldSlotRecord> listWorldSlots(const std::filesystem::path& root);
WorldSlotCreateResult createWorldSlot(const WorldSlotCreateRequest& request);
bool deleteWorldSlotFile(const std::filesystem::path& path);
std::filesystem::path defaultWorldSlotRoot();

}  // namespace iggy3d
