#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class InteractionTarget2DKind {
	Unknown,
	Inspectable,
	Usable,
	Pickup,
	Talk,
	Door,
};

struct InteractionTarget2D {
	ResourceId id;
	InteractionTarget2DKind kind = InteractionTarget2DKind::Unknown;
	Vec2 position;
	float radius = 0.0F;
	bool enabled = true;
};

enum class InteractionTarget2DRegistryIssueCode {
	EmptyId,
	DuplicateId,
	InvalidRadius,
};

struct InteractionTarget2DRegistryIssue {
	InteractionTarget2DRegistryIssueCode code = InteractionTarget2DRegistryIssueCode::EmptyId;
	std::size_t targetIndex = 0;
	InteractionTarget2D target;
};

class InteractionTarget2DRegistry {
public:
	InteractionTarget2DRegistry() = default;
	explicit InteractionTarget2DRegistry(std::vector<InteractionTarget2D> targets);

	[[nodiscard]] const std::vector<InteractionTarget2D> &targets() const;
	[[nodiscard]] const InteractionTarget2D *find(const ResourceId &id) const;
	[[nodiscard]] bool contains(const ResourceId &id) const;

private:
	std::vector<InteractionTarget2D> targets_;
};

struct InteractionTarget2DRegistryBuildResult {
	bool built = false;
	InteractionTarget2DRegistry registry;
	std::vector<InteractionTarget2DRegistryIssue> issues;
};

class InteractionTarget2DRegistryBuilder {
public:
	[[nodiscard]] InteractionTarget2DRegistryBuildResult build(const std::vector<InteractionTarget2D> &targets) const;
};

} // namespace iggy
