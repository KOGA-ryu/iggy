# Godot Math / Geometry Inventory

Source inspected:
- godot-master/core/math
- godot-master/tests/core/math
- engine/src/core/math

Useful names:
- Vector2
- Vector2i
- Rect2
- Rect2i
- AABB
- Transform2D
- Geometry2D
- AStar2D
- AStarGrid2D
- intersects
- intersects_segment
- has_point
- distance_to
- normalized
- dot

Current Iggy overlap:
- Vec2
- Rect2
- Aabb2
- Ray2

Take:
- Vec2 as the default float vector.
- Rect2 for 2D bounds.
- Aabb2 only if broad-phase or spatial queries need it.
- Ray2 for query vocabulary.
- Transform2D later, when scene/entity transforms exist.
- Function names like `intersects`, `contains`/`hasPoint`, `distanceTo` can stay small and local.

Skip:
- 3D math: AABB, Plane, Quaternion, Basis, Transform3D.
- Variant bindings and editor-facing bind helpers.
- Large geometry utility namespace until repeated callers exist.
- AStarGrid2D until navigation needs move beyond tile maps.

Decision:
- Continue building only the 2D primitives that current engine tests require. Do not port broad utility surfaces preemptively.
