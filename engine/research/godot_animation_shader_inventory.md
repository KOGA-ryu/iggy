# Godot Animation / Shader Inventory

Source inspected:
- godot-master/scene/2d/animated_sprite_2d.*
- godot-master/scene/resources/sprite_frames.*
- godot-master/scene/animation
- godot-master/scene/2d/light_2d.*
- godot-master/scene/2d/light_occluder_2d.*
- godot-master/scene/resources/material.*
- godot-master/scene/resources/shader.*
- engine/src/scene/animation
- engine/src/servers/render

Useful names:
- AnimatedSprite2D
- SpriteFrames
- AnimationPlayer
- AnimationTree
- AnimationNodeStateMachine
- play / pause / stop
- animation
- frame
- frame_progress
- speed_scale
- Shader
- Material
- CanvasItemMaterial
- Light2D
- LightOccluder2D
- blend_mode
- shadow

Ownership boundary:
- Frame animation belongs in scene/animation.
- Materials/shaders belong behind render server contracts.
- Gameplay should request animation state, not own rendering details.

Take:
- named animation clips.
- frame index and frame duration.
- play/stop state.
- simple sprite frame resource later.

Skip:
- AnimationTree blend graphs.
- shader language and compiler.
- full material system.
- 3D animation.

Defer:
- 2D lights and occluders.
- shader/material resources.
- animation transitions.

Next local target:
- engine/src/scene/animation/SpriteAnimation.*
- engine/tests/sprite_animation_tests.cpp
