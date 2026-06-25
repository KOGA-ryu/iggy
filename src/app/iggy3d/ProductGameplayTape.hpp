#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime/command/Command.hpp"

namespace iggy3d {

enum class ProductGameplayTapeAction : std::uint8_t {
  Move,
  Interact,
  Wait,
};

struct ProductGameplayTapeStep {
  ProductGameplayTapeAction action = ProductGameplayTapeAction::Wait;
  std::string targetStableName;
  bool expectRejection = false;
  CommandRejectionReason expectedRejection = CommandRejectionReason::None;
  std::uint64_t sourceLine = 0;
};

struct ProductGameplayTape {
  std::vector<ProductGameplayTapeStep> steps;
};

struct ProductGameplayTapeParseResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  ProductGameplayTape tape;
  std::uint64_t lineCount = 0;
  std::uint64_t failedLine = 0;
  std::string failedToken = "none";
};

std::string_view productGameplayTapeActionName(ProductGameplayTapeAction action);
std::string_view productGameplayTapeRejectionName(CommandRejectionReason reason);

ProductGameplayTapeParseResult parseProductGameplayTape(std::string_view text);
ProductGameplayTapeParseResult loadProductGameplayTapeFile(
    const std::filesystem::path& path);

}  // namespace iggy3d
