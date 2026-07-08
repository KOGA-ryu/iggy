#include "app/iggy3d/world/ProductWorldTemplateOperations.hpp"

#include "app/PackageRuntimeLookup.hpp"
#include "app/iggy3d/Options.hpp"
#include "content/PackageLoader.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

std::filesystem::path productPackagePathFromOptions(
    const ProductAppOptions& options) {
  if (!options.devPackageOverride.empty()) {
    return options.devPackageOverride;
  }

  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeProduct;
  lookupConfig.requireGraphicsRuntime = false;
  lookupConfig.requireShaderRoot = false;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);
  if (lookup.outcome == RenderOutcome::Ok && !lookup.lookup.resourceRoot.empty()) {
    return lookup.lookup.resourceRoot / "demos" / "first_room" /
           "package.iggy3d.toml";
  }

  return std::filesystem::path{"fixtures"} / "demos" / "first_room" /
         "package.iggy3d.toml";
}

ProductWorldTemplate productWorldTemplateFromOptions(
    const ProductAppOptions& options) {
  ProductWorldTemplate world =
      options.devPackageOverride.empty()
          ? defaultProductWorldTemplate()
          : devOverrideProductWorldTemplate(
                options.devPackageOverride.generic_string(), options.devScenario);
  const PackageLoadResult package =
      loadPackage({productPackagePathFromOptions(options).generic_string()});
  if (package.status == PackageLoadStatus::Ok) {
    world.packageId = package.manifest.packageId;
    world.scenarioId = package.scenario.scenarioId;
  }
  return world;
}

}  // namespace iggy3d
