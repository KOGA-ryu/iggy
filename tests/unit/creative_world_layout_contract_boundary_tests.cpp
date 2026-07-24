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

std::vector<fs::path> productionFiles() {
  constexpr std::array roots{
      std::string_view{"apps/iggy3d_creative"},
      std::string_view{"apps/iggy3d_playtest"},
      std::string_view{"src/app/iggy3d/creative"},
  };
  std::vector<fs::path> files;
  for (std::string_view root : roots) {
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(root)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const std::string extension = entry.path().extension().string();
      if (extension == ".cpp" || extension == ".hpp") {
        files.push_back(entry.path());
      }
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

bool productionLeavesAvoidCompatibilityUmbrella() {
  constexpr std::string_view kUmbrellaInclude =
      "#include \"EditorWorldLayout.hpp\"";
  bool clean = true;
  for (const fs::path& path : productionFiles()) {
    if (path.filename() == "EditorWorldLayout.hpp") {
      continue;
    }
    if (readFile(path).find(kUmbrellaInclude) != std::string::npos) {
      std::cerr << "FAIL: production leaf includes World Layout umbrella: "
                << path.generic_string() << '\n';
      clean = false;
    }
  }
  return expect(clean, "production leaves use domain World Layout contracts");
}

bool domainContractsRemainAcyclic() {
  constexpr std::array kDomainHeaders{
      std::string_view{"EditorWorldLayoutLifecycle.hpp"},
      std::string_view{"EditorWorldLayoutSources.hpp"},
      std::string_view{"EditorWorldLayoutPlan.hpp"},
      std::string_view{"EditorWorldLayoutBuildings.hpp"},
      std::string_view{"EditorWorldLayoutOpenings.hpp"},
  };
  bool clean = true;
  for (std::string_view header : kDomainHeaders) {
    const fs::path path = fs::path{"apps/iggy3d_creative"} / header;
    const std::string source = readFile(path);
    if (source.find("#include \"EditorWorldLayoutContracts.hpp\"") ==
        std::string::npos) {
      std::cerr << "FAIL: domain contract does not include common contract: "
                << header << '\n';
      clean = false;
    }
    for (std::string_view peer : kDomainHeaders) {
      if (peer == header) {
        continue;
      }
      const std::string include =
          "#include \"" + std::string{peer} + "\"";
      if (source.find(include) != std::string::npos) {
        std::cerr << "FAIL: World Layout domain contract cycle edge "
                  << header << " -> " << peer << '\n';
        clean = false;
      }
    }
  }
  return expect(clean, "World Layout domain contracts form a shallow DAG");
}

bool stateHasOneCanonicalDefinition() {
  constexpr std::string_view kDefinition =
      "struct CreativeEditorWorldLayoutState {";
  std::vector<fs::path> owners;
  for (const fs::path& path : productionFiles()) {
    if (readFile(path).find(kDefinition) != std::string::npos) {
      owners.push_back(path);
    }
  }
  const fs::path expected =
      "apps/iggy3d_creative/EditorWorldLayoutState.hpp";
  const bool exact = owners.size() == 1U && owners.front() == expected;
  if (!exact) {
    for (const fs::path& owner : owners) {
      std::cerr << "FAIL: World Layout state definition found in "
                << owner.generic_string() << '\n';
    }
  }
  return expect(exact, "World Layout state has one canonical owner");
}

bool compatibilityHeaderIsIncludesOnly() {
  const std::string source =
      readFile("apps/iggy3d_creative/EditorWorldLayout.hpp");
  bool clean = source.find("namespace ") == std::string::npos &&
               source.find("struct ") == std::string::npos &&
               source.find("[[nodiscard]]") == std::string::npos;
  constexpr std::array requiredIncludes{
      std::string_view{"EditorWorldLayoutBuildings.hpp"},
      std::string_view{"EditorWorldLayoutLifecycle.hpp"},
      std::string_view{"EditorWorldLayoutOpenings.hpp"},
      std::string_view{"EditorWorldLayoutPlan.hpp"},
      std::string_view{"EditorWorldLayoutSources.hpp"},
  };
  for (std::string_view header : requiredIncludes) {
    const std::string include =
        "#include \"" + std::string{header} + "\"";
    clean = source.find(include) != std::string::npos && clean;
  }
  return expect(clean, "World Layout compatibility header is includes-only");
}

}  // namespace

int main() {
  const bool umbrella = productionLeavesAvoidCompatibilityUmbrella();
  const bool contracts = domainContractsRemainAcyclic();
  const bool state = stateHasOneCanonicalDefinition();
  const bool compatibility = compatibilityHeaderIsIncludesOnly();
  return umbrella && contracts && state && compatibility
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
