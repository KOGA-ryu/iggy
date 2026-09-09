#!/usr/bin/env python3
"""Export readable learning documents and publish one verified library generation.

The target app interprets every document, include, reference and answer. This
module only transports captured bytes and manages immutable releases. POSIX
publication uses an advisory lock, fsync and an atomic active-record replacement.
"""
from __future__ import annotations

import argparse
import contextlib
import copy
import fcntl
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MAX_FILE = 128 * 1024
MAX_METADATA = 4 * 1024 * 1024
MAX_PACKAGE = 16 * 1024 * 1024


class ExportError(ValueError):
    def __init__(self, code, message, diagnostics=None):
        super().__init__(message)
        self.code, self.diagnostics = code, diagnostics or []


def require(condition, code, message):
    if not condition:
        raise ExportError(code, message)


def encoded(value):
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2,
                       allow_nan=False) + "\n").encode("utf-8")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decoded(data):
    def pairs(rows):
        result = {}
        for key, value in rows:
            require(key not in result, "json.duplicate", f"Repeated JSON key: {key}")
            result[key] = value
        return result
    return json.loads(data, object_pairs_hook=pairs,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError(f"Invalid JSON number: {value}")))


def real_path(value):
    path = Path(value).expanduser().absolute()
    for part in (*reversed(path.parents), path):
        require(not part.is_symlink(), "path.symlink", f"Symlinks are not supported: {part}")
    return path.resolve()


def relative(value):
    require(isinstance(value, str) and 0 < len(value.encode()) <= 240,
            "path.invalid", "Inventory paths must be bounded relative POSIX paths")
    path = PurePosixPath(value)
    require(not path.is_absolute() and "\\" not in value and "\0" not in value
            and all(p not in ("", ".", "..") for p in value.split("/"))
            and str(path) == value, "path.invalid", f"Invalid inventory path: {value}")
    return value


def identity(value):
    require(isinstance(value, str) and re.fullmatch(r"[a-z0-9_]{1,64}", value),
            "identity.invalid", "IDs use 1-64 lowercase letters, digits or underscores")
    return value


def read_bytes(path, limit=MAX_METADATA):
    path = real_path(path)
    with path.open("rb") as stream:
        before = os.fstat(stream.fileno())
        require(stat.S_ISREG(before.st_mode) and before.st_size <= limit,
                "file.limit", f"Not a bounded regular file: {path}")
        data = stream.read(limit + 1)
        after = os.fstat(stream.fileno())
    current = path.stat()
    require(len(data) <= limit and (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns, before.st_ctime_ns)
            == (after.st_dev, after.st_ino, len(data), after.st_mtime_ns, after.st_ctime_ns)
            and (after.st_dev, after.st_ino) == (current.st_dev, current.st_ino),
            "file.changed", f"File changed while being captured: {path}")
    real_path(path)
    return data


def capture(root, *, documents=False):
    root = real_path(root)
    require(root.is_dir(), "path.directory", f"Missing directory: {root}")
    files, count, total = {}, 0, 0
    for parent, dirs, names in os.walk(root, followlinks=False):
        for name in sorted(dirs + names):
            path = Path(parent) / name
            count += 1
            require(count <= (2048 if documents else 6144) and not path.is_symlink(),
                    "path.inventory", f"Too many entries or a symlink in {root}")
            if path.is_dir():
                continue
            require(path.is_file(), "path.special", f"Special files cannot be exported: {path}")
            if documents and path.suffix != ".md":
                continue
            name = relative(path.relative_to(root).as_posix())
            data = read_bytes(path, MAX_FILE if documents else MAX_METADATA)
            total += len(data)
            require(total <= (2 * 1024 * 1024 if documents else MAX_PACKAGE),
                    "file.limit", "Captured source exceeds its byte limit")
            files[name] = data
    return dict(sorted(files.items()))


def inventory(files):
    return [dict(path=name, bytes=len(data), sha256=sha(data)) for name, data in sorted(files.items())]


def verify_inventory(files, rows):
    require(isinstance(rows, list) and len(rows) <= 4096, "inventory.invalid", "Invalid file inventory")
    names = [relative(row["path"]) for row in rows]
    require(len(set(names)) == len(names) and set(names) == set(files),
            "inventory.files", "Missing, repeated or unlisted payload files")
    require(rows == inventory(files), "inventory.changed", "Payload bytes differ from the checked inventory")


def write_tree(root, files, *, durable=False):
    root.mkdir(parents=True, exist_ok=True)
    for name, data in sorted(files.items()):
        path = root / relative(name)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as stream:
            stream.write(data)
            if durable:
                stream.flush()
                os.fsync(stream.fileno())
    if durable:
        for directory in sorted((p for p in root.rglob("*") if p.is_dir()), key=lambda p: len(p.parts), reverse=True):
            sync_directory(directory)
        sync_directory(root)


def sync_directory(path):
    descriptor = os.open(path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def durable_directory(path):
    if not path.exists():
        durable_directory(path.parent)
        path.mkdir(exist_ok=True)
        sync_directory(path.parent)


@contextlib.contextmanager
def locked(path):
    path = real_path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor = os.open(path, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    try:
        require(stat.S_ISREG(os.fstat(descriptor).st_mode), "lock.invalid", "Publication lock must be a regular file")
        fcntl.flock(descriptor, fcntl.LOCK_EX)
        yield
    finally:
        os.close(descriptor)


def immutable_directory(destination, files):
    destination = real_path(destination)
    durable_directory(destination.parent)
    if destination.exists():
        require(capture(destination) == files, "release.immutable", f"Existing release has different bytes: {destination}")
        return False
    temporary = Path(tempfile.mkdtemp(prefix=".learning-stage-", dir=destination.parent)).resolve()
    try:
        write_tree(temporary, files, durable=True)
        require(capture(temporary) == files, "file.changed", "Staged files changed before installation")
        os.rename(temporary, destination)
        sync_directory(destination.parent)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)
    return True


class Target:
    def __init__(self, path, library=None):
        self.path = real_path(path)
        self.library = real_path(library) if library else None
        self.fingerprint = sha(read_bytes(self.path, 64 * 1024 * 1024))

    def inspect(self, *, documents=None, store=None):
        require((documents is None) != (store is None), "target.input", "Choose one document source")
        command = [str(self.path), "--inspect-documents", "--documents" if documents else "--document-store", str(documents or store)]
        if self.library:
            command += ["--library", str(self.library)]
        require(sha(read_bytes(self.path, 64 * 1024 * 1024)) == self.fingerprint,
                "target.changed", "Target executable changed; start a fresh publication")
        result = subprocess.run(command, capture_output=True, timeout=60)
        try:
            report = decoded(result.stdout)
        except (ValueError, KeyError):
            raise ExportError("target.protocol", "Target did not return a structured document report: " + result.stderr.decode(errors="replace")) from None
        require(isinstance(report, dict) and report.get("format") == "paths_document_report" and report.get("format_version") == 1,
                "target.protocol", "Unsupported document-report protocol")
        if result.returncode or report.get("accepted") is not True:
            raise ExportError("target.rejected", report.get("message", "Target rejected the content"), report.get("diagnostics"))
        require(sha(read_bytes(self.path, 64 * 1024 * 1024)) == self.fingerprint,
                "target.changed", "Target changed during validation")
        return report

    def inspect_bytes(self, documents):
        with tempfile.TemporaryDirectory(prefix="paths-document-check-") as temporary:
            folder = Path(temporary).resolve()
            write_tree(folder, documents)
            return self.inspect(documents=folder)


def scoped_contract(report, package_id):
    prefix = package_id + "/"
    entities = []
    for entry in report["entities"]:
        if entry["source"]["file"].startswith(prefix):
            entry = copy.deepcopy(entry)
            entry["source"]["file"] = entry["source"]["file"][len(prefix):]
            entities.append(entry)
    provided = {}
    for entry in entities:
        key = (entry["kind"], entry["id"])
        if key in provided:
            require({k: v for k, v in entry.items() if k != "source"}
                    == {k: v for k, v in provided[key].items() if k != "source"},
                    "identity.conflict", f"Conflicting declarations: {key}")
        else:
            provided[key] = entry
    external = set()
    for entry in entities:
        kind = "question" if entry["kind"] == "lesson" else "lesson"
        for link in entry.get("links", []):
            if (kind, link) not in provided:
                external.add(("question" if kind == "question" else "reading", link))
    figures = {encoded(e["figure"]): e["figure"] for e in entities if "figure" in e}
    requirements = dict(templates=sorted({e["template"] for e in entities if "template" in e}),
                        figures=[figures[key] for key in sorted(figures)],
                        catalogue=[dict(kind=kind, id=key) for kind, key in sorted(external)])
    entries = sorted("documents/" + p[len(prefix):] for p in report["entry_documents"] if p.startswith(prefix))
    return dict(provides=[provided[key] for key in sorted(provided)], requires=requirements, entry_documents=entries)


def provenance(author, provided):
    records = author["sources"]
    require(isinstance(records, list) and 0 < len(records) <= 1024, "source.required", "Provide bounded source records")
    known = {e["id"] for e in provided if e["kind"] in ("lesson", "question")}
    ids, covered = set(), set()
    for row in records:
        key = identity(row["id"])
        require(key not in ids and row["kind"] in ("original", "generated", "adapted"), "source.invalid", "Repeated or unknown source record")
        ids.add(key)
        for field in ("title", "uri", "revision", "attribution", "reuse"):
            require(isinstance(row[field], str) and 0 < len(row[field]) <= 2048, "source.invalid", f"Source needs {field}")
        require(not row["uri"].startswith(("/", "file:")), "source.local", "Portable provenance cannot use local file URIs")
        require(isinstance(row["content_ids"], list) and set(row["content_ids"]) <= known,
                "source.identity", "Source record names an unknown lesson/question")
        covered.update(row["content_ids"])
    require(covered == known, "source.missing", "Every exported lesson/question needs a source record")
    return encoded(dict(format="paths_learning_provenance", format_version=1, records=sorted(records, key=lambda r: r["id"])))


def prepare_pack(author_root, target, context):
    author_root = real_path(author_root)
    author = decoded(read_bytes(author_root / "authoring.json"))
    require(author["format"] == "paths_learning_authoring" and type(author["format_version"]) is int and author["format_version"] == 1,
            "author.format", "Unsupported authoring format")
    package_id = identity(author["package_id"])
    require(package_id != "baseline", "identity.reserved", "baseline is reserved for existing documents")
    version = author["package_version"]
    require(type(version) is int and 0 < version <= 2**31 - 1, "release.version", "Package versions are positive 32-bit integers")
    source = capture(author_root / "documents", documents=True)
    prefix = package_id + "/"
    prospective = {p: data for p, data in context.items() if not p.startswith(prefix)}
    prospective.update({prefix + p: data for p, data in source.items()})
    report = target.inspect_bytes(prospective)
    contract = scoped_contract(report, package_id)
    require(contract["entry_documents"], "author.empty", "Authoring folder contains no entry documents")
    payload = {}
    for item in report["files"]:
        if item["path"].startswith(prefix):
            name = item["path"][len(prefix):]
            data = source[name]
            require(item["bytes"] == len(data) and item["sha256"] == sha(data), "source.changed", "Validated bytes differ from captured source")
            payload["documents/" + name] = data
    payload["provenance.json"] = provenance(author, contract["provides"])
    bundle = dict(format="paths_learning_export", format_version=1, package_id=package_id,
                  package_version=version, files=inventory(payload), **contract)
    receipt = dict(format="paths_learning_receipt", format_version=1, accepted=True,
                   target_sha256=target.fingerprint, base_catalogue_sha256=report["base_catalogue_sha256"],
                   checks=["canonical document and typed-answer validation", "captured payload inventory"],
                   diagnostics=[], factual_review="not performed by exporter", visual_acceptance="pending")
    return {**payload, "bundle.json": encoded(bundle), "receipt.json": encoded(receipt)}


def check_pack(files):
    bundle = decoded(files["bundle.json"])
    require(bundle["format"] == "paths_learning_export" and type(bundle["format_version"]) is int and bundle["format_version"] == 1,
            "package.format", "Unsupported export package")
    identity(bundle["package_id"])
    require(bundle["package_id"] != "baseline" and type(bundle["package_version"]) is int and 0 < bundle["package_version"] <= 2**31 - 1,
            "package.identity", "Invalid package identity/version")
    require("receipt.json" in files and "provenance.json" in files, "package.metadata", "Package needs provenance and a receipt")
    payload = {p: data for p, data in files.items() if p not in ("bundle.json", "receipt.json")}
    require(all(p == "provenance.json" or (p.startswith("documents/") and p.endswith(".md") and len(data) <= MAX_FILE)
                for p, data in payload.items()), "package.files", "Unexpected payload file")
    verify_inventory(payload, bundle["files"])
    source = decoded(payload["provenance.json"])
    require(source["format"] == "paths_learning_provenance" and source["format_version"] == 1,
            "source.format", "Unsupported provenance record")
    provenance({"sources": source["records"]}, bundle["provides"])
    return bundle


def documents_in(files):
    return {name[len("documents/"):]: data for name, data in files.items() if name.startswith("documents/")}


def read_state(target, store, baseline):
    active_path = store / "active.json"
    if active_path.exists() or active_path.is_symlink():
        active_bytes = read_bytes(active_path, 4096)
        active = decoded(active_bytes)
        require(active["format"] == "paths_learning_store" and active["format_version"] == 1
                and re.fullmatch(r"[0-9a-f]{64}", active["generation"]), "store.active", "Invalid active record")
        root = store / "generations" / active["generation"]
        files = capture(root)
        require(sha(files["library.json"]) == active["generation"], "store.changed", "Published manifest changed")
        manifest = decoded(files.pop("library.json"))
        require(manifest["format"] == "paths_learning_library" and manifest["format_version"] == 1, "store.format", "Unknown library format")
        verify_inventory(files, manifest["files"])
        report = target.inspect(store=store)
        require(report["catalogue"]["questions"] == manifest["questions"] and read_bytes(active_path, 4096) == active_bytes,
                "store.changed", "Library changed while preparing publication")
        return active_bytes, manifest, files, report
    # A first publication may resume after staging was interrupted, using the
    # explicitly selected current documents. No active library exists to replace.
    source = capture(baseline, documents=True)
    documents = {"baseline/" + name: data for name, data in source.items()}
    report = target.inspect_bytes(documents)
    consumed = {"documents/" + row["path"]: documents[row["path"]] for row in report["files"]}
    return None, dict(packages={}, releases={}, questions=report["catalogue"]["questions"]), consumed, report


def install_proposal(pack, target, state):
    bundle = check_pack(pack)
    active_bytes, previous, current, _ = state
    package_id, version = bundle["package_id"], bundle["package_version"]
    release_hash = sha(pack["bundle.json"])
    key = f"{package_id}:{version}"
    releases = dict(previous["releases"])
    require(key not in releases or releases[key] == release_hash, "release.immutable", "A package ID/version was already used for different bytes")
    installed = previous["packages"].get(package_id)
    require(not installed or version >= installed["version"], "release.old", "Older package releases cannot replace the active release")
    prefixes = (f"documents/{package_id}/", f"packages/{package_id}/")
    files = {p: data for p, data in current.items() if not p.startswith(prefixes)}
    files.update({f"documents/{package_id}/" + p[len("documents/"):]: data for p, data in pack.items() if p.startswith("documents/")})
    files.update({f"packages/{package_id}/" + name: pack[name] for name in ("bundle.json", "provenance.json")})
    report = target.inspect_bytes(documents_in(files))
    expected = scoped_contract(report, package_id)
    require(all(bundle[field] == expected[field] for field in expected), "package.contract", "Package declarations do not match the target compiler's report")
    consumed = {"documents/" + row["path"] for row in report["files"]}
    require(consumed == {p for p in files if p.startswith("documents/")}, "package.closure", "Package contains unused or missing include files")
    questions = report["catalogue"]["questions"]
    stamps = {q["id"]: q["stamp_sha256"] for q in questions}
    for old in previous["questions"]:
        require(stamps.get(old["id"]) == old["stamp_sha256"], "question.changed",
                f"Existing question must remain unchanged: {old['id']}. Author a new ID while retaining the original.")
    packages = dict(previous["packages"])
    packages[package_id] = dict(version=version, bundle_sha256=release_hash)
    releases[key] = release_hash
    manifest = dict(format="paths_learning_library", format_version=1, packages=packages,
                    releases=releases, questions=questions, files=inventory(files))
    files["library.json"] = encoded(manifest)
    return sha(files["library.json"]), files, report


def replace_active(store, generation, expected):
    path = store / "active.json"
    current = read_bytes(path, 4096) if path.exists() else None
    require(current == expected, "store.changed", "Active generation changed; retry against the new library")
    data = encoded(dict(format="paths_learning_store", format_version=1, generation=generation))
    descriptor, temporary = tempfile.mkstemp(prefix=".active-", dir=store)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        try:
            sync_directory(store)
        except OSError as error:
            raise ExportError("store.durability", f"New generation is active but directory durability could not be confirmed: {error}") from error
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def disjoint(destination, inputs):
    for source in inputs:
        if source:
            source = real_path(source)
            require(not destination.is_relative_to(source) and not source.is_relative_to(destination),
                    "path.overlap", f"Output and input roots overlap: {destination}, {source}")


def draft(source, target, destination):
    """Create a mutable preview from one independently compilable authoring package."""
    destination = real_path(destination)
    require(not destination.exists(), "draft.exists", f"Draft already exists; keep editing it or choose a new output: {destination}")
    disjoint(destination, [source, target.path, target.library, ROOT / "build/question-batches",
                          ROOT / "build/exports", ROOT / "content/write",
                          target.path.parent / "content", target.path.parent / "learning-store"])
    for parent in destination.parents:
        require(not any((parent / name).exists() for name in
                        ("authoring.json", "authoring/authoring.json", "bundle.json", "library.json", "active.json")),
                "draft.location", f"Keep drafts outside authoring releases, exports and library stores: {parent}")
    # The same compiler selects the complete include closure; no Markdown parser
    # or published-library dependency is introduced for a session-only preview.
    pack = prepare_pack(source, target, {})
    bundle = decoded(pack["bundle.json"])
    documents = {name: data for name, data in pack.items() if name.startswith("documents/")}
    origin = dict(format="paths_learning_draft", format_version=1, preview_only=True,
                  source=str(source), package_id=bundle["package_id"], package_version=bundle["package_version"],
                  origin_files=inventory(documents), sources=decoded(pack["provenance.json"])["records"],
                  note="Origin snapshot only, not validation of later edits. Preview does not publish or load personal progress.")
    with locked(ROOT / "build/draft-locks" / (sha(str(destination).encode("utf-8")) + ".lock")):
        require(not destination.exists(), "draft.exists", f"Draft already exists; it was not overwritten: {destination}")
        immutable_directory(destination, {**documents, "draft.json": encoded(origin)})
    command = [str(target.path), "--documents", str(destination / "documents"), "--watch-documents"]
    if target.library:
        command += ["--library", str(target.library)]
    return dict(accepted=True, published=False, preview_only=True, draft=str(destination),
                documents=str(destination / "documents"),
                edit_files=[str(destination / name) for name in bundle["entry_documents"]],
                questions=sum(e["kind"] == "question" for e in bundle["provides"]),
                readings=sum(e["kind"] == "lesson" for e in bundle["provides"]),
                target_sha256=target.fingerprint, preview_argv=command, preview_command=shlex.join(command))


def run(args):
    target = Target(args.target, args.library)
    source = real_path(args.source)
    if args.command == "draft":
        return draft(source, target, args.output)
    store = real_path(args.store or target.path.parent / "learning-store")
    baseline = real_path(args.base_documents or target.path.parent / "content/write")
    disjoint(store, [source, baseline, target.library])
    with locked(store.parent / ("." + store.name + ".publish.lock")):
        state = read_state(target, store, baseline)
        pack = capture(source) if args.command == "install" else prepare_pack(source, target, documents_in(state[2]))
        bundle = check_pack(pack)
        output = real_path(args.output or ROOT / "build/exports" / bundle["package_id"] / str(bundle["package_version"]))
        if args.command != "install":
            disjoint(output, [source, baseline, store, target.library])
            with locked(output.parent / (".export-" + output.name + ".lock")):
                if output.exists():
                    existing = capture(output)
                    # A new target receipt may differ; immutable payload identity
                    # is the manifest and the exact files it inventories.
                    check_pack(existing)
                    require({p: b for p, b in existing.items() if p != "receipt.json"}
                            == {p: b for p, b in pack.items() if p != "receipt.json"},
                            "release.immutable", f"Existing export differs: {output}")
                else:
                    immutable_directory(output, pack)
        result = dict(accepted=True, package_id=bundle["package_id"], package_version=bundle["package_version"],
                      package=str(source if args.command == "install" else output), target_sha256=target.fingerprint)
        if args.command == "export":
            result["published"] = False
            return result
        generation, files, report = install_proposal(pack, target, state)
        immutable_directory(store / "generations" / generation, files)
        # Read back through the actual startup boundary before changing active.json.
        with tempfile.TemporaryDirectory(prefix="paths-store-check-") as temporary:
            probe = Path(temporary).resolve()
            write_tree(probe / "generations" / generation, files)
            write_tree(probe, {"active.json": encoded(dict(format="paths_learning_store", format_version=1, generation=generation))})
            target.inspect(store=probe)
        require(capture(store / "generations" / generation) == files, "store.changed", "Completed generation changed before activation")
        unchanged = state[0] is not None and decoded(state[0])["generation"] == generation
        if not unchanged:
            replace_active(store, generation, state[0])
        result.update(published=True, unchanged=unchanged, store=str(store), generation=generation,
                      questions=len(report["catalogue"]["questions"]), visual_acceptance="pending")
        return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    subcommands = parser.add_subparsers(dest="command", required=True)
    for verb in ("export", "publish", "install", "draft"):
        command = subcommands.add_parser(verb)
        command.add_argument("source", type=Path, help="authoring folder" if verb != "install" else "portable export directory")
        command.add_argument("--target", type=Path, default=ROOT / "b/sorter")
        command.add_argument("--library", type=Path, help="explicit target base catalogue")
        if verb == "draft":
            command.add_argument("--output", type=Path, required=True, help="new editable preview folder; existing folders are never overwritten")
            continue
        command.add_argument("--store", type=Path, help="defaults to learning-store beside the target")
        command.add_argument("--base-documents", type=Path, help="documents to retain on first publication")
        if verb != "install":
            command.add_argument("--output", type=Path, help="defaults to build/exports/PACKAGE/VERSION")
        else:
            command.set_defaults(output=None)
    try:
        print(json.dumps(run(parser.parse_args(argv)), indent=2))
        return 0
    except (ExportError, OSError, ValueError, KeyError, TypeError, subprocess.TimeoutExpired) as error:
        print(json.dumps(dict(accepted=False, code=getattr(error, "code", "export.failed"),
                              message=str(error), diagnostics=getattr(error, "diagnostics", [])), indent=2), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
