#include "modules/line_of_sight/LineOfSight.hpp"

#include <limits>

#include "core/math/Ray2.hpp"
#include "scene/level/LevelGridQuery.hpp"
#include "servers/physics2d/ShapeQuery2D.hpp"

namespace iggy::line_of_sight {

namespace {

LineOfSightTrace Result(LineOfSightStatus status, TileCoord startTile, TileCoord endTile)
{
	LineOfSightTrace trace;
	trace.status = status;
	trace.startTile = startTile;
	trace.endTile = endTile;
	return trace;
}

} // namespace

bool LineOfSightTrace::visible() const
{
	return status == LineOfSightStatus::Visible;
}

bool LineOfSightTrace::blocked() const
{
	return status == LineOfSightStatus::Blocked || status == LineOfSightStatus::StartBlocked || status == LineOfSightStatus::EndBlocked;
}

bool LineOfSightTrace::valid() const
{
	return status != LineOfSightStatus::StartOutOfBounds && status != LineOfSightStatus::EndOutOfBounds;
}

LineOfSightTrace Trace(const LevelTileMap &map, Vec2 from, Vec2 to)
{
	const TileCoord startTile = tileForPoint(from);
	const TileCoord endTile = tileForPoint(to);
	const LevelTile *start = tileAt(map, startTile);
	if (start == nullptr)
		return Result(LineOfSightStatus::StartOutOfBounds, startTile, endTile);

	const LevelTile *end = tileAt(map, endTile);
	if (end == nullptr)
		return Result(LineOfSightStatus::EndOutOfBounds, startTile, endTile);

	if (!start->walkable)
		return Result(LineOfSightStatus::StartBlocked, startTile, endTile);
	if (!end->walkable)
		return Result(LineOfSightStatus::EndBlocked, startTile, endTile);
	if (sameTile(startTile, endTile))
		return Result(LineOfSightStatus::Visible, startTile, endTile);

	const Vec2 segment = to - from;
	const float maxDistance = segment.length();
	if (maxDistance == 0.0F)
		return Result(LineOfSightStatus::Visible, startTile, endTile);

	const Ray2 ray { from, segment / maxDistance };
	LineOfSightTrace closest = Result(LineOfSightStatus::Visible, startTile, endTile);
	float closestDistance = std::numeric_limits<float>::infinity();

	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			const TileCoord tileCoord { x, y };
			const LevelTile *tile = tileAt(map, tileCoord);
			if (tile == nullptr || tile->walkable)
				continue;

			const physics2d::RaycastHit2D hit = physics2d::RaycastAabb(ray, tileBounds(tileCoord), maxDistance);
			if (!hit.hit || hit.distance >= closestDistance)
				continue;

			closest.status = LineOfSightStatus::Blocked;
			closest.hitTile = tileCoord;
			closest.hitPoint = hit.point;
			closest.distance = hit.distance;
			closestDistance = hit.distance;
		}
	}

	return closest;
}

bool CanSee(const LevelTileMap &map, Vec2 from, Vec2 to)
{
	return Trace(map, from, to).visible();
}

} // namespace iggy::line_of_sight
