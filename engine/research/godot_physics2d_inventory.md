# Godot Physics2D Inventory

Source inspected:
- godot-master/servers/physics_2d
- godot-master/modules/godot_physics_2d
- godot-master/scene/2d/physics
- godot-master/scene/resources/2d shape files
- engine/src/servers/physics2d

Useful names:
- PhysicsServer2D
- PhysicsDirectSpaceState2D
- PhysicsRayQueryParameters2D
- PhysicsPointQueryParameters2D
- PhysicsShapeQueryParameters2D
- PhysicsTestMotionParameters2D
- PhysicsTestMotionResult2D
- Shape2D
- CollisionObject2D
- CollisionShape2D
- Area2D
- RayCast2D
- ShapeCast2D
- intersect_ray
- intersect_point
- intersect_shape
- cast_motion
- collide_shape
- get_rest_info
- collision_layer
- collision_mask

Current Iggy overlap:
- servers/physics2d/ShapeQuery2D
- core/math/Ray2
- core/math/Rect2
- core/math/Aabb2

Take:
- Query-first API shape before full rigid-body simulation.
- Ray, point, and shape queries as separate request/result structs.
- Collision layer/mask naming when filtering is needed.
- Space/direct-state split later only if multiple physics worlds appear.

Skip:
- RigidBody2D, joints, force integration, sleep state.
- Area callbacks and full collision object ownership.
- RID/resource indirection.
- Threaded physics server wrapper.
- Editor nodes like RayCast2D and ShapeCast2D for now.

Decision:
- Keep Iggy physics2d as deterministic query helpers first. Add body simulation only after gameplay needs it.
