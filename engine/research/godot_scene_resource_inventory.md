# Godot Scene / Resource Inventory

Source inspected:
- godot-master/scene/2d
- godot-master/scene/resources
- godot-master/scene/resources/2d
- godot-master/core/io
- godot-master/core/object
- engine/src/scene
- engine/src/core/resource

Useful names:
- Node2D
- Sprite2D
- AnimatedSprite2D
- TileMap
- TileMapLayer
- Marker2D
- Camera2D
- Resource
- ResourceLoader
- ResourceSaver
- ResourceUID
- PackedScene
- World2D
- Texture2D
- SpriteFrames
- NavigationPolygon
- Shape2D
- ResourceFormatLoader
- ResourceFormatSaver

Current Iggy overlap:
- ResourceId
- scene/level/LevelBlueprint
- scene/level/LevelTileMap
- modules/blueprint/LevelBlueprintValidator

Take:
- ResourceId as a stable handle in data contracts.
- LevelBlueprint and LevelTileMap as plain runtime-facing scene data.
- Loader/saver concepts later, after in-memory contracts stabilize.
- Marker-style names for authored points can map to PlayerStart/spawn data.
- TileMap/TileMapLayer shape is relevant, but Iggy should keep level grids smaller.

Skip:
- Node2D scene tree.
- PackedScene serialization.
- ResourceUID cache.
- ResourceLoader/ResourceSaver registries.
- Texture, material, animation, shader resources.
- Editor import pipeline and inspector integration.

Decision:
- Keep level authoring data as in-memory structs now. Add file formats and loaders only when tests pin the data contract.
