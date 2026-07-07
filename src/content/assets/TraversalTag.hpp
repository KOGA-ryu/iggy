#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace iggy3d {

enum class TraversalTag : std::uint8_t {
  Walkable,
  Blocker,
  ProjectileBlocker,
  Opening,
  Clamber,
  ClamberCandidate,
  Vault,
  WireWalk,
  NoPlayer,
  DebugOnly,
};

std::span<const TraversalTag> allTraversalTags() noexcept;
std::string_view traversalTagId(TraversalTag tag) noexcept;
std::optional<TraversalTag> parseTraversalTag(std::string_view id) noexcept;
bool validTraversalTag(std::string_view id) noexcept;
bool isStructuralTraversalTag(TraversalTag tag) noexcept;
bool isMovementTraversalTag(TraversalTag tag) noexcept;
bool isAuthoringTraversalHintTag(TraversalTag tag) noexcept;

}  // namespace iggy3d
