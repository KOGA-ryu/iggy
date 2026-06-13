#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/interaction/InteractionEffect2D.hpp"

namespace iggy {

struct InteractionEffectEntry2D {
	ResourceId targetId;
	std::vector<InteractionEffect2D> effects;
};

enum class InteractionEffectCatalog2DIssueCode {
	MissingTargetId,
	EmptyEffects,
	InvalidEffect,
};

struct InteractionEffectCatalog2DIssue {
	InteractionEffectCatalog2DIssueCode code = InteractionEffectCatalog2DIssueCode::MissingTargetId;
	std::size_t entryIndex = 0;
	std::size_t effectIndex = 0;
	InteractionEffectEntry2D entry;
	InteractionEffect2DStatus effectStatus = InteractionEffect2DStatus::Valid;
};

class InteractionEffectCatalog2D {
public:
	InteractionEffectCatalog2D() = default;
	explicit InteractionEffectCatalog2D(std::vector<InteractionEffectEntry2D> entries);

	[[nodiscard]] const std::vector<InteractionEffectEntry2D> &entries() const;
	[[nodiscard]] const std::vector<InteractionEffect2D> *find(const ResourceId &targetId) const;
	[[nodiscard]] bool contains(const ResourceId &targetId) const;

private:
	std::vector<InteractionEffectEntry2D> entries_;
};

struct InteractionEffectCatalog2DBuildResult {
	bool built = false;
	InteractionEffectCatalog2D catalog;
	std::vector<InteractionEffectCatalog2DIssue> issues;
};

class InteractionEffectCatalog2DBuilder {
public:
	[[nodiscard]] InteractionEffectCatalog2DBuildResult build(const std::vector<InteractionEffectEntry2D> &entries) const;
};

} // namespace iggy
