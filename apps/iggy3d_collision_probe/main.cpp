#include "content/PackageLoader.hpp"
#include "runtime/collision/CollisionQuery.hpp"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr float kFeetToMeters = 0.3048F;

enum class InputUnits {
  Meters,
  Feet,
};

struct ProbeConfig {
  std::filesystem::path packagePath = "fixtures/demos/first_room/package.iggy3d.toml";
  std::string query = "suite";
  iggy3d::CollisionQueryKind kind = iggy3d::CollisionQueryKind::Actor;
  InputUnits units = InputUnits::Meters;
  iggy3d::Vec3 point;
  iggy3d::Vec3 start;
  iggy3d::Vec3 end;
  iggy3d::Vec3 min;
  iggy3d::Vec3 max;
  float toleranceMeters = 0.001F;
  bool hasPoint = false;
  bool hasStart = false;
  bool hasEnd = false;
  bool hasMin = false;
  bool hasMax = false;
  bool help = false;
};

struct LoadedSurfaces {
  bool ok = false;
  std::string reason = "not_loaded";
  iggy3d::PackageLoadResult package;
  iggy3d::SpatialSurfaceSet surfaces;
};

std::string formatFloat(float value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6) << value;
  return out.str();
}

std::string formatVec3(iggy3d::Vec3 value) {
  return formatFloat(value.x) + "," + formatFloat(value.y) + "," + formatFloat(value.z);
}

std::string boolText(bool value) {
  return value ? "true" : "false";
}

bool parseFloat(std::string_view text, float& value) {
  std::string owned{text};
  char* end = nullptr;
  errno = 0;
  const float parsed = std::strtof(owned.c_str(), &end);
  if (errno != 0 || end == owned.c_str() || *end != '\0' || !std::isfinite(parsed)) {
    return false;
  }
  value = parsed;
  return true;
}

bool parseVec3(std::string_view text, InputUnits units, iggy3d::Vec3& value) {
  const std::size_t first = text.find(',');
  const std::size_t second = first == std::string_view::npos ? std::string_view::npos
                                                             : text.find(',', first + 1U);
  if (first == std::string_view::npos || second == std::string_view::npos ||
      text.find(',', second + 1U) != std::string_view::npos) {
    return false;
  }
  iggy3d::Vec3 parsed;
  if (!parseFloat(text.substr(0, first), parsed.x) ||
      !parseFloat(text.substr(first + 1U, second - first - 1U), parsed.y) ||
      !parseFloat(text.substr(second + 1U), parsed.z)) {
    return false;
  }
  if (units == InputUnits::Feet) {
    parsed = parsed * kFeetToMeters;
  }
  value = parsed;
  return true;
}

bool parseKind(std::string_view text, iggy3d::CollisionQueryKind& kind) {
  if (text == "actor") {
    kind = iggy3d::CollisionQueryKind::Actor;
  } else if (text == "projectile") {
    kind = iggy3d::CollisionQueryKind::Projectile;
  } else if (text == "walkable") {
    kind = iggy3d::CollisionQueryKind::Walkable;
  } else if (text == "opening") {
    kind = iggy3d::CollisionQueryKind::Opening;
  } else if (text == "all") {
    kind = iggy3d::CollisionQueryKind::All;
  } else {
    return false;
  }
  return true;
}

std::string_view kindName(iggy3d::CollisionQueryKind kind) {
  switch (kind) {
    case iggy3d::CollisionQueryKind::All:
      return "all";
    case iggy3d::CollisionQueryKind::Actor:
      return "actor";
    case iggy3d::CollisionQueryKind::Projectile:
      return "projectile";
    case iggy3d::CollisionQueryKind::Walkable:
      return "walkable";
    case iggy3d::CollisionQueryKind::Opening:
      return "opening";
  }
  return "actor";
}

std::string_view unitsName(InputUnits units) {
  return units == InputUnits::Feet ? "feet" : "meters";
}

bool isQueryName(std::string_view query) {
  return query == "suite" || query == "list" || query == "height" || query == "normal" ||
         query == "segment" || query == "point" || query == "aabb";
}

bool parseArgs(int argc, const char* const* argv, ProbeConfig& config, std::string& diagnostic) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    if (arg != "--units") {
      continue;
    }
    if (index + 1 >= argc || std::string_view(argv[index + 1]).starts_with("--")) {
      diagnostic = "missing units";
      return false;
    }
    const std::string_view units = argv[index + 1];
    if (units == "meters" || units == "meter" || units == "m") {
      config.units = InputUnits::Meters;
    } else if (units == "feet" || units == "foot" || units == "ft") {
      config.units = InputUnits::Feet;
    } else {
      diagnostic = "invalid units";
      return false;
    }
    ++index;
  }
  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    const auto needsValue = [&]() {
      return index + 1 >= argc || std::string_view(argv[index + 1]).starts_with("--");
    };
    if (arg == "--help" || arg == "-h") {
      config.help = true;
    } else if (arg == "--package") {
      if (needsValue()) {
        diagnostic = "missing package path";
        return false;
      }
      config.packagePath = argv[++index];
    } else if (arg == "--query") {
      if (needsValue()) {
        diagnostic = "missing query";
        return false;
      }
      config.query = argv[++index];
      if (!isQueryName(config.query)) {
        diagnostic = "unknown query";
        return false;
      }
    } else if (arg == "--list-surfaces") {
      config.query = "list";
    } else if (arg == "--kind") {
      if (needsValue() || !parseKind(argv[index + 1], config.kind)) {
        diagnostic = "invalid kind";
        return false;
      }
      ++index;
    } else if (arg == "--units") {
      if (needsValue()) {
        diagnostic = "missing units";
        return false;
      }
      const std::string_view units = argv[++index];
      if (units != "meters" && units != "meter" && units != "m" && units != "feet" &&
          units != "foot" && units != "ft") {
        diagnostic = "invalid units";
        return false;
      }
    } else if (arg == "--point") {
      if (needsValue() || !parseVec3(argv[index + 1], config.units, config.point)) {
        diagnostic = "invalid point";
        return false;
      }
      ++index;
      config.hasPoint = true;
    } else if (arg == "--start") {
      if (needsValue() || !parseVec3(argv[index + 1], config.units, config.start)) {
        diagnostic = "invalid start";
        return false;
      }
      ++index;
      config.hasStart = true;
    } else if (arg == "--end") {
      if (needsValue() || !parseVec3(argv[index + 1], config.units, config.end)) {
        diagnostic = "invalid end";
        return false;
      }
      ++index;
      config.hasEnd = true;
    } else if (arg == "--min") {
      if (needsValue() || !parseVec3(argv[index + 1], config.units, config.min)) {
        diagnostic = "invalid min";
        return false;
      }
      ++index;
      config.hasMin = true;
    } else if (arg == "--max") {
      if (needsValue() || !parseVec3(argv[index + 1], config.units, config.max)) {
        diagnostic = "invalid max";
        return false;
      }
      ++index;
      config.hasMax = true;
    } else if (arg == "--tolerance") {
      if (needsValue() || !parseFloat(argv[index + 1], config.toleranceMeters) ||
          config.toleranceMeters < 0.0F) {
        diagnostic = "invalid tolerance";
        return false;
      }
      ++index;
      if (config.units == InputUnits::Feet) {
        config.toleranceMeters *= kFeetToMeters;
      }
    } else {
      diagnostic = "unknown option";
      return false;
    }
  }
  return true;
}

void printUsage() {
  std::cout << "app=iggy3d_collision_probe\n";
  std::cout << "usage=iggy3d_collision_probe --query suite|list|height|normal|segment|point|aabb\n";
  std::cout << "example_suite=./build/iggy3d_collision_probe --query suite\n";
  std::cout << "example_list=./build/iggy3d_collision_probe --list-surfaces\n";
  std::cout << "example_height=./build/iggy3d_collision_probe --query height --units feet --point 10,6.5,9\n";
  std::cout << "example_segment=./build/iggy3d_collision_probe --query segment --kind actor --start 3.048,1,2 --end 3.048,1,-1\n";
}

LoadedSurfaces loadSurfaces(const ProbeConfig& config) {
  LoadedSurfaces loaded;
  loaded.package = iggy3d::loadPackage(iggy3d::PackageLoadRequest{config.packagePath.string()});
  if (loaded.package.status != iggy3d::PackageLoadStatus::Ok) {
    loaded.reason = "package_load_failed";
    return loaded;
  }
  if (loaded.package.rooms.empty()) {
    loaded.reason = "package_has_no_rooms";
    return loaded;
  }
  loaded.surfaces = iggy3d::buildSpatialSurfaceSet(loaded.package.rooms.front());
  loaded.ok = true;
  loaded.reason = "loaded";
  return loaded;
}

void printCommon(const ProbeConfig& config, const LoadedSurfaces& loaded) {
  std::cout << "app=iggy3d_collision_probe\n";
  std::cout << "package=" << config.packagePath.string() << "\n";
  std::cout << "query=" << config.query << "\n";
  std::cout << "kind=" << kindName(config.kind) << "\n";
  std::cout << "units_input=" << unitsName(config.units) << "\n";
  std::cout << "surface_count=" << loaded.surfaces.size() << "\n";
}

void printResultFields(std::string_view prefix, const iggy3d::CollisionQueryResult& result) {
  std::cout << prefix << "status=" << iggy3d::collisionQueryStatusName(result.status) << "\n";
  std::cout << prefix << "reason_code=" << result.reasonCode << "\n";
  std::cout << prefix << "hit="
            << boolText(result.status == iggy3d::CollisionQueryStatus::Hit) << "\n";
  std::cout << prefix << "surface_id=" << result.surfaceId << "\n";
  std::cout << prefix << "role=" << iggy3d::collisionSurfaceRoleName(result.role) << "\n";
  std::cout << prefix << "shape=" << iggy3d::collisionSurfaceShapeName(result.shape) << "\n";
  std::cout << prefix << "point_meters=" << formatVec3(result.pointMeters) << "\n";
  std::cout << prefix << "point_feet=" << formatVec3(result.pointMeters / kFeetToMeters) << "\n";
  std::cout << prefix << "normal=" << formatVec3(result.normal) << "\n";
  std::cout << prefix << "distance_meters=" << formatFloat(result.distanceMeters) << "\n";
  std::cout << prefix << "time_of_impact=" << formatFloat(result.timeOfImpact) << "\n";
  std::cout << prefix << "height_meters=" << formatFloat(result.heightMeters) << "\n";
  std::cout << prefix << "height_feet=" << formatFloat(result.heightMeters / kFeetToMeters) << "\n";
  std::cout << prefix << "checked_surface_count=" << result.checkedSurfaceCount << "\n";
  std::cout << prefix << "blocking_surface_count=" << result.blockingSurfaceCount << "\n";
}

void printSurfaceList(const ProbeConfig& config, const LoadedSurfaces& loaded) {
  printCommon(config, loaded);
  std::size_t index = 0;
  for (const iggy3d::CollisionSurfaceView& surface : loaded.surfaces.surfaces()) {
    const std::string prefix = "surface_" + std::to_string(index) + "_";
    std::cout << prefix << "id=" << surface.id << "\n";
    std::cout << prefix << "role=" << iggy3d::collisionSurfaceRoleName(surface.role) << "\n";
    std::cout << prefix << "shape=" << iggy3d::collisionSurfaceShapeName(surface.shape) << "\n";
    std::cout << prefix << "bounds_min_meters=" << formatVec3(surface.bounds.min) << "\n";
    std::cout << prefix << "bounds_max_meters=" << formatVec3(surface.bounds.max) << "\n";
    std::cout << prefix << "normal=" << formatVec3(surface.normal) << "\n";
    std::cout << prefix << "blocks_actor=" << boolText(surface.blocksActor) << "\n";
    std::cout << prefix << "blocks_projectile=" << boolText(surface.blocksProjectile) << "\n";
    std::cout << prefix << "has_actor_mask=" << boolText(surface.hasActorMask) << "\n";
    std::cout << prefix << "has_projectile_mask=" << boolText(surface.hasProjectileMask) << "\n";
    std::cout << prefix << "opening=" << boolText(surface.opening) << "\n";
    ++index;
  }
  std::cout << "result=pass\n";
  std::cout << "reason_code=surface_list_printed\n";
}

iggy3d::CollisionQueryResult runConfiguredQuery(const ProbeConfig& config,
                                                const iggy3d::SpatialSurfaceSet& surfaces,
                                                std::string& diagnostic) {
  if (config.query == "height") {
    if (!config.hasPoint) {
      diagnostic = "missing point";
      return {};
    }
    return iggy3d::sampleSurfaceHeight(surfaces, config.point, config.toleranceMeters);
  }
  if (config.query == "normal") {
    if (!config.hasPoint) {
      diagnostic = "missing point";
      return {};
    }
    return iggy3d::sampleSurfaceNormal(surfaces, config.point, config.toleranceMeters);
  }
  if (config.query == "segment") {
    if (!config.hasStart || !config.hasEnd) {
      diagnostic = "missing segment endpoints";
      return {};
    }
    return iggy3d::querySegment(surfaces, config.start, config.end, config.kind);
  }
  if (config.query == "point") {
    if (!config.hasPoint) {
      diagnostic = "missing point";
      return {};
    }
    return iggy3d::queryPointOverlap(surfaces, config.point, config.kind);
  }
  if (config.query == "aabb") {
    if (!config.hasMin || !config.hasMax) {
      diagnostic = "missing aabb bounds";
      return {};
    }
    return iggy3d::queryAabbOverlap(surfaces, iggy3d::makeAabb3(config.min, config.max),
                                    config.kind);
  }
  diagnostic = "unsupported query";
  return {};
}

bool expected(std::string_view name,
              const iggy3d::CollisionQueryResult& result,
              iggy3d::CollisionQueryStatus status,
              std::string_view surfaceId) {
  if (result.status != status) {
    return false;
  }
  if (!surfaceId.empty() && result.surfaceId != surfaceId) {
    return false;
  }
  if (name == "normal_spawn") {
    return iggy3d::nearlyEqual(result.normal, {0.0F, 1.0F, 0.0F});
  }
  return true;
}

struct SuiteCase {
  std::string name;
  iggy3d::CollisionQueryResult result;
  iggy3d::CollisionQueryStatus expectedStatus = iggy3d::CollisionQueryStatus::Hit;
  std::string expectedSurfaceId;
};

std::vector<SuiteCase> runSuite(const iggy3d::SpatialSurfaceSet& surfaces) {
  const iggy3d::Vec3 spawn{10.0F * kFeetToMeters, 2.0F, 9.0F * kFeetToMeters};
  return {
      {"height_spawn", iggy3d::sampleSurfaceHeight(surfaces, spawn),
       iggy3d::CollisionQueryStatus::Hit, "spawn_floor_walkable"},
      {"normal_spawn", iggy3d::sampleSurfaceNormal(surfaces, spawn),
       iggy3d::CollisionQueryStatus::Hit, "spawn_floor_walkable"},
      {"actor_wall_segment",
       iggy3d::querySegment(surfaces, {10.0F * kFeetToMeters, 1.0F, 2.0F},
                            {10.0F * kFeetToMeters, 1.0F, -1.0F},
                            iggy3d::CollisionQueryKind::Actor),
       iggy3d::CollisionQueryStatus::Hit, "north_wall_actor_blocker"},
      {"actor_opening_segment",
       iggy3d::querySegment(surfaces,
                            {18.0F * kFeetToMeters, 1.0F, 9.0F * kFeetToMeters},
                            {22.0F * kFeetToMeters, 1.0F, 9.0F * kFeetToMeters},
                            iggy3d::CollisionQueryKind::Actor),
       iggy3d::CollisionQueryStatus::NoHit, ""},
      {"projectile_crate_segment",
       iggy3d::querySegment(surfaces,
                            {4.0F * kFeetToMeters, 1.0F * kFeetToMeters,
                             14.0F * kFeetToMeters},
                            {4.0F * kFeetToMeters, 1.0F * kFeetToMeters,
                             18.0F * kFeetToMeters},
                            iggy3d::CollisionQueryKind::Projectile),
       iggy3d::CollisionQueryStatus::Hit, "spawn_crate_projectile_blocker"},
      {"actor_ignores_projectile_only_crate",
       iggy3d::querySegment(surfaces,
                            {4.0F * kFeetToMeters, 1.0F * kFeetToMeters,
                             14.0F * kFeetToMeters},
                            {4.0F * kFeetToMeters, 1.0F * kFeetToMeters,
                             18.0F * kFeetToMeters},
                            iggy3d::CollisionQueryKind::Actor),
       iggy3d::CollisionQueryStatus::NoHit, ""},
      {"projectile_point_overlap",
       iggy3d::queryPointOverlap(surfaces,
                                 {4.0F * kFeetToMeters, 1.0F * kFeetToMeters,
                                  16.0F * kFeetToMeters},
                                 iggy3d::CollisionQueryKind::Projectile),
       iggy3d::CollisionQueryStatus::Hit, "spawn_crate_projectile_blocker"},
  };
}

int runProbe(const ProbeConfig& config) {
  if (config.help) {
    printUsage();
    return 0;
  }

  const LoadedSurfaces loaded = loadSurfaces(config);
  if (!loaded.ok) {
    std::cout << "app=iggy3d_collision_probe\n";
    std::cout << "package=" << config.packagePath.string() << "\n";
    std::cout << "result=fail\n";
    std::cout << "reason_code=" << loaded.reason << "\n";
    return 1;
  }

  if (config.query == "list") {
    printSurfaceList(config, loaded);
    return 0;
  }

  if (config.query == "suite") {
    printCommon(config, loaded);
    const std::vector<SuiteCase> cases = runSuite(loaded.surfaces);
    bool suitePassed = true;
    std::cout << "case_count=" << cases.size() << "\n";
    for (std::size_t index = 0; index < cases.size(); ++index) {
      const SuiteCase& test = cases[index];
      const std::string prefix = "case_" + std::to_string(index) + "_";
      const bool passed =
          expected(test.name, test.result, test.expectedStatus, test.expectedSurfaceId);
      suitePassed = suitePassed && passed;
      std::cout << prefix << "name=" << test.name << "\n";
      std::cout << prefix << "passed=" << boolText(passed) << "\n";
      std::cout << prefix << "expected_status="
                << iggy3d::collisionQueryStatusName(test.expectedStatus) << "\n";
      std::cout << prefix << "expected_surface_id=" << test.expectedSurfaceId << "\n";
      printResultFields(prefix, test.result);
    }
    std::cout << "surface_height_sampled="
              << boolText(cases.size() > 0U &&
                          cases[0].result.status == iggy3d::CollisionQueryStatus::Hit)
              << "\n";
    std::cout << "surface_normal_valid="
              << boolText(cases.size() > 1U &&
                          cases[1].result.status == iggy3d::CollisionQueryStatus::Hit &&
                          iggy3d::nearlyEqual(cases[1].result.normal, {0.0F, 1.0F, 0.0F}))
              << "\n";
    std::cout << "actor_blocker_hit="
              << boolText(cases.size() > 2U &&
                          cases[2].result.status == iggy3d::CollisionQueryStatus::Hit)
              << "\n";
    std::cout << "opening_blocks_actor="
              << boolText(cases.size() > 3U &&
                          cases[3].result.status == iggy3d::CollisionQueryStatus::Hit)
              << "\n";
    std::cout << "projectile_blocker_hit="
              << boolText(cases.size() > 4U &&
                          cases[4].result.status == iggy3d::CollisionQueryStatus::Hit)
              << "\n";
    std::cout << "query_result_order=stable\n";
    std::cout << "result=" << (suitePassed ? "pass" : "fail") << "\n";
    std::cout << "reason_code=" << (suitePassed ? "collision_probe_suite_pass"
                                                : "collision_probe_suite_failed")
              << "\n";
    return suitePassed ? 0 : 1;
  }

  std::string diagnostic;
  const iggy3d::CollisionQueryResult result =
      runConfiguredQuery(config, loaded.surfaces, diagnostic);
  printCommon(config, loaded);
  if (!diagnostic.empty()) {
    std::cout << "result=fail\n";
    std::cout << "reason_code=" << diagnostic << "\n";
    return 2;
  }
  printResultFields("", result);
  const bool queryOk = result.status != iggy3d::CollisionQueryStatus::InvalidInput;
  std::cout << "result=" << (queryOk ? "pass" : "fail") << "\n";
  return queryOk ? 0 : 2;
}

}  // namespace

int main(int argc, const char* const* argv) {
  ProbeConfig config;
  std::string diagnostic;
  if (!parseArgs(argc, argv, config, diagnostic)) {
    std::cout << "app=iggy3d_collision_probe\n";
    std::cout << "result=fail\n";
    std::cout << "reason_code=" << diagnostic << "\n";
    printUsage();
    return 2;
  }
  return runProbe(config);
}
