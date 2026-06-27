#include "app/iggy3d/ProductAppOptions.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAppOptionsParseResult parse(std::vector<std::string> args) {
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (std::string& arg : args) {
    argv.push_back(arg.data());
  }
  return iggy3d::parseProductAppOptions(static_cast<int>(argv.size()), argv.data());
}

bool defaultsPreferManualRendererWithoutDebugOverlay() {
  const iggy3d::ProductAppOptions options = iggy3d::defaultProductAppOptions();
  return expect(options.renderer == iggy3d::ProductRendererRequest::Vulkan,
                "default renderer vulkan") &&
         expect(options.windowMode == iggy3d::ProductWindowMode::Window,
                "default window mode") &&
         expect(!options.debugOverlay, "debug overlay default disabled");
}

bool explicitNullRendererRemainsDiagnosticFallback() {
  const iggy3d::ProductAppOptionsParseResult parsed =
      parse({"iggy3d", "--renderer", "null"});
  return expect(parsed.status == iggy3d::ProductAppOptionStatus::Ok,
                "parse null ok") &&
         expect(parsed.options.renderer == iggy3d::ProductRendererRequest::Null,
                "parse null renderer") &&
         expect(!parsed.options.debugOverlay, "parse null debug disabled");
}

bool debugOverlayRequiresExplicitOption() {
  const iggy3d::ProductAppOptionsParseResult parsed =
      parse({"iggy3d", "--debug-overlay"});
  return expect(parsed.status == iggy3d::ProductAppOptionStatus::Ok,
                "parse debug overlay ok") &&
         expect(parsed.options.renderer == iggy3d::ProductRendererRequest::Vulkan,
                "debug overlay keeps default renderer") &&
         expect(parsed.options.debugOverlay, "debug overlay enabled");
}

bool helpListsDebugOverlay() {
  return expect(iggy3d::productAppHelpText().find("--debug-overlay") !=
                    std::string::npos,
                "help lists debug overlay");
}

}  // namespace

int main() {
  const bool ok = defaultsPreferManualRendererWithoutDebugOverlay() &&
                  explicitNullRendererRemainsDiagnosticFallback() &&
                  debugOverlayRequiresExplicitOption() && helpListsDebugOverlay();
  if (!ok) {
    return 1;
  }
  std::cout << "product_app_options_tests=pass\n";
  return 0;
}
