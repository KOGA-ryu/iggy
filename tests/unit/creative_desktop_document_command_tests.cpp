#include "creative_desktop_command_test_runners.hpp"
#include "creative_desktop_command_test_support.hpp"

namespace {

bool newDocumentReplacesAndClearsHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd New");
  static_cast<void>(document.assignId(410U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  static_cast<void>(createCrate(appState.facade, 0.0));
  static_cast<void>(createCrate(appState.facade, 2.0));
  selectPrimary(appState.facade,
                createCrate(appState.facade, 4.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{}, "seed"));

  app::CreativeEditorState editor;
  app::markCreativeEditorDocumentSaved(
      editor.persistence, appState.facade.document());
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult result =
      dispatchOne(app::CreativeDesktopCommandId::NewDocument, context);

  return expect(result.accepted && result.documentReplaced,
                "new document is accepted and replaces the document") &&
         expect(appState.facade.document().objectCount() == 0U,
                "new document is blank") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U,
                "new document clears undo history") &&
         expect(!editor.persistence.hasSavePoint &&
                    editor.persistence.documentId ==
                        appState.facade.document().id() &&
                    app::creativeEditorDocumentDirty(
                        editor.persistence, appState.facade.document()),
                "new document has no save point and is dirty");
}

bool builderEstateRegenerationIsExplicitUndoableAndUnsaved() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_desktop_builder_estate_regeneration_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);

  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Custom Before Regeneration");
  static_cast<void>(document.assignId(499U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  static_cast<void>(createCrate(appState.facade, 3.0));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "custom_before_regeneration");
  std::string saveId = "custom_slot";
  const app::CreativeDesktopCommandContext context{appState, editor, root,
                                                    &saveId};
  const cr::CreativeMapTemplateResult expected = cr::buildCreativeMapTemplate(
      cr::kBuilderEstateMapTemplateId, appState.facade.document().id());
  if (!expected.accepted || !expected.worldLayoutPresent) {
    std::filesystem::remove_all(root, error);
    return expect(false, "builder estate regeneration fixture generated");
  }

  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const auto regenerated = dispatchPayload(
      app::CreativeDesktopCommandId::RegenerateMapTemplate, context,
      app::CreativeDesktopMapTemplatePayload{
          std::string(cr::kBuilderEstateMapTemplateId)});
  const bool noSaveWritten = !std::filesystem::exists(
      root / "custom_slot.iggy3d.save", error);
  const bool replaced =
      regenerated.accepted && regenerated.changed &&
      regenerated.documentReplaced && regenerated.sceneChanged &&
      regenerated.worldLayoutChanged && saveId == "custom_slot" &&
      appState.facade.document().id() == 499U &&
      appState.facade.document().name() == expected.document.name() &&
      appState.facade.document().objectCount() ==
          expected.document.objectCount() &&
      regenerated.affectedObjectCount == expected.document.objectCount() &&
      editor.worldLayout.source.stableKey ==
          expected.worldLayout.stableKey &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U &&
      noSaveWritten;

  const auto undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool restoredCustom =
      undone.accepted && undone.changed && undone.sceneChanged &&
      undone.worldLayoutChanged && appState.facade.document().id() == 499U &&
      appState.facade.document().name() == "Custom Before Regeneration" &&
      appState.facade.document().objectCount() == 1U &&
      editor.worldLayout.source.stableKey ==
          "custom_before_regeneration";

  const auto redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool restoredEstate =
      redone.accepted && redone.changed && redone.sceneChanged &&
      redone.worldLayoutChanged && appState.facade.document().id() == 499U &&
      appState.facade.document().objectCount() ==
          expected.document.objectCount() &&
      editor.worldLayout.source.stableKey == expected.worldLayout.stableKey &&
      saveId == "custom_slot" &&
      !std::filesystem::exists(root / "custom_slot.iggy3d.save", error);

  const std::string buildingKey =
      editor.worldLayout.source.buildings.front().stableKey;
  const auto pendingRename = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutRenameSource, context,
      app::CreativeDesktopWorldLayoutSourceRenamePayload{
          cr::CreativeWorldLayoutTable::Building, 0U, buildingKey,
          "Pending Estate Rename"});
  const std::uint64_t documentRevisionBeforeRejectedRegeneration =
      appState.facade.document().revision();
  const auto rejectedPending = dispatchPayload(
      app::CreativeDesktopCommandId::RegenerateMapTemplate, context,
      app::CreativeDesktopMapTemplatePayload{
          std::string(cr::kBuilderEstateMapTemplateId)});
  const bool pendingSourceProtected =
      pendingRename.accepted && pendingRename.changed &&
      !rejectedPending.accepted && !rejectedPending.changed &&
      appState.facade.document().revision() ==
          documentRevisionBeforeRejectedRegeneration &&
      editor.worldLayout.source.buildings[0].name == "Pending Estate Rename";

  const bool ok =
      expect(replaced,
             "builder estate regeneration replaces source and 3D without saving") &&
      expect(restoredCustom,
             "builder estate regeneration undo restores document and layout") &&
      expect(restoredEstate,
             "builder estate regeneration redo restores document and layout") &&
      expect(pendingSourceProtected,
             "map regeneration refuses to overwrite pending source edits");
  std::filesystem::remove_all(root, error);
  return ok;
}

bool deleteAndDuplicateHitTheKernels() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Edit");
  static_cast<void>(document.assignId(411U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  static_cast<void>(createCrate(appState.facade, 0.0));
  const cr::CreativeObjectId target = createCrate(appState.facade, 2.0);
  selectPrimary(appState.facade, target);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const std::uint64_t before = appState.facade.document().objectCount();
  const app::CreativeDesktopCommandResult dup =
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context);
  const bool grew = appState.facade.document().objectCount() == before + 1U;
  const app::CreativeDesktopCommandResult del =
      dispatchOne(app::CreativeDesktopCommandId::DeleteSelection, context);

  return expect(
             dup.accepted && dup.changed && grew &&
                 dup.objectAction.action ==
                     cr::CreativeSemanticObjectAction::Duplicate &&
                 dup.objectAction.target ==
                     app::CreativeEditorObjectActionTarget::Selection &&
                 dup.objectAction.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                 app::creativeEditorObjectActionOutcomeValid(
                     dup.objectAction),
                "duplicate adds one object via the kernel") &&
         expect(del.accepted && del.changed &&
                    appState.facade.document().objectCount() == before &&
                    del.objectAction.action ==
                        cr::CreativeSemanticObjectAction::Delete &&
                    del.objectAction.target ==
                        app::CreativeEditorObjectActionTarget::Selection &&
                    del.objectAction.status ==
                        app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                    app::creativeEditorObjectActionOutcomeValid(
                        del.objectAction),
                "delete removes the duplicated object via the kernel");
}

bool deleteGeneratedWorldLayoutOutputEditsItsSource() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Generated Source Delete");
  static_cast<void>(document.assignId(4101U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "generated_delete_source");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Room));
  const app::CreativeEditorWorldLayoutEditReceipt began =
      app::applyCreativeEditorWorldLayoutGesture(
          editor.worldLayout,
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0});
  const app::CreativeEditorWorldLayoutEditReceipt committed =
      app::applyCreativeEditorWorldLayoutGesture(
          editor.worldLayout,
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {6.0, 4.0});
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated =
      dispatchOne(app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!began.accepted || !committed.accepted || !generated.accepted ||
      appState.facade.document().objects().empty()) {
    return expect(false, "generated source delete setup accepted");
  }

  const auto generatedObject = std::find_if(
      appState.facade.document().objects().begin(),
      appState.facade.document().objects().end(),
      [&](const cr::CreativeObject& object) {
        return cr::resolveCreativeWorldLayoutObjectProvenance(
                   editor.worldLayout.source, object)
            .owned;
      });
  if (generatedObject == appState.facade.document().objects().end()) {
    return expect(false, "generated source delete found owned output");
  }
  selectPrimary(appState.facade, generatedObject->id);
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::size_t documentObjectCountBefore =
      appState.facade.document().objectCount();
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::size_t sourceUndoBefore =
      editor.worldLayout.sourceHistory.undoEntries.size();
  const app::CreativeDesktopCommandResult deleted =
      dispatchOne(app::CreativeDesktopCommandId::DeleteSelection, context);
  const bool sourceOnlyDelete =
      deleted.accepted && deleted.changed && deleted.worldLayoutChanged &&
      !deleted.sceneChanged &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      editor.worldLayout.generatedRevision != editor.worldLayout.revision &&
      editor.worldLayout.sourceHistory.undoEntries.size() ==
          sourceUndoBefore + 1U;
  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);

  return expect(sourceOnlyDelete,
                "3D generated delete routes to the World Layout source") &&
         expect(appState.facade.document().revision() ==
                        documentRevisionBefore &&
                    appState.facade.document().objectCount() ==
                        documentObjectCountBefore &&
                    editor.worldLayout.sourceHistory.undoEntries.size() ==
                        sourceUndoBefore &&
                    undone.accepted && undone.changed &&
                    editor.worldLayout.revision == sourceRevisionBefore,
                "source delete leaves compiled geometry intact until generation and undoes through source history");
}

bool duplicateGeneratedWorldLayoutOutputEditsItsSource() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Generated Source Duplicate");
  static_cast<void>(document.assignId(4102U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "generated_duplicate_source");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Room));
  const app::CreativeEditorWorldLayoutEditReceipt began =
      app::applyCreativeEditorWorldLayoutGesture(
          editor.worldLayout,
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0});
  const app::CreativeEditorWorldLayoutEditReceipt committed =
      app::applyCreativeEditorWorldLayoutGesture(
          editor.worldLayout,
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {6.0, 4.0});
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};
  const app::CreativeDesktopCommandResult generated =
      dispatchOne(app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!began.accepted || !committed.accepted || !generated.accepted) {
    return expect(false, "generated source duplicate setup accepted");
  }

  const auto generatedRoomObject = std::find_if(
      appState.facade.document().objects().begin(),
      appState.facade.document().objects().end(),
      [&](const cr::CreativeObject& object) {
        const cr::CreativeWorldLayoutObjectProvenance provenance =
            cr::resolveCreativeWorldLayoutObjectProvenance(
                editor.worldLayout.source, object);
        return provenance.owned &&
               provenance.table == cr::CreativeWorldLayoutTable::Room &&
               provenance.index == 0U;
      });
  if (generatedRoomObject == appState.facade.document().objects().end()) {
    return expect(false, "generated source duplicate found room output");
  }

  selectPrimary(appState.facade, generatedRoomObject->id);
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::size_t documentObjectCountBefore =
      appState.facade.document().objectCount();
  const std::uint64_t documentUndoBefore =
      cr::creativeUndoDepth(appState.history);
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::size_t sourceUndoBefore =
      editor.worldLayout.sourceHistory.undoEntries.size();
  const std::size_t roomCountBefore =
      editor.worldLayout.source.rooms.size();

  const app::CreativeDesktopCommandResult duplicated =
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context);
  const bool sourceOnlyDuplicate =
      duplicated.accepted && duplicated.changed &&
      duplicated.worldLayoutChanged && !duplicated.sceneChanged &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      editor.worldLayout.generatedRevision != editor.worldLayout.revision &&
      editor.worldLayout.source.rooms.size() == roomCountBefore + 1U &&
      editor.worldLayout.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Room &&
      editor.worldLayout.selection.index == roomCountBefore &&
      editor.worldLayout.sourceHistory.undoEntries.size() ==
          sourceUndoBefore + 1U &&
      appState.facade.document().revision() == documentRevisionBefore &&
      appState.facade.document().objectCount() == documentObjectCountBefore &&
      cr::creativeUndoDepth(appState.history) == documentUndoBefore;
  const app::CreativeDesktopCommandResult staleDuplicate =
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context);
  const bool staleRejected =
      !staleDuplicate.accepted && !staleDuplicate.changed &&
      editor.worldLayout.source.rooms.size() == roomCountBefore + 1U &&
      editor.worldLayout.sourceHistory.undoEntries.size() ==
          sourceUndoBefore + 1U &&
      appState.facade.document().revision() == documentRevisionBefore &&
      appState.facade.document().objectCount() == documentObjectCountBefore;
  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);

  return expect(
             sourceOnlyDuplicate,
             "3D generated duplicate routes to the World Layout source") &&
         expect(staleRejected,
                "stale generated output cannot duplicate source twice") &&
         expect(
             undone.accepted && undone.changed &&
                 editor.worldLayout.revision == sourceRevisionBefore &&
                 editor.worldLayout.source.rooms.size() == roomCountBefore &&
                 editor.worldLayout.selection.kind ==
                     app::CreativeEditorWorldLayoutSelectionKind::Room &&
                 editor.worldLayout.selection.index == 0U &&
                 editor.worldLayout.sourceHistory.undoEntries.size() ==
                     sourceUndoBefore &&
                 appState.facade.document().revision() ==
                     documentRevisionBefore &&
                 appState.facade.document().objectCount() ==
                     documentObjectCountBefore &&
                 cr::creativeUndoDepth(appState.history) ==
                     documentUndoBefore,
             "source duplicate leaves compiled geometry intact and undoes through source history");
}

bool generatedWorldObjectGenericEditsRespectSourceOwnership() {
  bool renameRouted = false;
  {
    cr::CreativeAppState appState;
    cr::CreativeDocument document =
        cr::CreativeDocument::create("Generated Source Rename");
    static_cast<void>(document.assignId(4103U));
    static_cast<void>(appState.facade.installDocument(std::move(document)));
    app::CreativeEditorState editor;
    const cr::CreativeObjectId generatedId =
        addSynchronizedGeneratedWorldObject(
            appState, editor.worldLayout, "generated_source_rename");
    appState.history = {};
    const std::uint64_t documentRevisionBefore =
        appState.facade.document().revision();
    const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
    std::string saveId = "unused";
    const app::CreativeDesktopCommandContext context{
        appState, editor, std::filesystem::path{}, &saveId};

    const app::CreativeDesktopCommandResult renamed = dispatchPayload(
        app::CreativeDesktopCommandId::RenameObject, context,
        app::CreativeDesktopRenamePayload{generatedId, "Renamed Source"});
    const cr::CreativeObject* compiled =
        appState.facade.findObject(generatedId);
    renameRouted =
        renamed.accepted && renamed.changed && renamed.worldLayoutChanged &&
        editor.worldLayout.source.objects[0].name == "Renamed Source" &&
        editor.worldLayout.revision == sourceRevisionBefore + 1U &&
        editor.worldLayout.generatedRevision == sourceRevisionBefore &&
        compiled != nullptr && compiled->name == "Compiled Crate" &&
        appState.facade.document().revision() == documentRevisionBefore &&
        cr::creativeUndoDepth(appState.history) == 0U;
  }

  bool flagsRouted = false;
  {
    cr::CreativeAppState appState;
    cr::CreativeDocument document =
        cr::CreativeDocument::create("Generated Source Flags");
    static_cast<void>(document.assignId(4104U));
    static_cast<void>(appState.facade.installDocument(std::move(document)));
    app::CreativeEditorState editor;
    const cr::CreativeObjectId generatedId =
        addSynchronizedGeneratedWorldObject(
            appState, editor.worldLayout, "generated_source_flags");
    appState.history = {};
    const std::uint64_t documentRevisionBefore =
        appState.facade.document().revision();
    const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
    std::string saveId = "unused";
    const app::CreativeDesktopCommandContext context{
        appState, editor, std::filesystem::path{}, &saveId};

    const app::CreativeDesktopCommandResult locked = dispatchPayload(
        app::CreativeDesktopCommandId::SetObjectsLocked, context,
        app::CreativeDesktopObjectFlagPayload{{generatedId}, true});
    const app::CreativeDesktopCommandResult hidden = dispatchPayload(
        app::CreativeDesktopCommandId::SetObjectsVisible, context,
        app::CreativeDesktopObjectFlagPayload{{generatedId}, false});
    const cr::CreativeObject* compiled =
        appState.facade.findObject(generatedId);
    flagsRouted =
        !locked.accepted && !locked.changed && hidden.accepted &&
        hidden.changed && hidden.worldLayoutChanged &&
        !editor.worldLayout.source.objects[0].visible &&
        editor.worldLayout.revision == sourceRevisionBefore + 1U &&
        compiled != nullptr && compiled->visible && !compiled->locked &&
        appState.facade.document().revision() == documentRevisionBefore &&
        cr::creativeUndoDepth(appState.history) == 0U;
  }

  bool staleRejected = false;
  {
    cr::CreativeAppState appState;
    cr::CreativeDocument document =
        cr::CreativeDocument::create("Generated Source Stale");
    static_cast<void>(document.assignId(4105U));
    static_cast<void>(appState.facade.installDocument(std::move(document)));
    app::CreativeEditorState editor;
    const cr::CreativeObjectId generatedId =
        addSynchronizedGeneratedWorldObject(
            appState, editor.worldLayout, "generated_source_stale");
    selectPrimary(appState.facade, generatedId);
    appState.history = {};
    ++editor.worldLayout.revision;
    const std::uint64_t documentRevisionBefore =
        appState.facade.document().revision();
    const std::size_t objectCountBefore =
        appState.facade.document().objectCount();
    std::string saveId = "unused";
    const app::CreativeDesktopCommandContext context{
        appState, editor, std::filesystem::path{}, &saveId};

    const app::CreativeDesktopCommandResult deleted =
        dispatchOne(app::CreativeDesktopCommandId::DeleteSelection, context);
    staleRejected =
        !deleted.accepted && !deleted.changed &&
        appState.facade.findObject(generatedId) != nullptr &&
        appState.facade.document().revision() == documentRevisionBefore &&
        appState.facade.document().objectCount() == objectCountBefore &&
        cr::creativeUndoDepth(appState.history) == 0U;
  }

  return expect(renameRouted,
                "generated rename edits source without touching output") &&
         expect(flagsRouted,
                "generated visibility edits source while lock rejects") &&
         expect(staleRejected,
                "stale generated output rejects source mutation atomically");
}

bool generatedOutputRejectsStructuralDesktopMutations() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Generated Structural Mutations");
  static_cast<void>(document.assignId(4112U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  const cr::CreativeObjectId generatedId =
      addSynchronizedGeneratedWorldObject(
          appState, editor.worldLayout, "generated_structural_mutations",
          cr::CreativeObjectKind::MovingPlatform);
  const cr::CreativeObjectId authoredId =
      createCrate(appState.facade, 4.0);
  selectPrimary(appState.facade, generatedId);
  appState.history = {};
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};

  const std::array<app::CreativeDesktopCommandResult, 9U> rejected{
      dispatchPayload(
          app::CreativeDesktopCommandId::SetLogicLink, context,
          app::CreativeDesktopLogicLinkPayload{
              generatedId, authoredId,
              cr::CreativeLogicLinkAction::Toggle}),
      dispatchPayload(
          app::CreativeDesktopCommandId::RemoveLogicLink, context,
          app::CreativeDesktopLogicLinkPayload{authoredId, generatedId}),
      dispatchPayload(
          app::CreativeDesktopCommandId::SetGroupPivot, context,
          app::CreativeDesktopGroupPivotPayload{generatedId, {1.0, 2.0, 3.0}}),
      dispatchPayload(
          app::CreativeDesktopCommandId::SetMovingPlatformSettings, context,
          app::CreativeDesktopMovingPlatformPayload{generatedId, {}}),
      dispatchPayload(
          app::CreativeDesktopCommandId::SetPlayerSpawnSettings, context,
          app::CreativeDesktopPlayerSpawnPayload{generatedId, {}}),
      dispatchPayload(
          app::CreativeDesktopCommandId::SetNpcSpawnSettings, context,
          app::CreativeDesktopNpcSpawnPayload{generatedId, {}}),
      dispatchPayload(
          app::CreativeDesktopCommandId::SetLootPointSettings, context,
          app::CreativeDesktopLootPointPayload{generatedId, {}}),
      dispatchPayload(
          app::CreativeDesktopCommandId::SetExitPointSettings, context,
          app::CreativeDesktopExitPointPayload{generatedId, {}}),
      dispatchPayload(
          app::CreativeDesktopCommandId::SetMovingPlatformWaypointDwell,
          context,
          app::CreativeDesktopMovingPlatformWaypointPayload{
              generatedId, 0U, 1.0})};
  const bool allRejected =
      std::all_of(rejected.begin(), rejected.end(),
                  [](const app::CreativeDesktopCommandResult& result) {
                    return !result.accepted && !result.changed &&
                           !result.sceneChanged &&
                           result.affectedObjectCount == 0U &&
                           result.message.find(
                               "creative_semantic_action_world_layout_owned") !=
                               std::string::npos;
                  });

  return expect(allRejected,
                "generated outputs reject every structural desktop mutation") &&
         expect(appState.facade.document().revision() ==
                        documentRevisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "rejected structural mutations preserve document history") &&
         expect(editor.worldLayout.revision == sourceRevisionBefore,
                "rejected structural mutations preserve source history");
}

bool mixedOwnershipDeleteRejectsAtomically() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Mixed Ownership Delete");
  static_cast<void>(document.assignId(4106U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  const cr::CreativeObjectId generatedId =
      addSynchronizedGeneratedWorldObject(
          appState, editor.worldLayout, "mixed_ownership_delete");
  const cr::CreativeObjectId authoredId =
      createCrate(appState.facade, 3.0);
  appState.history = {};
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::size_t objectCountBefore =
      appState.facade.document().objectCount();
  const std::size_t sourceUndoBefore =
      editor.worldLayout.sourceHistory.undoEntries.size();
  const std::array objectIds{generatedId, authoredId};
  const app::CreativeEditorDeleteReceipt deleted =
      app::deleteCreativeEditorObjectsWithUndo(
          appState, objectIds, "desktop_delete_objects", &appState.history,
          &editor.worldLayout);

  return expect(!deleted.accepted && !deleted.changed,
                "mixed ownership delete is rejected") &&
         expect(appState.facade.findObject(generatedId) != nullptr &&
                    appState.facade.findObject(authoredId) != nullptr &&
                    appState.facade.document().objectCount() ==
                        objectCountBefore &&
                    appState.facade.document().revision() ==
                        documentRevisionBefore,
                "mixed ownership delete removes no document objects") &&
         expect(editor.worldLayout.revision == sourceRevisionBefore &&
                    editor.worldLayout.sourceHistory.undoEntries.size() ==
                        sourceUndoBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "mixed ownership delete records no source or document history");
}

bool patternOutputRejectsGenericAndStructuralEdits() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Pattern Output Generic Edits");
  static_cast<void>(document.assignId(4107U));
  cr::CreativeDocumentCreateRequest sourceRequest;
  sourceRequest.kind = cr::CreativeObjectKind::Crate;
  sourceRequest.name = "Pattern Source";
  const cr::CreativeObjectId sourceId =
      document.createObject(sourceRequest).objectId;
  cr::CreativeDocumentCreateRequest generatedRequest;
  generatedRequest.kind = cr::CreativeObjectKind::MovingPlatform;
  generatedRequest.name = "Pattern Output";
  generatedRequest.transform.position = {1.0, 0.5, 2.0};
  generatedRequest.hasTransformOverride = true;
  generatedRequest.bounds = {{0.0, 0.25, 1.0}, {2.0, 0.75, 3.0}};
  generatedRequest.hasBoundsOverride = true;
  generatedRequest.pathPoints = {{{1.0, 0.5, 2.0}},
                                 {{1.0, 3.5, 2.0}}};
  generatedRequest.hasPathOverride = true;
  const cr::CreativeObjectId generatedId =
      document.createObject(generatedRequest).objectId;
  cr::CreativePatternRecipeMutationRequest recipeRequest;
  recipeRequest.kind = cr::CreativePatternRecipeMutationKind::Add;
  recipeRequest.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  recipeRequest.recipe.sourceObjectIds = {sourceId};
  recipeRequest.recipe.generatedObjectIds = {generatedId};
  const cr::CreativePatternRecipeMutationReceipt recipe =
      document.applyPatternRecipeMutation(recipeRequest);

  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  appState.history = {};
  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();
  const cr::CreativeTransform transformBefore =
      appState.facade.findObject(generatedId)->transform;
  cr::CreativeTransform requested = transformBefore;
  requested.position.x += 4.0;

  const app::CreativeDesktopCommandResult renamed = dispatchPayload(
      app::CreativeDesktopCommandId::RenameObject, context,
      app::CreativeDesktopRenamePayload{generatedId, "Raw Rename"});
  const app::CreativeDesktopCommandResult hidden = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsVisible, context,
      app::CreativeDesktopObjectFlagPayload{{generatedId}, false});
  const app::CreativeDesktopCommandResult locked = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsLocked, context,
      app::CreativeDesktopObjectFlagPayload{{generatedId}, true});
  const app::CreativeDesktopCommandResult transformed = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{
          generatedId, requested, true, false, false});
  cr::CreativeMovingPlatformSettings settings;
  settings.speedMetersPerSecond = 3.0;
  const app::CreativeDesktopCommandResult settingsChanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformSettings, context,
      app::CreativeDesktopMovingPlatformPayload{generatedId, settings});
  const cr::CreativeObject* output =
      appState.facade.findObject(generatedId);

  return expect(recipe.accepted && recipe.changed,
                "pattern output ownership fixture is valid") &&
         expect(!renamed.accepted && !hidden.accepted && !locked.accepted &&
                    !transformed.accepted && !settingsChanged.accepted &&
                    settingsChanged.message ==
                        "creative_semantic_action_pattern_owned",
                "pattern output rejects generic and structural mutations") &&
         expect(output != nullptr && output->name == "Pattern Output" &&
                    output->visible && !output->locked &&
                    cr::creativeVec3ExactlyEqual(
                        output->transform.position,
                        transformBefore.position) &&
                    output->movingPlatform ==
                        cr::CreativeMovingPlatformSettings{},
                "rejected pattern edits preserve object state") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U &&
                    appState.facade.document()
                            .patternRecipeStore()
                            .recipes.size() == 1U,
                "rejected pattern edits preserve revision, history, and recipe");
}

bool explicitPatternDeleteUsesSemanticKernel() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Pattern Output Semantic Delete");
  static_cast<void>(document.assignId(4108U));
  cr::CreativeDocumentCreateRequest sourceRequest;
  sourceRequest.kind = cr::CreativeObjectKind::Crate;
  sourceRequest.name = "Pattern Source";
  const cr::CreativeObjectId sourceId =
      document.createObject(sourceRequest).objectId;
  cr::CreativeDocumentCreateRequest outputRequest;
  outputRequest.kind = cr::CreativeObjectKind::Crate;
  outputRequest.name = "Pattern Output A";
  const cr::CreativeObjectId outputA =
      document.createObject(outputRequest).objectId;
  outputRequest.name = "Pattern Output B";
  const cr::CreativeObjectId outputB =
      document.createObject(outputRequest).objectId;
  cr::CreativePatternRecipeMutationRequest recipeRequest;
  recipeRequest.kind = cr::CreativePatternRecipeMutationKind::Add;
  recipeRequest.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  recipeRequest.recipe.sourceObjectIds = {sourceId};
  recipeRequest.recipe.generatedObjectIds = {outputA, outputB};
  const cr::CreativePatternRecipeMutationReceipt recipe =
      document.applyPatternRecipeMutation(recipeRequest);

  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  appState.history = {};
  const std::array objectIds{outputA};
  const app::CreativeEditorDeleteReceipt deleted =
      app::deleteCreativeEditorObjectsWithUndo(
          appState, objectIds, "desktop_delete_objects", &appState.history);

  return expect(recipe.accepted && recipe.changed,
                "explicit pattern delete fixture is valid") &&
         expect(deleted.accepted && deleted.changed,
                "explicit pattern delete is accepted") &&
         expect(appState.facade.findObject(sourceId) == nullptr &&
                    appState.facade.findObject(outputA) == nullptr &&
                    appState.facade.findObject(outputB) == nullptr &&
                    appState.facade.document().objectCount() == 0U &&
                    appState.facade.document()
                        .patternRecipeStore()
                        .recipes.empty(),
                "semantic delete removes the editable recipe closure") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "semantic pattern delete records one undo entry");
}

bool undoRedoMoveTheHistoryRings() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Undo");
  static_cast<void>(document.assignId(412U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  static_cast<void>(
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context));
  const bool recorded = cr::creativeUndoDepth(appState.history) == 1U;
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undone = cr::creativeUndoDepth(appState.history) == 0U;
  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redone = cr::creativeUndoDepth(appState.history) == 1U;

  return expect(recorded, "duplicate records one undo step") &&
         expect(undo.accepted && undone, "undo command pops the ring") &&
         expect(redo.accepted && redone, "redo command restores the ring");
}

bool worldLayoutSourceUndoRedoRoutesThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Layout Source History");
  static_cast<void>(document.assignId(421U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "desktop_source_history");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {5, 4}}, 0.0, 3U, 0.25, 1U});
  const auto preview = app::previewCreativeEditorWorldLayout(
      editor.worldLayout, appState.facade.document());
  const std::uint64_t documentRevision =
      appState.facade.document().revision();

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undone =
      undo.accepted && undo.changed && undo.worldLayoutChanged &&
      undo.sceneChanged && editor.worldLayout.source.buildings.empty() &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      app::creativeEditorWorldLayoutSourceRedoAvailable(editor.worldLayout) &&
      appState.facade.document().revision() == documentRevision;

  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redone =
      redo.accepted && redo.changed && redo.worldLayoutChanged &&
      !redo.sceneChanged && editor.worldLayout.source.buildings.size() == 1U &&
      app::creativeEditorWorldLayoutSourceUndoAvailable(editor.worldLayout) &&
      appState.facade.document().revision() == documentRevision;

  return expect(shell.accepted && shell.changed && preview.accepted,
                "World Layout desktop history fixture is valid") &&
         expect(undone,
                "desktop Undo restores source and invalidates its preview") &&
         expect(redone,
                "desktop Redo restores source without touching the document");
}

bool saveAsRebindsTheActiveSlotAndPreservesHistory() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_desktop_cmd_tests";
  std::filesystem::remove_all(saveRoot);
  std::filesystem::create_directories(saveRoot);

  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Save");
  static_cast<void>(document.assignId(413U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{},
      "save_seed_one"));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{},
      "save_seed_two"));
  const bool preparedRedo =
      app::undoLastEdit(appState, "save_seed_two_undo") &&
      cr::creativeUndoDepth(appState.history) == 1U &&
      cr::creativeRedoDepth(appState.history) == 1U &&
      appState.facade.document().objectCount() == 2U;

  app::CreativeEditorState editor;
  std::string saveId = "world_start";
  const app::CreativeDesktopCommandContext context{appState, editor, saveRoot,
                                                    &saveId};

  const app::CreativeDesktopCommandResult saveAs =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocumentAs, context,
                  "world_named");
  const bool rebounded = saveId == "world_named";
  const bool historyPreserved =
      cr::creativeUndoDepth(appState.history) == 1U &&
      cr::creativeRedoDepth(appState.history) == 1U;
  const bool liveDocumentAcknowledged =
      appState.facade.document().dirtyFlags() == 0U &&
      editor.persistence.hasSavePoint &&
      editor.persistence.documentId == appState.facade.document().id() &&
      editor.persistence.savedRevision ==
          appState.facade.document().revision() &&
      !app::creativeEditorDocumentDirty(editor.persistence,
                                        appState.facade.document());
  const bool redoneAwayFromCheckpoint =
      app::redoLastEdit(appState, "save_checkpoint_redo") &&
      appState.facade.document().objectCount() == 3U &&
      app::creativeEditorDocumentDirty(editor.persistence,
                                       appState.facade.document());
  const bool undoneBackToCheckpoint =
      app::undoLastEdit(appState, "save_checkpoint_undo") &&
      appState.facade.document().objectCount() == 2U &&
      !app::creativeEditorDocumentDirty(editor.persistence,
                                        appState.facade.document());

  const app::CreativeDesktopCommandResult emptyName =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocumentAs, context,
                  std::string{});

  std::filesystem::remove_all(saveRoot);
  return expect(preparedRedo && saveAs.accepted && rebounded,
                "save as accepts and rebinds the active save id") &&
         expect(historyPreserved,
                "save preserves both undo and redo history") &&
         expect(liveDocumentAcknowledged,
                "save as acknowledges the exact durable document content") &&
         expect(redoneAwayFromCheckpoint && undoneBackToCheckpoint,
                "history moves away from and back to the saved content") &&
         expect(!emptyName.accepted,
                "save as with an empty name is rejected");
}

bool failedSaveAndOpenPreserveLiveState() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() /
      "iggy3d_desktop_failed_persistence_tests";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Persistence Failure");
  static_cast<void>(document.assignId(414U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{},
      "seed"));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "failed_persistence");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {5, 4}}, 0.0, 3U, 0.25, 1U});
  app::markCreativeEditorDocumentSaved(
      editor.persistence, appState.facade.document());
  static_cast<void>(createCrate(appState.facade, 4.0));

  const cr::CreativeDocumentId documentIdBefore =
      appState.facade.document().id();
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const cr::CreativeObjectDirtyFlags dirtyFlagsBefore =
      appState.facade.document().dirtyFlags();
  const std::vector<cr::CreativeObjectId> objectIdsBefore =
      documentObjectIds(appState.facade.document());
  const std::size_t undoDepthBefore =
      cr::creativeUndoDepth(appState.history);
  const std::size_t buildingCountBefore =
      editor.worldLayout.source.buildings.size();
  const std::uint64_t layoutRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t layoutEpochBefore = editor.worldLayout.sourceEpoch;
  const app::CreativeEditorPersistenceState persistenceBefore =
      editor.persistence;

  std::string saveId;
  const app::CreativeDesktopCommandContext context{
      appState, editor, saveRoot, &saveId};
  const app::CreativeDesktopCommandResult save =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocument, context);

  saveId = "missing_scene";
  const app::CreativeDesktopCommandResult open =
      dispatchOne(app::CreativeDesktopCommandId::OpenDocument, context);

  const bool liveStatePreserved =
      appState.facade.document().id() == documentIdBefore &&
      appState.facade.document().revision() == documentRevisionBefore &&
      appState.facade.document().dirtyFlags() == dirtyFlagsBefore &&
      documentObjectIds(appState.facade.document()) == objectIdsBefore &&
      cr::creativeUndoDepth(appState.history) == undoDepthBefore &&
      editor.worldLayout.source.buildings.size() == buildingCountBefore &&
      editor.worldLayout.revision == layoutRevisionBefore &&
      editor.worldLayout.sourceEpoch == layoutEpochBefore &&
      editor.persistence.documentId == persistenceBefore.documentId &&
      editor.persistence.savedRevision == persistenceBefore.savedRevision &&
      editor.persistence.savedFingerprint ==
          persistenceBefore.savedFingerprint &&
      editor.persistence.hasSavePoint == persistenceBefore.hasSavePoint;

  std::filesystem::remove_all(saveRoot, error);
  return expect(shell.accepted && shell.changed,
                "failed persistence fixture has a World Layout source") &&
         expect(!save.accepted && !save.changed,
                "invalid save id is rejected") &&
         expect(!open.accepted && !open.changed &&
                    !open.documentReplaced,
                "missing open target is rejected") &&
         expect(liveStatePreserved,
                "failed save and open preserve document, source, history, and "
                "save point");
}

bool semanticPersistenceCommandsShareDocumentDispatcher() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() /
      "iggy3d_keyboard_persistence_tests";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);
  std::filesystem::create_directories(saveRoot, error);

  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Keyboard Persistence");
  static_cast<void>(document.assignId(415U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{},
      "keyboard_save_seed"));

  app::CreativeEditorState editor;
  const std::string saveId = "keyboard_scene";
  const auto route = [](cr::CreativeInputActionId action) {
    cr::CreativeInputRouteResult routed;
    routed.context = cr::CreativeInputContext::EditorViewport;
    routed.actions[0].action = action;
    routed.actionCount = 1U;
    return routed;
  };

  app::applyCreativeEditorCommandInput(
      route(cr::CreativeInputActionId::Undo), appState, editor, saveRoot,
      saveId);
  const bool semanticUndo =
      appState.facade.document().objectCount() == 1U &&
      cr::creativeUndoDepth(appState.history) == 0U &&
      editor.desktopUi.statusMessage == "undo";
  app::applyCreativeEditorCommandInput(
      route(cr::CreativeInputActionId::Redo), appState, editor, saveRoot,
      saveId);
  const bool semanticRedo =
      appState.facade.document().objectCount() == 2U &&
      cr::creativeUndoDepth(appState.history) == 1U &&
      editor.desktopUi.statusMessage == "redo";

  app::applyCreativeEditorCommandInput(
      route(cr::CreativeInputActionId::Save), appState, editor, saveRoot,
      saveId);
  const bool saved =
      appState.facade.document().objectCount() == 2U &&
      appState.facade.document().dirtyFlags() == 0U &&
      cr::creativeUndoDepth(appState.history) == 1U &&
      editor.persistence.hasSavePoint &&
      !app::creativeEditorDocumentDirty(editor.persistence,
                                        appState.facade.document()) &&
      editor.desktopUi.statusMessage == "saved " + saveId;
  const bool undoneAwayFromCheckpoint =
      app::undoLastEdit(appState, "keyboard_saved_undo") &&
      appState.facade.document().objectCount() == 1U &&
      app::creativeEditorDocumentDirty(editor.persistence,
                                       appState.facade.document());
  const bool redoneBackToCheckpoint =
      app::redoLastEdit(appState, "keyboard_saved_redo") &&
      appState.facade.document().objectCount() == 2U &&
      !app::creativeEditorDocumentDirty(editor.persistence,
                                        appState.facade.document());

  selectPrimary(appState.facade, createCrate(appState.facade, 4.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{},
      "keyboard_new_seed"));
  app::applyCreativeEditorCommandInput(
      route(cr::CreativeInputActionId::NewDocument), appState, editor,
      saveRoot, saveId);
  const bool replaced =
      appState.facade.document().objectCount() == 0U &&
      cr::creativeUndoDepth(appState.history) == 0U &&
      !editor.persistence.hasSavePoint &&
      app::creativeEditorDocumentDirty(editor.persistence,
                                       appState.facade.document()) &&
      editor.desktopUi.statusMessage == "new document";

  app::applyCreativeEditorCommandInput(
      route(cr::CreativeInputActionId::Load), appState, editor, saveRoot,
      saveId);
  const bool loaded =
      appState.facade.document().objectCount() == 2U &&
      appState.facade.document().dirtyFlags() == 0U &&
      cr::creativeUndoDepth(appState.history) == 0U &&
      editor.persistence.hasSavePoint &&
      !app::creativeEditorDocumentDirty(editor.persistence,
                                        appState.facade.document()) &&
      editor.desktopUi.statusMessage == "opened " + saveId;

  std::filesystem::remove_all(saveRoot, error);
  return expect(semanticUndo && semanticRedo,
                "semantic Undo and Redo use the desktop document owner") &&
         expect(!error && saved && undoneAwayFromCheckpoint &&
                    redoneBackToCheckpoint,
                "semantic Save preserves history and tracks saved content") &&
         expect(replaced,
                "semantic New clears history and creates an unsaved document") &&
         expect(loaded,
                "semantic Open restores content and the shared save point");
}

bool semanticAssetUndoKeepsRootWorldLayoutHistorySeparate() {
  cr::CreativeAppState rootAppState;
  cr::CreativeDocument rootDocument =
      cr::CreativeDocument::create("Command Root");
  static_cast<void>(rootDocument.assignId(416U));
  static_cast<void>(
      rootAppState.facade.installDocument(std::move(rootDocument)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "command_asset_history_root");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {4, 4}}, 0.0, 3U, 0.25, 1U});
  const std::size_t rootSourceUndoBefore =
      editor.worldLayout.sourceHistory.undoEntries.size();

  cr::CreativeDocument assetDocument =
      cr::CreativeDocument::create("Command Asset");
  static_cast<void>(assetDocument.assignId(417U));
  static_cast<void>(
      editor.assetEdit.workspace.facade.installDocument(
          std::move(assetDocument)));
  selectPrimary(editor.assetEdit.workspace.facade,
                createCrate(editor.assetEdit.workspace.facade, 0.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      editor.assetEdit.workspace, editor.assetEdit.workspace.history,
      cr::CreativeDuplicateCommandRequest{}, "command_asset_seed"));
  editor.assetEdit.active = true;

  const auto route = [](cr::CreativeInputActionId action) {
    cr::CreativeInputRouteResult routed;
    routed.context = cr::CreativeInputContext::EditorViewport;
    routed.actions[0].action = action;
    routed.actionCount = 1U;
    return routed;
  };
  app::applyCreativeEditorCommandInput(
      route(cr::CreativeInputActionId::Undo), editor.assetEdit.workspace,
      editor, {}, "unused", &rootAppState);
  const bool assetUndone =
      editor.assetEdit.workspace.facade.document().objectCount() == 1U &&
      cr::creativeUndoDepth(editor.assetEdit.workspace.history) == 0U &&
      editor.worldLayout.source.buildings.size() == 1U &&
      editor.worldLayout.sourceHistory.undoEntries.size() ==
          rootSourceUndoBefore;

  app::applyCreativeEditorCommandInput(
      route(cr::CreativeInputActionId::Redo), editor.assetEdit.workspace,
      editor, {}, "unused", &rootAppState);
  const bool assetRedone =
      editor.assetEdit.workspace.facade.document().objectCount() == 2U &&
      cr::creativeUndoDepth(editor.assetEdit.workspace.history) == 1U &&
      editor.worldLayout.sourceHistory.undoEntries.size() ==
          rootSourceUndoBefore;

  return expect(shell.accepted && shell.changed,
                "asset command history fixture has root source history") &&
         expect(assetUndone,
                "semantic asset Undo does not consume root source history") &&
         expect(assetRedone,
                "semantic asset Redo stays in the asset workspace");
}

bool playRefusesInvalidDocumentAndFrameRemainsBounded() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Misc");
  static_cast<void>(document.assignId(414U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult play =
      dispatchOne(app::CreativeDesktopCommandId::Play, context);

  app::CreativeDesktopCommandFrame frame;
  for (std::size_t index = 0U;
       index < app::kCreativeDesktopCommandCapacity + 4U; ++index) {
    frame.push(app::CreativeDesktopCommandId::Undo);
  }

  return expect(!play.accepted &&
                    play.message.starts_with("playtest refused:"),
                "Play refuses an invalid document with a reason") &&
         expect(frame.overflowed &&
                    frame.count == app::kCreativeDesktopCommandCapacity,
                "the command frame is bounded and records overflow");
}

bool playCommandsPreserveProcessAndAuthoringContracts() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() /
      "iggy3d_desktop_play_command_tests";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  cr::CreativeAppState appState;
  cr::CreativeDocument invalidDocument =
      cr::CreativeDocument::create("Invalid Desktop Play");
  static_cast<void>(invalidDocument.assignId(415U));
  const bool invalidInstalled =
      appState.facade.installDocument(std::move(invalidDocument)).accepted;

  app::CreativeEditorState editor;
  iggy3d::StaticMeshAssetCatalog catalog;
  FakePlaytestProcessControl process;
  process.running_ = true;
  process.observedSnapshotPath =
      saveRoot / std::string(app::kPlaytestSnapshotDirName) /
      (std::string(app::kPlaytestSnapshotSaveId) + ".iggy3d.save");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, saveRoot, &saveId, &catalog, &process};

  const app::CreativeDesktopCommandResult invalidPlay =
      dispatchOne(app::CreativeDesktopCommandId::Play, context);
  const bool invalidPlayPreservedChild =
      !invalidPlay.accepted &&
      invalidPlay.message.starts_with("playtest refused:") &&
      process.running_ && process.stopCount == 0U &&
      process.launchCount == 0U && process.nameInstallCount == 0U &&
      process.operations.empty() &&
      !std::filesystem::exists(process.observedSnapshotPath);

  cr::CreativeDocument document =
      cr::CreativeDocument::create("Desktop Play Commands");
  static_cast<void>(document.assignId(416U));

  cr::CreativeDocumentCreateRequest floor;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.name = "Play Floor";
  floor.bounds = {{-8.0, 0.0, -8.0}, {8.0, 0.25, 8.0}};
  floor.hasBoundsOverride = true;
  const bool floorAccepted = document.createObject(floor).accepted;

  cr::CreativeDocumentCreateRequest spawn;
  spawn.kind = cr::CreativeObjectKind::SpawnPoint;
  spawn.name = "Player Spawn";
  spawn.transform.position = {0.0, 0.25, 0.0};
  spawn.hasTransformOverride = true;
  const bool spawnAccepted = document.createObject(spawn).accepted;
  const bool installed =
      appState.facade.installDocument(std::move(document)).accepted;

  editor.playtestWindowPreferences.present = true;
  editor.playtestWindowPreferences.width = 1280U;
  editor.playtestWindowPreferences.height = 720U;

  const app::CreativeDesktopCommandContext noProcessContext{
      appState, editor, saveRoot, &saveId, &catalog};
  const app::CreativeDesktopCommandResult noProcess =
      dispatchOne(app::CreativeDesktopCommandId::PlaytestPause,
                  noProcessContext);

  const app::CreativeDesktopCommandResult launched =
      dispatchOne(app::CreativeDesktopCommandId::Play, context);
  const bool replaceOrder =
      process.operations ==
      std::vector<std::string>{"stop", "launch", "names"};
  bool windowPreferencesPreserved = false;
  for (std::size_t index = 0U; index + 1U < process.lastPlan.argv.size();
       ++index) {
    if (process.lastPlan.argv[index] == "--resolution" &&
        process.lastPlan.argv[index + 1U] == "1280x720") {
      windowPreferencesPreserved = true;
      break;
    }
  }
  const auto playerName = process.snapshotEntityNames.find(1U);
  const bool launchedLatestSnapshot =
      launched.accepted && !launched.changed &&
      launched.message == "playtest launched" && process.running_ &&
      process.stopCount == 1U && process.launchCount == 1U &&
      process.nameInstallCount == 1U && replaceOrder &&
      !process.snapshotPresentAtStop && process.snapshotPresentAtLaunch &&
      process.lastPlan.valid && windowPreferencesPreserved &&
      playerName != process.snapshotEntityNames.end() &&
      playerName->second == "player";

  const app::CreativeDesktopCommandResult paused =
      dispatchOne(app::CreativeDesktopCommandId::PlaytestPause, context);
  const bool pauseSent =
      paused.accepted && paused.message == "pause sent" &&
      process.sendCount == 1U && process.lastVerb == "pause";

  process.sendSucceeds = false;
  process.sendReason = "stalled";
  const app::CreativeDesktopCommandResult resumed =
      dispatchOne(app::CreativeDesktopCommandId::PlaytestResume, context);
  const bool resumeRejected =
      !resumed.accepted &&
      resumed.message == "playtest command rejected: stalled" &&
      process.sendCount == 2U && process.lastVerb == "resume";

  const app::CreativeDesktopCommandResult editWhileChildRuns =
      dispatchOne(app::CreativeDesktopCommandId::NewDocument, context);
  const bool editingStayedLive =
      editWhileChildRuns.accepted && editWhileChildRuns.changed &&
      editWhileChildRuns.documentReplaced && process.running_ &&
      process.stopCount == 1U && process.launchCount == 1U;

  std::filesystem::remove_all(saveRoot, error);
  return expect(invalidInstalled && invalidPlayPreservedChild,
                "invalid Play refuses before disturbing a running child") &&
         expect(floorAccepted && spawnAccepted && installed,
                "play command fixture is valid") &&
         expect(launchedLatestSnapshot,
                "Play stops, snapshots, launches, and installs names in order") &&
         expect(!noProcess.accepted &&
                    noProcess.message ==
                        "playtest command rejected: no child",
                "playtest command rejects a missing process owner") &&
         expect(pauseSent,
                "pause command forwards the exact protocol verb") &&
         expect(resumeRejected,
                "resume command preserves process send failure") &&
         expect(editingStayedLive,
                "Creative editing remains live while i3dp is running");
}

bool commandFramesAccumulatePreviewImpacts() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Cumulative Preview Impact");
  static_cast<void>(document.assignId(4141U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {6, 4}}, 0.0, 3U, 0.25, 1U});
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();

  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::WorldLayoutPreview);
  frame.push(app::CreativeDesktopCommandId::None);
  const app::CreativeDesktopCommandResult result =
      app::dispatchCreativeDesktopCommands(frame, context);

  return expect(shell.accepted && shell.changed,
                "cumulative preview fixture creates a building shell") &&
         expect(result.lastCommand == app::CreativeDesktopCommandId::None &&
                    !result.accepted && !result.changed,
                "command-specific outcome describes the last queued command") &&
         expect(
             result.sceneChanged && !result.documentReplaced &&
                 !result.worldLayoutChanged &&
                 app::creativeDesktopCommandHasImpact(
                     result, app::CreativeDesktopCommandImpact::SceneChanged) &&
                 !app::creativeDesktopCommandHasImpact(
                     result,
                     app::CreativeDesktopCommandImpact::DocumentChanged) &&
                 app::creativeDesktopCommandRequiresSceneRefresh(result) &&
                 app::creativeEditorWorldLayoutPreviewActive(
                     editor.worldLayout) &&
                 appState.facade.document().revision() ==
                     documentRevisionBefore,
             "preview-only scene impact survives a later no-op command");
}

bool commandFramesInferDocumentImpactsFromActiveRevision() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Cumulative Document Impact");
  static_cast<void>(document.assignId(4142U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};
  const std::uint64_t rootRevisionBefore =
      appState.facade.document().revision();

  app::CreativeDesktopCommandFrame rootFrame;
  rootFrame.push(app::CreativeDesktopCommandId::DeleteSelection);
  rootFrame.push(app::CreativeDesktopCommandId::None);
  const app::CreativeDesktopCommandResult rootResult =
      app::dispatchCreativeDesktopCommands(rootFrame, context);
  const bool rootImpact =
      appState.facade.document().objectCount() == 0U &&
      appState.facade.document().revision() > rootRevisionBefore &&
      app::creativeDesktopCommandHasImpact(
          rootResult, app::CreativeDesktopCommandImpact::DocumentChanged) &&
      !rootResult.documentReplaced && !rootResult.sceneChanged &&
      !rootResult.worldLayoutChanged &&
      app::creativeDesktopCommandRequiresSceneRefresh(rootResult);

  cr::CreativeDocument assetDocument =
      cr::CreativeDocument::create("Cmd Active Asset Impact");
  static_cast<void>(assetDocument.assignId(4143U));
  static_cast<void>(
      editor.assetEdit.workspace.facade.installDocument(
          std::move(assetDocument)));
  selectPrimary(editor.assetEdit.workspace.facade,
                createCrate(editor.assetEdit.workspace.facade, 0.0));
  editor.assetEdit.workspace.history = {};
  editor.assetEdit.active = true;
  const std::uint64_t rootRevisionBeforeAssetEdit =
      appState.facade.document().revision();
  const std::uint64_t assetRevisionBefore =
      editor.assetEdit.workspace.facade.document().revision();

  app::CreativeDesktopCommandFrame assetFrame;
  assetFrame.push(app::CreativeDesktopCommandId::DeleteSelection);
  assetFrame.push(app::CreativeDesktopCommandId::None);
  const app::CreativeDesktopCommandResult assetResult =
      app::dispatchCreativeDesktopCommands(assetFrame, context);
  const bool assetImpact =
      editor.assetEdit.workspace.facade.document().objectCount() == 0U &&
      editor.assetEdit.workspace.facade.document().revision() >
          assetRevisionBefore &&
      appState.facade.document().revision() == rootRevisionBeforeAssetEdit &&
      app::creativeDesktopCommandHasImpact(
          assetResult, app::CreativeDesktopCommandImpact::DocumentChanged) &&
      !assetResult.documentReplaced && !assetResult.sceneChanged &&
      !assetResult.worldLayoutChanged &&
      app::creativeDesktopCommandRequiresSceneRefresh(assetResult);

  return expect(rootImpact,
                "root document revision creates a cumulative refresh impact") &&
         expect(assetImpact,
                "active asset document revision creates a cumulative refresh impact");
}

bool desktopCommandOwnershipIsExhaustive() {
  constexpr std::size_t ownerCount =
      static_cast<std::size_t>(app::CreativeDesktopCommandOwner::Invalid);
  std::array<std::size_t, ownerCount> actual{};
  bool allLiveCommandsOwned = true;
  for (std::size_t index = 0U;
       index <
       static_cast<std::size_t>(app::CreativeDesktopCommandId::Count);
       ++index) {
    const app::CreativeDesktopCommandOwner owner =
        app::creativeDesktopCommandOwner(
            static_cast<app::CreativeDesktopCommandId>(index));
    if (owner == app::CreativeDesktopCommandOwner::Invalid) {
      allLiveCommandsOwned = false;
      continue;
    }
    ++actual[static_cast<std::size_t>(owner)];
  }

  constexpr std::array<std::size_t, ownerCount> expected{
      1U, 9U, 25U, 19U, 3U, 13U, 2U,
      5U, 21U, 11U, 5U, 9U, 5U,
  };
  return expect(app::creativeDesktopCommandOwnershipIsExhaustive() &&
                    allLiveCommandsOwned,
                "every live desktop command has one declared owner") &&
         expect(actual == expected,
                "desktop command owner populations match the live handlers") &&
         expect(app::creativeDesktopCommandOwner(
                    app::CreativeDesktopCommandId::Count) ==
                    app::CreativeDesktopCommandOwner::Invalid &&
                    app::creativeDesktopCommandOwner(
                        static_cast<app::CreativeDesktopCommandId>(255U)) ==
                        app::CreativeDesktopCommandOwner::Invalid,
                "sentinel and out-of-range command ids have no owner");
}

// --- Step 3: Desktop Command Expansion -------------------------------------


}  // namespace

bool runCreativeDesktopDocumentCommandTests() {
  bool ok = true;
  ok = newDocumentReplacesAndClearsHistory() && ok;
  ok = builderEstateRegenerationIsExplicitUndoableAndUnsaved() && ok;
  ok = deleteAndDuplicateHitTheKernels() && ok;
  ok = deleteGeneratedWorldLayoutOutputEditsItsSource() && ok;
  ok = duplicateGeneratedWorldLayoutOutputEditsItsSource() && ok;
  ok = generatedWorldObjectGenericEditsRespectSourceOwnership() && ok;
  ok = generatedOutputRejectsStructuralDesktopMutations() && ok;
  ok = mixedOwnershipDeleteRejectsAtomically() && ok;
  ok = patternOutputRejectsGenericAndStructuralEdits() && ok;
  ok = explicitPatternDeleteUsesSemanticKernel() && ok;
  ok = undoRedoMoveTheHistoryRings() && ok;
  ok = worldLayoutSourceUndoRedoRoutesThroughDispatcher() && ok;
  ok = saveAsRebindsTheActiveSlotAndPreservesHistory() && ok;
  ok = failedSaveAndOpenPreserveLiveState() && ok;
  ok = semanticPersistenceCommandsShareDocumentDispatcher() && ok;
  ok = semanticAssetUndoKeepsRootWorldLayoutHistorySeparate() && ok;
  ok = playRefusesInvalidDocumentAndFrameRemainsBounded() && ok;
  ok = playCommandsPreserveProcessAndAuthoringContracts() && ok;
  ok = commandFramesAccumulatePreviewImpacts() && ok;
  ok = commandFramesInferDocumentImpactsFromActiveRevision() && ok;
  ok = desktopCommandOwnershipIsExhaustive() && ok;
  return ok;
}
