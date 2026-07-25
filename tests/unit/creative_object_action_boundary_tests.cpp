#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
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

struct SourceOwnershipRule {
  std::string_view token;
  std::span<const std::string_view> owners;
};

bool sourceIsOwner(std::string_view source,
                   const SourceOwnershipRule& rule) {
  return std::find(rule.owners.begin(), rule.owners.end(), source) !=
         rule.owners.end();
}

bool tokenHasOnlyNamedOwners(
    const SourceOwnershipRule& rule,
    std::span<const fs::path> productionSources) {
  bool clean = true;
  std::vector<std::string> foundOwners;
  for (const fs::path& path : productionSources) {
    const std::string source = readFile(path);
    if (source.find(rule.token) == std::string::npos) {
      continue;
    }
    const std::string relative = path.lexically_normal().generic_string();
    if (!sourceIsOwner(relative, rule)) {
      std::cerr << "FAIL: semantic object-action token " << rule.token
                << " escaped its owners into " << relative << '\n';
      clean = false;
      continue;
    }
    foundOwners.push_back(relative);
  }

  for (std::string_view owner : rule.owners) {
    if (std::find(foundOwners.begin(), foundOwners.end(), owner) ==
        foundOwners.end()) {
      std::cerr << "FAIL: semantic object-action owner " << owner
                << " no longer contains " << rule.token
                << "; remove or correct the stale allowlist row\n";
      clean = false;
    }
  }
  return clean;
}

std::vector<fs::path> creativeProductionSources() {
  constexpr std::array roots{
      std::string_view{"apps/iggy3d_creative"},
      std::string_view{"src/app/iggy3d/creative"},
  };
  std::vector<fs::path> sources;
  for (std::string_view root : roots) {
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(root)) {
      if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
        sources.push_back(entry.path());
      }
    }
  }
  std::sort(sources.begin(), sources.end());
  return sources;
}

bool semanticObjectActionCallsStayInsideNamedBoundaries() {
  static constexpr std::array kRawPolicyOwners{
      std::string_view{
          "src/app/iggy3d/creative/tools/SelectionResolution.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorTransformSource.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorWorldLayoutSource.cpp"},
  };
  static constexpr std::array kPolicyTypeOwners{
      std::string_view{
          "src/app/iggy3d/creative/tools/SelectionResolution.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorTransformSource.cpp"},
  };
  static constexpr std::array kFactsOwners{
      std::string_view{
          "src/app/iggy3d/creative/tools/SelectionResolution.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorObjectActions.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorEditsInternal.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorDesktopOutliner.cpp"},
  };
  static constexpr std::array kAdmissionOwners{
      std::string_view{
          "src/app/iggy3d/creative/tools/SelectionResolution.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorEditsInternal.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorAssetScatter.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorAuthoredAssets.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorMaterialStroke.cpp"},
  };
  static constexpr std::array kAdmissionSetOwners{
      std::string_view{
          "src/app/iggy3d/creative/tools/SelectionResolution.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorObjectActions.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorDesktopOutliner.cpp"},
  };
  static constexpr std::array kAdmissionReasonOwners{
      std::string_view{
          "src/app/iggy3d/creative/tools/SelectionResolution.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorObjectActionOutcome.cpp"},
  };
  static constexpr std::array kOutcomeUseOwners{
      std::string_view{
          "apps/iggy3d_creative/EditorObjectActionOutcome.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorObjectActionExecutor.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorDesktopObjectCommands.cpp"},
  };
  static constexpr std::array kSceneObjectKernelOwners{
      std::string_view{
          "apps/iggy3d_creative/EditorEditObjectActions.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorObjectActionExecutor.cpp"},
  };
  static constexpr std::array kTransformKernelOwners{
      std::string_view{
          "apps/iggy3d_creative/EditorEditObjectProperties.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorObjectActionExecutor.cpp"},
  };
  static constexpr std::array kOutcomePresentationOwner{
      std::string_view{
          "apps/iggy3d_creative/EditorObjectActionOutcome.cpp"},
  };
  const std::array rules{
      SourceOwnershipRule{"resolveCreativeSemanticObjectAction(",
                          kRawPolicyOwners},
      SourceOwnershipRule{"CreativeSemanticObjectActionPolicy",
                          kPolicyTypeOwners},
      SourceOwnershipRule{"resolveCreativeSemanticObjectActionFacts(",
                          kFactsOwners},
      SourceOwnershipRule{"resolveCreativeSemanticObjectActionAdmission(",
                          kAdmissionOwners},
      SourceOwnershipRule{"resolveCreativeSemanticObjectActionAdmissions(",
                          kAdmissionSetOwners},
      SourceOwnershipRule{
          "creative_editor_object_action_world_layout_unsynchronized",
          kAdmissionReasonOwners},
      SourceOwnershipRule{
          "creative_editor_object_action_selection_locked",
          kAdmissionReasonOwners},
      SourceOwnershipRule{"formatCreativeEditorObjectActionOutcome(",
                          kOutcomeUseOwners},
      SourceOwnershipRule{"deleteCreativeEditorSelectionWithUndo(",
                          kSceneObjectKernelOwners},
      SourceOwnershipRule{"duplicateCreativeEditorSelectionWithUndo(",
                          kSceneObjectKernelOwners},
      SourceOwnershipRule{"transformCreativeEditorSelectionWithUndo(",
                          kSceneObjectKernelOwners},
      SourceOwnershipRule{"setCreativeEditorObjectTransformWithUndo(",
                          kTransformKernelOwners},
      SourceOwnershipRule{"\"duplicated selection\"",
                          kOutcomePresentationOwner},
      SourceOwnershipRule{"\"nothing to duplicate\"",
                          kOutcomePresentationOwner},
      SourceOwnershipRule{"\"deleted selection\"",
                          kOutcomePresentationOwner},
      SourceOwnershipRule{"\"deleted objects\"", kOutcomePresentationOwner},
      SourceOwnershipRule{"\"renamed object\"", kOutcomePresentationOwner},
      SourceOwnershipRule{"\"visibility updated\"",
                          kOutcomePresentationOwner},
      SourceOwnershipRule{"\"lock updated\"", kOutcomePresentationOwner},
      SourceOwnershipRule{"\"transform set\"",
                          kOutcomePresentationOwner},
      SourceOwnershipRule{"\"transform unchanged\"",
                          kOutcomePresentationOwner},
      SourceOwnershipRule{"\"transform set; adopt 3D edit\"",
                          kOutcomePresentationOwner},
  };
  const std::vector<fs::path> sources = creativeProductionSources();
  bool clean = expect(!sources.empty(),
                      "creative production source inventory is nonempty");
  for (const SourceOwnershipRule& rule : rules) {
    clean = tokenHasOnlyNamedOwners(rule, sources) && clean;
  }
  return expect(
             clean,
             "semantic object-action policy and admission stay in named owners");
}

}  // namespace

int main() {
  return semanticObjectActionCallsStayInsideNamedBoundaries()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
