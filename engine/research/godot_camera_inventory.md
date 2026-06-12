# Godot Camera Inventory

Source inspected:
- godot-master/scene/2d/camera_2d.h
- godot-master/scene/2d/camera_2d.cpp
- engine/src/core/math
- engine/src/scene

Useful names:
- Camera2D
- get_camera_transform
- set_zoom / get_zoom
- set_offset / get_offset
- set_limit_rect / get_limit_rect
- position_smoothing_enabled
- position_smoothing_speed
- drag_margin
- force_update_scroll
- make_current / clear_current

Ownership boundary:
- Camera is scene/view state, not physics or level data.
- Camera consumes Vec2/Rect2/Transform2D-style math but should not own math primitives.

Take:
- Camera2D-style runtime object later.
- zoom, offset, viewport bounds, target position.
- optional bounds clamp against level Rect2.

Skip:
- editor drawing helpers.
- current-camera scene tree registration.
- rotation smoothing.
- device camera server.

Defer:
- drag margins.
- smoothing/interpolation.
- multiple cameras.

Next local target:
- engine/src/scene/camera/Camera2D.*
- engine/tests/camera2d_tests.cpp
