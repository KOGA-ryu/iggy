#include "app/iggy3d/ProductAppOptions.hpp"

#include <charconv>
#include <cstdlib>
#include <string_view>

namespace iggy3d {

namespace {

bool needsValue(int index, int argc) {
  return index + 1 >= argc;
}

bool parseU32(std::string_view text, std::uint32_t& out) {
  const char* begin = text.data();
  const char* end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, out);
  return ec == std::errc{} && ptr == end;
}

std::filesystem::path defaultSaveRoot() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path{home} / ".iggy3d" / "saves";
  }
  return ".iggy3d/saves";
}

}  // namespace

ProductAppOptions defaultProductAppOptions() {
  ProductAppOptions options;
  options.saveRoot = defaultSaveRoot();
  options.windowMode = ProductWindowMode::Window;
  return options;
}

ProductAppOptionsParseResult parseProductAppOptions(int argc, char** argv) {
  ProductAppOptionsParseResult result;
  result.options = defaultProductAppOptions();

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--help" || arg == "-h") {
      result.options.help = true;
      result.status = ProductAppOptionStatus::Help;
      return result;
    }
    if (arg == "--renderer") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      const std::string_view value{argv[++i]};
      if (value == "null") {
        result.options.renderer = ProductRendererRequest::Null;
      } else if (value == "vulkan") {
        result.options.renderer = ProductRendererRequest::Vulkan;
      } else {
        result.status = ProductAppOptionStatus::InvalidRenderer;
        result.option = std::string(value);
        return result;
      }
      continue;
    }
    if (arg == "--window") {
      result.options.windowMode = ProductWindowMode::Window;
      continue;
    }
    if (arg == "--no-window") {
      result.options.windowMode = ProductWindowMode::NoWindow;
      continue;
    }
    if (arg == "--input") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      const std::string_view value{argv[++i]};
      if (value == "keyboard") {
        result.options.inputBackend = ProductInputBackend::Keyboard;
      } else if (value == "gamepad") {
        result.options.inputBackend = ProductInputBackend::Gamepad;
      } else if (value == "auto") {
        result.options.inputBackend = ProductInputBackend::Auto;
      } else {
        result.status = ProductAppOptionStatus::InvalidInputBackend;
        result.option = std::string(value);
        return result;
      }
      continue;
    }
    if (arg == "--save-root") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      result.options.saveRoot = argv[++i];
      continue;
    }
    if (arg == "--print-render-receipt") {
      result.options.printRenderReceipt = true;
      continue;
    }
    if (arg == "--auto-new-world") {
      result.options.autoNewWorld = true;
      continue;
    }
    if (arg == "--scripted-gameplay-smoke") {
      result.options.scriptedGameplaySmoke = true;
      result.options.autoNewWorld = true;
      result.options.inputBackend = ProductInputBackend::Auto;
      continue;
    }
    if (arg == "--frames") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      const std::string_view value{argv[++i]};
      if (!parseU32(value, result.options.frames)) {
        result.status = ProductAppOptionStatus::UnknownOption;
        result.option = std::string(value);
        return result;
      }
      continue;
    }
    if (arg == "--hold-seconds") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      const std::string_view value{argv[++i]};
      if (!parseU32(value, result.options.holdSeconds)) {
        result.status = ProductAppOptionStatus::UnknownOption;
        result.option = std::string(value);
        return result;
      }
      continue;
    }
    if (arg == "--dev-package-override" || arg == "--package") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      result.options.devPackageOverride = argv[++i];
      continue;
    }
    if (arg == "--dev-scenario") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      result.options.devScenario = argv[++i];
      continue;
    }
    if (arg == "--automation-control" || arg == "--script-control") {
      if (needsValue(i, argc)) {
        result.status = ProductAppOptionStatus::MissingOptionValue;
        result.option = std::string(arg);
        return result;
      }
      result.options.automationControlPath = argv[++i];
      continue;
    }

    result.status = ProductAppOptionStatus::UnknownOption;
    result.option = std::string(arg);
    return result;
  }

  return result;
}

std::string_view productRendererRequestName(ProductRendererRequest renderer) {
  switch (renderer) {
    case ProductRendererRequest::Null:
      return "null";
    case ProductRendererRequest::Vulkan:
      return "vulkan";
  }
  return "null";
}

std::string_view productWindowModeName(ProductWindowMode mode) {
  switch (mode) {
    case ProductWindowMode::NoWindow:
      return "no_window";
    case ProductWindowMode::Window:
      return "window";
  }
  return "no_window";
}

std::string_view productInputBackendName(ProductInputBackend backend) {
  switch (backend) {
    case ProductInputBackend::Keyboard:
      return "keyboard";
    case ProductInputBackend::Gamepad:
      return "gamepad";
    case ProductInputBackend::Auto:
      return "auto";
  }
  return "auto";
}

std::string_view productAppOptionStatusReason(ProductAppOptionStatus status) {
  switch (status) {
    case ProductAppOptionStatus::Ok:
      return "product_app.ok";
    case ProductAppOptionStatus::Help:
      return "product_app.help";
    case ProductAppOptionStatus::MissingOptionValue:
      return "product_app.missing_option_value";
    case ProductAppOptionStatus::InvalidRenderer:
      return "product_app.invalid_renderer";
    case ProductAppOptionStatus::InvalidInputBackend:
      return "product_app.invalid_input_backend";
    case ProductAppOptionStatus::UnknownOption:
      return "product_app.unknown_option";
  }
  return "product_app.internal_error";
}

std::string productAppHelpText() {
  return "iggy3d\n"
         "  --renderer null|vulkan\n"
         "  --window | --no-window\n"
         "  --input keyboard|gamepad|auto\n"
         "  --save-root <path>\n"
         "  --frames <count>\n"
         "  --hold-seconds <count>\n"
         "  --auto-new-world\n"
         "  --scripted-gameplay-smoke\n"
         "  --print-render-receipt\n"
         "  --dev-package-override <package.iggy3d.toml>\n"
         "  --dev-scenario <id>\n"
         "  --automation-control <path>\n";
}

}  // namespace iggy3d
