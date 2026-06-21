#include "runtime3d/Runtime3DWorldState.hpp"

#include <algorithm>
#include <utility>

namespace iggy::runtime3d {

Runtime3DEntityState *Runtime3DWorldState::find(Runtime3DEntityId id)
{
	const auto iterator = std::find_if(entities.begin(), entities.end(), [id](const Runtime3DEntityState &entity) {
		return entity.id == id;
	});
	if (iterator == entities.end())
		return nullptr;
	return &*iterator;
}

const Runtime3DEntityState *Runtime3DWorldState::find(Runtime3DEntityId id) const
{
	const auto iterator = std::find_if(entities.begin(), entities.end(), [id](const Runtime3DEntityState &entity) {
		return entity.id == id;
	});
	if (iterator == entities.end())
		return nullptr;
	return &*iterator;
}

bool Runtime3DWorldState::add(Runtime3DEntityState entity)
{
	if (!entity.id.valid() || find(entity.id) != nullptr)
		return false;
	entities.push_back(std::move(entity));
	return true;
}

bool Runtime3DWorldState::upsert(Runtime3DEntityState entity)
{
	if (!entity.id.valid())
		return false;

	Runtime3DEntityState *existing = find(entity.id);
	if (existing != nullptr) {
		*existing = std::move(entity);
		return true;
	}

	entities.push_back(std::move(entity));
	return true;
}

} // namespace iggy::runtime3d
