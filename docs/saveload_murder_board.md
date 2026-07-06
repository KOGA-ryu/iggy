<!-- Save/Load Murder Board — 2026-07-06. Real call graph, keep/delete/merge, exact symbols. No new roles, no abstractions, no renames. For review; not committed. -->

# Save/Load Murder Board — Collapse Plan

Repo janitor pass over the save surface. Four traced segments merged. Every touched symbol is classified and given KEEP / DELETE / MERGE. Source-verified: the four Flow overloads (Flow.hpp:41/48/56/64) and the two dead SaveFileStore functions were grepped for callers — results below drive the kill list.

## 1. Unified SAVE call graph

Three funnels, one encoder, one durable pipeline.

```
ActionHandlers.cpp:318/325 handleProductPauseAction
  -> Flow.cpp:209 executeProductPauseSaveFlow(7-arg)
       |-- editor INACTIVE --> Flow.cpp:140 (base) -> Operations.cpp:967 writeProductCurrentSessionSave
       |                          -> SaveBridge writeProductSessionSaveDurably
       |                          -> SaveFileStore.cpp:683 writeSessionSaveFileDurably
       |                          -> SaveLoad.cpp:500 saveSessionState -> :170 envelopeFromState
       |                                           \
       |-- editor ACTIVE ------> Flow.cpp:92 executeCreativePauseSaveFlow
                                   -> Operations.cpp:1507 saveProductCurrentCreativeWorld
                                   -> WorldService.cpp:247 saveCreativeWorld   <== standalone joins here
                                   -> SaveBridge.cpp:613 writeCreativeDocumentSaveDurably
                                   -> DocumentSection.cpp:372 buildSaveCreativeDocumentSection
                                   -> SaveFileStore.cpp:739 writeSaveEnvelopeFileDurably
                                                            \
   both paths converge --> SaveCodec.cpp:1957 encodeSaveEnvelope
                        -> SaveFileStore.cpp:124 writeEncodedEnvelopeFileDurably
                             -> :273 writeDurableSaveTempFile (write+readback+fsync)
                             -> :340 validateDurableSaveTempFile (re-decode)
                             -> :374 commitDurableSaveTempFile (rename+dir fsync) -> DISK

STANDALONE: main.cpp:599 F5 -> StandalonePersistenceProof.cpp:90 saveStandaloneScene -> WorldService.cpp:247 (converges)
```

Product-session vs creative diverge at the `productCreativeDocumentEditorActiveForSource` branch (Flow.cpp:209) and re-converge only at `encodeSaveEnvelope` + the durable pipeline. Standalone and creative converge earlier at `saveCreativeWorld`. There is exactly ONE encoder and ONE durable writer for all three.

## 2. Unified LOAD call graph

```
PRODUCT-SESSION:
  SaveBridge loadProductSessionSave
   -> SaveFileStore.cpp:770 readSaveFile [readWholeFile + SaveCodec.cpp:1961 decodeSaveEnvelope]
   -> SaveLoad.cpp:570 loadEncodedSaveIntoSession -> :531 loadEnvelopeIntoSession
   -> SaveCompatibility.cpp:32 checkSaveCompatibility
   -> SaveLoad.cpp:344 buildCandidate [SaveEnvelope -> SessionState]
   -> Session::replaceStateFromLoad

CREATIVE / STANDALONE / PRODUCT-LAUNCH (shared tail):
  main.cpp:610 F9 -> StandalonePersistenceProof.cpp:111 loadStandaloneScene
    OR Operations.cpp:1438 launchProductCreativeOpenWorld
   -> WorldService.cpp:214 openCreativeWorld
   -> SaveBridge.cpp:692 loadCreativeDocumentSave [readSaveFile + SaveCodec.cpp:1961 decodeSaveEnvelope]
   -> DocumentSection.cpp:439 restoreCreativeDocumentFromSaveSection -> CreativeDocument::restoreForLoad
   -> Facade.cpp:785 Facade::installDocument (truth swap)
        [standalone stops; product also runs recordActiveCreativeSaveIdentity window mirrors]
```

Single decoder (`decodeSaveEnvelope`). Diverges on payload: session -> `buildCandidate` -> `replaceStateFromLoad`; creative -> `restoreCreativeDocumentFromSaveSection` -> `installDocument`.

## 3. Minimum path (must survive)

```
Facade::installDocument -> saveCreativeWorld -> writeCreativeDocumentSaveDurably
 -> buildSaveCreativeDocumentSection -> encodeSaveEnvelope
 -> writeEncodedEnvelopeFileDurably (write/validate/commit) -> DISK
 -> readSaveFile -> decodeSaveEnvelope -> loadCreativeDocumentSave
 -> restoreCreativeDocumentFromSaveSection -> CreativeDocument::restoreForLoad
 -> openCreativeWorld -> Facade::installDocument
```

Every hop adds encode / validate / decode / transform work. Nothing here is deletable.

## 4. KEEP (load-bearing) — summary

The entire runtime codec core (`SaveCodec` encode/decode, `SaveEnvelope` schema, `SaveCompatibility`, `SaveLoad` envelopeFromState/buildCandidate/loadEnvelopeIntoSession, `SaveFileStore` readSaveFile + the durable temp/validate/commit pipeline + the two durable writers) is clean and load-bearing. The creative transform pair (`buildSaveCreativeDocumentSection` / `restoreCreativeDocumentFromSaveSection`) and the two SaveBridge engines are the doc<->bytes marshalling. `Facade::installDocument` owns the load-back truth swap. Full list in keepList.

Boundary is CLEAN: SaveCodec/SaveEnvelope include zero IO headers; all disk IO is isolated in SaveFileStore. No inversion.

## 5. KILL table

| file:symbol | class | verdict | reason |
|---|---|---|---|
| Flow.cpp:194 executeProductPauseSaveFlow(6-arg settings&) | forwards-only | DELETE | ZERO callers repo-wide; body is a verbatim subset of @209. Drop Flow.hpp:56 too |
| Flow.cpp:174 executeProductPauseSaveFlow(6-arg creativeApp*) | forwards-only | DELETE | Single caller product_creative_world_launch_tests.cpp:1665; migrate to @209, drop Flow.hpp:48 |
| SaveFileStore.cpp:618 writeSessionSaveFile | duplicate | DELETE | Non-durable twin of writeSessionSaveFileDurably; only tests call it. Drop SaveFileStore.hpp:241 |
| SaveFileStore.cpp:789 deleteSaveFile | duplicate | DELETE | Hard-delete; only save_file_store_tests:1101 calls it; prod uses softDeleteSaveFile. Drop SaveFileStore.hpp:247 |
| WorldService.cpp:25 setCreateStatus, :31 setOpenStatus | duplicate-mirror | MERGE -> WorldService.cpp:37 setSaveStatus | three byte-identical status setters differing only in result type |

## 6. Ordered collapse plan (suite stays green each step)

1. Migrate the one test at product_creative_world_launch_tests.cpp:1665 to call @209 (add a FrontendSettings& arg). Safe: @209 with editor inactive reproduces the routing; same asserted result.
2. Delete Flow.cpp:174 + Flow.hpp:48. Safe: zero callers after step 1.
3. Delete Flow.cpp:194 + Flow.hpp:56. Safe: grep-confirmed zero callers; @209 already carries the identical tail.
4. Repoint writeSessionSaveFile call-sites (save_slot_model_tests, save_file_store_tests) to writeSessionSaveFileDurably; repoint deleteSaveFile (save_file_store_tests:1101) to softDeleteSaveFile.
5. Delete SaveFileStore.cpp:618 writeSessionSaveFile + :789 deleteSaveFile + SaveFileStore.hpp:241/:247. Safe: no callers after step 4.
6. Merge setCreateStatus/setOpenStatus into setSaveStatus. Safe: byte-identical bodies, same fields written.

## 7. Acceptance test (must pass unchanged)

`create CreativeDocument (Facade::installDocument) -> snapshotDocument(before) -> saveCreativeWorld -> openCreativeWorld -> installDocument -> snapshotDocument(afterLoad) -> snapshotsMatch(before, afterLoad) == true`. Compares document.id / objectCount / nextObjectId and each object's {id, kindId, position, bounds, pathPoints}. This is exactly `StandaloneCaptureScenario.cpp:572 runPersistenceProof` and must stay green through steps 1-6.

## Corrections to the seed

- SEED claim "Flow hands off to save/SaveBridge" is INACCURATE for Segment A: the actual handoff targets are `Operations.cpp:967 writeProductCurrentSessionSave` and `Operations.cpp:1507 saveProductCurrentCreativeWorld`. SaveBridge is reached one hop later.
- `saveProductCurrentCreativeWorld` (Operations) is NOT a duplicate of `saveCreativeWorld` (WorldService): the former is the app-state/undo/window adapter, the latter the document-only request executor that drains dirty flags. Both KEEP.
- Standalone save is NOT a third codec: `saveStandaloneScene`/`loadStandaloneScene` are thin forwarders into the SAME `saveCreativeWorld`/`openCreativeWorld` kernel API. Standalone is the clean reference proving the product-side window mirrors (recordActiveCreativeSaveIdentity, selectProductSaveSlotById, recordDeletedProductSaveSlots) are UI scaffolding, NOT persistence — they die WITH the ProductAppWindowState god-struct in the creative-decoupling lane, not in this pass (deleting now breaks product UI reads).
