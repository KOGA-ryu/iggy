# Creative Object Kind Pattern

Room is the active template object kind for future `CreativeObjectKind` work.
Repeat this exact loop for the next kind:

1. Add or expose the kind in `Object.hpp`, then update `Object.cpp` so
   `toString` and the correct category helper know about it. Keep the shared
   object shape as `id`, `kind`, `name`, `transform`, `bounds`, `layerId`,
   `visible`, `locked`, `tags`, and optional `parentId`.
2. Add one kind-specific constructor helper beside `makeRoomObject`. It must
   set `kind`, copy every shared shape field, and leave unrelated behavior out.
3. Add one command packet in `Commands.hpp` and any tiny factory in
   `Commands.cpp`. Commands are data only; do not add UI, rendering,
   serialization, filesystem, async work, or editor mouse/tool state.
4. Add one `CreativeDocument::create<Kind>` method. It must allocate from
   `nextObjectId_`, never return `kInvalidObjectId` on success, store the object
   in `objects_` and `objectIndex_`, and call `markContentChanged()` exactly
   once. Rename and remove remain shared object operations.
5. Add one `Facade::create<Kind>` method. It must record command attempt first,
   call the document method, record success or failure, and increment the shared
   object-created metric plus the kind-specific created metric.
6. Add focused unit coverage like `creative_room_tests.cpp`: id is nonzero,
   the object is stored and findable, all shared fields survive, create/rename/
   remove revision rules hold, facade metrics count attempts/successes/failures,
   and vocabulary classification is stable.

Do not generalize beyond the object kind being activated. Leave future kinds
dormant until they have the same complete object/document/command/facade/metric
test loop.
