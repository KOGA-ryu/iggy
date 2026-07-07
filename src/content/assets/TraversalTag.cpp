#include "content/assets/TraversalTag.hpp"

#include <array>

namespace iggy3d {
namespace {

struct TraversalTagRow {
  TraversalTag tag;
  std::string_view id;
};

constexpr std::array<TraversalTagRow, 10U> kTraversalTagRows{{
    {TraversalTag::Walkable, "walkable"},
    {TraversalTag::Blocker, "blocker"},
    {TraversalTag::ProjectileBlocker, "projectile_blocker"},
    {TraversalTag::Opening, "opening"},
    {TraversalTag::Clamber, "clamber"},
    {TraversalTag::ClamberCandidate, "clamber_candidate"},
    {TraversalTag::Vault, "vault"},
    {TraversalTag::WireWalk, "wire_walk"},
    {TraversalTag::NoPlayer, "no_player"},
    {TraversalTag::DebugOnly, "debug_only"},
}};

constexpr std::array<TraversalTag, 10U> kTraversalTags{{
    TraversalTag::Walkable,
    TraversalTag::Blocker,
    TraversalTag::ProjectileBlocker,
    TraversalTag::Opening,
    TraversalTag::Clamber,
    TraversalTag::ClamberCandidate,
    TraversalTag::Vault,
    TraversalTag::WireWalk,
    TraversalTag::NoPlayer,
    TraversalTag::DebugOnly,
}};

}  // namespace

std::span<const TraversalTag> allTraversalTags() noexcept {
  return kTraversalTags;
}

std::string_view traversalTagId(TraversalTag tag) noexcept {
  for (const TraversalTagRow& row : kTraversalTagRows) {
    if (row.tag == tag) {
      return row.id;
    }
  }
  return {};
}

std::optional<TraversalTag> parseTraversalTag(std::string_view id) noexcept {
  for (const TraversalTagRow& row : kTraversalTagRows) {
    if (row.id == id) {
      return row.tag;
    }
  }
  return std::nullopt;
}

bool validTraversalTag(std::string_view id) noexcept {
  return parseTraversalTag(id).has_value();
}

bool isStructuralTraversalTag(TraversalTag tag) noexcept {
  switch (tag) {
    case TraversalTag::Walkable:
    case TraversalTag::Blocker:
    case TraversalTag::ProjectileBlocker:
    case TraversalTag::Opening:
      return true;
    case TraversalTag::Clamber:
    case TraversalTag::ClamberCandidate:
    case TraversalTag::Vault:
    case TraversalTag::WireWalk:
    case TraversalTag::NoPlayer:
    case TraversalTag::DebugOnly:
      return false;
  }
  return false;
}

bool isMovementTraversalTag(TraversalTag tag) noexcept {
  switch (tag) {
    case TraversalTag::Clamber:
    case TraversalTag::Vault:
    case TraversalTag::WireWalk:
      return true;
    case TraversalTag::Walkable:
    case TraversalTag::Blocker:
    case TraversalTag::ProjectileBlocker:
    case TraversalTag::Opening:
    case TraversalTag::ClamberCandidate:
    case TraversalTag::NoPlayer:
    case TraversalTag::DebugOnly:
      return false;
  }
  return false;
}

bool isAuthoringTraversalHintTag(TraversalTag tag) noexcept {
  switch (tag) {
    case TraversalTag::ClamberCandidate:
      return true;
    case TraversalTag::Walkable:
    case TraversalTag::Blocker:
    case TraversalTag::ProjectileBlocker:
    case TraversalTag::Opening:
    case TraversalTag::Clamber:
    case TraversalTag::Vault:
    case TraversalTag::WireWalk:
    case TraversalTag::NoPlayer:
    case TraversalTag::DebugOnly:
      return false;
  }
  return false;
}

}  // namespace iggy3d
