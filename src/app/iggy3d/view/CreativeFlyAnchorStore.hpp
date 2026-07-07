#pragma once

#include <cstdint>
#include <string_view>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct ProductAppWindowState;
class Session;

enum class ProductCreativeFlyAnchorProvenance {
  Unseeded,
  OriginFramed,
  PlayerSeeded,
  SceneSeeded,
  FlyIntegrated,
};

struct ProductCreativeFlyAnchorStore {
  Vec3 positionMeters;
  ProductCreativeFlyAnchorProvenance provenance =
      ProductCreativeFlyAnchorProvenance::Unseeded;
  std::uint64_t seededFromWorldEpoch = 0;
};

[[nodiscard]] inline std::string_view productCreativeFlyAnchorProvenanceName(
    ProductCreativeFlyAnchorProvenance provenance) noexcept {
  switch (provenance) {
    case ProductCreativeFlyAnchorProvenance::Unseeded:
      return "Unseeded";
    case ProductCreativeFlyAnchorProvenance::OriginFramed:
      return "OriginFramed";
    case ProductCreativeFlyAnchorProvenance::PlayerSeeded:
      return "PlayerSeeded";
    case ProductCreativeFlyAnchorProvenance::SceneSeeded:
      return "SceneSeeded";
    case ProductCreativeFlyAnchorProvenance::FlyIntegrated:
      return "FlyIntegrated";
  }
  return "Unseeded";
}

[[nodiscard]] inline bool productCreativeFlyAnchorAvailable(
    const ProductCreativeFlyAnchorStore& store) noexcept {
  return store.provenance != ProductCreativeFlyAnchorProvenance::Unseeded;
}

[[nodiscard]] inline bool productCreativeFlyAnchorFreshForEpoch(
    const ProductCreativeFlyAnchorStore& store,
    std::uint64_t worldEpoch) noexcept {
  return productCreativeFlyAnchorAvailable(store) &&
         store.seededFromWorldEpoch == worldEpoch;
}

std::uint64_t bumpCreativeWorldEpoch(ProductAppWindowState& window) noexcept;

ProductCreativeFlyAnchorStore& seedCreativeFlyAnchorFromOrigin(
    ProductAppWindowState& window) noexcept;

ProductCreativeFlyAnchorStore& ensureFreshCreativeFlyAnchor(
    ProductAppWindowState& window,
    const Session* activeSession);

ProductCreativeFlyAnchorStore& seedCreativeFlyAnchorFromScene(
    ProductAppWindowState& window,
    Vec3 positionMeters) noexcept;

ProductCreativeFlyAnchorStore& recordCreativeFlyAnchorIntegrated(
    ProductAppWindowState& window,
    Vec3 positionMeters) noexcept;

}  // namespace iggy3d
