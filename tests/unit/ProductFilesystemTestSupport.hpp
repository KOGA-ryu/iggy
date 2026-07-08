#pragma once

#include "app/iggy3d/Options.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace iggy3d::test {

inline std::filesystem::path cleanProductTestRoot(std::string_view suite,
                                                  std::string_view name) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / std::string{suite} /
      std::string{name};
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

inline ProductAppOptions productTestOptions(std::string_view suite,
                                            std::string_view name) {
  ProductAppOptions options;
  options.saveRoot = cleanProductTestRoot(suite, name);
  return options;
}

inline ProductAppOptions productTestOptions(std::string_view suite,
                                            std::string_view name,
                                            ProductWindowMode windowMode) {
  ProductAppOptions options = productTestOptions(suite, name);
  options.windowMode = windowMode;
  return options;
}

}  // namespace iggy3d::test
