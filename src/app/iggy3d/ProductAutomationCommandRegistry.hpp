#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {

enum class ProductAutomationCommandCategory : std::uint8_t {
  Owner,
  MenuInput,
  MenuShortcut,
  FrontendSelect,
  FrontendExecute,
  WorldSetup,
  DungeonDraft,
  AsciiRoom,
  SaveBrowser,
  DeletedSaveBrowser,
  RoomEdit,
  RoomEditor,
  GameplayInput,
  GameplayTape,
  Settings,
  DevTools,
  System,
  Unknown,
};

enum class ProductAutomationValueKind : std::uint8_t {
  Bool,
  String,
  Float,
  Size,
  Csv,
  Action,
  Direction,
  Tool,
  Raw,
  Unknown,
};

struct ProductAutomationCommandSpec {
  std::string canonicalKey;
  std::vector<std::string> aliases;
  ProductAutomationCommandCategory category =
      ProductAutomationCommandCategory::Unknown;
  ProductAutomationValueKind valueKind = ProductAutomationValueKind::Unknown;
  bool requiresValue = true;
  bool ignoresFalseBool = false;
  std::string ownerHint;
};

struct ProductAutomationCommandRegistry {
  std::vector<ProductAutomationCommandSpec> specs;
};

std::string_view productAutomationCommandCategoryName(
    ProductAutomationCommandCategory category);

std::string_view productAutomationValueKindName(
    ProductAutomationValueKind valueKind);

ProductAutomationCommandRegistry makeProductAutomationCommandRegistry();

const ProductAutomationCommandSpec& findProductAutomationCommandSpec(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key);

std::string_view productAutomationCanonicalKey(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key);

}  // namespace iggy3d
