# Godot Physics Query Inventory

Source inspected:
- godot-master/scene/2d/physics/ray_cast_2d.*
- godot-master/scene/2d/physics/shape_cast_2d.*
- godot-master/servers/physics_2d
- godot-master/modules/godot_physics_2d
- engine/src/servers/physics2d

Useful names:
- RayCast2D
- ShapeCast2D
- PhysicsServer2D
- PhysicsDirectSpaceState2D
- PhysicsRayQueryParameters2D
- PhysicsShapeQueryParameters2D
- intersect_ray
- intersect_shape
- collide_shape
- cast_motion
- collision_mask
- collision_point
- collision_normal
- safe_fraction
- unsafe_fraction

Ownership boundary:
- Query structs belong in physics2d server code.
- Scene nodes can wrap query structs later, but should not be first.

Take:
- raycast query object.
- raycast hit result.
- shape query boundary.
- collision point and normal fields.

Skip:
- full physics server.
- rigid bodies and joints.
- RID/object exception plumbing.
- editor bindings.
- 3D.

Defer:
- collision layers/masks.
- swept shape casts.
- area/body filtering.

Next local target:
- engine/src/servers/physics2d/ShapeQuery2D.*
- engine/tests/physics2d_query_tests.cpp
