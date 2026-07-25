#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace fs = std::filesystem;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::string readFile(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

std::vector<fs::path> creativeEditorProductionFiles() {
  std::vector<fs::path> files;
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator("apps/iggy3d_creative")) {
    const fs::path extension = entry.path().extension();
    if (entry.is_regular_file() &&
        (extension == ".cpp" || extension == ".hpp")) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

bool coreTransactionsStayBehindEditorEdits() {
  constexpr std::string_view kOwner =
      "apps/iggy3d_creative/EditorEditHistory.cpp";
  constexpr std::array kCoreTokens{
      std::string_view{"beginCreativeHistoryTransaction("},
      std::string_view{"setCreativeHistoryTransactionOperation("},
      std::string_view{"cancelCreativeHistoryTransaction("},
      std::string_view{"commitCreativeHistoryTransaction("},
  };

  const std::vector<fs::path> files = creativeEditorProductionFiles();
  bool clean = expect(!files.empty(),
                      "creative editor production inventory is nonempty");
  std::array<bool, kCoreTokens.size()> ownerContains{};
  for (const fs::path& path : files) {
    const std::string source = readFile(path);
    const std::string relative = path.lexically_normal().generic_string();
    for (std::size_t index = 0; index < kCoreTokens.size(); ++index) {
      if (source.find(kCoreTokens[index]) == std::string::npos) {
        continue;
      }
      if (relative != kOwner) {
        std::cerr << "FAIL: core history token " << kCoreTokens[index]
                  << " escaped EditorEdits into " << relative << '\n';
        clean = false;
      } else {
        ownerContains[index] = true;
      }
    }
  }

  for (std::size_t index = 0; index < kCoreTokens.size(); ++index) {
    if (!ownerContains[index]) {
        std::cerr << "FAIL: EditorEditHistory no longer owns core history token "
                << kCoreTokens[index]
                << "; remove or update the stale boundary rule\n";
      clean = false;
    }
  }
  return expect(clean,
                "creative editor transactions use the EditorEdits boundary");
}

}  // namespace

int main() {
  return coreTransactionsStayBehindEditorEdits() ? EXIT_SUCCESS
                                                 : EXIT_FAILURE;
}
