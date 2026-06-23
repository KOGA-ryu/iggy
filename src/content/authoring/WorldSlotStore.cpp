#include "content/authoring/WorldSlotStore.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace iggy3d {
namespace {

constexpr std::string_view kWorldSlotExtension = ".iggy3d.world.toml";

std::string trim(std::string_view value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) {
    return {};
  }
  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1U));
}

bool stripQuotedValue(std::string value, std::string& out) {
  value = trim(value);
  if (value.size() < 2U || value.front() != '"' || value.back() != '"') {
    return false;
  }
  out = value.substr(1U, value.size() - 2U);
  return true;
}

bool parseWorldSlotFile(const std::filesystem::path& path, WorldSlotRecord& out) {
  std::ifstream input(path);
  if (!input) {
    return false;
  }

  WorldSlotRecord record;
  record.path = path;
  std::string line;
  while (std::getline(input, line)) {
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      continue;
    }
    const std::string key = trim(std::string_view(line).substr(0U, equals));
    std::string value;
    if (!stripQuotedValue(line.substr(equals + 1U), value)) {
      continue;
    }
    if (key == "id") {
      record.id = value;
    } else if (key == "name") {
      record.name = value;
    } else if (key == "package") {
      record.packagePath = value;
    }
  }

  if (record.id.empty()) {
    record.id = path.stem().string();
  }
  if (record.name.empty()) {
    record.name = record.id;
  }
  if (record.packagePath.empty()) {
    return false;
  }
  out = std::move(record);
  return true;
}

bool hasWorldSlotExtension(const std::filesystem::path& path) {
  const std::string filename = path.filename().string();
  return filename.size() > kWorldSlotExtension.size() &&
         filename.ends_with(kWorldSlotExtension);
}

std::string makeSlotId(std::size_t index) {
  std::ostringstream output;
  output << "world_" << std::setw(3) << std::setfill('0') << index;
  return output.str();
}

std::filesystem::path slotPathFor(const std::filesystem::path& root, std::string_view id) {
  return root / (std::string(id) + std::string(kWorldSlotExtension));
}

}  // namespace

std::vector<WorldSlotRecord> listWorldSlots(const std::filesystem::path& root) {
  std::vector<WorldSlotRecord> slots;
  std::error_code error;
  if (!std::filesystem::exists(root, error) || !std::filesystem::is_directory(root, error)) {
    return slots;
  }
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(root, error)) {
    if (error || !entry.is_regular_file(error) || !hasWorldSlotExtension(entry.path())) {
      continue;
    }
    WorldSlotRecord slot;
    if (parseWorldSlotFile(entry.path(), slot)) {
      slots.push_back(std::move(slot));
    }
  }
  std::sort(slots.begin(), slots.end(), [](const WorldSlotRecord& lhs,
                                           const WorldSlotRecord& rhs) {
    return lhs.id < rhs.id;
  });
  return slots;
}

WorldSlotCreateResult createWorldSlot(const WorldSlotCreateRequest& request) {
  WorldSlotCreateResult result;
  if (request.packagePath.empty()) {
    result.reason = "world_package_missing";
    return result;
  }

  std::error_code error;
  std::filesystem::create_directories(request.root, error);
  if (error) {
    result.reason = "world_root_create_failed";
    return result;
  }

  std::size_t index = listWorldSlots(request.root).size() + 1U;
  std::string id = makeSlotId(index);
  std::filesystem::path path = slotPathFor(request.root, id);
  while (std::filesystem::exists(path, error)) {
    ++index;
    id = makeSlotId(index);
    path = slotPathFor(request.root, id);
  }

  std::ofstream output(path);
  if (!output) {
    result.reason = "world_slot_write_failed";
    return result;
  }
  output << "[world]\n";
  output << "id = \"" << id << "\"\n";
  output << "name = \"" << request.name << "\"\n";
  output << "package = \"" << request.packagePath.string() << "\"\n";
  output << "format = \"iggy3d.world.v1\"\n";
  output << "state = \"authoring_stub\"\n";
  if (!output) {
    result.reason = "world_slot_write_failed";
    return result;
  }

  result.ok = true;
  result.reason = "world_slot_created";
  result.slot.id = id;
  result.slot.name = request.name;
  result.slot.path = path;
  result.slot.packagePath = request.packagePath;
  return result;
}

bool deleteWorldSlotFile(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::remove(path, error) && !error;
}

std::filesystem::path defaultWorldSlotRoot() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / ".iggy3d" / "worlds";
  }
  return std::filesystem::path(".iggy3d") / "worlds";
}

}  // namespace iggy3d
