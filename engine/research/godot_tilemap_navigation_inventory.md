# Godot Tilemap / Navigation Inventory

Source inspected:
- godot-master/scene/2d/tile_map.h
- godot-master/scene/2d/tile_map_layer.*
- godot-master/scene/resources/2d/tile_set.*
- godot-master/servers/navigation_2d
- godot-master/modules/navigation_2d
- engine/src/scene/level
- engine/src/servers/navigation

Useful names:
- TileMap
- TileMapLayer
- TileSet
- TileData
- set_cell / get_cell
- get_cell_tile_data
- get_used_cells_by_id
- terrain_fill_path
- NavigationServer2D
- NavigationAgent2D
- NavigationRegion2D
- NavigationPolygon
- NavigationPathQueryParameters2D
- NavigationPathQueryResult2D

Ownership boundary:
- Tile occupancy belongs in scene/level.
- Path queries belong in servers/navigation when shared beyond one level.
- Blueprint validation stays in modules/blueprint.

Take:
- LevelTileMap as small TileMap equivalent.
- walkable/blocking tile data.
- navigation query boundary that consumes tile maps.
- path result object later.

Skip:
- TileSet atlas/source system.
- terrain painting helpers.
- editor navigation plugins.
- polygon navmesh generation.

Defer:
- multiple tile layers.
- navigation regions.
- dynamic obstacles.

Next local target:
- engine/src/scene/level/LevelTileMap.*
- engine/src/servers/navigation/TilePathQuery2D.*
- engine/tests/level_tile_map_tests.cpp
