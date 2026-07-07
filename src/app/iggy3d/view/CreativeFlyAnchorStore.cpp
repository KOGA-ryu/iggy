#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {
namespace {

constexpr Vec3 kCreativeFlyOriginAnchor{0.0F, 6.0F, 10.0F};

Vec3 activePlayerPositionOrOrigin(const Session* activeSession) {
  if (activeSession == nullptr) {
    return {};
  }
  const EntityId playerActor = activeSession->state().players.actorForSlot(0);
  const EntityState* player = activeSession->state().world.findById(playerActor);
  if (player == nullptr) {
    return {};
  }
  return player->transform.position;
}

ProductCreativeFlyAnchorStore& writeCreativeFlyAnchor(
    ProductAppWindowState& window,
    Vec3 positionMeters,
    ProductCreativeFlyAnchorProvenance provenance) noexcept {
  ProductCreativeFlyAnchorStore& store = window.viewport.creativeFlyAnchor;
  store.positionMeters = positionMeters;
  store.provenance = provenance;
  store.seededFromWorldEpoch = window.creativeWorldEpoch;
  return store;
}

}  // namespace

std::uint64_t bumpCreativeWorldEpoch(ProductAppWindowState& window) noexcept {
  ++window.creativeWorldEpoch;
  return window.creativeWorldEpoch;
}

ProductCreativeFlyAnchorStore& seedCreativeFlyAnchorFromOrigin(
    ProductAppWindowState& window) noexcept {
  return writeCreativeFlyAnchor(window, kCreativeFlyOriginAnchor,
                                ProductCreativeFlyAnchorProvenance::OriginFramed);
}

ProductCreativeFlyAnchorStore& ensureFreshCreativeFlyAnchor(
    ProductAppWindowState& window,
    const Session* activeSession) {
  ProductCreativeFlyAnchorStore& store = window.viewport.creativeFlyAnchor;
  if (productCreativeFlyAnchorFreshForEpoch(store, window.creativeWorldEpoch)) {
    return store;
  }
  return writeCreativeFlyAnchor(window, activePlayerPositionOrOrigin(activeSession),
                                ProductCreativeFlyAnchorProvenance::PlayerSeeded);
}

ProductCreativeFlyAnchorStore& seedCreativeFlyAnchorFromScene(
    ProductAppWindowState& window,
    Vec3 positionMeters) noexcept {
  return writeCreativeFlyAnchor(window, positionMeters,
                                ProductCreativeFlyAnchorProvenance::SceneSeeded);
}

ProductCreativeFlyAnchorStore& recordCreativeFlyAnchorIntegrated(
    ProductAppWindowState& window,
    Vec3 positionMeters) noexcept {
  return writeCreativeFlyAnchor(window, positionMeters,
                                ProductCreativeFlyAnchorProvenance::FlyIntegrated);
}

}  // namespace iggy3d
