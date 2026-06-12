# DevilutionX Save / Load Inventory

Source inspected:
- DevilutionX-master/Source/loadsave.cpp
- DevilutionX-master/Source/loadsave.h
- player-movement-system/src/save
- player-movement-system/src/session
- engine/src/core/io
- engine/src/core/resource

Useful names:
- LoadHelper
- SaveHelper
- SaveReader
- SaveWriter
- ReadArchive
- LoadPlayer / SavePlayer
- LoadMonster / SaveMonster
- LoadItemData / SaveItem
- LoadDroppedItems / SaveDroppedItems
- LoadLighting / SaveLighting
- LoadLevel / SaveLevel
- LoadGame
- LoadHotkeys / SaveHotkeys
- LoadHeroItems
- SaveLevelSeeds
- ConvertLevels

Ownership boundary:
- Save/load belongs in core/io or runtime/session once data contracts settle.
- Durable state should not save transient frame events.
- ResourceId should remain stable across serialized data.

Take:
- helper reader/writer boundary.
- per-domain save functions.
- explicit version/conversion surface.
- validate item/resource data on load.

Skip:
- MPQ/archive compatibility.
- endian compatibility details for now.
- Diablo/Hellfire branching.
- hotkey-specific save paths.

Defer:
- binary format.
- migration/version table.
- per-level archive files.

Next local target:
- engine/src/core/io/SaveBuffer.*
- engine/src/runtime/LevelSnapshot.*
- engine/tests/save_load_tests.cpp
