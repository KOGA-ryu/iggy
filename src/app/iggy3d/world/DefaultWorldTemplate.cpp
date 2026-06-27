#include "app/iggy3d/world/DefaultWorldTemplate.hpp"

namespace iggy3d {

ProductWorldTemplate defaultProductWorldTemplate() {
  return {};
}

ProductWorldTemplate devOverrideProductWorldTemplate(std::string_view packagePath,
                                                     std::string_view scenarioId) {
  ProductWorldTemplate world;
  world.packageId = packagePath.empty() ? "iggy3d.dev_override" : std::string(packagePath);
  world.scenarioId = scenarioId.empty() ? "default" : std::string(scenarioId);
  world.displayName = "Dev Override World";
  world.source = "dev_package_override";
  return world;
}

}  // namespace iggy3d
