#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace iggy3d {

enum class ProductRendererRequest : std::uint8_t {
  Null,
  Vulkan,
};

enum class ProductWindowMode : std::uint8_t {
  NoWindow,
  Window,
};

enum class ProductInputBackend : std::uint8_t {
  Keyboard,
  Gamepad,
  Auto,
};

enum class ProductAppOptionStatus : std::uint8_t {
  Ok,
  Help,
  MissingOptionValue,
  InvalidRenderer,
  InvalidInputBackend,
  UnknownOption,
};

struct ProductAppOptions {
  ProductRendererRequest renderer = ProductRendererRequest::Null;
  ProductWindowMode windowMode = ProductWindowMode::Window;
  ProductInputBackend inputBackend = ProductInputBackend::Auto;
  std::filesystem::path saveRoot;
  std::filesystem::path devPackageOverride;
  std::filesystem::path automationControlPath;
  std::string devScenario = "default";
  std::uint32_t frames = 0;
  std::uint32_t holdSeconds = 0;
  bool autoNewWorld = false;
  bool scriptedGameplaySmoke = false;
  bool printRenderReceipt = false;
  bool help = false;
};

struct ProductAppOptionsParseResult {
  ProductAppOptions options;
  ProductAppOptionStatus status = ProductAppOptionStatus::Ok;
  std::string option;
};

ProductAppOptions defaultProductAppOptions();
ProductAppOptionsParseResult parseProductAppOptions(int argc, char** argv);

std::string_view productRendererRequestName(ProductRendererRequest renderer);
std::string_view productWindowModeName(ProductWindowMode mode);
std::string_view productInputBackendName(ProductInputBackend backend);
std::string_view productAppOptionStatusReason(ProductAppOptionStatus status);
std::string productAppHelpText();

}  // namespace iggy3d
