#pragma once

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

struct EntityState;
class Session;

EntityId productPlayerActor(const Session& session);

const EntityState* productPlayerEntity(const Session& session);

bool setProductPlayerPosition(Session& session, EntityId actor, const Vec3& position);

}  // namespace iggy3d
